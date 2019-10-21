// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "Reject.hpp"

#include "opentxs/core/crypto/OTPassword.hpp"

#include <boost/endian/buffers.hpp>

//#define OT_METHOD " opentxs::blockchain::p2p::bitcoin::message::Reject::"

namespace be = boost::endian;

namespace opentxs
{
blockchain::p2p::bitcoin::message::Reject* Factory::BitcoinP2PReject(
    const api::internal::Core& api,
    std::unique_ptr<blockchain::p2p::bitcoin::Header> pHeader,
    const blockchain::p2p::bitcoin::ProtocolVersion version,
    const void* payload,
    const std::size_t size)
{
    namespace bitcoin = blockchain::p2p::bitcoin;
    using ReturnType = bitcoin::message::Reject;

    if (false == bool(pHeader)) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": Invalid header")
            .Flush();

        return nullptr;
    }

    auto expectedSize = sizeof(std::byte);

    if (expectedSize > size) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Size below minimum for Reject 1")
            .Flush();

        return nullptr;
    }

    auto* it{static_cast<const std::byte*>(payload)};
    // -----------------------------------------------
    auto messageSize = std::size_t{0};
    const bool decodedSize = blockchain::bitcoin::DecodeCompactSizeFromPayload(
        it, expectedSize, size, messageSize);

    if (!decodedSize) {
        LogOutput(__FUNCTION__)(": CompactSize incomplete for message field")
            .Flush();

        return nullptr;
    }
    // -----------------------------------------------
    expectedSize += messageSize;

    if (expectedSize > size) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Size below minimum for message field")
            .Flush();

        return nullptr;
    }
    const std::string message{reinterpret_cast<const char*>(it), messageSize};
    it += messageSize;
    // -----------------------------------------------
    expectedSize += sizeof(std::uint8_t);

    if (expectedSize > size) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Size below minimum for code field")
            .Flush();

        return nullptr;
    }
    be::little_uint8_buf_t raw_code{};

    OTPassword::safe_memcpy(&raw_code, sizeof(raw_code), it, sizeof(raw_code));
    it += sizeof(raw_code);

    const std::uint8_t code = raw_code.value();
    // -----------------------------------------------
    expectedSize += sizeof(std::byte);

    if (expectedSize > size) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Size below minimum for Reject 1")
            .Flush();

        return nullptr;
    }
    // -----------------------------------------------
    std::size_t reasonSize{0};
    const bool decodedReasonSize =
        blockchain::bitcoin::DecodeCompactSizeFromPayload(
            it, expectedSize, size, reasonSize);

    if (!decodedReasonSize) {
        LogOutput(__FUNCTION__)(": CompactSize incomplete for reason field")
            .Flush();

        return nullptr;
    }
    // -----------------------------------------------
    expectedSize += reasonSize;

    if (expectedSize > size) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(
            ": Size below minimum for reason field")
            .Flush();

        return nullptr;
    }
    const std::string reason{reinterpret_cast<const char*>(it), reasonSize};
    it += reasonSize;
    // -----------------------------------------------
    // This next field is "sometimes there".
    // Sometimes it's there (or not) for a single code!
    // Because the code means different things depending on
    // what got rejected.
    //
    auto extra = Data::Factory();

    // better than hardcoding "32"
    expectedSize += sizeof(bitcoin::BlockHeaderHashField);

    if (expectedSize <= size) {
        extra->Concatenate(it, sizeof(bitcoin::BlockHeaderHashField));
        it += sizeof(bitcoin::BlockHeaderHashField);
    }
    // -----------------------------------------------
    try {
        return new ReturnType(
            api,
            std::move(pHeader),
            message,
            static_cast<bitcoin::RejectCode>(code),
            reason,
            extra);
    } catch (...) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": Checksum failure")
            .Flush();

        return nullptr;
    }
}

blockchain::p2p::bitcoin::message::Reject* Factory::BitcoinP2PReject(
    const api::internal::Core& api,
    const blockchain::Type network,
    const std::string& message,
    const std::uint8_t code,
    const std::string& reason,
    const Data& extra)
{
    namespace bitcoin = blockchain::p2p::bitcoin;
    using ReturnType = bitcoin::message::Reject;

    return new ReturnType(
        api,
        network,
        message,
        static_cast<bitcoin::RejectCode>(code),
        reason,
        extra);
}
}  // namespace opentxs

namespace opentxs::blockchain::p2p::bitcoin::message
{

Reject::Reject(
    const api::internal::Core& api,
    const blockchain::Type network,
    const std::string& message,
    const bitcoin::RejectCode code,
    const std::string& reason,
    const Data& extra) noexcept
    : Message(api, network, bitcoin::Command::reject)
    , message_(message)
    , code_(code)
    , reason_(reason)
    , extra_(Data::Factory(extra))
{
    init_hash();
}

Reject::Reject(
    const api::internal::Core& api,
    std::unique_ptr<Header> header,
    const std::string& message,
    const bitcoin::RejectCode code,
    const std::string& reason,
    const Data& extra) noexcept(false)
    : Message(api, std::move(header))
    , message_(message)
    , code_(code)
    , reason_(reason)
    , extra_(Data::Factory(extra))
{
    verify_checksum();
}

OTData Reject::payload() const noexcept
{
    auto output = Data::Factory(BitcoinString(message_));

    be::little_uint8_buf_t code(static_cast<std::uint8_t>(code_));
    output->Concatenate(&code, sizeof(code));

    output += Data::Factory(BitcoinString(reason_));

    if (false == extra_->empty()) { output += extra_; }

    return output;
}
}  // namespace  opentxs::blockchain::p2p::bitcoin::message
