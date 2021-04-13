// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Helpers.hpp"  // IWYU pragma: associated

#include <gtest/gtest.h>
#include <deque>
#include <optional>

#include "UIHelpers.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/UI.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/api/client/blockchain/BalanceNodeType.hpp"
#include "opentxs/api/client/blockchain/BalanceTree.hpp"
#include "opentxs/api/client/blockchain/HD.hpp"
#include "opentxs/api/client/blockchain/PaymentCode.hpp"
#include "opentxs/api/client/blockchain/Subchain.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/blockchain/block/bitcoin/Input.hpp"  // IWYU pragma: keep
#include "opentxs/blockchain/block/bitcoin/Inputs.hpp"
#include "opentxs/blockchain/block/bitcoin/Output.hpp"
#include "opentxs/blockchain/block/bitcoin/Outputs.hpp"
#include "opentxs/blockchain/block/bitcoin/Script.hpp"  // IWYU pragma: keep
#include "opentxs/blockchain/block/bitcoin/Transaction.hpp"
#include "opentxs/blockchain/client/BlockOracle.hpp"
#include "opentxs/blockchain/client/FilterOracle.hpp"
#include "opentxs/blockchain/client/Wallet.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/crypto/Language.hpp"
#include "opentxs/crypto/SeedStyle.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/ui/AccountActivity.hpp"
#include "opentxs/ui/BalanceItem.hpp"
#include "paymentcode/VectorsV3.hpp"

Counter account_activity_alice_{};
Counter account_activity_bob_{};

namespace bca = opentxs::api::client::blockchain;

class Regtest_payment_code : public Regtest_fixture_normal
{
protected:
    using Subchain = bca::Subchain;
    using Transactions = std::deque<ot::blockchain::block::pTxid>;

    static ot::Nym_p alice_p_;
    static ot::Nym_p bob_p_;
    static Transactions transactions_;
    static std::unique_ptr<ScanListener> listener_alice_p_;
    static std::unique_ptr<ScanListener> listener_bob_p_;

    const ot::identity::Nym& alice_;
    const ot::identity::Nym& bob_;
    const ot::Identifier& expected_account_;
    const ot::identifier::Server& expected_notary_;
    const ot::identifier::UnitDefinition& expected_unit_;
    const std::string expected_display_unit_;
    const std::string expected_account_name_;
    const std::string expected_notary_name_;
    const std::string memo_outgoing_;
    const ot::AccountType expected_account_type_;
    const ot::contact::ContactItemType expected_unit_type_;
    const Generator mine_to_alice_;
    const GetBytes get_p2pk_bytes_;
    const GetBytes get_p2ms_bytes_;
    const GetPattern get_p2pk_patterns_;
    const GetPattern get_p2ms_patterns_;
    ScanListener& listener_alice_;
    ScanListener& listener_bob_;

    auto SendHD() const noexcept -> const bca::HD&
    {
        return client_1_.Blockchain()
            .Account(alice_.ID(), test_chain_)
            .GetHD()
            .at(0);
    }
    auto SendPC() const noexcept -> const bca::PaymentCode&
    {
        return client_1_.Blockchain()
            .Account(alice_.ID(), test_chain_)
            .GetPaymentCode()
            .at(0);
    }

    auto Shutdown() noexcept -> void final
    {
        listener_bob_p_.reset();
        listener_alice_p_.reset();
        transactions_.clear();
        bob_p_.reset();
        alice_p_.reset();
        Regtest_fixture_normal::Shutdown();
    }

