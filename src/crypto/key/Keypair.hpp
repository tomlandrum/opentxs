// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <memory>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/identity/Types.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace proto
{
class AsymmetricKey;
}  // namespace proto

class Data;
class OTPassword;
class PasswordPrompt;
class Secret;
class Signature;
}  // namespace opentxs

namespace opentxs::crypto::key::implementation
{
class Keypair final : virtual public key::Keypair
{
public:
    operator bool() const noexcept final { return true; }

    auto CheckCapability(const NymCapability& capability) const noexcept
        -> bool final;
    auto GetPrivateKey() const noexcept(false) -> const Asymmetric& final;
    auto GetPublicKey() const noexcept(false) -> const Asymmetric& final;
    auto GetPublicKeyBySignature(
        Keys& listOutput,
        const Signature& theSignature,
        bool bInclusive = false) const noexcept -> std::int32_t final;
    auto GetSerialized(bool privateKey = false) const noexcept
        -> std::shared_ptr<proto::AsymmetricKey> final;
    auto GetTransportKey(
        Data& publicKey,
        Secret& privateKey,
        const PasswordPrompt& reason) const noexcept -> bool final;

    Keypair(
        const api::internal::Core& api,
        const identity::KeyRole role,
        std::unique_ptr<crypto::key::Asymmetric> publicKey,
        std::unique_ptr<crypto::key::Asymmetric> privateKey) noexcept;

    ~Keypair() final = default;

private:
    friend key::Keypair;

    const api::internal::Core& api_;
    OTAsymmetricKey m_pkeyPrivate;
    OTAsymmetricKey m_pkeyPublic;
    const identity::KeyRole role_;

    auto clone() const -> Keypair* final { return new Keypair(*this); }

    Keypair() = delete;
    Keypair(const Keypair&) noexcept;
    Keypair(Keypair&&) = delete;
    auto operator=(const Keypair&) -> Keypair& = delete;
    auto operator=(Keypair&&) -> Keypair& = delete;
};
}  // namespace opentxs::crypto::key::implementation
