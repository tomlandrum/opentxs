// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_LIBRARY_ECDSAPROVIDER_HPP
#define OPENTXS_CRYPTO_LIBRARY_ECDSAPROVIDER_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include "opentxs/crypto/library/AsymmetricProvider.hpp"

namespace opentxs
{
namespace crypto
{
class EcdsaProvider : virtual public AsymmetricProvider
{
public:
    OPENTXS_EXPORT virtual bool PubkeyAdd(
        const ReadView pubkey,
        const ReadView scalar,
        const AllocateOutput result) const noexcept = 0;
    OPENTXS_EXPORT virtual bool ScalarAdd(
        const ReadView lhs,
        const ReadView rhs,
        const AllocateOutput result) const noexcept = 0;
    OPENTXS_EXPORT virtual bool ScalarMultiplyBase(
        const ReadView scalar,
        const AllocateOutput result) const noexcept = 0;
    OPENTXS_EXPORT virtual bool SignDER(
        const api::internal::Core& api,
        const ReadView plaintext,
        const key::Asymmetric& key,
        const crypto::HashType hash,
        Space& signature,
        const PasswordPrompt& reason) const noexcept = 0;

    ~EcdsaProvider() override = default;

protected:
    EcdsaProvider() = default;

private:
    EcdsaProvider(const EcdsaProvider&) = delete;
    EcdsaProvider(EcdsaProvider&&) = delete;
    EcdsaProvider& operator=(const EcdsaProvider&) = delete;
    EcdsaProvider& operator=(EcdsaProvider&&) = delete;
};
}  // namespace crypto
}  // namespace opentxs
#endif