    Regtest_payment_code()
        : Regtest_fixture_normal(2)
        , alice_([&]() -> const ot::identity::Nym& {
            if (!alice_p_) {
                const auto reason =
                    client_1_.Factory().PasswordPrompt(__FUNCTION__);
                const auto& vector = vectors_3_.alice_;
                const auto seedID = [&] {
                    const auto words =
                        client_1_.Factory().SecretFromText(vector.words_);
                    const auto phrase = client_1_.Factory().Secret(0);

                    return client_1_.Seeds().ImportSeed(
                        words,
                        phrase,
                        ot::crypto::SeedStyle::BIP39,
                        ot::crypto::Language::en,
                        reason);
                }();

                alice_p_ = client_1_.Wallet().Nym(reason, "Alice", {seedID, 0});

                OT_ASSERT(alice_p_)
                OT_ASSERT(alice_p_->PaymentCode() == vector.payment_code_)

                client_1_.Blockchain().NewHDSubaccount(
                    alice_p_->ID(),
                    ot::BlockchainAccountType::BIP44,
                    test_chain_,
                    reason);
            }

            OT_ASSERT(alice_p_)

            return *alice_p_;
        }())
        , bob_([&]() -> const ot::identity::Nym& {
            if (!bob_p_) {
                const auto reason =
                    client_2_.Factory().PasswordPrompt(__FUNCTION__);
                const auto& vector = vectors_3_.bob_;
                const auto seedID = [&] {
                    const auto words =
                        client_2_.Factory().SecretFromText(vector.words_);
                    const auto phrase = client_2_.Factory().Secret(0);

                    return client_2_.Seeds().ImportSeed(
                        words,
                        phrase,
                        ot::crypto::SeedStyle::BIP39,
                        ot::crypto::Language::en,
                        reason);
                }();

                bob_p_ = client_2_.Wallet().Nym(reason, "Alice", {seedID, 0});

                OT_ASSERT(bob_p_)

                client_2_.Blockchain().NewHDSubaccount(
                    bob_p_->ID(),
                    ot::BlockchainAccountType::BIP44,
                    test_chain_,
                    reason);
            }

            OT_ASSERT(bob_p_)

            return *bob_p_;
        }())
        , expected_account_(client_1_.UI().BlockchainAccountID(test_chain_))
        , expected_notary_(client_1_.UI().BlockchainNotaryID(test_chain_))
        , expected_unit_(client_1_.UI().BlockchainUnitID(test_chain_))
        , expected_display_unit_(u8"UNITTEST")
        , expected_account_name_(u8"This device")
        , expected_notary_name_(u8"Unit Test Simulation")
        , memo_outgoing_("memo for outgoing transaction")
        , expected_account_type_(ot::AccountType::Blockchain)
        , expected_unit_type_(ot::contact::ContactItemType::Regtest)
        , mine_to_alice_([&](Height height) -> Transaction {
            using OutputBuilder = ot::api::Factory::OutputBuilder;

            auto output = miner_.Factory().BitcoinGenerationTransaction(
                test_chain_,
                height,
                [&] {
                    auto output = std::vector<OutputBuilder>{};
                    const auto& account = SendHD();
                    const auto reason =
                        client_1_.Factory().PasswordPrompt(__FUNCTION__);
                    const auto keys = std::set<bca::Key>{};
                    static const auto amount =
                        ot::blockchain::Amount{10000000000};
                    const auto index =
                        account.Reserve(Subchain::External, reason);

                    EXPECT_TRUE(index.has_value());

                    const auto& element = account.BalanceElement(
                        Subchain::External, index.value_or(0));
                    const auto key = element.Key();

                    OT_ASSERT(key);
                    output.emplace_back(
                        amount,
                        miner_.Factory().BitcoinScriptP2PK(test_chain_, *key),
                        keys);

                    return output;
                }(),
                coinbase_fun_);

            OT_ASSERT(output);

            transactions_.emplace_back(output->ID());

            return output;
        })
        , get_p2pk_bytes_(
              [&](const auto& script, auto index) { return script.Pubkey(); })
        , get_p2ms_bytes_([&](const auto& script, auto index) {
            return script.MultisigPubkey(0);
        })
        , get_p2pk_patterns_([&](auto index) { return Pattern::PayToPubkey; })
        , get_p2ms_patterns_([&](auto index) { return Pattern::PayToMultisig; })
        , listener_alice_([&]() -> ScanListener& {
            if (!listener_alice_p_) {
                listener_alice_p_ = std::make_unique<ScanListener>(client_1_);
            }

            OT_ASSERT(listener_alice_p_);

            return *listener_alice_p_;
        }())
        , listener_bob_([&]() -> ScanListener& {
            if (!listener_bob_p_) {
                listener_bob_p_ = std::make_unique<ScanListener>(client_2_);
            }

            OT_ASSERT(listener_bob_p_);

            return *listener_bob_p_;
        }())
    {
    }
};

ot::Nym_p Regtest_payment_code::alice_p_{};
ot::Nym_p Regtest_payment_code::bob_p_{};
Regtest_payment_code::Transactions Regtest_payment_code::transactions_{};
std::unique_ptr<ScanListener> Regtest_payment_code::listener_alice_p_{};
std::unique_ptr<ScanListener> Regtest_payment_code::listener_bob_p_{};

