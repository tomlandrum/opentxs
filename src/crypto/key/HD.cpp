// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"       // IWYU pragma: associated
#include "1_Internal.hpp"     // IWYU pragma: associated
#include "crypto/key/HD.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "crypto/key/EllipticCurve.hpp"
#include "internal/api/Api.hpp"
#include "internal/crypto/key/Factory.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/crypto/AsymmetricKeyType.hpp"
#include "opentxs/crypto/Bip32.hpp"
#include "opentxs/crypto/Bip32Child.hpp"
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
#include "opentxs/crypto/key/Ed25519.hpp"
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519
#include "opentxs/crypto/key/HD.hpp"
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#include "opentxs/crypto/key/Secp256k1.hpp"
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/protobuf/AsymmetricKey.pb.h"
#include "opentxs/protobuf/Ciphertext.pb.h"
#include "opentxs/protobuf/HDPath.pb.h"
#include "util/HDIndex.hpp"

#define OT_METHOD "opentxs::crypto::key::implementation::HD::"

namespace opentxs::crypto::key
{
auto HD::CalculateFingerprint(
    const api::crypto::Hash& hash,
    const ReadView key) noexcept -> Bip32Fingerprint
{
    auto output = Bip32Fingerprint{0};
    auto digest = Data::Factory();

    if (33 != key.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid public key").Flush();

        return output;
    }

    const auto hashed =
        hash.Digest(proto::HASHTYPE_BITCOIN, key, digest->WriteInto());

    if (false == hashed) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to calculate public key hash")
            .Flush();

        return output;
    }

    if (false == digest->Extract(output)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to set output").Flush();

        return {};
    }

    return output;
}
}  // namespace opentxs::crypto::key

