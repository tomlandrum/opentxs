// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "opentxs/crypto/library/AsymmetricProvider.hpp"

namespace opentxs::crypto::implementation
{
class AsymmetricProviderNull final : virtual public crypto::AsymmetricProvider
{
public:
    auto RandomKeypair(
        const AllocateOutput,
        const AllocateOutput,
        const identity::KeyRole,
        const NymParameters&,
        const AllocateOutput) const noexcept -> bool final
    {
        return false;
    }
    auto SeedToCurveKey(
        const ReadView,
        const AllocateOutput,
        const AllocateOutput) const noexcept -> bool final
    {
        return false;
    }
    auto SharedSecret(
        const key::Asymmetric&,
        const key::Asymmetric&,
        const SecretStyle,
        const PasswordPrompt&,
        Secret&) const noexcept -> bool final
    {
        return false;
    }
    auto Sign(
        const api::internal::Core&,
        const ReadView,
        const key::Asymmetric&,
        const proto::HashType,
        const AllocateOutput,
        const PasswordPrompt&) const -> bool final
    {
        return false;
    }
    auto SignContract(
        const api::internal::Core&,
        const String&,
        const key::Asymmetric&,
        Signature&,  // output
        const proto::HashType,
        const PasswordPrompt&) const -> bool final
    {
        return false;
    }
    auto Verify(
        const Data&,
        const key::Asymmetric&,
        const Data&,
        const proto::HashType) const -> bool final
    {
        return false;
    }
    auto VerifyContractSignature(
        const String&,
        const key::Asymmetric&,
        const Signature&,
        const proto::HashType) const -> bool final
    {
        return false;
    }

    AsymmetricProviderNull() = default;
    ~AsymmetricProviderNull() final = default;

private:
    AsymmetricProviderNull(const AsymmetricProviderNull&) = delete;
    AsymmetricProviderNull(AsymmetricProviderNull&&) = delete;
    auto operator=(const AsymmetricProviderNull&)
        -> AsymmetricProviderNull& = delete;
    auto operator=(AsymmetricProviderNull&&)
        -> AsymmetricProviderNull& = delete;
};
}  // namespace opentxs::crypto::implementation
