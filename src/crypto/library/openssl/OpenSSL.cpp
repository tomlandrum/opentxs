// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                        // IWYU pragma: associated
#include "1_Internal.hpp"                      // IWYU pragma: associated
#include "crypto/library/openssl/OpenSSL.hpp"  // IWYU pragma: associated

extern "C" {
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
}

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string_view>

#include "crypto/library/AsymmetricProvider.hpp"
#include "internal/crypto/library/Factory.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/crypto/SecretStyle.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/asymmetric/Algorithm.hpp"
#include "opentxs/crypto/library/HashingProvider.hpp"

#define OT_METHOD "opentxs::OpenSSL::"

namespace opentxs::factory
{
auto OpenSSL() noexcept -> std::unique_ptr<crypto::OpenSSL>
{
    using ReturnType = crypto::implementation::OpenSSL;

    return std::make_unique<ReturnType>();
}
}  // namespace opentxs::factory

namespace opentxs::crypto::implementation
{
OpenSSL::OpenSSL() noexcept
#if OT_CRYPTO_SUPPORTED_KEY_RSA
    : AsymmetricProvider()
#endif
{
}

#if OPENSSL_VERSION_NUMBER >= 0x1010000fl
OpenSSL::BIO::BIO(const BIO_METHOD* type) noexcept
#else
OpenSSL::BIO::BIO(BIO_METHOD* type) noexcept
#endif
    : bio_(::BIO_new(type), ::BIO_free)
{
    OT_ASSERT(bio_);
}

OpenSSL::MD::MD() noexcept
#if OPENSSL_VERSION_NUMBER >= 0x1010000fl
    : md_(::EVP_MD_CTX_new(), ::EVP_MD_CTX_free)
#else
    : md_(std::make_unique<::EVP_MD_CTX>())
#endif
    , hash_(nullptr)
    , key_(nullptr, ::EVP_PKEY_free)
{
    OT_ASSERT(md_);
}

OpenSSL::DH::DH() noexcept
    : dh_(nullptr, ::EVP_PKEY_CTX_free)
    , local_(nullptr, ::EVP_PKEY_free)
    , remote_(nullptr, ::EVP_PKEY_free)
{
}

auto OpenSSL::BIO::Export(const AllocateOutput allocator) noexcept -> bool
{
    if (false == bool(allocator)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator")
            .Flush();

        return false;
    }

    auto bytes = ::BIO_ctrl_pending(bio_.get());
    auto out = allocator(bytes);

    if (false == out.valid(bytes)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to allocate space for output")
            .Flush();

        return false;
    }

    const auto size{static_cast<int>(out.size())};

    if (size != BIO_read(bio_.get(), out, size)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed write output").Flush();

        return false;
    }

    return true;
}

auto OpenSSL::BIO::Import(const ReadView in) noexcept -> bool
{
    const auto size{static_cast<int>(in.size())};

    if (size != BIO_write(bio_.get(), in.data(), size)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed read input").Flush();

        return false;
    }

    return true;
}

auto OpenSSL::DH::init_keys(
    const key::Asymmetric& local,
    const key::Asymmetric& remote,
    const PasswordPrompt& reason) noexcept -> bool
{
    if (false ==
        init_key(
            local.PrivateKey(reason),
            [](auto* bio) {
                return PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
            },
            local_)) {
        return false;
    }

    if (false ==
        init_key(
            remote.PublicKey(),
            [](auto* bio) {
                return PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
            },
            remote_)) {
        return false;
    }

    dh_.reset(EVP_PKEY_CTX_new(local_.get(), nullptr));

    if (false == bool(dh_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed initializing dh context")
            .Flush();

        return false;
    }

    return true;
}

auto OpenSSL::MD::init_digest(const crypto::HashType hash) noexcept -> bool
{
    hash_ = HashTypeToOpenSSLType(hash);

    if (nullptr == hash_) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Unsupported hash algorithm.")
            .Flush();

        return false;
    }

    return true;
}

auto OpenSSL::MD::init_sign(
    const crypto::HashType hash,
    const key::Asymmetric& key,
    const PasswordPrompt& reason) noexcept -> bool
{
    if (false == init_digest(hash)) { return false; }

    if (false == key.HasPrivate()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Not a private key.").Flush();

        return false;
    }

    return init_key(
        key.PrivateKey(reason),
        [](auto* bio) {
            return PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
        },
        key_);
}

auto OpenSSL::MD::init_verify(
    const crypto::HashType hash,
    const key::Asymmetric& key) noexcept -> bool
{
    if (false == init_digest(hash)) { return false; }

    if (false == key.HasPublic()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing public key.").Flush();

        return false;
    }

    return init_key(
        key.PublicKey(),
        [](auto* bio) {
            return PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        },
        key_);
}

auto OpenSSL::HashTypeToOpenSSLType(const crypto::HashType hashType) noexcept
    -> const EVP_MD*
{
    switch (hashType) {
        // NOTE: libressl doesn't support these yet
        // case crypto::HashType::Blake2b256: {
        //     return ::EVP_blake2s256();
        // }
        // case crypto::HashType::Blake2b512: {
        //     return ::EVP_blake2b512();
        // }
        case crypto::HashType::Ripemd160: {
            return ::EVP_ripemd160();
        }
        case crypto::HashType::Sha256: {
            return ::EVP_sha256();
        }
        case crypto::HashType::Sha512: {
            return ::EVP_sha512();
        }
        case crypto::HashType::Sha1: {
            return ::EVP_sha1();
        }
        default: {
            return nullptr;
        }
    }
}

auto OpenSSL::Digest(
    const crypto::HashType type,
    const std::uint8_t* input,
    const size_t inputSize,
    std::uint8_t* output) const -> bool

{
    auto md = MD{};

    if (false == md.init_digest(type)) { return false; }

    if (1 != EVP_DigestInit_ex(md, md, nullptr)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to initialize digest operation")
            .Flush();

        return false;
    }

    if (1 != EVP_DigestUpdate(md, input, inputSize)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to process plaintext")
            .Flush();

        return false;
    }

    unsigned int bytes{};

    if (1 != EVP_DigestFinal_ex(md, output, &bytes)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to write digest").Flush();

        return false;
    }

    OT_ASSERT(
        HashingProvider::HashSize(type) == static_cast<std::size_t>(bytes));

    return true;
}

// Calculate an HMAC given some input data and a key
auto OpenSSL::HMAC(
    const crypto::HashType hashType,
    const std::uint8_t* input,
    const size_t inputSize,
    const std::uint8_t* key,
    const size_t keySize,
    std::uint8_t* output) const -> bool
{
    unsigned int size = 0;
    const auto* evp_md = HashTypeToOpenSSLType(hashType);

    if (nullptr != evp_md) {
        void* data = ::HMAC(
            evp_md,
            key,
            static_cast<int>(keySize),
            input,
            inputSize,
            nullptr,
            &size);

        if (nullptr != data) {
            std::memcpy(output, data, size);

            return true;
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to produce a valid HMAC.")
                .Flush();

            return false;
        }
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid hash type.").Flush();

        return false;
    }
}

auto OpenSSL::init_key(
    const ReadView bytes,
    const Instantiate function,
    OpenSSL_EVP_PKEY& output) noexcept -> bool
{
    auto pem = BIO{};

    if (false == pem.Import(bytes)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed read private key").Flush();

        return false;
    }

    OT_ASSERT(function);

    output.reset(function(pem));

    if (false == bool(output)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to instantiate EVP_PKEY")
            .Flush();

        return false;
    }

    OT_ASSERT(output);

    return true;
}

auto OpenSSL::PKCS5_PBKDF2_HMAC(
    const void* input,
    const std::size_t inputSize,
    const void* salt,
    const std::size_t saltSize,
    const std::size_t iterations,
    const crypto::HashType hashType,
    const std::size_t bytes,
    void* output) const noexcept -> bool
{
    const auto max = static_cast<std::size_t>(std::numeric_limits<int>::max());

    if (inputSize > max) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid input size").Flush();

        return false;
    }

