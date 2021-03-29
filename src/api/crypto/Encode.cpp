// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"           // IWYU pragma: associated
#include "1_Internal.hpp"         // IWYU pragma: associated
#include "api/crypto/Encode.hpp"  // IWYU pragma: associated

#include <cstddef>
#include <limits>
#include <memory>
#include <regex>
#include <sstream>
#include <string_view>
#include <vector>

#include "base58/base58.h"
#include "base64/base64.h"
#include "internal/api/crypto/Factory.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Primitives.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/crypto/HashType.hpp"
#include "opentxs/network/zeromq/Context.hpp"

// #define OT_METHOD opentxs::api::crypto::implementation::Encode::

namespace opentxs::factory
{
auto Encode(const api::Crypto& crypto) noexcept
    -> std::unique_ptr<api::crypto::Encode>
{
    using ReturnType = api::crypto::implementation::Encode;

    return std::make_unique<ReturnType>(crypto);
}
}  // namespace opentxs::factory

namespace opentxs::api::crypto::implementation
{
Encode::Encode(const api::Crypto& crypto) noexcept
    : crypto_(crypto)
{
}

auto Encode::Base64Encode(
    const std::uint8_t* inputStart,
    const std::size_t& size) const -> std::string
{
    auto output = std::string{};

    if (std::numeric_limits<int>::max() < size) { return {}; }

    const auto bytes = static_cast<int>(size);
    output.resize(::Base64encode_len(bytes));
    ::Base64encode(
        const_cast<char*>(output.data()),
        reinterpret_cast<const char*>(inputStart),
        bytes);

    return BreakLines(output);
}

auto Encode::Base64Decode(const std::string&& input, RawData& output) const
    -> bool
{
    output.resize(::Base64decode_len(input.data()), 0x0);

    const size_t decoded =
        ::Base64decode(reinterpret_cast<char*>(output.data()), input.data());

    if (0 == decoded) { return false; }

    OT_ASSERT(decoded <= output.size());

    output.resize(decoded);

    return true;
}

auto Encode::BreakLines(const std::string& input) const -> std::string
{
    std::string output;

    if (0 == input.size()) { return output; }

    std::size_t width = 0;

    for (auto& character : input) {
        output.push_back(character);

        if (++width >= LineWidth) {
            output.push_back('\n');
            width = 0;
        }
    }

    if ('\n' != output.back()) { output.push_back('\n'); }

    return output;
}

auto Encode::DataEncode(const std::string& input) const -> std::string
{
    return Base64Encode(
        reinterpret_cast<const std::uint8_t*>(input.data()), input.size());
}

auto Encode::DataEncode(const Data& input) const -> std::string
{
    return Base64Encode(
        static_cast<const std::uint8_t*>(input.data()), input.size());
}

auto Encode::DataDecode(const std::string& input) const -> std::string
{
    RawData decoded;

    if (Base64Decode(SanatizeBase64(input), decoded)) {

        return std::string(
            reinterpret_cast<const char*>(decoded.data()), decoded.size());
    }

    return "";
}

auto Encode::IdentifierEncode(const void* data, const std::size_t size) const
    -> std::string
{
    if (0 == size) { return {}; }

    auto preimage = Data::Factory(data, size);
    auto checksum = Data::Factory();
    auto hash = crypto_.Hash().Digest(
        opentxs::crypto::HashType::Sha256DC,
        preimage->Bytes(),
        checksum->WriteInto());

    OT_ASSERT(4 == checksum->size())
    OT_ASSERT(hash)

    preimage += checksum;

    return bitcoin_base58::EncodeBase58(
        static_cast<const unsigned char*>(preimage->data()),
        static_cast<const unsigned char*>(preimage->data()) + preimage->size());
}

auto Encode::IdentifierEncode(const Data& input) const -> std::string
{
    return IdentifierEncode(input.data(), input.size());
}

auto Encode::IdentifierEncode(const Secret& input) const -> std::string
{
    const auto bytes = input.Bytes();

    if (0 == bytes.size()) { return {}; }

    auto preimage =
        Data::Factory(bytes.data(), bytes.size());  // TODO should be secret
    auto checksum = Data::Factory();
    auto hash = crypto_.Hash().Digest(
        opentxs::crypto::HashType::Sha256DC,
        preimage->Bytes(),
        checksum->WriteInto());

    OT_ASSERT(4 == checksum->size())
    OT_ASSERT(hash)

    preimage += checksum;

    return bitcoin_base58::EncodeBase58(
        static_cast<const unsigned char*>(preimage->data()),
        static_cast<const unsigned char*>(preimage->data()) + preimage->size());
}

auto Encode::IdentifierDecode(const std::string& input) const -> std::string
{
    const auto sanitized{SanatizeBase58(input)};
    auto vector = std::vector<unsigned char>{};
    const auto decoded =
        bitcoin_base58::DecodeBase58(sanitized.c_str(), vector);

    if (false == decoded) { return {}; }

    if (4 > vector.size()) { return {}; }

    const auto output = std::string{
        reinterpret_cast<const char*>(vector.data()), vector.size() - 4};
    auto checksum = Data::Factory();
    const auto incoming = Data::Factory(vector.data() + (vector.size() - 4), 4);
    auto hash = crypto_.Hash().Digest(
        opentxs::crypto::HashType::Sha256DC, output, checksum->WriteInto());

    OT_ASSERT(4 == checksum->size())
    OT_ASSERT(hash)

    if (incoming != checksum) {
        LogTrace()(__FUNCTION__)(": Checksum failure").Flush();

        return {};
    }

    return output;
}

auto Encode::IsBase62(const std::string& str) const -> bool
{
    return str.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHI"
                                 "JKLMNOPQRSTUVWXYZ") == std::string::npos;
}

