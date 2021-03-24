// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                  // IWYU pragma: associated
#include "1_Internal.hpp"                // IWYU pragma: associated
#include "crypto/key/EllipticCurve.hpp"  // IWYU pragma: associated

#include <stdexcept>
#include <utility>

#include "crypto/key/Asymmetric.hpp"
#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"
#include "opentxs/crypto/library/EcdsaProvider.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/protobuf/AsymmetricKey.pb.h"
#include "opentxs/protobuf/Ciphertext.pb.h"

#define OT_METHOD "opentxs::crypto::key::implementation::EllipticCurve::"

namespace opentxs::crypto::key
{
const VersionNumber EllipticCurve::DefaultVersion{2};
const VersionNumber EllipticCurve::MaxVersion{2};
}  // namespace opentxs::crypto::key

namespace opentxs::crypto::key::implementation
{
EllipticCurve::EllipticCurve(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const proto::AsymmetricKey& serialized) noexcept(false)
    : Asymmetric(
          api,
          ecdsa,
          serialized,
          [&](auto& pubkey, auto&) -> EncryptedKey {
              return extract_key(api, ecdsa, serialized, pubkey);
          })
    , ecdsa_(ecdsa)
{
}

EllipticCurve::EllipticCurve(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const crypto::AsymmetricKeyType keyType,
    const proto::KeyRole role,
    const VersionNumber version,
    const PasswordPrompt& reason) noexcept(false)
    : Asymmetric(
          api,
          ecdsa,
          keyType,
          role,
          version,
          [&](auto& pub, auto& prv) -> EncryptedKey {
              return create_key(
                  api,
                  ecdsa,
                  {},
                  role,
                  pub.WriteInto(),
                  prv.WriteInto(Secret::Mode::Mem),
                  prv,
                  {},
                  reason);
          })
    , ecdsa_(ecdsa)
{
    if (false == bool(encrypted_key_)) {
        throw std::runtime_error("Failed to instantiate encrypted_key_");
    }
}

EllipticCurve::EllipticCurve(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const crypto::AsymmetricKeyType keyType,
    const Secret& privateKey,
    const Data& publicKey,
    const proto::KeyRole role,
    const VersionNumber version,
    key::Symmetric& sessionKey,
    const PasswordPrompt& reason) noexcept(false)
    : Asymmetric(
          api,
          ecdsa,
          keyType,
          role,
          true,
          (false == privateKey.empty()),
          version,
          OTData{publicKey},
          [&](auto&, auto&) -> EncryptedKey {
              return encrypt_key(sessionKey, reason, true, privateKey.Bytes());
          })
    , ecdsa_(ecdsa)
{
    if (has_private_ && !encrypted_key_) {
        throw std::runtime_error("Failed to instantiate encrypted_key_");
    }
}

EllipticCurve::EllipticCurve(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const proto::AsymmetricKeyType keyType,
    const Secret& privateKey,
    const Data& publicKey,
    const proto::KeyRole role,
    const VersionNumber version) noexcept(false)
    : Asymmetric(
          api,
          ecdsa,
          keyType,
          role,
          true,
          (false == privateKey.empty()),
          version,
          OTData{publicKey},
          {})
    , ecdsa_(ecdsa)
{
}

EllipticCurve::EllipticCurve(const EllipticCurve& rhs) noexcept
    : Asymmetric(rhs)
    , ecdsa_(rhs.ecdsa_)
{
}

EllipticCurve::EllipticCurve(
    const EllipticCurve& rhs,
    const ReadView newPublic) noexcept
    : Asymmetric(rhs, newPublic)
    , ecdsa_(rhs.ecdsa_)
{
}

EllipticCurve::EllipticCurve(
    const EllipticCurve& rhs,
    OTSecret&& newSecretKey) noexcept
    : Asymmetric(
          rhs,
          [&] {
              auto pubkey = rhs.api_.Factory().Data();
              const auto rc = rhs.ecdsa_.ScalarMultiplyBase(
                  newSecretKey->Bytes(), pubkey->WriteInto());

              if (rc) {

                  return pubkey;
              } else {
                  LogOutput(OT_METHOD)(__FUNCTION__)(
                      ": Failed to calculate public key")
                      .Flush();

                  return rhs.api_.Factory().Data();
              }
          }(),
          std::move(newSecretKey))
    , ecdsa_(rhs.ecdsa_)
{
}

auto EllipticCurve::asPublicEC() const noexcept
    -> std::unique_ptr<key::EllipticCurve>
{
    auto output = std::unique_ptr<EllipticCurve>{clone_ec()};

    OT_ASSERT(output);

    auto& copy = *output;
    copy.erase_private_data();

    OT_ASSERT(false == copy.HasPrivate());

    return std::move(output);
}

auto EllipticCurve::extract_key(
    const api::internal::Core& api,
    const crypto::EcdsaProvider& ecdsa,
    const proto::AsymmetricKey& proto,
    Data& publicKey) -> std::unique_ptr<proto::Ciphertext>
{
    auto output = std::unique_ptr<proto::Ciphertext>{};
    publicKey.Assign(proto.key());

    if ((proto::KEYMODE_PRIVATE == proto.mode()) && proto.has_encryptedkey()) {
        output = std::make_unique<proto::Ciphertext>(proto.encryptedkey());

        OT_ASSERT(output);
    }

    return output;
}

auto EllipticCurve::IncrementPrivate(
    const Secret& rhs,
    const PasswordPrompt& reason) const noexcept
    -> std::unique_ptr<key::EllipticCurve>
{
    try {
        const auto& lhs = get_private_key(reason);
        auto newKey = api_.Factory().Secret(0);
        auto rc =
            ecdsa_.ScalarAdd(lhs.Bytes(), rhs.Bytes(), newKey->WriteInto());

        if (false == rc) {
            throw std::runtime_error("Failed to increment private key");
        }

        return replace_secret_key(std::move(newKey));
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto EllipticCurve::IncrementPublic(const Secret& rhs) const noexcept
    -> std::unique_ptr<key::EllipticCurve>
{
    try {
        auto newKey = Space{};
        auto rc = ecdsa_.PubkeyAdd(key_->Bytes(), rhs.Bytes(), writer(newKey));

        if (false == rc) {
            throw std::runtime_error("Failed to increment public key");
        }

        return replace_public_key(reader(newKey));
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto EllipticCurve::serialize_public(EllipticCurve* in)
    -> std::shared_ptr<proto::AsymmetricKey>
{
    std::unique_ptr<EllipticCurve> copy{in};

    OT_ASSERT(copy);

    copy->erase_private_data();

    return copy->Serialize();
}

auto EllipticCurve::SignDER(
    const ReadView preimage,
    const proto::HashType hash,
    Space& output,
    const PasswordPrompt& reason) const noexcept -> bool
{
    if (false == has_private_) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing private key").Flush();

        return false;
    }

    bool success = ecdsa_.SignDER(api_, preimage, *this, hash, output, reason);

    if (false == success) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to sign preimage").Flush();
    }

    return success;
}
}  // namespace opentxs::crypto::key::implementation