namespace
{
TEST_F(Regtest_payment_code, init_opentxs) {}

TEST_F(Regtest_payment_code, start_chains) { EXPECT_TRUE(Start()); }

TEST_F(Regtest_payment_code, connect_peers) { EXPECT_TRUE(Connect()); }

TEST_F(Regtest_payment_code, init_account_activity)
{
    account_activity_alice_.expected_ += 0;
    account_activity_bob_.expected_ += 0;
    client_1_.UI().AccountActivity(
        alice_.ID(),
        client_1_.UI().BlockchainAccountID(test_chain_),
        make_cb(account_activity_alice_, u8"account_activity_alice"));
    client_2_.UI().AccountActivity(
        bob_.ID(),
        client_2_.UI().BlockchainAccountID(test_chain_),
        make_cb(account_activity_bob_, u8"account_activity_bob"));
    wait_for_counter(account_activity_alice_);
    wait_for_counter(account_activity_bob_);
}

TEST_F(Regtest_payment_code, mine)
{
    auto future = listener_alice_.get_future(SendHD(), Subchain::External, 1);
    constexpr auto count{1};
    account_activity_alice_.expected_ += (2 * count);

    EXPECT_TRUE(Mine(0, count, mine_to_alice_));
    EXPECT_TRUE(listener_alice_.wait(future));
}

TEST_F(Regtest_payment_code, first_block)
{
    const auto& blockchain = client_1_.Blockchain().GetChain(test_chain_);
    const auto blockHash = blockchain.HeaderOracle().BestHash(1);

    ASSERT_FALSE(blockHash->empty());

    const auto pBlock = blockchain.BlockOracle().LoadBitcoin(blockHash).get();

    ASSERT_TRUE(pBlock);

    const auto& block = *pBlock;

    ASSERT_EQ(block.size(), 1);

    const auto pTx = block.at(0);

    ASSERT_TRUE(pTx);

    const auto& tx = *pTx;

    EXPECT_EQ(tx.ID(), transactions_.at(0));
    EXPECT_EQ(tx.BlockPosition(), 0);
    ASSERT_EQ(tx.Outputs().size(), 1);
}

TEST_F(Regtest_payment_code, alice_after_receive_wallet)
{
    const auto& network = client_1_.Blockchain().GetChain(test_chain_);
    const auto& wallet = network.Wallet();
    const auto& nym = alice_.ID();
    const auto& account = SendHD().ID();
    const auto blankNym = client_1_.Factory().NymID();
    const auto blankAccount = client_1_.Factory().Identifier();
    using Balance = ot::blockchain::Balance;
    const auto balance = Balance{10000000000, 10000000000};
    const auto noBalance = Balance{0, 0};
    // const auto outpointsConfirmedNew = [&] {
    //     auto out = std::vector<Outpoint>{};
    //     const auto& txid = transactions_.at(0);
    //     out.emplace_back(txid->Bytes(), 0);
    //
    //     return out;
    // }();
    // const auto keysConfirmedNew = [&] {
    //     auto out = std::vector<ot::OTData>{};
    //     const auto& element = SendHD().BalanceElement(Subchain::External,
    //     0u); const auto k = element.Key();
    //
    //     OT_ASSERT(k);
    //
    //     out.emplace_back(client_1_.Factory().Data(k->PublicKey()));
    //
    //     return out;
    // }();
    // const auto testConfirmedNew = [&](const auto& utxos) -> bool {
    //     return TestUTXOs(
    //         outpointsConfirmedNew,
    //         keysConfirmedNew,
    //         utxos,
    //         get_p2pk_bytes_,
    //         get_p2pk_patterns_,
    //         [](const auto index) -> std::int64_t {
    //             const auto amount = ot::blockchain::Amount{10000000000};
    //
    //             return amount + index;
    //         });
    // };

    // ASSERT_EQ(outpointsConfirmedNew.size(), 1u);
    // ASSERT_EQ(keysConfirmedNew.size(), 1u);

    EXPECT_EQ(wallet.GetBalance(), balance);
    EXPECT_EQ(network.GetBalance(), balance);
    EXPECT_EQ(wallet.GetBalance(nym), balance);
    EXPECT_EQ(network.GetBalance(nym), balance);
    EXPECT_EQ(wallet.GetBalance(nym, account), balance);
    EXPECT_EQ(wallet.GetBalance(blankNym), noBalance);
    EXPECT_EQ(network.GetBalance(blankNym), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym, blankAccount), noBalance);
    EXPECT_EQ(wallet.GetBalance(nym, blankAccount), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym, account), noBalance);

    using TxoState = ot::blockchain::client::Wallet::TxoState;
    auto type = TxoState::All;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, account, type)));

    type = TxoState::UnconfirmedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    type = TxoState::UnconfirmedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    type = TxoState::ConfirmedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, account, type)));

    type = TxoState::ConfirmedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    type = TxoState::OrphanedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);

    type = TxoState::OrphanedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, account, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, account, type).size(), 0u);
}

