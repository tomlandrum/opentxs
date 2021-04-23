// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/api/crypto/Symmetric.cpp"

#pragma once

#include <cstdint>
#include <iosfwd>

#include "opentxs/Proto.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/crypto/key/symmetric/Algorithm.hpp"
#include "opentxs/crypto/key/symmetric/Source.hpp"

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
class SymmetricProvider;
}  // namespace crypto

namespace proto
{
class SymmetricKey;
}  // namespace proto

class OTPassword;
class PasswordPrompt;
class Secret;
}  // namespace opentxs

namespace opentxs::api::crypto::implementation
{
class Symmetric final : virtual public api::crypto::Symmetric
{
public:
    virtual auto IvSize(const opentxs::crypto::key::symmetric::Algorithm mode)
        const -> std::size_t final;
    auto Key(
        const PasswordPrompt& password,
        const opentxs::crypto::key::symmetric::Algorithm mode =
            opentxs::crypto::key::symmetric::Algorithm::ChaCha20Poly1305) const
        -> OTSymmetricKey final;
    auto Key(
        const proto::SymmetricKey& serialized,
        const opentxs::crypto::key::symmetric::Algorithm mode) const
        -> OTSymmetricKey final;
    auto Key(
        const Space& serializedCiphertext,
        const opentxs::crypto::key::symmetric::Algorithm mode) const
        -> OTSymmetricKey final;
    auto Key(
        const Secret& seed,
        const std::uint64_t operations = 0,
        const std::uint64_t difficulty = 0,
        const opentxs::crypto::key::symmetric::Algorithm mode =
            opentxs::crypto::key::symmetric::Algorithm::ChaCha20Poly1305,
        const opentxs::crypto::key::symmetric::Source type =
            opentxs::crypto::key::symmetric::Source::Argon2) const
        -> OTSymmetricKey final;

    Symmetric(const api::internal::Core& api) noexcept;

    ~Symmetric() final = default;

private:
    const api::internal::Core& api_;

    auto GetEngine(const opentxs::crypto::key::symmetric::Algorithm mode) const
        -> const opentxs::crypto::SymmetricProvider*;

    Symmetric() = delete;
    Symmetric(const Symmetric&) = delete;
    Symmetric(Symmetric&&) = delete;
    auto operator=(const Symmetric&) -> Symmetric& = delete;
    auto operator=(Symmetric&&) -> Symmetric& = delete;
};
}  // namespace opentxs::api::crypto::implementation
