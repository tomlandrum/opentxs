// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                            // IWYU pragma: associated
#include "1_Internal.hpp"                          // IWYU pragma: associated
#include "crypto/library/secp256k1/Secp256k1.hpp"  // IWYU pragma: associated

extern "C" {
#include <secp256k1.h>
#include <secp256k1_ecdh.h>
}

#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string_view>

#include "crypto/library/EcdsaProvider.hpp"
#include "internal/crypto/library/Factory.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Primitives.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/api/crypto/Util.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/AsymmetricKeyType.hpp"
#include "opentxs/crypto/SecretStyle.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"

#define OT_METHOD "opentxs::crypto::implementation::Secp256k1::"

extern "C" {
int get_x_value(
    unsigned char* output,
    const unsigned char* x32,
    const unsigned char* y32,
    void*);
int get_x_value(
    unsigned char* output,
    const unsigned char* x32,
    const unsigned char* y32,
    void*)
{
    std::memcpy(output, x32, 32);

    return 1;
}
}

namespace opentxs::factory
{
auto Secp256k1(
    const api::Crypto& crypto,
    const api::crypto::Util& util) noexcept
    -> std::unique_ptr<crypto::Secp256k1>
{
    using ReturnType = crypto::implementation::Secp256k1;

    return std::make_unique<ReturnType>(crypto, util);
}
}  // namespace opentxs::factory

namespace opentxs::crypto::implementation
{
bool Secp256k1::Initialized_ = false;

Secp256k1::Secp256k1(
    const api::Crypto& crypto,
    const api::crypto::Util& ssl) noexcept
    : EcdsaProvider(crypto)
    , context_(secp256k1_context_create(
          SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY))
    , ssl_(ssl)
{
}

auto Secp256k1::PubkeyAdd(
    const ReadView pubkey,
    const ReadView scalar,
    const AllocateOutput result) const noexcept -> bool
{
    if (false == bool(result)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator")
            .Flush();

        return false;
    }

    if ((0 == pubkey.size()) || (nullptr == pubkey.data())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing pubkey").Flush();

        return false;
    }

    if ((PrivateKeySize != scalar.size()) || (nullptr == scalar.data())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid scalar").Flush();

        return false;
    }

    auto parsed = ::secp256k1_pubkey{};
    auto rc = 1 == ::secp256k1_ec_pubkey_parse(
                       context_,
                       &parsed,
                       reinterpret_cast<const unsigned char*>(pubkey.data()),
                       pubkey.size());

    if (false == rc) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid public key").Flush();

        return false;
    }

    rc = 1 == ::secp256k1_ec_pubkey_tweak_add(
                  context_,
                  &parsed,
                  reinterpret_cast<const unsigned char*>(scalar.data()));

    if (false == rc) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to add scalar to public key")
            .Flush();

        return false;
    }

    auto out = result(PublicKeySize);

    if (false == out.valid(PublicKeySize)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to allocate space for result")
            .Flush();

        return false;
    }

    auto size = out.size();
    rc = ::secp256k1_ec_pubkey_serialize(
        context_,
        out.as<unsigned char>(),
        &size,
        &parsed,
        SECP256K1_EC_COMPRESSED);

    if (false == rc) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to serialize public key")
            .Flush();

        return false;
    }

    return true;
}

auto Secp256k1::RandomKeypair(
    const AllocateOutput privateKey,
    const AllocateOutput publicKey,
    const proto::KeyRole,
    const NymParameters&,
    const AllocateOutput) const noexcept -> bool
{
    if (nullptr == context_) { return false; }

    if (false == bool(privateKey)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator")
            .Flush();

        return false;
    }

    auto counter{0};
    auto valid{false};
    auto temp = Context().Factory().Secret(0);

    while (false == valid) {
        temp->Randomize(PrivateKeySize);
        auto writer = temp->WriteInto(Secret::Mode::Mem)(PrivateKeySize);

        OT_ASSERT(writer.valid(PrivateKeySize));

        valid = 1 == ::secp256k1_ec_seckey_verify(
                         context_, writer.as<unsigned char>());

        OT_ASSERT(3 > ++counter);
    }

    auto prv = privateKey(PrivateKeySize);

    if (false == prv.valid(PrivateKeySize)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to allocate space for private key")
            .Flush();

        return false;
    }

    std::memcpy(prv, temp->data(), prv);

    return ScalarMultiplyBase({prv.as<const char>(), prv.size()}, publicKey);
}

