// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "blockchain/client/wallet/BitcoinTransactionBuilder.hpp"  // IWYU pragma: associated

#include <boost/endian/buffers.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "blockchain/bitcoin/CompactSize.hpp"
#include "internal/blockchain/bitcoin/Bitcoin.hpp"
#include "internal/blockchain/block/Block.hpp"
#include "internal/blockchain/block/bitcoin/Bitcoin.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/api/client/blockchain/BalanceTree.hpp"
#include "opentxs/api/client/blockchain/PaymentCode.hpp"
#include "opentxs/api/client/blockchain/Subchain.hpp"
#include "opentxs/api/client/blockchain/Types.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Hash.hpp"  // IWYU pragma: keep
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/block/bitcoin/Script.hpp"
#include "opentxs/blockchain/block/bitcoin/Transaction.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/protobuf/BlockchainOutputMultisigDetails.pb.h"
#include "opentxs/protobuf/BlockchainTransactionOutput.pb.h"
#include "opentxs/protobuf/BlockchainTransactionProposedNotification.pb.h"
#include "opentxs/protobuf/BlockchainTransactionProposedOutput.pb.h"
#include "opentxs/protobuf/HDPath.pb.h"
#include "util/ScopeGuard.hpp"

#define OT_METHOD                                                              \
    "opentxs::blockchain::client::wallet::BitcoinTransactionBuilder::"

namespace be = boost::endian;

namespace opentxs::blockchain::client::wallet
{
struct BitcoinTransactionBuilder::Imp {
    auto IsFunded() const noexcept -> bool
    {
        return input_value_ > (output_value_ + required_fee());
    }
    auto Spender() const noexcept -> const identifier::Nym&
    {
        return sender_->ID();
    }