    if (saltSize > max) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid salt size").Flush();

        return false;
    }

    if (iterations > max) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid iteration count").Flush();

        return false;
    }

    if (bytes > max) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output size").Flush();

        return false;
    }

    const auto* algorithm = HashTypeToOpenSSLType(hashType);

    if (nullptr == algorithm) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Error: invalid hash type: ")(
            HashingProvider::HashTypeToString(hashType))
            .Flush();

        return false;
    }

    return 1 == ::PKCS5_PBKDF2_HMAC(
                    static_cast<const char*>(input),
                    static_cast<int>(inputSize),
                    static_cast<const unsigned char*>(salt),
                    static_cast<int>(saltSize),
                    static_cast<int>(iterations),
                    algorithm,
                    static_cast<int>(bytes),
                    static_cast<unsigned char*>(output));
}

auto OpenSSL::RIPEMD160(
    const std::uint8_t* input,
    const std::size_t inputSize,
    std::uint8_t* output) const -> bool
{
    return Digest(crypto::HashType::Ripemd160, input, inputSize, output);
}

#if OT_CRYPTO_SUPPORTED_KEY_RSA
auto OpenSSL::generate_dh(const NymParameters& options, ::EVP_PKEY* output)
    const noexcept -> bool
{
    auto context = OpenSSL_EVP_PKEY_CTX{
        ::EVP_PKEY_CTX_new_id(EVP_PKEY_DH, nullptr), ::EVP_PKEY_CTX_free};

    if (false == bool(context)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed EVP_PKEY_CTX_new_id")
            .Flush();

        return false;
    }

    if (1 != ::EVP_PKEY_paramgen_init(context.get())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed EVP_PKEY_paramgen_init")
            .Flush();

        return false;
    }

    if (1 != EVP_PKEY_CTX_set_dh_paramgen_prime_len(
                 context.get(), options.keySize())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed EVP_PKEY_CTX_set_dh_paramgen_prime_len")
            .Flush();

        return false;
    }

    if (1 != EVP_PKEY_paramgen(context.get(), &output)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed EVP_PKEY_paramgen")
            .Flush();

        return false;
    }

    return true;
}