namespace opentxs::crypto::key::implementation
{
HD::HD(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const proto::AsymmetricKey& serializedKey) noexcept(false)
    : EllipticCurve(api, ecdsa, serializedKey)
    , path_(
          serializedKey.has_path()
              ? std::make_shared<proto::HDPath>(serializedKey.path())
              : nullptr)
    , chain_code_(
          serializedKey.has_chaincode()
              ? std::make_unique<proto::Ciphertext>(serializedKey.chaincode())
              : nullptr)
    , plaintext_chain_code_(api.Factory().Secret(0))
    , parent_(serializedKey.bip32_parent())
{
}

HD::HD(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const crypto::AsymmetricKeyType keyType,
    const proto::KeyRole role,
    const VersionNumber version,
    const PasswordPrompt& reason) noexcept(false)
    : EllipticCurve(api, ecdsa, keyType, role, version, reason)
    , path_(nullptr)
    , chain_code_(nullptr)
    , plaintext_chain_code_(api.Factory().Secret(0))
    , parent_(0)
{
}

HD::HD(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const crypto::AsymmetricKeyType keyType,
    const Secret& privateKey,
    const Data& publicKey,
    const proto::KeyRole role,
    const VersionNumber version,
    key::Symmetric& sessionKey,
    const PasswordPrompt& reason) noexcept(false)
    : EllipticCurve(
          api,
          ecdsa,
          keyType,
          privateKey,
          publicKey,
          role,
          version,
          sessionKey,
          reason)
    , path_(nullptr)
    , chain_code_(nullptr)
    , plaintext_chain_code_(api.Factory().Secret(0))
    , parent_(0)
{
}

#if OT_CRYPTO_WITH_BIP32
HD::HD(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const crypto::AsymmetricKeyType keyType,
    const Secret& privateKey,
    const Secret& chainCode,
    const Data& publicKey,
    const proto::HDPath& path,
    const Bip32Fingerprint parent,
    const proto::KeyRole role,
    const VersionNumber version,
    key::Symmetric& sessionKey,
    const PasswordPrompt& reason) noexcept(false)
    : EllipticCurve(
          api,
          ecdsa,
          keyType,
          privateKey,
          publicKey,
          role,
          version,
          sessionKey,
          reason)
    , path_(std::make_shared<proto::HDPath>(path))
    , chain_code_(encrypt_key(sessionKey, reason, false, chainCode.Bytes()))
    , plaintext_chain_code_(chainCode)
    , parent_(parent)
{
    OT_ASSERT(path_);
    OT_ASSERT(chain_code_);
}

HD::HD(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const proto::AsymmetricKeyType keyType,
    const Secret& privateKey,
    const Secret& chainCode,
    const Data& publicKey,
    const proto::HDPath& path,
    const Bip32Fingerprint parent,
    const proto::KeyRole role,
    const VersionNumber version) noexcept(false)
    : EllipticCurve(api, ecdsa, keyType, privateKey, publicKey, role, version)
    , path_(std::make_shared<proto::HDPath>(path))
    , chain_code_()
    , plaintext_chain_code_(chainCode)
    , parent_(parent)
{
    OT_ASSERT(path_);
}
#endif  // OT_CRYPTO_WITH_BIP32

HD::HD(const HD& rhs) noexcept
    : EllipticCurve(rhs)
    , path_(bool(rhs.path_) ? new proto::HDPath(*rhs.path_) : nullptr)
    , chain_code_(
          bool(rhs.chain_code_) ? new proto::Ciphertext(*rhs.chain_code_)
                                : nullptr)
    , plaintext_chain_code_(rhs.plaintext_chain_code_)
    , parent_(rhs.parent_)
{
}

HD::HD(const HD& rhs, const ReadView newPublic) noexcept
    : EllipticCurve(rhs, newPublic)
    , path_()
    , chain_code_()
    , plaintext_chain_code_(api_.Factory().Secret(0))
    , parent_()
{
}

HD::HD(const HD& rhs, OTSecret&& newSecretKey) noexcept
    : EllipticCurve(rhs, std::move(newSecretKey))
    , path_()
    , chain_code_()
    , plaintext_chain_code_(api_.Factory().Secret(0))
    , parent_()
{
}

auto HD::Chaincode(const PasswordPrompt& reason) const noexcept -> ReadView
{
    try {

        return get_chain_code(reason).Bytes();
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto HD::ChildKey(const Bip32Index index, const PasswordPrompt& reason)
    const noexcept -> std::unique_ptr<key::HD>
{
    try {
#if OT_CRYPTO_WITH_BIP32
        static const auto blank = api_.Factory().Secret(0);
        const auto serialized = [&] {
            const auto path = [&] {
                auto out = Bip32::Path{};

                if (path_) {
                    std::copy(
                        path_->child().begin(),
                        path_->child().end(),
                        std::back_inserter(out));
                }

                return out;
            }();

            if (HasPrivate()) {
                return api_.Crypto().BIP32().DerivePrivateKey(
                    *this, {index}, reason);
            } else {
                return api_.Crypto().BIP32().DerivePublicKey(
                    *this, {index}, reason);
            }
        }();
        const auto& [privkey, ccode, pubkey, spath, parent] = serialized;
        const auto path = [&] {
            auto out = proto::HDPath{};

            if (path_) {
                out = *path_;
                out.add_child(index);
            }

            return out;
        }();

        switch (type_) {
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
            case crypto::AsymmetricKeyType::ED25519: {
                return factory::Ed25519Key(
                    api_,
                    api_.Crypto().ED25519(),
                    HasPrivate() ? privkey : blank,
                    ccode,
                    pubkey,
                    path,
                    parent,
                    role_,
                    version_,
                    reason);
            }
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
            case crypto::AsymmetricKeyType::Secp256k1: {
                return factory::Secp256k1Key(
                    api_,
                    api_.Crypto().SECP256K1(),
                    HasPrivate() ? privkey : blank,
                    ccode,
                    pubkey,
                    path,
                    parent,
                    role_,
                    version_,
                    reason);
            }
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1
            default: {
                throw std::runtime_error{"Unsupported key type"};
            }
        }
#else
        throw std::runtime_error{"HD key support missing but required"};
#endif
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto HD::Depth() const noexcept -> int
{
    if (false == bool(path_)) { return -1; }

    return path_->child_size();
}

auto HD::erase_private_data() -> void
{
    EllipticCurve::erase_private_data();
    const_cast<std::shared_ptr<const proto::HDPath>&>(path_).reset();
    const_cast<std::unique_ptr<const proto::Ciphertext>&>(chain_code_).reset();
}

auto HD::Fingerprint() const noexcept -> Bip32Fingerprint
{
    return CalculateFingerprint(api_.Crypto().Hash(), PublicKey());
}

auto HD::get_chain_code(const PasswordPrompt& reason) const noexcept(false)
    -> Secret&
{
    if (0 == plaintext_chain_code_->size()) {
        if (false == bool(encrypted_key_)) {
            throw std::runtime_error{"Missing encrypted private key"};
        }
        if (false == bool(chain_code_)) {
            throw std::runtime_error{"Missing encrypted chain code"};
        }

        const auto& chaincode = *chain_code_;
        const auto& privateKey = *encrypted_key_;
        // Private key data and chain code are encrypted to the same session
        // key, and this session key is only embedded in the private key
        // ciphertext
        auto sessionKey = api_.Symmetric().Key(
            privateKey.key(), proto::SMODE_CHACHA20POLY1305);

        if (false == sessionKey.get()) {
            throw std::runtime_error{"Failed to extract session key"};
        }

        auto allocator = plaintext_chain_code_->WriteInto(Secret::Mode::Mem);

        if (false == sessionKey->Decrypt(chaincode, reason, allocator)) {
            throw std::runtime_error{"Failed to decrypt chain code"};
        }
    }

    return plaintext_chain_code_;
}

auto HD::get_params() const noexcept -> std::tuple<bool, Bip32Depth, Bip32Index>
{
    std::tuple<bool, Bip32Depth, Bip32Index> output{false, 0x0, 0x0};
    auto& [success, depth, child] = output;

    if (false == bool(path_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": missing path").Flush();

        return output;
    }

    const auto& path = *path_;
    auto size = path.child_size();

    if (0 > size) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid depth (")(size)(")")
            .Flush();

        return output;
    }

    if (std::numeric_limits<Bip32Depth>::max() < size) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid depth (")(size)(")")
            .Flush();

        return output;
    }

    depth = static_cast<std::uint8_t>(size);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
    if (0 < depth) {
        const auto& index = *(path_->child().rbegin());
        child = index;
    }
#pragma GCC diagnostic pop

    success = true;

    return output;
}

auto HD::Path() const noexcept -> const std::string
{
    auto path = String::Factory();

    if (path_) {
        if (path_->has_root()) {
            auto root = Identifier::Factory();
            root->SetString(path_->root());
            path->Concatenate(String::Factory(root));

            for (auto& it : path_->child()) {
                path->Concatenate(" / ");
                if (it < HDIndex{Bip32Child::HARDENED}) {
                    path->Concatenate(String::Factory(std::to_string(it)));
                } else {
                    path->Concatenate(String::Factory(
                        std::to_string(it - HDIndex{Bip32Child::HARDENED})));
                    path->Concatenate("'");
                }
            }
        }
    }

    return path->Get();
}

auto HD::Path(proto::HDPath& output) const noexcept -> bool
{
    if (path_) {
        output = *path_;

        return true;
    }

    LogOutput(OT_METHOD)(__FUNCTION__)(": HDPath not instantiated.").Flush();

    return false;
}

auto HD::Serialize() const noexcept -> std::shared_ptr<proto::AsymmetricKey>
{
    auto output = EllipticCurve::Serialize();

    OT_ASSERT(output)

    if (HasPrivate()) {
        if (path_) { *(output->mutable_path()) = *path_; }

        if (chain_code_) { *output->mutable_chaincode() = *chain_code_; }
    }

    if (1 < version_) { output->set_bip32_parent(parent_); }

    return output;
}

auto HD::Xprv(const PasswordPrompt& reason) const noexcept -> std::string
{
    const auto [ready, depth, child] = get_params();

    if (false == ready) { return {}; }

    auto privateKey = api_.Factory().SecretFromBytes(PrivateKey(reason));

    // FIXME Bip32::SerializePrivate should accept ReadView

    return api_.Crypto().BIP32().SerializePrivate(
        0x0488ADE4,
        depth,
        parent_,
        child,
        api_.Factory().Data(Chaincode(reason)),
        privateKey);
}

auto HD::Xpub(const PasswordPrompt& reason) const noexcept -> std::string
{
    const auto [ready, depth, child] = get_params();

    if (false == ready) { return {}; }

    // FIXME Bip32::SerializePublic should accept ReadView

    return api_.Crypto().BIP32().SerializePublic(
        0x0488B21E,
        depth,
        parent_,
        child,
        api_.Factory().Data(Chaincode(reason)),
        api_.Factory().Data(PublicKey()));
}
}  // namespace opentxs::crypto::key::implementation