TEST_F(Regtest_payment_code, alice_after_receive_ui)
{
    wait_for_counter(account_activity_alice_);
    const auto& widget =
        client_1_.UI().AccountActivity(alice_.ID(), expected_account_);

    EXPECT_EQ(widget.AccountID(), expected_account_.str());
    EXPECT_EQ(widget.Balance(), 10000000000);
    EXPECT_EQ(widget.BalancePolarity(), 1);
    EXPECT_EQ(widget.ContractID(), expected_unit_.str());
    EXPECT_FALSE(widget.DepositAddress().empty());
    EXPECT_FALSE(widget.DepositAddress(test_chain_).empty());
    EXPECT_TRUE(widget.DepositAddress(ot::blockchain::Type::Bitcoin).empty());

    const auto deposit = widget.DepositChains();

    ASSERT_EQ(deposit.size(), 1);
    EXPECT_EQ(deposit.at(0), test_chain_);
    EXPECT_EQ(widget.DisplayBalance(), u8"100 units");
    EXPECT_EQ(widget.DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(widget.Name(), expected_account_name_);
    EXPECT_EQ(widget.NotaryID(), expected_notary_.str());
    EXPECT_EQ(widget.NotaryName(), expected_notary_name_);
    EXPECT_EQ(widget.SyncPercentage(), 100);

    constexpr auto progress = std::pair<int, int>{1, 1};

    EXPECT_EQ(widget.SyncProgress(), progress);
    EXPECT_EQ(widget.Type(), expected_account_type_);
    EXPECT_EQ(widget.Unit(), expected_unit_type_);

    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->Amount(), 10000000000);
    EXPECT_EQ(row->Contacts().size(), 0);
    EXPECT_EQ(row->DisplayAmount(), u8"100 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Incoming Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_EQ(row->UUID(), transactions_.at(0)->asHex());
    EXPECT_TRUE(row->Last());
}

TEST_F(Regtest_payment_code, send_to_bob)
{
    account_activity_alice_.expected_ += 2;
    const auto& network = client_1_.Blockchain().GetChain(test_chain_);
    auto future = network.SendToPaymentCode(
        alice_.ID(),
        client_1_.Factory().PaymentCode(vectors_3_.bob_.payment_code_),
        1000000000,
        memo_outgoing_);
    const auto& txid = transactions_.emplace_back(future.get());

    EXPECT_FALSE(txid->empty());
}

TEST_F(Regtest_payment_code, alice_after_unconfirmed_spend_wallet)
{
    const auto& network = client_1_.Blockchain().GetChain(test_chain_);
    const auto& wallet = network.Wallet();
    const auto& nym = alice_.ID();
    const auto& accountHD = SendHD().ID();
    const auto& accountPC = SendPC().ID();
    const auto blankNym = client_1_.Factory().NymID();
    const auto blankAccount = client_1_.Factory().Identifier();
    using Balance = ot::blockchain::Balance;
    const auto balance = Balance{10000000000, 8999999684};
    const auto noBalance = Balance{0, 0};
    // const auto outpointsUnconfirmedNew = [&] {
    //     auto out = std::vector<Outpoint>{};
    //     const auto& txid = transactions_.at(1);
    //     out.emplace_back(txid->Bytes(), 1);
    //
    //     return out;
    // }();
    // const auto outpointsUnconfirmedSpend = [&] {
    //     auto out = std::vector<Outpoint>{};
    //     const auto& txid = transactions_.at(0);
    //     out.emplace_back(txid->Bytes(), 0);
    //
    //     return out;
    // }();
    // const auto keysUnconfirmedNew = [&] {
    //     auto out = std::vector<ot::OTData>{};
    //     const auto& element = SendHD().BalanceElement(Subchain::Internal, 0);
    //     const auto k = element.Key();
    //
    //     OT_ASSERT(k);
    //
    //     out.emplace_back(client_1_.Factory().Data(k->PublicKey()));
    //
    //     return out;
    // }();
    // const auto keysUnconfirmedSpend = [&] {
    //     auto out = std::vector<ot::OTData>{};
    //     const auto& element = SendHD().BalanceElement(Subchain::External,
    //     0u); const auto k = element.Key();
    //
    //     OT_ASSERT(k);
    //
    //     out.emplace_back(client_1_.Factory().Data(k->PublicKey()));
    //
    //     return out;
    // }();
    // const auto testUnconfirmedNew = [&](const auto& utxos) -> bool {
    //     return TestUTXOs(
    //         outpointsUnconfirmedNew,
    //         keysUnconfirmedNew,
    //         utxos,
    //         get_p2ms_bytes_,
    //         get_p2ms_patterns_,
    //         [](const auto index) -> std::int64_t {
    //             const auto amount = ot::blockchain::Amount{8999999684};
    //
    //             return amount;
    //         });
    // };
    // const auto testUnconfirmedSpend = [&](const auto& utxos) -> bool {
    //     return TestUTXOs(
    //         outpointsUnconfirmedSpend,
    //         keysUnconfirmedSpend,
    //         utxos,
    //         get_p2pk_bytes_,
    //         get_p2pk_patterns_,
    //         [](const auto index) -> std::int64_t {
    //             const auto amount = ot::blockchain::Amount{10000000000};
    //
    //             return amount + index;
    //         });
    // };
    //
    // ASSERT_EQ(outpointsUnconfirmedSpend.size(), 1u);
    // ASSERT_EQ(keysUnconfirmedSpend.size(), 1u);
    // ASSERT_EQ(outpointsUnconfirmedNew.size(), 1u);
    // ASSERT_EQ(keysUnconfirmedNew.size(), 1u);

    EXPECT_EQ(wallet.GetBalance(), balance);
    EXPECT_EQ(network.GetBalance(), balance);
    EXPECT_EQ(wallet.GetBalance(nym), balance);
    EXPECT_EQ(network.GetBalance(nym), balance);
    EXPECT_EQ(wallet.GetBalance(nym, accountHD), balance);
    EXPECT_EQ(wallet.GetBalance(nym, accountPC), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym), noBalance);
    EXPECT_EQ(network.GetBalance(blankNym), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym, blankAccount), noBalance);
    EXPECT_EQ(wallet.GetBalance(nym, blankAccount), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym, accountHD), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym, accountPC), noBalance);

    using TxoState = ot::blockchain::client::Wallet::TxoState;
    auto type = TxoState::All;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 2u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 2u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 2u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    type = TxoState::UnconfirmedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    // EXPECT_TRUE(testUnconfirmedNew(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testUnconfirmedNew(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testUnconfirmedNew(wallet.GetOutputs(nym, accountHD, type)));

    type = TxoState::UnconfirmedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    // EXPECT_TRUE(testUnconfirmedSpend(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testUnconfirmedSpend(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testUnconfirmedSpend(wallet.GetOutputs(nym, accountHD,
    // type)));

    type = TxoState::ConfirmedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    type = TxoState::ConfirmedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    type = TxoState::OrphanedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    type = TxoState::OrphanedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);
}