    auto AddChange(const Proposal& data) noexcept -> bool
    {
        try {
            const auto reason = api_.Factory().PasswordPrompt(__FUNCTION__);
            const auto& account = blockchain_.Account(sender_->ID(), chain_);
            const auto& element = account.GetNextChangeKey(reason);
            const auto keyID = element.KeyID();
            change_keys_.emplace(keyID);

            auto pOutput = [&] {
                auto elements = [&] {
                    namespace bb = opentxs::blockchain::block::bitcoin;
                    namespace bi = bb::internal;
                    auto out = bb::ScriptElements{};

                    if (const auto size{data.notification().size()}; 1 < size) {
                        throw std::runtime_error{
                            "Multiple notifications not yet supported"};
                    } else if (1 == size) {
                        const auto& notif = data.notification(0);
                        const auto recipient =
                            api_.Factory().PaymentCode(notif.recipient());
                        const auto message =
                            std::string{
                                "Constructing notification transaction to "} +
                            recipient->asBase58();
                        const auto reason =
                            api_.Factory().PasswordPrompt(message);
                        const auto pc = [&] {
                            auto out =
                                api_.Factory().PaymentCode(notif.sender());
                            const auto& path = notif.path();
                            auto seed{path.root()};
                            const auto rc = out->AddPrivateKeys(
                                seed, *path.child().rbegin(), reason);

                            if (false == rc) {
                                throw std::runtime_error{
                                    "Failed to load private keys"};
                            }

                            return out;
                        }();
                        const auto pKey = element.PrivateKey(reason);

                        if (!pKey) {
                            throw std::runtime_error{
                                "Failed to load private change key"};
                        }

                        const auto& key = *pKey;
                        const auto keys = pc->GenerateNotificationElements(
                            recipient, key, reason);

                        if (3u != keys.size()) {
                            throw std::runtime_error{
                                "Failed to obtain notification elements"};
                        }

                        out.emplace_back(bi::Opcode(bb::OP::ONE));
                        out.emplace_back(bi::PushData(reader(keys.at(0))));
                        out.emplace_back(bi::PushData(reader(keys.at(1))));
                        out.emplace_back(bi::PushData(reader(keys.at(2))));
                        out.emplace_back(bi::Opcode(bb::OP::THREE));
                        out.emplace_back(bi::Opcode(bb::OP::CHECKMULTISIG));
                    } else {
                        const auto pkh = element.PubkeyHash();
                        out.emplace_back(bi::Opcode(bb::OP::DUP));
                        out.emplace_back(bi::Opcode(bb::OP::HASH160));
                        out.emplace_back(bi::PushData(pkh->Bytes()));
                        out.emplace_back(bi::Opcode(bb::OP::EQUALVERIFY));
                        out.emplace_back(bi::Opcode(bb::OP::CHECKSIG));
                    }

                    return out;
                }();
                using Position = block::bitcoin::Script::Position;
                auto pScript = factory::BitcoinScript(
                    chain_, std::move(elements), Position::Output);

                if (false == bool(pScript)) {
                    throw std::runtime_error{"Failed to construct script"};
                }

                if (std::numeric_limits<std::uint32_t>::max() <
                    outputs_.size()) {
                    throw std::runtime_error{"too many outputs"};
                }

                return factory::BitcoinTransactionOutput(
                    api_,
                    blockchain_,
                    chain_,
                    static_cast<std::uint32_t>(outputs_.size()),
                    0,
                    std::move(pScript),
                    {keyID});
            }();

            if (false == bool(pOutput)) {
                throw std::runtime_error{"Failed to construct output"};
            }

            {
                auto& output = *pOutput;
                output_value_ += output.Value();
                output_total_ += output.CalculateSize();

                OT_ASSERT(0 < output.Keys().size());
            }

            change_.emplace_back(std::move(pOutput));
            output_count_ = outputs_.size() + change_.size();

            return true;
        } catch (const std::exception& e) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

            return false;
        }
    }
    auto AddInput(const UTXO& utxo) noexcept -> bool
    {
        auto pInput =
            factory::BitcoinTransactionInput(api_, blockchain_, chain_, utxo);

        if (false == bool(pInput)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to construct input")
                .Flush();

            return false;
        }

        const auto& input = *pInput;
        LogTrace(OT_METHOD)(__FUNCTION__)(": adding previous output ")(
            utxo.first.str())(" to transaction")
            .Flush();
        input_count_ = inputs_.size();
        input_total_ += input.CalculateSize();
        const auto amount = static_cast<Amount>(utxo.second.value());
        input_value_ += amount;
        inputs_.emplace_back(std::move(pInput), amount);

        return true;
    }
    auto CreateOutputs(const Proposal& proposal) noexcept -> bool
    {
        namespace bb = opentxs::blockchain::block::bitcoin;
        namespace bi = bb::internal;

        auto index = std::int32_t{-1};

        for (const auto& output : proposal.output()) {
            auto pScript = std::unique_ptr<bi::Script>{};
            using Position = block::bitcoin::Script::Position;

            if (output.has_raw()) {
                pScript = factory::BitcoinScript(
                    chain_, output.raw(), Position::Output);
            } else {
                auto elements = bb::ScriptElements{};

                if (output.has_pubkeyhash()) {
                    if (output.segwit()) {  // P2WPKH
                        elements.emplace_back(bi::Opcode(bb::OP::ZERO));
                        elements.emplace_back(
                            bi::PushData(output.pubkeyhash()));
                    } else {  // P2PKH
                        elements.emplace_back(bi::Opcode(bb::OP::DUP));
                        elements.emplace_back(bi::Opcode(bb::OP::HASH160));
                        elements.emplace_back(
                            bi::PushData(output.pubkeyhash()));
                        elements.emplace_back(bi::Opcode(bb::OP::EQUALVERIFY));
                        elements.emplace_back(bi::Opcode(bb::OP::CHECKSIG));
                    }
                } else if (output.has_scripthash()) {
                    if (output.segwit()) {  // P2WSH
                        elements.emplace_back(bi::Opcode(bb::OP::ZERO));
                        elements.emplace_back(
                            bi::PushData(output.scripthash()));
                    } else {  // P2SH
                        elements.emplace_back(bi::Opcode(bb::OP::HASH160));
                        elements.emplace_back(
                            bi::PushData(output.scripthash()));
                        elements.emplace_back(bi::Opcode(bb::OP::EQUAL));
                    }
                } else if (output.has_pubkey()) {
                    if (output.segwit()) {  // P2TR
                        elements.emplace_back(bi::Opcode(bb::OP::ONE));
                        elements.emplace_back(bi::PushData(output.pubkey()));
                    } else {  // P2PK
                        elements.emplace_back(bi::PushData(output.pubkey()));
                        elements.emplace_back(bi::Opcode(bb::OP::CHECKSIG));
                    }
                } else if (output.has_multisig()) {  // P2MS
                    const auto& ms = output.multisig();
                    const auto M = static_cast<std::uint8_t>(ms.m());
                    const auto N = static_cast<std::uint8_t>(ms.n());
                    elements.emplace_back(
                        bi::Opcode(static_cast<bb::OP>(M + 80)));

                    for (const auto& key : ms.pubkey()) {
                        elements.emplace_back(bi::PushData(key));
                    }

                    elements.emplace_back(
                        bi::Opcode(static_cast<bb::OP>(N + 80)));
                    elements.emplace_back(bi::Opcode(bb::OP::CHECKMULTISIG));
                } else {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Unsupported output type")
                        .Flush();

                    return false;
                }

                pScript = factory::BitcoinScript(
                    chain_, std::move(elements), Position::Output);
            }

            if (false == bool(pScript)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to construct script")
                    .Flush();

                return false;
            }

            auto pOutput = factory::BitcoinTransactionOutput(
                api_,
                blockchain_,
                chain_,
                static_cast<std::uint32_t>(++index),
                static_cast<std::int64_t>(output.amount()),
                std::move(pScript),
                {});

            if (false == bool(pOutput)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to construct output")
                    .Flush();

                return false;
            }

            if (output.has_paymentcodechannel()) {
                try {
                    const auto accountID =
                        api_.Factory().Identifier(output.paymentcodechannel());
                    const auto& account = blockchain_.PaymentCodeSubaccount(
                        blockchain_.Owner(accountID), accountID);
                    static constexpr auto subchain{
                        api::client::blockchain::Subchain::Outgoing};
                    const auto& element = account.BalanceElement(subchain, 0);
                    pOutput->SetPayee(element.Contact());
                } catch (const std::exception& e) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();
                }
            }

            output_value_ += pOutput->Value();
            output_total_ += pOutput->CalculateSize();
            outputs_.emplace_back(std::move(pOutput));
        }