auto Secp256k1::ScalarAdd(
    const ReadView lhs,
    const ReadView rhs,
    const AllocateOutput result) const noexcept -> bool
{
    if (false == bool(result)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator")
            .Flush();

        return false;
    }

    if (PrivateKeySize != lhs.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid lhs scalar").Flush();

        return false;
    }

    if (PrivateKeySize != rhs.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid rhs scalar").Flush();

        return false;
    }

    auto key = result(PrivateKeySize);

    if (false == key.valid(PrivateKeySize)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to allocate space for result")
            .Flush();

        return false;
    }

    std::memcpy(key.data(), lhs.data(), lhs.size());

    return 1 == ::secp256k1_ec_seckey_tweak_add(
                    context_,
                    key.as<unsigned char>(),
                    reinterpret_cast<const unsigned char*>(rhs.data()));
}

auto Secp256k1::ScalarMultiplyBase(
    const ReadView scalar,
    const AllocateOutput result) const noexcept -> bool
{
    if (false == bool(result)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator")
            .Flush();

        return false;
    }

    if (PrivateKeySize != scalar.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid scalar").Flush();

        return false;
    }

    auto key = secp256k1_pubkey{};
    const auto created =
        1 == ::secp256k1_ec_pubkey_create(
                 context_,
                 &key,
                 reinterpret_cast<const unsigned char*>(scalar.data()));

    if (1 != created) { return false; }

    auto pub = result(PublicKeySize);

    if (false == pub.valid(PublicKeySize)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to allocate space for public key")
            .Flush();

        return false;
    }

    auto size{pub.size()};

    return 1 == ::secp256k1_ec_pubkey_serialize(
                    context_,
                    pub.as<unsigned char>(),
                    &size,
                    &key,
                    SECP256K1_EC_COMPRESSED);
}

auto Secp256k1::SharedSecret(
    const key::Asymmetric& publicKey,
    const key::Asymmetric& privateKey,
    const SecretStyle style,
    const PasswordPrompt& reason,
    Secret& secret) const noexcept -> bool
{
    if (publicKey.keyType() != crypto::AsymmetricKeyType::Secp256k1) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Public key is wrong type")
            .Flush();

        return false;
    }

    if (privateKey.keyType() != crypto::AsymmetricKeyType::Secp256k1) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Private key is wrong type")
            .Flush();

        return false;
    }

    const auto pub = publicKey.PublicKey();
    const auto prv = privateKey.PrivateKey(reason);

    if (PrivateKeySize != prv.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid private key").Flush();

        return false;
    }

    auto key = ::secp256k1_pubkey{};

    if (1 != ::secp256k1_ec_pubkey_parse(
                 context_,
                 &key,
                 reinterpret_cast<const unsigned char*>(pub.data()),
                 pub.size())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid public key").Flush();

        return false;
    }

    auto writer = secret.WriteInto(Secret::Mode::Mem)(PrivateKeySize);

    OT_ASSERT(writer.valid(PrivateKeySize));

    const auto function = [&] {
        switch (style) {
            case SecretStyle::X_only: {

                return get_x_value;
            }
            case SecretStyle::Default:
            default: {

                return secp256k1_ecdh_hash_function_sha256;
            }
        }
    }();

    return 1 == ::secp256k1_ecdh(
                    context_,
                    static_cast<unsigned char*>(writer.data()),
                    &key,
                    reinterpret_cast<const unsigned char*>(prv.data()),
                    function,
                    nullptr);
}

auto Secp256k1::Sign(
    const api::internal::Core& api,
    const ReadView plaintext,
    const key::Asymmetric& key,
    const proto::HashType type,
    const AllocateOutput signature,
    const PasswordPrompt& reason) const -> bool
{
    if (crypto::AsymmetricKeyType::Secp256k1 != key.keyType()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid key type").Flush();

        return false;
    }

    if (false == key.HasPrivate()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": A private key required when generating signatures")
            .Flush();

        return false;
    }

    try {
        const auto digest = hash(type, plaintext);
        const auto priv = key.PrivateKey(reason);

        if (nullptr == priv.data() || 0 == priv.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Missing private key").Flush();

            return false;
        }

        if (PrivateKeySize != priv.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid private key").Flush();

            return false;
        }

        if (false == bool(signature)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator")
                .Flush();

            return false;
        }

        const auto size = sizeof(secp256k1_ecdsa_signature);
        auto output = signature(size);

        if (false == output.valid(size)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to allocate space for signature")
                .Flush();

            return false;
        }

        const bool signatureCreated = ::secp256k1_ecdsa_sign(
            context_,
            output.as<::secp256k1_ecdsa_signature>(),
            reinterpret_cast<const unsigned char*>(digest->data()),
            reinterpret_cast<const unsigned char*>(priv.data()),
            nullptr,
            nullptr);

        if (false == signatureCreated) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Call to secp256k1_ecdsa_sign() failed.")
                .Flush();

            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return false;
    }
}