TEST_F(Regtest_payment_code, alice_after_send)
{
    wait_for_counter(account_activity_alice_);
    const auto& widget =
        client_1_.UI().AccountActivity(alice_.ID(), expected_account_);

    EXPECT_EQ(widget.AccountID(), expected_account_.str());
    EXPECT_EQ(widget.Balance(), 8999999684);
    EXPECT_EQ(widget.BalancePolarity(), 1);
    EXPECT_EQ(widget.ContractID(), expected_unit_.str());
    EXPECT_FALSE(widget.DepositAddress().empty());
    EXPECT_FALSE(widget.DepositAddress(test_chain_).empty());
    EXPECT_TRUE(widget.DepositAddress(ot::blockchain::Type::Bitcoin).empty());

    const auto deposit = widget.DepositChains();

    ASSERT_EQ(deposit.size(), 1);
    EXPECT_EQ(deposit.at(0), test_chain_);
    EXPECT_EQ(widget.DisplayBalance(), u8"89.999 996 84 units");
    EXPECT_EQ(widget.DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(widget.Name(), expected_account_name_);
    EXPECT_EQ(widget.NotaryID(), expected_notary_.str());
    EXPECT_EQ(widget.NotaryName(), expected_notary_name_);
    EXPECT_EQ(widget.SyncPercentage(), 100);

    constexpr auto progress = std::pair<int, int>{1, 1};

    EXPECT_EQ(widget.SyncProgress(), progress);
    EXPECT_EQ(widget.Type(), expected_account_type_);
    EXPECT_EQ(widget.Unit(), expected_unit_type_);

    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->Amount(), -1000000316);
    EXPECT_EQ(row->DisplayAmount(), u8"-10.000 003 16 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Outgoing Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_EQ(row->UUID(), transactions_.at(1)->asHex());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_EQ(row->Amount(), 10000000000);
    EXPECT_EQ(row->Contacts().size(), 0);
    EXPECT_EQ(row->DisplayAmount(), u8"100 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Incoming Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_EQ(row->UUID(), transactions_.at(0)->asHex());
    EXPECT_TRUE(row->Last());
}

TEST_F(Regtest_payment_code, first_outgoing_transaction)
{
    const auto& txid = transactions_.at(1).get();
    const auto& pTX = client_1_.Blockchain().LoadTransactionBitcoin(txid);

    ASSERT_TRUE(pTX);

    const auto& tx = *pTX;

    ASSERT_EQ(tx.Inputs().size(), 1u);

    const auto& script = tx.Inputs().at(0u).Script();

    ASSERT_EQ(script.size(), 1u);

    const auto& sig = script.at(0u);

    ASSERT_TRUE(sig.data_.has_value());
    EXPECT_GE(sig.data_.value().size(), 70u);
    EXPECT_LE(sig.data_.value().size(), 74u);
}

TEST_F(Regtest_payment_code, confirm_send)
{
    auto future1 = listener_alice_.get_future(SendHD(), Subchain::External, 2);
    auto future2 = listener_alice_.get_future(SendHD(), Subchain::Internal, 2);
    account_activity_alice_.expected_ += 2;
    account_activity_bob_.expected_ += 2;
    const auto& txid = transactions_.at(1).get();
    const auto extra = [&] {
        auto output = std::vector<Transaction>{};
        const auto& pTX = output.emplace_back(
            client_1_.Blockchain().LoadTransactionBitcoin(txid));

        OT_ASSERT(pTX);

        return output;
    }();

    EXPECT_TRUE(Mine(1, 1, default_, extra));
    EXPECT_TRUE(listener_alice_.wait(future1));
    EXPECT_TRUE(listener_alice_.wait(future2));
}

TEST_F(Regtest_payment_code, second_block)
{
    const auto& blockchain = client_1_.Blockchain().GetChain(test_chain_);
    const auto blockHash = blockchain.HeaderOracle().BestHash(2);
    auto expected = std::vector<ot::Space>{};

    ASSERT_FALSE(blockHash->empty());

    const auto pBlock = blockchain.BlockOracle().LoadBitcoin(blockHash).get();

    ASSERT_TRUE(pBlock);

    const auto& block = *pBlock;

    ASSERT_EQ(block.size(), 2);

    {
        const auto pTx = block.at(0);

        ASSERT_TRUE(pTx);

        const auto& tx = *pTx;
        expected.emplace_back(ot::space(tx.ID().Bytes()));

        EXPECT_EQ(tx.BlockPosition(), 0);
        EXPECT_EQ(tx.Outputs().size(), 1);
    }

    {
        const auto pTx = block.at(1);

        ASSERT_TRUE(pTx);

        const auto& tx = *pTx;
        expected.emplace_back(ot::space(tx.ID().Bytes()));

        EXPECT_EQ(tx.ID(), transactions_.at(1));
        EXPECT_EQ(tx.BlockPosition(), 1);

        ASSERT_EQ(tx.Inputs().size(), 1);

        {
            const auto& input = tx.Inputs().at(0);
            expected.emplace_back(ot::space(input.PreviousOutput().Bytes()));
        }

        ASSERT_EQ(tx.Outputs().size(), 2);

        {
            const auto& output = tx.Outputs().at(0);
            const auto& script = output.Script();

            ASSERT_EQ(script.Type(), Pattern::PayToPubkey);

            const auto bytes = script.Pubkey();

            ASSERT_TRUE(bytes.has_value());

            expected.emplace_back(ot::space(bytes.value()));
        }
        {
            const auto& output = tx.Outputs().at(1);
            const auto& script = output.Script();

            ASSERT_EQ(script.Type(), Pattern::PayToMultisig);

            for (auto i{0u}; i < 3u; ++i) {
                const auto bytes = script.MultisigPubkey(i);

                ASSERT_TRUE(bytes.has_value());

                expected.emplace_back(ot::space(bytes.value()));
            }
        }
    }

    {
        auto elements = block.ExtractElements(FilterType::ES);
        std::sort(elements.begin(), elements.end());
        std::sort(expected.begin(), expected.end());

        EXPECT_EQ(expected.size(), 7);
        EXPECT_EQ(elements.size(), expected.size());
        EXPECT_EQ(elements, expected);
    }

    const auto pFilter =
        blockchain.FilterOracle().LoadFilter(FilterType::ES, blockHash);

    ASSERT_TRUE(pFilter);

    const auto& filter = *pFilter;

    for (const auto& element : expected) {
        EXPECT_TRUE(filter.Test(ot::reader(element)));
    }
}

TEST_F(Regtest_payment_code, alice_after_confirmed_spend_wallet)
{
    const auto& network = client_1_.Blockchain().GetChain(test_chain_);
    const auto& wallet = network.Wallet();
    const auto& nym = alice_.ID();
    const auto& accountHD = SendHD().ID();
    const auto& accountPC = SendPC().ID();
    const auto blankNym = client_1_.Factory().NymID();
    const auto blankAccount = client_1_.Factory().Identifier();
    using Balance = ot::blockchain::Balance;
    const auto balance = Balance{8999999684, 8999999684};
    const auto noBalance = Balance{0, 0};
    // const auto outpointsConfirmedNew = [&] {
    //     auto out = std::vector<Outpoint>{};
    //     const auto& txid = transactions_.at(1);
    //     out.emplace_back(txid->Bytes(), 1);
    //
    //     return out;
    // }();
    // const auto outpointsConfirmedSpend = [&] {
    //     auto out = std::vector<Outpoint>{};
    //     const auto& txid = transactions_.at(0);
    //     out.emplace_back(txid->Bytes(), 0);
    //
    //     return out;
    // }();
    // const auto keysConfirmedNew = [&] {
    //     auto out = std::vector<ot::OTData>{};
    //     const auto& element = SendHD().BalanceElement(Subchain::Internal, 0);
    //     const auto k = element.Key();
    //
    //     OT_ASSERT(k);
    //
    //     out.emplace_back(client_1_.Factory().Data(k->PublicKey()));
    //
    //     return out;
    // }();
    // const auto keysConfirmedSpend = [&] {
    //     auto out = std::vector<ot::OTData>{};
    //     const auto& element = SendHD().BalanceElement(Subchain::External,
    //     0u); const auto k = element.Key();
    //
    //     OT_ASSERT(k);
    //
    //     out.emplace_back(client_1_.Factory().Data(k->PublicKey()));
    //
    //     return out;
    // }();
    // const auto testConfirmedNew = [&](const auto& utxos) -> bool {
    //     return TestUTXOs(
    //         outpointsConfirmedNew,
    //         keysConfirmedNew,
    //         utxos,
    //         get_p2ms_bytes_,
    //         get_p2ms_patterns_,
    //         [](const auto index) -> std::int64_t {
    //             const auto amount = ot::blockchain::Amount{8999999684};
    //
    //             return amount;
    //         });
    // };
    // const auto testConfirmedSpend = [&](const auto& utxos) -> bool {
    //     return TestUTXOs(
    //         outpointsConfirmedSpend,
    //         keysConfirmedSpend,
    //         utxos,
    //         get_p2pk_bytes_,
    //         get_p2pk_patterns_,
    //         [](const auto index) -> std::int64_t {
    //             const auto amount = ot::blockchain::Amount{10000000000};
    //
    //             return amount + index;
    //         });
    // };

    // ASSERT_EQ(outpointsConfirmedSpend.size(), 1u);
    // ASSERT_EQ(keysConfirmedSpend.size(), 1u);
    // ASSERT_EQ(outpointsConfirmedNew.size(), 1u);
    // ASSERT_EQ(keysConfirmedNew.size(), 1u);

    EXPECT_EQ(wallet.GetBalance(), balance);
    EXPECT_EQ(network.GetBalance(), balance);
    EXPECT_EQ(wallet.GetBalance(nym), balance);
    EXPECT_EQ(network.GetBalance(nym), balance);
    EXPECT_EQ(wallet.GetBalance(nym, accountHD), balance);
    EXPECT_EQ(wallet.GetBalance(nym, accountPC), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym), noBalance);
    EXPECT_EQ(network.GetBalance(blankNym), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym, blankAccount), noBalance);
    EXPECT_EQ(wallet.GetBalance(nym, blankAccount), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym, accountHD), noBalance);
    EXPECT_EQ(wallet.GetBalance(blankNym, accountPC), noBalance);

    using TxoState = ot::blockchain::client::Wallet::TxoState;
    auto type = TxoState::All;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 2u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 2u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 2u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    type = TxoState::UnconfirmedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    type = TxoState::UnconfirmedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    type = TxoState::ConfirmedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testConfirmedNew(wallet.GetOutputs(nym, accountHD, type)));

    type = TxoState::ConfirmedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 1u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    // EXPECT_TRUE(testConfirmedSpend(wallet.GetOutputs(type)));
    // EXPECT_TRUE(testConfirmedSpend(wallet.GetOutputs(nym, type)));
    // EXPECT_TRUE(testConfirmedSpend(wallet.GetOutputs(nym, accountHD, type)));

    type = TxoState::OrphanedNew;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);

    type = TxoState::OrphanedSpend;

    EXPECT_EQ(wallet.GetOutputs(type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, accountPC, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(nym, blankAccount, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountHD, type).size(), 0u);
    EXPECT_EQ(wallet.GetOutputs(blankNym, accountPC, type).size(), 0u);
}