auto OpenSSL::get_params(
    const AllocateOutput params,
    const NymParameters& options,
    ::EVP_PKEY* output) const noexcept -> bool
{
    const auto existing = options.DHParams();

    if (0 < existing.size()) {
        if (false == import_dh(existing, output)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to import dh params")
                .Flush();

            return false;
        }
    } else {
        if (false == generate_dh(options, output)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to generate dh params")
                .Flush();

            return false;
        }
    }

    return write_dh(params, output);
}

auto OpenSSL::import_dh(const ReadView existing, ::EVP_PKEY* output)
    const noexcept -> bool
{
    struct DH {
        ::DH* dh_;

        DH()
            : dh_(::DH_new())
        {
            OT_ASSERT(nullptr != dh_);
        }

        ~DH()
        {
            if (nullptr != dh_) {
                ::DH_free(dh_);
                dh_ = nullptr;
            }
        }

    private:
        DH(const DH&) = delete;
        DH(DH&&) = delete;
        auto operator=(const DH&) -> DH& = delete;
        auto operator=(DH&&) -> DH& = delete;
    };

    auto dh = DH{};
    auto params = BIO{};

    if (false == params.Import(existing)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to read dh params")
            .Flush();

        return false;
    }

    if (nullptr == ::PEM_read_bio_DHparams(params, &dh.dh_, nullptr, nullptr)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed PEM_read_bio_DHparams")
            .Flush();

        return false;
    }

    OT_ASSERT(nullptr != dh.dh_);

    if (1 != ::EVP_PKEY_set1_DH(output, dh.dh_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed EVP_PKEY_set1_DH").Flush();

        return false;
    }

    return true;
}

auto OpenSSL::make_dh_key(
    const AllocateOutput privateKey,
    const AllocateOutput publicKey,
    const AllocateOutput dhParams,
    const NymParameters& options) const noexcept -> bool
{
    struct Key {
        ::EVP_PKEY* key_;

        Key()
            : key_(::EVP_PKEY_new())
        {
            OT_ASSERT(nullptr != key_);
        }

        ~Key()
        {
            if (nullptr != key_) {
                ::EVP_PKEY_free(key_);
                key_ = nullptr;
            }
        }

    private:
        Key(const Key&) = delete;
        Key(Key&&) = delete;
        auto operator=(const Key&) -> Key& = delete;
        auto operator=(Key&&) -> Key& = delete;
    };

    auto params = Key{};
    auto key = Key{};

    if (false == get_params(dhParams, options, params.key_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to set up dh params")
            .Flush();

        return false;
    }

    auto context = OpenSSL_EVP_PKEY_CTX{
        ::EVP_PKEY_CTX_new(params.key_, nullptr), ::EVP_PKEY_CTX_free};

    if (false == bool(context)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed EVP_PKEY_CTX_new").Flush();

        return false;
    }

    if (1 != ::EVP_PKEY_keygen_init(context.get())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed EVP_PKEY_keygen_init")
            .Flush();

        return false;
    }

    if (1 != ::EVP_PKEY_keygen(context.get(), &key.key_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed EVP_PKEY_keygen").Flush();

        return false;
    }

    return write_keypair(privateKey, publicKey, key.key_);
}

auto OpenSSL::make_signing_key(
    const AllocateOutput privateKey,
    const AllocateOutput publicKey,
    const NymParameters& options) const noexcept -> bool
{
    auto evp = OpenSSL_EVP_PKEY{::EVP_PKEY_new(), ::EVP_PKEY_free};

    OT_ASSERT(evp);

    {
        auto exponent = OpenSSL_BN{::BN_new(), ::BN_free};
        auto rsa = OpenSSL_RSA{::RSA_new(), ::RSA_free};

        OT_ASSERT(exponent);
        OT_ASSERT(rsa);

        auto rc = ::BN_set_word(exponent.get(), 65537);

        if (1 != rc) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed BN_set_word").Flush();

            return false;
        }

        rc = ::RSA_generate_multi_prime_key(
            rsa.get(),
            options.keySize(),
            primes(options.keySize()),
            exponent.get(),
            nullptr);

        if (1 != rc) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to generate key")
                .Flush();

            return false;
        }

        rc = ::EVP_PKEY_assign_RSA(evp.get(), rsa.release());

        if (1 != rc) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed EVP_PKEY_assign_RSA")
                .Flush();

            return false;
        }
    }

    return write_keypair(privateKey, publicKey, evp.get());
}