auto Secp256k1::SignDER(
    const api::internal::Core& api,
    const ReadView plaintext,
    const key::Asymmetric& key,
    const proto::HashType type,
    Space& output,
    const PasswordPrompt& reason) const noexcept -> bool
{
    if (crypto::AsymmetricKeyType::Secp256k1 != key.keyType()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid key type").Flush();

        return false;
    }

    if (false == key.HasPrivate()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": A private key required when generating signatures")
            .Flush();

        return false;
    }

    try {
        const auto digest = hash(type, plaintext);
        const auto priv = key.PrivateKey(reason);

        if (nullptr == priv.data() || 0 == priv.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Missing private key").Flush();

            return false;
        }

        if (PrivateKeySize != priv.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid private key").Flush();

            return false;
        }

        auto sig = secp256k1_ecdsa_signature{};

        const bool signatureCreated = ::secp256k1_ecdsa_sign(
            context_,
            &sig,
            reinterpret_cast<const unsigned char*>(digest->data()),
            reinterpret_cast<const unsigned char*>(priv.data()),
            nullptr,
            nullptr);

        if (false == signatureCreated) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Call to secp256k1_ecdsa_sign() failed.")
                .Flush();

            return false;
        }

        output.resize(80);
        auto allocated{output.size()};
        const auto wrote = ::secp256k1_ecdsa_signature_serialize_der(
            context_,
            reinterpret_cast<unsigned char*>(output.data()),
            &allocated,
            &sig);

        if (1 != wrote) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Call to secp256k1_ecdsa_signature_serialize_der() failed.")
                .Flush();

            return false;
        }

        output.resize(allocated);

        return true;
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return false;
    }
}

auto Secp256k1::Verify(
    const Data& plaintext,
    const key::Asymmetric& key,
    const Data& signature,
    const proto::HashType type) const -> bool
{
    if (crypto::AsymmetricKeyType::Secp256k1 != key.keyType()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid key type").Flush();

        return false;
    }

    try {
        const auto digest = hash(type, plaintext.Bytes());
        const auto parsed = parsed_public_key(key.PublicKey());
        const auto sig = parsed_signature(signature.Bytes());

        return 1 == ::secp256k1_ecdsa_verify(
                        context_,
                        &sig,
                        reinterpret_cast<const unsigned char*>(digest->data()),
                        &parsed);
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return false;
    }
}

auto Secp256k1::hash(const proto::HashType type, const ReadView data) const
    noexcept(false) -> OTData
{
    auto output = Data::Factory();

    if (false == crypto_.Hash().Digest(type, data, output->WriteInto())) {
        throw std::runtime_error("Failed to obtain contract hash");
    }

    if (0 == output->size()) { throw std::runtime_error("Invalid hash"); }

    output->resize(32);

    OT_ASSERT(nullptr != output->data());
    OT_ASSERT(32 == output->size());

    return output;
}

void Secp256k1::Init()
{
    OT_ASSERT(false == Initialized_);

    auto seed = std::array<std::uint8_t, 32>{};
    ssl_.RandomizeMemory(seed.data(), seed.size());

    OT_ASSERT(nullptr != context_);

    const auto randomize = secp256k1_context_randomize(context_, seed.data());

    OT_ASSERT(1 == randomize);

    Initialized_ = true;
}

auto Secp256k1::parsed_public_key(const ReadView bytes) const noexcept(false)
    -> ::secp256k1_pubkey
{
    if (nullptr == bytes.data() || 0 == bytes.size()) {
        throw std::runtime_error("Missing public key");
    }

    auto output = ::secp256k1_pubkey{};

    if (1 != ::secp256k1_ec_pubkey_parse(
                 context_,
                 &output,
                 reinterpret_cast<const unsigned char*>(bytes.data()),
                 bytes.size())) {
        throw std::runtime_error("Invalid public key");
    }

    return output;
}

auto Secp256k1::parsed_signature(const ReadView bytes) const noexcept(false)
    -> ::secp256k1_ecdsa_signature
{
    auto output = ::secp256k1_ecdsa_signature{};

    if (sizeof(output.data) != bytes.size()) {
        throw std::runtime_error("Invalid signature");
    }

    std::memcpy(&output.data, bytes.data(), sizeof(output.data));

    return output;
}

Secp256k1::~Secp256k1()
{
    if (nullptr != context_) {
        secp256k1_context_destroy(context_);
        context_ = nullptr;
    }

    Initialized_ = false;
}
}  // namespace opentxs::crypto::implementation