TEST_F(Regtest_payment_code, bob_after_receive)
{
    wait_for_counter(account_activity_bob_);
    const auto& widget =
        client_2_.UI().AccountActivity(bob_.ID(), expected_account_);

    EXPECT_EQ(widget.AccountID(), expected_account_.str());
    EXPECT_EQ(widget.Balance(), 1000000000);
    EXPECT_EQ(widget.BalancePolarity(), 1);
    EXPECT_EQ(widget.ContractID(), expected_unit_.str());
    EXPECT_FALSE(widget.DepositAddress().empty());
    EXPECT_FALSE(widget.DepositAddress(test_chain_).empty());
    EXPECT_TRUE(widget.DepositAddress(ot::blockchain::Type::Bitcoin).empty());

    const auto deposit = widget.DepositChains();

    ASSERT_EQ(deposit.size(), 1);
    EXPECT_EQ(deposit.at(0), test_chain_);
    EXPECT_EQ(widget.DisplayBalance(), u8"10 units");
    EXPECT_EQ(widget.DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(widget.Name(), expected_account_name_);
    EXPECT_EQ(widget.NotaryID(), expected_notary_.str());
    EXPECT_EQ(widget.NotaryName(), expected_notary_name_);
    EXPECT_EQ(widget.SyncPercentage(), 100);

    constexpr auto progress = std::pair<int, int>{2, 2};

    EXPECT_EQ(widget.SyncProgress(), progress);
    EXPECT_EQ(widget.Type(), expected_account_type_);
    EXPECT_EQ(widget.Unit(), expected_unit_type_);

    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->Amount(), 1000000000);
    /*
    ASSERT_EQ(row->Contacts().size(), 1);

    const auto contacts = row->Contacts();
    const auto& contact = contacts.front();

    EXPECT_FALSE(contact.empty());
    // TODO verify contact id
    */
    EXPECT_EQ(row->DisplayAmount(), u8"10 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Incoming Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_EQ(row->UUID(), transactions_.at(1)->asHex());
    EXPECT_TRUE(row->Last());

    const auto& tree = client_2_.Blockchain().Account(bob_.ID(), test_chain_);
    const auto& pc = tree.GetPaymentCode();

    ASSERT_EQ(pc.size(), 1);

    const auto& account = pc.at(0);
    const auto lookahead = account.Lookahead() - 1u;

    EXPECT_EQ(account.Type(), bca::BalanceNodeType::PaymentCode);
    EXPECT_FALSE(account.IsNotified());

    {
        constexpr auto subchain{Subchain::Incoming};

        const auto gen = account.LastGenerated(subchain);

        ASSERT_TRUE(gen.has_value());
        EXPECT_EQ(gen.value(), lookahead);
    }
    {
        constexpr auto subchain{Subchain::Outgoing};

        const auto gen = account.LastGenerated(subchain);

        ASSERT_TRUE(gen.has_value());
        EXPECT_EQ(gen.value(), lookahead);
    }
    {
        constexpr auto subchain{Subchain::External};

        const auto gen = account.LastGenerated(subchain);

        EXPECT_FALSE(gen.has_value());
    }
    {
        constexpr auto subchain{Subchain::Internal};

        const auto gen = account.LastGenerated(subchain);

        EXPECT_FALSE(gen.has_value());
    }
}

TEST_F(Regtest_payment_code, alice_after_confirm)
{
    wait_for_counter(account_activity_alice_, false);
    const auto& widget =
        client_1_.UI().AccountActivity(alice_.ID(), expected_account_);

    EXPECT_EQ(widget.AccountID(), expected_account_.str());
    EXPECT_EQ(widget.Balance(), 8999999684);
    EXPECT_EQ(widget.BalancePolarity(), 1);
    EXPECT_EQ(widget.ContractID(), expected_unit_.str());
    EXPECT_FALSE(widget.DepositAddress().empty());
    EXPECT_FALSE(widget.DepositAddress(test_chain_).empty());
    EXPECT_TRUE(widget.DepositAddress(ot::blockchain::Type::Bitcoin).empty());

    const auto deposit = widget.DepositChains();

    ASSERT_EQ(deposit.size(), 1);
    EXPECT_EQ(deposit.at(0), test_chain_);
    EXPECT_EQ(widget.DisplayBalance(), u8"89.999 996 84 units");
    EXPECT_EQ(widget.DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(widget.Name(), expected_account_name_);
    EXPECT_EQ(widget.NotaryID(), expected_notary_.str());
    EXPECT_EQ(widget.NotaryName(), expected_notary_name_);
    EXPECT_EQ(widget.SyncPercentage(), 100);

    constexpr auto progress = std::pair<int, int>{2, 2};

    EXPECT_EQ(widget.SyncProgress(), progress);
    EXPECT_EQ(widget.Type(), expected_account_type_);
    EXPECT_EQ(widget.Unit(), expected_unit_type_);

    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->Amount(), -1000000316);
    EXPECT_EQ(row->Contacts().size(), 0);
    EXPECT_EQ(row->DisplayAmount(), u8"-10.000 003 16 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Outgoing Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_EQ(row->UUID(), transactions_.at(1)->asHex());
    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_EQ(row->Amount(), 10000000000);
    EXPECT_EQ(row->Contacts().size(), 0);
    EXPECT_EQ(row->DisplayAmount(), u8"100 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Incoming Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_EQ(row->UUID(), transactions_.at(0)->asHex());
    EXPECT_TRUE(row->Last());

    const auto& tree = client_1_.Blockchain().Account(alice_.ID(), test_chain_);
    const auto& pc = tree.GetPaymentCode();

    ASSERT_EQ(pc.size(), 1);

    const auto& account = pc.at(0);
    const auto lookahead = account.Lookahead() - 1;

    EXPECT_EQ(account.Type(), bca::BalanceNodeType::PaymentCode);
    EXPECT_TRUE(account.IsNotified());

    {
        constexpr auto subchain{Subchain::Incoming};

        const auto gen = account.LastGenerated(subchain);

        ASSERT_TRUE(gen.has_value());
        EXPECT_EQ(gen.value(), lookahead);
    }
    {
        constexpr auto subchain{Subchain::Outgoing};

        const auto gen = account.LastGenerated(subchain);

        ASSERT_TRUE(gen.has_value());
        EXPECT_EQ(gen.value(), lookahead);
    }
    {
        constexpr auto subchain{Subchain::External};

        const auto gen = account.LastGenerated(subchain);

        EXPECT_FALSE(gen.has_value());
    }
    {
        constexpr auto subchain{Subchain::Internal};

        const auto gen = account.LastGenerated(subchain);

        EXPECT_FALSE(gen.has_value());
    }
}

TEST_F(Regtest_payment_code, second_send_to_bob)
{
    const auto& widget =
        client_1_.UI().AccountActivity(alice_.ID(), expected_account_);
    account_activity_alice_.expected_ += 2;
    const auto sent =
        widget.Send(vectors_3_.bob_.payment_code_, 1500000000, memo_outgoing_);

    EXPECT_TRUE(sent);
}

TEST_F(Regtest_payment_code, alice_after_second_send)
{
    wait_for_counter(account_activity_alice_);
    const auto& widget =
        client_1_.UI().AccountActivity(alice_.ID(), expected_account_);

    EXPECT_EQ(widget.AccountID(), expected_account_.str());
    EXPECT_EQ(widget.Balance(), 7499999448);
    EXPECT_EQ(widget.BalancePolarity(), 1);
    EXPECT_EQ(widget.ContractID(), expected_unit_.str());
    EXPECT_FALSE(widget.DepositAddress().empty());
    EXPECT_FALSE(widget.DepositAddress(test_chain_).empty());
    EXPECT_TRUE(widget.DepositAddress(ot::blockchain::Type::Bitcoin).empty());

    const auto deposit = widget.DepositChains();

    ASSERT_EQ(deposit.size(), 1);
    EXPECT_EQ(deposit.at(0), test_chain_);
    EXPECT_EQ(widget.DisplayBalance(), u8"74.999 994 48 units");
    EXPECT_EQ(widget.DisplayUnit(), expected_display_unit_);
    EXPECT_EQ(widget.Name(), expected_account_name_);
    EXPECT_EQ(widget.NotaryID(), expected_notary_.str());
    EXPECT_EQ(widget.NotaryName(), expected_notary_name_);
    EXPECT_EQ(widget.SyncPercentage(), 100);

    constexpr auto progress = std::pair<int, int>{2, 2};

    EXPECT_EQ(widget.SyncProgress(), progress);
    EXPECT_EQ(widget.Type(), expected_account_type_);
    EXPECT_EQ(widget.Unit(), expected_unit_type_);

    auto row = widget.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_EQ(row->Amount(), -1500000236);
    EXPECT_EQ(row->DisplayAmount(), u8"-15.000 002 36 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Outgoing Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);

    transactions_.emplace_back(
        client_1_.Factory().Data(row->UUID(), ot::StringStyle::Hex));

    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_EQ(row->Amount(), -1000000316);
    EXPECT_EQ(row->Contacts().size(), 0);
    EXPECT_EQ(row->DisplayAmount(), u8"-10.000 003 16 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Outgoing Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_EQ(row->UUID(), transactions_.at(1)->asHex());

    ASSERT_FALSE(row->Last());

    row = widget.Next();

    EXPECT_EQ(row->Amount(), 10000000000);
    EXPECT_EQ(row->Contacts().size(), 0);
    EXPECT_EQ(row->DisplayAmount(), u8"100 units");
    EXPECT_EQ(row->Memo(), "");
    EXPECT_EQ(row->Workflow(), "");
    EXPECT_EQ(row->Text(), "Incoming Unit Test Simulation transaction");
    EXPECT_EQ(row->Type(), ot::StorageBox::BLOCKCHAIN);
    EXPECT_EQ(row->UUID(), transactions_.at(0)->asHex());
    EXPECT_TRUE(row->Last());

    const auto& tree = client_1_.Blockchain().Account(alice_.ID(), test_chain_);
    const auto& pc = tree.GetPaymentCode();

    ASSERT_EQ(pc.size(), 1);

    const auto& account = pc.at(0);
    const auto lookahead = account.Lookahead() - 1u;

    EXPECT_EQ(account.Type(), bca::BalanceNodeType::PaymentCode);
    EXPECT_TRUE(account.IsNotified());

    {
        constexpr auto subchain{Subchain::Incoming};

        const auto gen = account.LastGenerated(subchain);

        ASSERT_TRUE(gen.has_value());
        EXPECT_EQ(gen.value(), lookahead);
    }
    {
        constexpr auto subchain{Subchain::Outgoing};

        const auto gen = account.LastGenerated(subchain);

        ASSERT_TRUE(gen.has_value());
        EXPECT_EQ(gen.value(), lookahead);
    }
    {
        constexpr auto subchain{Subchain::External};

        const auto gen = account.LastGenerated(subchain);

        EXPECT_FALSE(gen.has_value());
    }
    {
        constexpr auto subchain{Subchain::Internal};

        const auto gen = account.LastGenerated(subchain);

        EXPECT_FALSE(gen.has_value());
    }
}

TEST_F(Regtest_payment_code, second_outgoing_transaction)
{
    const auto& txid = transactions_.at(2).get();
    const auto& pTX = client_1_.Blockchain().LoadTransactionBitcoin(txid);

    ASSERT_TRUE(pTX);

    const auto& tx = *pTX;

    ASSERT_EQ(tx.Inputs().size(), 1u);

    const auto& script = tx.Inputs().at(0u).Script();

    ASSERT_EQ(script.size(), 2u);

    const auto& placeholder = script.at(0u);
    using OP = ot::blockchain::block::bitcoin::OP;

    EXPECT_EQ(placeholder.opcode_, OP::ZERO);

    const auto& sig = script.at(1u);

    ASSERT_TRUE(sig.data_.has_value());
    EXPECT_GE(sig.data_.value().size(), 70u);
    EXPECT_LE(sig.data_.value().size(), 74u);
}

TEST_F(Regtest_payment_code, shutdown) { Shutdown(); }
}  // namespace