auto Encode::Nonce(const std::uint32_t size) const -> OTString
{
    auto unusedOutput = Data::Factory();

    return Nonce(size, unusedOutput);
}

auto Encode::Nonce(const std::uint32_t size, Data& rawOutput) const -> OTString
{
    rawOutput.zeroMemory();
    rawOutput.SetSize(size);
    auto source = opentxs::Context().Factory().Secret(0);
    source->Randomize(size);
    auto nonce = String::Factory(IdentifierEncode(source));
    rawOutput.Assign(source->Bytes());

    return nonce;
}

auto Encode::RandomFilename() const -> std::string { return Nonce(16)->Get(); }

auto Encode::SanatizeBase58(const std::string& input) const -> std::string
{
    return std::regex_replace(input, std::regex("[^1-9A-HJ-NP-Za-km-z]"), "");
}

auto Encode::SanatizeBase64(const std::string& input) const -> std::string
{
    return std::regex_replace(input, std::regex("[^0-9A-Za-z+/=]"), "");
}

using zmq = opentxs::network::zeromq::Context;

auto Encode::Z85Encode(const Data& input) const -> std::string
{
    auto output = std::string{};

    if (zmq::Context::RawToZ85(input.Bytes(), writer(output))) {
        return output;
    } else {
        return {};
    }
}

auto Encode::Z85Encode(const std::string& input) const -> std::string
{
    auto output = std::string{};

    if (zmq::Context::RawToZ85(input, writer(output))) {
        return output;
    } else {
        return {};
    }
}

auto Encode::Z85Decode(const Data& input) const -> OTData
{
    auto output = Data::Factory();

    if (zmq::Context::Z85ToRaw(input.Bytes(), output->WriteInto())) {
        return output;
    } else {
        return Data::Factory();
    }
}

auto Encode::Z85Decode(const std::string& input) const -> std::string
{
    auto output = std::string{};

    if (zmq::Context::Z85ToRaw(input, writer(output))) {
        return output;
    } else {
        return {};
    }
}
}  // namespace opentxs::api::crypto::implementation