auto OpenSSL::primes(const int bits) -> int
{
    if (8192 <= bits) {
        return 5;
    } else if (4096 <= bits) {
        return 4;
    } else if (1024 <= bits) {
        return 3;
    } else {
        return 2;
    }
}

auto OpenSSL::RandomKeypair(
    const AllocateOutput privateKey,
    const AllocateOutput publicKey,
    const crypto::key::asymmetric::Role role,
    const NymParameters& options,
    const AllocateOutput params) const noexcept -> bool
{
    if (crypto::key::asymmetric::Role::Encrypt == role) {

        return make_dh_key(privateKey, publicKey, params, options);
    } else {

        return make_signing_key(privateKey, publicKey, options);
    }
}

auto OpenSSL::SharedSecret(
    const key::Asymmetric& publicKey,
    const key::Asymmetric& privateKey,
    const SecretStyle style,
    const PasswordPrompt& reason,
    Secret& secret) const noexcept -> bool
{
    if (SecretStyle::Default != style) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported secret style")
            .Flush();

        return false;
    }

    if (crypto::key::asymmetric::Algorithm::Legacy != publicKey.keyType()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid public key type").Flush();

        return false;
    }

    if (crypto::key::asymmetric::Role::Encrypt != publicKey.Role()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid public key role").Flush();

        return false;
    }

    if (crypto::key::asymmetric::Algorithm::Legacy != privateKey.keyType()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid private key type")
            .Flush();

        return false;
    }

    if (crypto::key::asymmetric::Role::Encrypt != privateKey.Role()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid private key role")
            .Flush();

        return false;
    }

    auto dh = DH{};

    if (false == dh.init_keys(privateKey, publicKey, reason)) { return false; }

    if (1 != ::EVP_PKEY_derive_init(dh)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed EVP_PKEY_derive_init")
            .Flush();

        return false;
    }

    if (1 != ::EVP_PKEY_derive_set_peer(dh, dh.Remote())) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Failed EVP_PKEY_derive_set_peer")
            .Flush();

        return false;
    }

    auto allocate = secret.WriteInto(Secret::Mode::Mem);

    if (false == bool(allocate)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to get output allocator")
            .Flush();

        return false;
    }

    auto size = std::size_t{};

    if (1 != ::EVP_PKEY_derive(dh, nullptr, &size)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to calculate shared secret size")
            .Flush();

        return false;
    }

    auto output = allocate(size);

    if (false == output.valid(size)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to allocate space for shared secret")
            .Flush();

        return false;
    }

    if (1 != EVP_PKEY_derive(dh, output.as<unsigned char>(), &size)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to derive shared secret")
            .Flush();

        return false;
    }

    return true;
}

