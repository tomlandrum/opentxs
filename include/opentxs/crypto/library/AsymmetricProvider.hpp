// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_LIBRARY_ASYMMETRICPROVIDER_HPP
#define OPENTXS_CRYPTO_LIBRARY_ASYMMETRICPROVIDER_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <optional>

#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/identity/KeyRole.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace crypto
{
namespace key
{
class Asymmetric;
}  // namespace key
}  // namespace crypto
}  // namespace opentxs

namespace opentxs
{
namespace crypto
{
class AsymmetricProvider
{
public:
    OPENTXS_EXPORT static crypto::AsymmetricKeyType CurveToKeyType(
        const EcdsaCurve& curve);
    OPENTXS_EXPORT static EcdsaCurve KeyTypeToCurve(
        const crypto::AsymmetricKeyType& type);

    OPENTXS_EXPORT virtual bool SeedToCurveKey(
        const ReadView seed,
        const AllocateOutput privateKey,
        const AllocateOutput publicKey) const noexcept = 0;
    OPENTXS_EXPORT virtual bool SharedSecret(
        const key::Asymmetric& publicKey,
        const key::Asymmetric& privateKey,
        const SecretStyle style,
        const PasswordPrompt& reason,
        Secret& secret) const noexcept = 0;
    OPENTXS_EXPORT virtual bool RandomKeypair(
        const AllocateOutput privateKey,
        const AllocateOutput publicKey,
        const identity::KeyRole role = identity::KeyRole::Sign,
        const NymParameters& options = {},
        const AllocateOutput params = {}) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Sign(
        const api::internal::Core& api,
        const ReadView plaintext,
        const key::Asymmetric& key,
        const proto::HashType hash,
        const AllocateOutput signature,
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual bool SignContract(
        const api::internal::Core& api,
        const String& strContractUnsigned,
        const key::Asymmetric& theKey,
        Signature& theSignature,  // output
        const proto::HashType hashType,
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual bool Verify(
        const Data& plaintext,
        const key::Asymmetric& theKey,
        const Data& signature,
        const proto::HashType hashType) const = 0;
    OPENTXS_EXPORT virtual bool VerifyContractSignature(
        const String& strContractToVerify,
        const key::Asymmetric& theKey,
        const Signature& theSignature,
        const proto::HashType hashType) const = 0;

    virtual ~AsymmetricProvider() = default;

protected:
    AsymmetricProvider() = default;

private:
    AsymmetricProvider(const AsymmetricProvider&) = delete;
    AsymmetricProvider(AsymmetricProvider&&) = delete;
    AsymmetricProvider& operator=(const AsymmetricProvider&) = delete;
    AsymmetricProvider& operator=(AsymmetricProvider&&) = delete;
};
}  // namespace crypto
}  // namespace opentxs
#endif
