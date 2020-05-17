// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "blockchain/p2p/bitcoin/message/Merkleblock.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>

#include "Factory.hpp"
#include "blockchain/p2p/bitcoin/Header.hpp"
#include "internal/blockchain/bitcoin/Bitcoin.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"

//#define OT_METHOD " opentxs::blockchain::p2p::bitcoin::message::Merkleblock::"

namespace opentxs
{
// We have a header and a raw payload. Parse it.
auto Factory::BitcoinP2PMerkleblock(
    const api::internal::Core& api,
    std::unique_ptr<blockchain::p2p::bitcoin::Header> pHeader,
    const blockchain::p2p::bitcoin::ProtocolVersion version,
    const void* payload,
    const std::size_t size) -> blockchain::p2p::bitcoin::message::Merkleblock*
{
    namespace bitcoin = blockchain::p2p::bitcoin;
    using ReturnType = bitcoin::message::Merkleblock;

    if (false == bool(pHeader)) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": Invalid header")
            .Flush();

        return nullptr;
    }

    auto raw_item = ReturnType::Raw{};

    auto expectedSize = sizeof(raw_item);

    if (expectedSize > size) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Size below minimum for Merkleblock 1")
            .Flush();

        return nullptr;
    }
    auto* it{static_cast<const std::byte*>(payload)};
    // --------------------------------------------------------
    std::memcpy(static_cast<void*>(&raw_item), it, sizeof(raw_item));
    it += sizeof(raw_item);

    const auto block_header = Data::Factory(
        raw_item.block_header_.data(), raw_item.block_header_.size());
    const bitcoin::TxnCount txn_count = raw_item.txn_count_.value();
    // --------------------------------------------------------
    expectedSize += sizeof(std::byte);

    if (expectedSize > size) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Size below minimum for Merkleblock 1")
            .Flush();

        return nullptr;
    }

    std::size_t hashCount{0};
    const bool decodedSize = blockchain::bitcoin::DecodeCompactSizeFromPayload(
        it, expectedSize, size, hashCount);

    if (!decodedSize) {
        LogOutput(__FUNCTION__)(": CompactSize incomplete").Flush();

        return nullptr;
    }

    std::vector<OTData> hashes;

    if (hashCount > 0) {
        for (std::size_t ii = 0; ii < hashCount; ii++) {
            expectedSize += sizeof(bitcoin::BlockHeaderHashField);

            if (expectedSize > size) {
                LogOutput("opentxs::Factory::")(__FUNCTION__)(
                    ": Hash entries incomplete at entry index ")(ii)
                    .Flush();

                return nullptr;
            }

            hashes.push_back(
                Data::Factory(it, sizeof(bitcoin::BlockHeaderHashField)));
            it += sizeof(bitcoin::BlockHeaderHashField);
        }
    }
    // --------------------------------------------------------
    expectedSize += sizeof(std::byte);

    if (expectedSize > size) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Size below minimum for Merkleblock 1")
            .Flush();

        return nullptr;
    }

    std::size_t flagByteCount{0};
    const bool decodedFlagSize =
        blockchain::bitcoin::DecodeCompactSizeFromPayload(
            it, expectedSize, size, flagByteCount);

    if (!decodedFlagSize) {
        LogOutput(__FUNCTION__)(": CompactSize incomplete").Flush();

        return nullptr;
    }

    std::vector<std::byte> flags;

    if (flagByteCount > 0) {
        expectedSize += flagByteCount;

        if (expectedSize > size) {
            LogOutput("opentxs::Factory::")(__FUNCTION__)(
                ": Flag field incomplete")
                .Flush();

            return nullptr;
        }

        std::vector<std::byte> temp_flags(flagByteCount);
        std::memcpy(temp_flags.data(), it, flagByteCount);
        it += flagByteCount;
        flags = temp_flags;
    }
    // --------------------------------------------------------
    try {
        return new ReturnType(
            api, std::move(pHeader), block_header, txn_count, hashes, flags);
    } catch (...) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": Checksum failure")
            .Flush();

        return nullptr;
    }
}

// We have all the data members to create the message from scratch (for sending)
auto Factory::BitcoinP2PMerkleblock(
    const api::internal::Core& api,
    const blockchain::Type network,
    const Data& block_header,
    const std::uint32_t txn_count,
    const std::vector<OTData>& hashes,
    const std::vector<std::byte>& flags)
    -> blockchain::p2p::bitcoin::message::Merkleblock*
{
    namespace bitcoin = blockchain::p2p::bitcoin;
    using ReturnType = bitcoin::message::Merkleblock;

    return new ReturnType(api, network, block_header, txn_count, hashes, flags);
}
}  // namespace opentxs

namespace opentxs::blockchain::p2p::bitcoin::message
{

auto Merkleblock::payload() const noexcept -> OTData
{
    try {
        Raw raw_data(block_header_, txn_count_);
        auto output = Data::Factory(&raw_data, sizeof(raw_data));
        const auto hashSize = CompactSize(hashes_.size()).Encode();
        output->Concatenate(hashSize.data(), hashSize.size());
        for (const auto& hash_data : hashes_) { output += hash_data; }
        const auto flagSize = CompactSize(flags_.size()).Encode();
        output->Concatenate(flagSize.data(), flagSize.size());
        output->Concatenate(flags_.data(), flags_.size());

        return output;
    } catch (...) {
        return Data::Factory();
    }
}

Merkleblock::Raw::Raw(
    const Data& block_header,
    const TxnCount txn_count) noexcept
    : block_header_()
    , txn_count_(txn_count)
{
    OT_ASSERT(sizeof(block_header_) == block_header.size());

    std::memcpy(block_header_.data(), block_header.data(), block_header.size());
}

Merkleblock::Raw::Raw() noexcept
    : block_header_()
    , txn_count_(0)
{
}

// We have all the data members to create the message from scratch (for sending)
Merkleblock::Merkleblock(
    const api::internal::Core& api,
    const blockchain::Type network,
    const Data& block_header,
    const TxnCount txn_count,
    const std::vector<OTData>& hashes,
    const std::vector<std::byte>& flags) noexcept
    : Message(api, network, bitcoin::Command::merkleblock)
    , block_header_(Data::Factory(block_header))
    , txn_count_(txn_count)
    , hashes_(hashes)
    , flags_(flags)
{
    init_hash();
}

// We have a header and the data members. They've been parsed, so now we are
// instantiating the message from them.
Merkleblock::Merkleblock(
    const api::internal::Core& api,
    std::unique_ptr<Header> header,
    const Data& block_header,
    const TxnCount txn_count,
    const std::vector<OTData>& hashes,
    const std::vector<std::byte>& flags) noexcept(false)
    : Message(api, std::move(header))
    , block_header_(Data::Factory(block_header))
    , txn_count_(txn_count)
    , hashes_(hashes)
    , flags_(flags)
{
    verify_checksum();
}

}  // namespace  opentxs::blockchain::p2p::bitcoin::message