        output_count_ = outputs_.size() + change_.size();

        return true;
    }
    auto FinalizeOutputs() noexcept -> void
    {
        OT_ASSERT(IsFunded());

        const auto excessValue =
            input_value_ - (output_value_ + required_fee());

        if (excessValue <= dust()) {
            for (const auto& key : change_keys_) { blockchain_.Release(key); }
        } else {
            OT_ASSERT(1 == change_.size());  // TODO

            auto& change = *change_.begin();
            change->SetValue(excessValue);
            output_value_ += change->Value();
            outputs_.emplace_back(std::move(change));
        }

        change_.clear();
        bip_69();
    }
    auto FinalizeTransaction() noexcept -> Transaction
    {
        LogTrace(OT_METHOD)(__FUNCTION__)(": ")(print()).Flush();
        auto inputs = factory::BitcoinTransactionInputs([&] {
            auto output = std::vector<Input>{};
            output.reserve(inputs_.size());

            for (auto& [input, value] : inputs_) {
                output.emplace_back(std::move(input));
            }

            return output;
        }());

        if (false == bool(inputs)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to construct inputs")
                .Flush();

            return {};
        }

        auto outputs = factory::BitcoinTransactionOutputs(std::move(outputs_));

        if (false == bool(outputs)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to construct outputs")
                .Flush();

            return {};
        }

        return factory::BitcoinTransaction(
            api_,
            blockchain_,
            chain_,
            Clock::now(),
            version_,
            lock_time_,
            std::move(inputs),
            std::move(outputs));
    }
    auto ReleaseKeys() noexcept -> void
    {
        for (const auto& key : outgoing_keys_) { blockchain_.Release(key); }

        for (const auto& key : change_keys_) { blockchain_.Release(key); }
    }
    auto SignInputs() noexcept -> bool
    {
        auto index = int{-1};
        auto txcopy = Transaction{};
        auto bip143 = std::optional<bitcoin::Bip143Hashes>{};

        for (const auto& [input, value] : inputs_) {
            if (false == sign_input(++index, *input, txcopy, bip143)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to sign input")
                    .Flush();

                return false;
            }
        }

        return true;
    }

    Imp(const api::Core& api,
        const api::client::Blockchain& blockchain,
        const internal::WalletDatabase& db,
        const Identifier& id,
        const Proposal& proposal,
        const Type chain,
        const Amount feeRate) noexcept
        : api_(api)
        , blockchain_(blockchain)
        , sender_([&] {
            const auto id = [&] {
                auto out = api_.Factory().NymID();
                const auto& sender = proposal.initiator();
                out->Assign(sender.data(), sender.size());

                return out;
            }();

            OT_ASSERT(false == id->empty());

            return api_.Wallet().Nym(id);
        }())
        , chain_(chain)
        , fee_rate_(feeRate)
        , version_(1)
        , lock_time_(0)
        , outputs_()
        , change_()
        , inputs_()
        , fixed_overhead_(sizeof(version_) + sizeof(lock_time_))
        , input_count_()
        , output_count_()
        , input_total_()
        , output_total_()
        , input_value_()
        , output_value_()
        , change_keys_()
        , outgoing_keys_([&] {
            auto out = std::set<KeyID>{};

            for (const auto& output : proposal.output()) {
                if (false == output.has_paymentcodechannel()) { continue; }

                using Subchain = api::client::blockchain::Subchain;
                out.emplace(
                    output.paymentcodechannel(),
                    Subchain::Outgoing,
                    output.index());
            }

            return out;
        }())
    {
        OT_ASSERT(sender_);
    }

