// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"            // IWYU pragma: associated
#include "1_Internal.hpp"          // IWYU pragma: associated
#include "crypto/key/Keypair.hpp"  // IWYU pragma: associated

#include <stdexcept>
#include <utility>

#include "crypto/key/Null.hpp"
#include "internal/crypto/key/Factory.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/crypto/OTSignatureMetadata.hpp"
#include "opentxs/core/crypto/Signature.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/identity/KeyRole.hpp"

// #define OT_METHOD "opentxs::crypto::key::implementation::Keypair::"

template class opentxs::Pimpl<opentxs::crypto::key::Keypair>;

namespace opentxs::factory
{
using ReturnType = crypto::key::implementation::Keypair;

auto Keypair() noexcept -> std::unique_ptr<crypto::key::Keypair>
{
    return std::make_unique<crypto::key::implementation::NullKeypair>();
}

auto Keypair(
    const api::internal::Core& api,
    const identity::KeyRole role,
    std::unique_ptr<crypto::key::Asymmetric> publicKey,
    std::unique_ptr<crypto::key::Asymmetric> privateKey) noexcept(false)
    -> std::unique_ptr<crypto::key::Keypair>
{
    if (false == bool(publicKey)) {
        throw std::runtime_error("Invalid public key");
    }

    if (false == bool(privateKey)) {
        throw std::runtime_error("Invalid private key");
    }

    return std::make_unique<ReturnType>(
        api, role, std::move(publicKey), std::move(privateKey));
}
}  // namespace opentxs::factory

namespace opentxs::crypto::key::implementation
{
Keypair::Keypair(
    const api::internal::Core& api,
    const identity::KeyRole role,
    std::unique_ptr<crypto::key::Asymmetric> publicKey,
    std::unique_ptr<crypto::key::Asymmetric> privateKey) noexcept
    : api_(api)
    , m_pkeyPrivate(privateKey.release())
    , m_pkeyPublic(publicKey.release())
    , role_(role)
{
    OT_ASSERT(m_pkeyPublic.get());
}

Keypair::Keypair(const Keypair& rhs) noexcept
    : api_(rhs.api_)
    , m_pkeyPrivate(rhs.m_pkeyPrivate)
    , m_pkeyPublic(rhs.m_pkeyPublic)
    , role_(rhs.role_)
{
}

auto Keypair::CheckCapability(const NymCapability& capability) const noexcept
    -> bool
{
    bool output{false};

    if (m_pkeyPrivate.get()) {
        output |= m_pkeyPrivate->hasCapability(capability);
    } else if (m_pkeyPublic.get()) {
        output |= m_pkeyPublic->hasCapability(capability);
    }

    return output;
}

// Return the private key as an Asymmetric object
auto Keypair::GetPrivateKey() const -> const Asymmetric&
{
    if (m_pkeyPrivate.get()) { return m_pkeyPrivate; }

    throw std::runtime_error("private key missing");
}

// Return the public key as an Asymmetric object
auto Keypair::GetPublicKey() const -> const Asymmetric&
{
    if (m_pkeyPublic.get()) { return m_pkeyPublic; }

    throw std::runtime_error("public key missing");
}

auto Keypair::GetPublicKeyBySignature(
    Keys& listOutput,  // Inclusive means, return the key even
                       // when theSignature has no metadata.
    const Signature& theSignature,
    bool bInclusive) const noexcept -> std::int32_t
{
    OT_ASSERT(m_pkeyPublic.get());

    const auto* metadata = m_pkeyPublic->GetMetadata();

    OT_ASSERT(nullptr != metadata);

    // We know that EITHER exact metadata matches must occur, and the signature
    // MUST have metadata, (bInclusive=false)
    // OR if bInclusive=true, we know that metadata is still used to eliminate
    // keys where possible, but that otherwise,
    // if the signature has no metadata, then the key is still returned, "just
    // in case."
    //
    if ((false == bInclusive) &&
        (false == theSignature.getMetaData().HasMetadata()))
        return 0;

    // Below this point, metadata is used if it's available.
    // It's assumed to be "okay" if it's not available, since any non-inclusive
    // calls would have already returned by now, if that were the case.
    // (But if it IS available, then it must match, or the key won't be
    // returned.)
    //
    // If the signature has no metadata, or if m_pkeyPublic has no metadata, or
    // if they BOTH have metadata, and their metadata is a MATCH...
    if (!theSignature.getMetaData().HasMetadata() || !metadata->HasMetadata() ||
        (metadata->HasMetadata() && theSignature.getMetaData().HasMetadata() &&
         (theSignature.getMetaData() == *(metadata)))) {
        // ...Then add m_pkeyPublic as a possible match, to listOutput.
        //
        listOutput.push_back(&m_pkeyPublic.get());
        return 1;
    }
    return 0;
}

auto Keypair::GetSerialized(bool privateKey) const noexcept
    -> std::shared_ptr<proto::AsymmetricKey>
{
    OT_ASSERT(m_pkeyPublic.get());

    if (privateKey) {
        OT_ASSERT(m_pkeyPrivate.get());

        return m_pkeyPrivate->Serialize();
    } else {
        return m_pkeyPublic->Serialize();
    }
}

auto Keypair::GetTransportKey(
    Data& publicKey,
    Secret& privateKey,
    const opentxs::PasswordPrompt& reason) const noexcept -> bool
{
    return m_pkeyPrivate->TransportKey(publicKey, privateKey, reason);
}
}  // namespace opentxs::crypto::key::implementation