auto OpenSSL::Sign(
    [[maybe_unused]] const api::internal::Core& api,
    const ReadView in,
    const key::Asymmetric& key,
    const crypto::HashType type,
    const AllocateOutput signature,
    const PasswordPrompt& reason) const -> bool
{
    if (crypto::key::asymmetric::Algorithm::Legacy != key.keyType()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid key type").Flush();

        return false;
    }

    switch (type) {
        case crypto::HashType::Blake2b160:
        case crypto::HashType::Blake2b256:
        case crypto::HashType::Blake2b512: {
            LogVerbose(OT_METHOD)(__FUNCTION__)(": Unsupported hash algorithm.")
                .Flush();

            return false;
        }
        default: {
        }
    }

    auto md = MD{};

    if (false == md.init_sign(type, key, reason)) { return false; }

    if (1 != EVP_DigestSignInit(md, nullptr, md, nullptr, md)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to initialize signing operation")
            .Flush();

        return false;
    }

    if (1 != EVP_DigestSignUpdate(md, in.data(), in.size())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to process plaintext")
            .Flush();

        return false;
    }

    auto bytes = std::size_t{};

    if (1 != EVP_DigestSignFinal(md, nullptr, &bytes)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to calculate signature size")
            .Flush();

        return false;
    }

    if (false == bool(signature)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator")
            .Flush();

        return false;
    }

    auto out = signature(bytes);

    if (false == out.valid(bytes)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to allocate space for signature")
            .Flush();

        return false;
    }

    if (1 != EVP_DigestSignFinal(
                 md, reinterpret_cast<unsigned char*>(out.data()), &bytes)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to write signature")
            .Flush();

        return false;
    }

    return true;
}

auto OpenSSL::Verify(
    const Data& in,
    const key::Asymmetric& key,
    const Data& sig,
    const crypto::HashType type) const -> bool
{
    switch (type) {
        case crypto::HashType::Blake2b160:
        case crypto::HashType::Blake2b256:
        case crypto::HashType::Blake2b512: {
            LogVerbose(OT_METHOD)(__FUNCTION__)(": Unsupported hash algorithm.")
                .Flush();

            return false;
        }
        default: {
        }
    }

    auto md = MD{};

    if (false == md.init_verify(type, key)) { return false; }

    if (1 != EVP_DigestVerifyInit(md, nullptr, md, nullptr, md)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to initialize verification operation")
            .Flush();

        return false;
    }

    if (1 != EVP_DigestVerifyUpdate(md, in.data(), in.size())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to process plaintext")
            .Flush();

        return false;
    }

    if (1 != EVP_DigestVerifyFinal(
                 md,
                 reinterpret_cast<const unsigned char*>(sig.data()),
                 sig.size())) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Invalid signature").Flush();

        return false;
    }

    return true;
}

auto OpenSSL::write_dh(const AllocateOutput dhParams, ::EVP_PKEY* input)
    const noexcept -> bool
{
    auto dh = OpenSSL_DH{::EVP_PKEY_get1_DH(input), ::DH_free};

    if (false == bool(dh)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed EVP_PKEY_get1_DH").Flush();

        return false;
    }

    auto params = BIO{};
    const auto rc = ::PEM_write_bio_DHparams(params, dh.get());

    if (1 != rc) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed PEM_write_DHparams")
            .Flush();

        return false;
    }

    if (false == params.Export(dhParams)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed write dh params").Flush();

        return false;
    }

    return true;
}

auto OpenSSL::write_keypair(
    const AllocateOutput privateKey,
    const AllocateOutput publicKey,
    ::EVP_PKEY* evp) const noexcept -> bool
{
    OT_ASSERT(nullptr != evp);

    auto privKey = BIO{};
    auto pubKey = BIO{};

    auto rc = ::PEM_write_bio_PrivateKey(
        privKey, evp, nullptr, nullptr, 0, nullptr, nullptr);

    if (1 != rc) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed PEM_write_bio_PrivateKey")
            .Flush();

        return false;
    }

    rc = ::PEM_write_bio_PUBKEY(pubKey, evp);

    if (1 != rc) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed PEM_write_bio_PUBKEY")
            .Flush();

        return false;
    }

    if (false == pubKey.Export(publicKey)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed write public key").Flush();

        return false;
    }

    if (false == privKey.Export(privateKey)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed write private key")
            .Flush();

        return false;
    }

    return true;
}
#endif
}  // namespace opentxs::crypto::implementation