private:
    using Input = std::unique_ptr<block::bitcoin::internal::Input>;
    using Output = std::unique_ptr<block::bitcoin::internal::Output>;
    using Bip143 = std::optional<bitcoin::Bip143Hashes>;
    using Hash = std::array<std::byte, 32>;

    static constexpr auto p2pkh_output_bytes_ = std::size_t{34};

    const api::Core& api_;
    const api::client::Blockchain& blockchain_;
    const Nym_p sender_;
    const Type chain_;
    const Amount fee_rate_;
    const be::little_int32_buf_t version_;
    const be::little_uint32_buf_t lock_time_;
    std::vector<Output> outputs_;
    std::vector<Output> change_;
    std::vector<std::pair<Input, Amount>> inputs_;
    const std::size_t fixed_overhead_;
    bitcoin::CompactSize input_count_;
    bitcoin::CompactSize output_count_;
    std::size_t input_total_;
    std::size_t output_total_;
    Amount input_value_;
    Amount output_value_;
    std::set<KeyID> change_keys_;
    std::set<KeyID> outgoing_keys_;

    static auto is_segwit(const block::bitcoin::internal::Input& input) noexcept
        -> bool
    {
        // TODO

        return false;
    }

    auto add_signatures(
        const ReadView preimage,
        const blockchain::bitcoin::SigHash& sigHash,
        block::bitcoin::internal::Input& input) const noexcept -> bool
    {
        const auto reason = api_.Factory().PasswordPrompt(__FUNCTION__);
        const auto& output = input.Spends();
        using Pattern = block::bitcoin::Script::Pattern;

        switch (output.Script().Type()) {
            case Pattern::PayToPubkeyHash: {
                return add_signatures_p2pkh(
                    preimage, sigHash, reason, output, input);
            }
            case Pattern::PayToPubkey: {
                return add_signatures_p2pk(
                    preimage, sigHash, reason, output, input);
            }
            case Pattern::PayToMultisig: {
                return add_signatures_p2ms(
                    preimage, sigHash, reason, output, input);
            }
            default: {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported input type")
                    .Flush();

                return false;
            }
        }
    }
    auto add_signatures_p2ms(
        const ReadView preimage,
        const blockchain::bitcoin::SigHash& sigHash,
        const PasswordPrompt& reason,
        const block::bitcoin::internal::Output& spends,
        block::bitcoin::internal::Input& input) const noexcept -> bool
    {
        const auto& script = spends.Script();

        if ((1u != script.M().value()) || (3u != script.N().value())) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported multisig pattern")
                .Flush();

            return false;
        }

        auto keys = std::vector<OTData>{};
        auto signatures = std::vector<Space>{};
        auto views = block::bitcoin::internal::Input::Signatures{};

        for (const auto& id : input.Keys()) {
            const auto& node = blockchain_.GetKey(id);

            OT_ASSERT(node.KeyID() == id);

            const auto pKey = node.PrivateKey(reason);

            OT_ASSERT(pKey);

            const auto& key = *pKey;

            if (key.PublicKey() != script.MultisigPubkey(0).value()) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Pubkey mismatch").Flush();

                continue;
            }

            auto& sig = signatures.emplace_back();
            sig.reserve(80);
            const auto haveSig =
                key.SignDER(preimage, hash_type(), sig, reason);

            if (false == haveSig) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to obtain signature")
                    .Flush();

                return false;
            }

            sig.emplace_back(sigHash.flags_);

            OT_ASSERT(0 < key.PublicKey().size());

            views.emplace_back(reader(sig), ReadView{});
        }

        if (0 == views.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": No keys available for signing ")(
                input.PreviousOutput().str())
                .Flush();

            return false;
        }

        if (false == input.AddMultisigSignatures(views)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to apply signature")
                .Flush();

            return false;
        }

        return true;
    }
    auto add_signatures_p2pk(
        const ReadView preimage,
        const blockchain::bitcoin::SigHash& sigHash,
        const PasswordPrompt& reason,
        const block::bitcoin::internal::Output& spends,
        block::bitcoin::internal::Input& input) const noexcept -> bool
    {
        auto keys = std::vector<OTData>{};
        auto signatures = std::vector<Space>{};
        auto views = block::bitcoin::internal::Input::Signatures{};

        for (const auto& id : input.Keys()) {
            const auto& element = blockchain_.GetKey(id);

            OT_ASSERT(element.KeyID() == id);

            const auto pPublic = validate(
                Match::ByValue, element, input.PreviousOutput(), spends);

            if (!pPublic) { continue; }

            const auto& pub = *pPublic;
            const auto pKey = get_private_key(pub, element, reason);

            if (!pKey) { continue; }

            const auto& key = *pKey;
            auto& sig = signatures.emplace_back();
            sig.reserve(80);
            const auto haveSig =
                key.SignDER(preimage, hash_type(), sig, reason);

            if (false == haveSig) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to obtain signature")
                    .Flush();

                return false;
            }

            sig.emplace_back(sigHash.flags_);

            OT_ASSERT(0 < key.PublicKey().size());

            views.emplace_back(reader(sig), ReadView{});
        }

        if (0 == views.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": No keys available for signing ")(
                input.PreviousOutput().str())
                .Flush();

            return false;
        }

        if (false == input.AddSignatures(views)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to apply signature")
                .Flush();

            return false;
        }

        return true;
    }
    auto add_signatures_p2pkh(
        const ReadView preimage,
        const blockchain::bitcoin::SigHash& sigHash,
        const PasswordPrompt& reason,
        const block::bitcoin::internal::Output& spends,
        block::bitcoin::internal::Input& input) const noexcept -> bool
    {
        auto keys = std::vector<OTData>{};
        auto signatures = std::vector<Space>{};
        auto views = block::bitcoin::internal::Input::Signatures{};

        for (const auto& id : input.Keys()) {
            const auto& element = blockchain_.GetKey(id);

            OT_ASSERT(element.KeyID() == id);

            const auto pPublic = validate(
                Match::ByHash, element, input.PreviousOutput(), spends);

            if (!pPublic) { continue; }

            const auto& pub = *pPublic;
            const auto pKey = get_private_key(pub, element, reason);

            if (!pKey) { continue; }

            const auto& key = *pKey;

            const auto& pubkey =
                keys.emplace_back(api_.Factory().Data(key.PublicKey()));
            auto& sig = signatures.emplace_back();
            sig.reserve(80);
            const auto haveSig =
                key.SignDER(preimage, hash_type(), sig, reason);

            if (false == haveSig) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to obtain signature")
                    .Flush();

                return false;
            }

            sig.emplace_back(sigHash.flags_);

            OT_ASSERT(0 < key.PublicKey().size());

            views.emplace_back(reader(sig), pubkey->Bytes());
        }

        if (0 == views.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": No keys available for signing ")(
                input.PreviousOutput().str())
                .Flush();

            return false;
        }

        if (false == input.AddSignatures(views)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to apply signature")
                .Flush();

            return false;
        }

        return true;
    }
    auto bytes() const noexcept -> std::size_t
    {
        // NOTE assumes one additional output to account for change
        const auto outputs = bitcoin::CompactSize{output_count_.Value() + 1};

        return fixed_overhead_ + input_count_.Size() + input_total_ +
               outputs.Size() + output_total_ + p2pkh_output_bytes_;
    }
    auto dust() const noexcept -> std::size_t
    {
        // TODO this should account for script type

        return std::size_t{148} * fee_rate_ / 1000;
    }
    auto get_single(
        const std::size_t index,
        const blockchain::bitcoin::SigHash& sigHash) const noexcept
        -> std::unique_ptr<Hash>
    {
        if (bitcoin::SigOption::Single != sigHash.Type()) { return nullptr; }

        if (index >= outputs_.size()) { return nullptr; }

        // TODO not implemented yet

        return nullptr;
    }
    auto get_private_key(
        const crypto::key::EllipticCurve& pubkey,
        const api::client::blockchain::BalanceNode::Element& element,
        const PasswordPrompt& reason) const noexcept
        -> api::client::blockchain::ECKey
    {
        auto pKey = element.PrivateKey(reason);

        if (!pKey) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": failed to obtain private key")
                .Flush();

            return {};
        }

        const auto& key = *pKey;

        if (key.PublicKey() != pubkey.PublicKey()) {
            const auto got = api_.Factory().Data(key.PublicKey());
            const auto expected = api_.Factory().Data(pubkey.PublicKey());
            const auto [account, subchain, index] = element.KeyID();
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Derived private key for account ")(account)(" subchain ")(
                static_cast<std::uint32_t>(subchain))(" index ")(index)(
                " does not correspond to the expected public key. Got ")(
                got->asHex())(" expected ")(expected->asHex())
                .Flush();

            OT_FAIL;
        }

        return pKey;
    }
    auto hash_type() const noexcept -> crypto::HashType
    {
        return crypto::HashType::Sha256D;
    }
    auto init_bip143(Bip143& bip143) const noexcept -> bool
    {
        if (bip143.has_value()) { return true; }

        auto success{false};
        const auto postcondition = ScopeGuard{[&]() {
            if (false == success) { bip143 = std::nullopt; }
        }};
        bip143.emplace();

        OT_ASSERT(bip143.has_value());

        auto& output = bip143.value();
        auto cb = [&](const auto& preimage, auto& output) -> bool {
            return api_.Crypto().Hash().Digest(
                crypto::HashType::Sha256D,
                reader(preimage),
                preallocated(output.size(), output.data()));
        };

        {
            auto preimage = space(inputs_.size() * sizeof(block::Outpoint));
            auto it = preimage.data();

            for (const auto& [input, amount] : inputs_) {
                const auto& outpoint = input->PreviousOutput();
                std::memcpy(it, &outpoint, sizeof(outpoint));
                std::advance(it, sizeof(outpoint));
            }

            if (false == cb(preimage, output.outpoints_)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to hash outpoints")
                    .Flush();

                return false;
            }
        }

        {
            auto preimage = space(inputs_.size() * sizeof(std::uint32_t));
            auto it = preimage.data();

            for (const auto& [input, value] : inputs_) {
                const auto sequence = input->Sequence();
                std::memcpy(it, &sequence, sizeof(sequence));
                std::advance(it, sizeof(sequence));
            }

            if (false == cb(preimage, output.sequences_)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to hash sequences")
                    .Flush();

                return false;
            }
        }

        {
            auto preimage = space(output_total_);
            auto it = preimage.data();

            for (const auto& output : outputs_) {
                const auto size = output->CalculateSize();

                if (false ==
                    output->Serialize(preallocated(size, it)).has_value()) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Failed to serialize output")
                        .Flush();

                    return false;
                }

                std::advance(it, size);
            }

            if (false == cb(preimage, output.outputs_)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to hash outputs")
                    .Flush();

                return false;
            }
        }

        success = true;

        return true;
    }
    auto init_txcopy(Transaction& txcopy) const noexcept -> bool
    {
        if (txcopy) { return true; }

        auto inputCopy = std::vector<Input>{};
        std::transform(
            std::begin(inputs_),
            std::end(inputs_),
            std::back_inserter(inputCopy),
            [](const auto& input) -> auto {
                return input.first->SignatureVersion();
            });
        auto inputs = factory::BitcoinTransactionInputs(std::move(inputCopy));

        if (false == bool(inputs)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to construct inputs")
                .Flush();

            return {};
        }

        auto outputCopy = std::vector<Output>{};
        std::transform(
            std::begin(outputs_),
            std::end(outputs_),
            std::back_inserter(outputCopy),
            [](const auto& output) -> auto { return output->clone(); });
        auto outputs =
            factory::BitcoinTransactionOutputs(std::move(outputCopy));

        if (false == bool(outputs)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to construct outputs")
                .Flush();

            return {};
        }

        txcopy = factory::BitcoinTransaction(
            api_,
            blockchain_,
            chain_,
            Clock::now(),
            version_,
            lock_time_,
            std::move(inputs),
            std::move(outputs));

        return bool(txcopy);
    }
    auto print() const noexcept -> std::string
    {
        auto text = std::stringstream{};
        text << "\n     version: " << std::to_string(version_.value()) << '\n';
        text << "   lock time: " << std::to_string(lock_time_.value()) << '\n';
        text << " input count: " << std::to_string(inputs_.size()) << '\n';

        for (const auto& [input, value] : inputs_) {
            const auto& outpoint = input->PreviousOutput();
            text << " * " << outpoint.str()
                 << ", sequence: " << std::to_string(input->Sequence())
                 << ", value: " << std::to_string(value) << '\n';
        }

        text << "output count: " << std::to_string(outputs_.size()) << '\n';

        for (const auto& output : outputs_) {
            text << " * bytes: " << std::to_string(output->CalculateSize())
                 << ", value: " << std::to_string(output->Value()) << '\n';
        }

        text << "total output value: " << std::to_string(output_value_) << '\n';
        text << " total input value: " << std::to_string(input_value_) << '\n';
        text << "               fee: "
             << std::to_string(input_value_ - output_value_);

        return text.str();
    }
    auto required_fee() const noexcept -> Amount
    {
        return (bytes() * fee_rate_) / 1000;
    }
    auto sign_input(
        const int index,
        block::bitcoin::internal::Input& input,
        Transaction& txcopy,
        Bip143& bip143) const noexcept -> bool
    {
        switch (chain_) {
            case Type::BitcoinCash:
            case Type::BitcoinCash_testnet3: {

                return sign_input_bch(index, input, bip143);
            }
            case Type::Bitcoin:
            case Type::Bitcoin_testnet3:
            case Type::Litecoin:
            case Type::Litecoin_testnet4:
            case Type::PKT:
            case Type::PKT_testnet:
            case Type::UnitTest: {
                if (is_segwit(input)) {
                    // TODO not implemented yet

                    return false;
                }

                return sign_input_btc(index, input, txcopy);
            }
            case Type::Unknown:
            case Type::Ethereum_frontier:
            case Type::Ethereum_ropsten:
            default: {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported chain")
                    .Flush();

                return false;
            }
        }
    }
    auto sign_input_bch(
        const int index,
        block::bitcoin::internal::Input& input,
        Bip143& bip143) const noexcept -> bool
    {
        if (false == init_bip143(bip143)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Error instantiating txcopy")
                .Flush();

            return false;
        }

        const auto sigHash = blockchain::bitcoin::SigHash{chain_};
        const auto& outpoints = bip143->Outpoints(sigHash);
        const auto& sequences = bip143->Sequences(sigHash);
        const auto& outpoint = input.PreviousOutput();
        const auto pScript = input.Spends().SigningSubscript();

        OT_ASSERT(pScript);

        const auto& script = *pScript;
        const auto scriptBytes = script.CalculateSize();
        const auto cs = blockchain::bitcoin::CompactSize{scriptBytes};
        const auto& output = input.Spends();
        const auto value = output.Value();
        const auto sequence = input.Sequence();
        const auto single = get_single(index, sigHash);
        const auto& outputs = bip143->Outputs(sigHash, single.get());
        // clang-format off
    auto preimage = space(
        sizeof(version_) +
        sizeof(outpoints) +
        sizeof(sequences) +
        sizeof(outpoint) +
        cs.Total() +
        sizeof(value) +
        sizeof(sequence) +
        sizeof(outputs) +
        sizeof(lock_time_) +
        sizeof(sigHash));
        // clang-format on
        auto it = preimage.data();
        std::memcpy(it, &version_, sizeof(version_));
        std::advance(it, sizeof(version_));
        std::memcpy(it, outpoints.data(), outpoints.size());
        std::advance(it, outpoints.size());
        std::memcpy(it, sequences.data(), sequences.size());
        std::advance(it, sequences.size());
        std::memcpy(it, &outpoint, sizeof(outpoint));
        std::advance(it, sizeof(outpoint));

        if (false == cs.Encode(preallocated(cs.Size(), it))) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": CompactSize encoding failure")
                .Flush();

            return false;
        }

        std::advance(it, cs.Size());

        if (false == script.Serialize(preallocated(scriptBytes, it))) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Script encoding failure")
                .Flush();

            return false;
        }

        std::advance(it, scriptBytes);
        std::memcpy(it, &value, sizeof(value));
        std::advance(it, sizeof(value));
        std::memcpy(it, &sequence, sizeof(sequence));
        std::advance(it, sizeof(sequence));
        std::memcpy(it, outputs.data(), outputs.size());
        std::advance(it, outputs.size());
        std::memcpy(it, &lock_time_, sizeof(lock_time_));
        std::advance(it, sizeof(lock_time_));
        std::memcpy(it, &sigHash, sizeof(sigHash));
        std::advance(it, sizeof(sigHash));

        return add_signatures(reader(preimage), sigHash, input);
    }
    auto sign_input_btc(
        const int index,
        block::bitcoin::internal::Input& input,
        Transaction& txcopy) const noexcept -> bool
    {
        if (false == init_txcopy(txcopy)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Error instantiating txcopy")
                .Flush();

            return false;
        }

        const auto sigHash = blockchain::bitcoin::SigHash{chain_};
        auto preimage = txcopy->GetPreimageBTC(index, sigHash);

        if (0 == preimage.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Error obtaining signing preimage")
                .Flush();

            return false;
        }

        std::copy(sigHash.begin(), sigHash.end(), std::back_inserter(preimage));

        return add_signatures(reader(preimage), sigHash, input);
    }
    enum class Match : bool { ByValue, ByHash };
    auto validate(
        const Match match,
        const api::client::blockchain::BalanceNode::Element& element,
        const block::Outpoint& outpoint,
        const block::bitcoin::internal::Output& output) const noexcept
        -> api::client::blockchain::ECKey
    {
        const auto [account, subchain, index] = element.KeyID();
        LogTrace(OT_METHOD)(__FUNCTION__)(": considering spend key ")(index)(
            " from subchain ")(static_cast<std::uint32_t>(subchain))(
            " of account ")(account)(" for previous output ")(outpoint.str())
            .Flush();

        auto pKey = element.Key();

        if (!pKey) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": missing public key").Flush();

            return {};
        }

        const auto& key = *pKey;

        if (Match::ByValue == match) {
            const auto expected = output.Script().Pubkey();

            if (false == expected.has_value()) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": wrong output script type")
                    .Flush();

                return {};
            }

            if (key.PublicKey() != expected.value()) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Provided public key does not match expected value")
                    .Flush();

                return {};
            }
        } else {
            const auto expected = output.Script().PubkeyHash();

            if (false == expected.has_value()) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": wrong output script type")
                    .Flush();

                return {};
            }

            if (element.PubkeyHash()->Bytes() != expected.value()) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Provided public key does not match expected hash")
                    .Flush();

                return {};
            }
        }

        return pKey;
    }

    auto bip_69() noexcept -> void
    {
        auto inputSort = [](const auto& lhs, const auto& rhs) -> auto
        {
            return lhs.first->PreviousOutput() < rhs.first->PreviousOutput();
        };
        auto outputSort = [](const auto& lhs, const auto& rhs) -> auto
        {
            if (lhs->Value() == rhs->Value()) {
                auto lScript = Space{};
                auto rScript = Space{};
                lhs->Script().Serialize(writer(lScript));
                rhs->Script().Serialize(writer(rScript));

                return std::lexicographical_compare(
                    std::begin(lScript),
                    std::end(lScript),
                    std::begin(rScript),
                    std::end(rScript));
            } else {

                return lhs->Value() < rhs->Value();
            }
        };

        std::sort(std::begin(inputs_), std::end(inputs_), inputSort);
        std::sort(std::begin(outputs_), std::end(outputs_), outputSort);
        auto index{-1};

        for (const auto& output : outputs_) { output->SetIndex(++index); }
    }
};

