// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/crypto/library/AsymmetricProvider.hpp"

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

class PasswordPrompt;
class Signature;
class String;
}  // namespace opentxs

namespace opentxs::crypto::implementation
{
class AsymmetricProvider : virtual public crypto::AsymmetricProvider
{
public:
    auto SeedToCurveKey(
        const ReadView seed,
        const AllocateOutput privateKey,
        const AllocateOutput publicKey) const noexcept -> bool final;
    auto SignContract(
        const api::internal::Core& api,
        const String& strContractUnsigned,
        const key::Asymmetric& theKey,
        Signature& theSignature,  // output
        const crypto::HashType hashType,
        const PasswordPrompt& reason) const -> bool override;
    auto VerifyContractSignature(
        const String& strContractToVerify,
        const key::Asymmetric& theKey,
        const Signature& theSignature,
        const crypto::HashType hashType) const -> bool override;

    ~AsymmetricProvider() override = default;

protected:
    AsymmetricProvider() noexcept;

private:
    AsymmetricProvider(const AsymmetricProvider&) = delete;
    AsymmetricProvider(AsymmetricProvider&&) = delete;
    auto operator=(const AsymmetricProvider&) -> AsymmetricProvider& = delete;
    auto operator=(AsymmetricProvider&&) -> AsymmetricProvider& = delete;
};
}  // namespace opentxs::crypto::implementation
