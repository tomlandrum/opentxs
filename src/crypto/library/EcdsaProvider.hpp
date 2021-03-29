// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "opentxs/Bytes.hpp"
#include "opentxs/crypto/library/EcdsaProvider.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal

class Crypto;
}  // namespace api

namespace crypto
{
namespace key
{
class Asymmetric;
}  // namespace key
}  // namespace crypto

class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::crypto::implementation
{
class EcdsaProvider : virtual public crypto::EcdsaProvider
{
public:
    bool SignDER(
        [[maybe_unused]] const api::internal::Core& api,
        [[maybe_unused]] const ReadView plaintext,
        [[maybe_unused]] const key::Asymmetric& key,
        [[maybe_unused]] const crypto::HashType hash,
        [[maybe_unused]] Space& signature,
        [[maybe_unused]] const PasswordPrompt& reason) const noexcept override
    {
        return false;
    }

    ~EcdsaProvider() override = default;

protected:
    const api::Crypto& crypto_;

    EcdsaProvider(const api::Crypto& crypto);

private:
    EcdsaProvider() = delete;
    EcdsaProvider(const EcdsaProvider&) = delete;
    EcdsaProvider(EcdsaProvider&&) = delete;
    auto operator=(const EcdsaProvider&) -> EcdsaProvider& = delete;
    auto operator=(EcdsaProvider&&) -> EcdsaProvider& = delete;
};
}  // namespace opentxs::crypto::implementation