BitcoinTransactionBuilder::BitcoinTransactionBuilder(
    const api::Core& api,
    const api::client::Blockchain& blockchain,
    const internal::WalletDatabase& db,
    const Identifier& id,
    const Proposal& proposal,
    const Type chain,
    const Amount feeRate) noexcept
    : imp_(std::make_unique<
           Imp>(api, blockchain, db, id, proposal, chain, feeRate))
{
    OT_ASSERT(imp_);
}

auto BitcoinTransactionBuilder::AddChange(const Proposal& data) noexcept -> bool
{
    return imp_->AddChange(data);
}

auto BitcoinTransactionBuilder::AddInput(const UTXO& utxo) noexcept -> bool
{
    return imp_->AddInput(utxo);
}

auto BitcoinTransactionBuilder::CreateOutputs(const Proposal& proposal) noexcept
    -> bool
{
    return imp_->CreateOutputs(proposal);
}

auto BitcoinTransactionBuilder::FinalizeOutputs() noexcept -> void
{
    return imp_->FinalizeOutputs();
}

auto BitcoinTransactionBuilder::FinalizeTransaction() noexcept -> Transaction
{
    return imp_->FinalizeTransaction();
}

auto BitcoinTransactionBuilder::IsFunded() const noexcept -> bool
{
    return imp_->IsFunded();
}

auto BitcoinTransactionBuilder::ReleaseKeys() noexcept -> void
{
    return imp_->ReleaseKeys();
}

auto BitcoinTransactionBuilder::SignInputs() noexcept -> bool
{
    return imp_->SignInputs();
}
auto BitcoinTransactionBuilder::Spender() const noexcept
    -> const identifier::Nym&
{
    return imp_->Spender();
}

BitcoinTransactionBuilder::~BitcoinTransactionBuilder() = default;
}  // namespace opentxs::blockchain::client::wallet
