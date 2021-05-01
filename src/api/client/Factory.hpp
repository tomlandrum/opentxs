// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "api/Factory.hpp"
#include "internal/api/Api.hpp"
#include "internal/api/crypto/Crypto.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Primitives.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/blockchain/Blockchain.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/Types.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/blockchain/p2p/Address.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Item.hpp"
#include "opentxs/core/Ledger.hpp"
#include "opentxs/core/OTTransaction.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/CurrencyContract.hpp"
#include "opentxs/core/contract/SecurityContract.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/contract/basket/BasketContract.hpp"
#include "opentxs/core/contract/peer/BailmentNotice.hpp"
#include "opentxs/core/contract/peer/BailmentReply.hpp"
#include "opentxs/core/contract/peer/BailmentRequest.hpp"
#include "opentxs/core/contract/peer/ConnectionReply.hpp"
#include "opentxs/core/contract/peer/ConnectionRequest.hpp"
#include "opentxs/core/contract/peer/NoticeAcknowledgement.hpp"
#include "opentxs/core/contract/peer/OutBailmentReply.hpp"
#include "opentxs/core/contract/peer/OutBailmentRequest.hpp"
#include "opentxs/core/contract/peer/PeerReply.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/core/contract/peer/StoreSecret.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/crypto/Envelope.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/network/zeromq/Pipeline.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/PeerEnums.pb.h"

namespace opentxs
{
namespace api
{
namespace client
{
namespace internal
{
struct Manager;
}  // namespace internal
}  // namespace client

namespace internal
{
struct Factory;
}  // namespace internal
}  // namespace api

namespace blind
{
class Purse;
}  // namespace blind

namespace blockchain
{
namespace block
{
namespace bitcoin
{
class Block;
class Transaction;
}  // namespace bitcoin

class Block;
class Header;
}  // namespace block
}  // namespace blockchain

namespace proto
{
class BlockchainBlockHeader;
class PeerObject;
}  // namespace proto

class Armored;
class Data;
class PasswordPrompt;
class PeerObject;
}  // namespace opentxs

namespace opentxs::api::client::implementation
{
class Factory final : public opentxs::api::implementation::Factory
{
public:
#if OT_BLOCKCHAIN
    auto BitcoinBlock(
        const opentxs::blockchain::Type chain,
        const ReadView bytes) const noexcept
        -> std::shared_ptr<
            const opentxs::blockchain::block::bitcoin::Block> final;
    auto BitcoinBlock(
        const opentxs::blockchain::block::Header& previous,
        const Transaction_p generationTransaction,
        const std::uint32_t nBits,
        const std::vector<Transaction_p>& extraTransactions,
        const std::int32_t version,
        const AbortFunction abort) const noexcept
        -> std::shared_ptr<
            const opentxs::blockchain::block::bitcoin::Block> final;
    auto BitcoinGenerationTransaction(
        const opentxs::blockchain::Type chain,
        const opentxs::blockchain::block::Height height,
        std::vector<OutputBuilder>&& outputs,
        const std::string& coinbase,
        const std::int32_t version) const noexcept -> Transaction_p final;
    auto BitcoinTransaction(
        const opentxs::blockchain::Type chain,
        const ReadView bytes,
        const bool isGeneration) const noexcept
        -> std::unique_ptr<
            const opentxs::blockchain::block::bitcoin::Transaction> final;
    auto BlockHeader(const proto::BlockchainBlockHeader& serialized) const
        -> BlockHeaderP final;
    auto BlockHeader(const ReadView& serialized) const -> BlockHeaderP final;
    auto BlockHeader(
        const opentxs::blockchain::Type type,
        const opentxs::Data& raw) const -> BlockHeaderP final;
    auto BlockHeader(const opentxs::blockchain::block::Block& block) const
        -> BlockHeaderP final;
    auto BlockHeaderForUnitTests(
        const opentxs::blockchain::block::Hash& hash,
        const opentxs::blockchain::block::Hash& parent,
        const opentxs::blockchain::block::Height height) const
        -> BlockHeaderP final;
#endif  // OT_BLOCKCHAIN
    auto PeerObject(const Nym_p& senderNym, const std::string& message) const
        -> std::unique_ptr<opentxs::PeerObject> final;
    auto PeerObject(
        const Nym_p& senderNym,
        const std::string& payment,
        const bool isPayment) const
        -> std::unique_ptr<opentxs::PeerObject> final;
#if OT_CASH
    auto PeerObject(
        const Nym_p& senderNym,
        const std::shared_ptr<blind::Purse> purse) const
        -> std::unique_ptr<opentxs::PeerObject> final;
#endif
    auto PeerObject(
        const OTPeerRequest request,
        const OTPeerReply reply,
        const VersionNumber version) const
        -> std::unique_ptr<opentxs::PeerObject> final;
    auto PeerObject(const OTPeerRequest request, const VersionNumber version)
        const -> std::unique_ptr<opentxs::PeerObject> final;
    auto PeerObject(const Nym_p& signerNym, const proto::PeerObject& serialized)
        const -> std::unique_ptr<opentxs::PeerObject> final;
    auto PeerObject(
        const Nym_p& recipientNym,
        const opentxs::Armored& encrypted,
        const opentxs::PasswordPrompt& reason) const
        -> std::unique_ptr<opentxs::PeerObject> final;

    Factory(const api::client::internal::Manager& client);

    ~Factory() final = default;

private:
    const api::client::internal::Manager& client_;

    Factory() = delete;
    Factory(const Factory&) = delete;
    Factory(Factory&&) = delete;
    auto operator=(const Factory&) -> Factory& = delete;
    auto operator=(Factory&&) -> Factory& = delete;
};
}  // namespace opentxs::api::client::implementation
