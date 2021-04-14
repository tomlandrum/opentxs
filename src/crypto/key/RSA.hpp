// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>

#include "crypto/key/Asymmetric.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/key/RSA.hpp"

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

class AsymmetricProvider;
}  // namespace crypto

namespace proto
{
class AsymmetricKey;
class Ciphertext;
}  // namespace proto

class NymParameters;
class OTPassword;
class PasswordPrompt;
class Secret;
}  // namespace opentxs

namespace opentxs::crypto::key::implementation
{
class RSA final : public key::RSA, public Asymmetric
{
public:
    auto asPublic() const noexcept -> std::unique_ptr<key::Asymmetric> final;
    auto Params() const noexcept -> ReadView final { return params_->Bytes(); }
    auto Serialize() const noexcept
        -> std::shared_ptr<proto::AsymmetricKey> final;
    auto SigHashType() const noexcept -> crypto::HashType final
    {
        return crypto::HashType::Sha256;
    }

    RSA(const api::internal::Core& api,
        const crypto::AsymmetricProvider& engine,
        const crypto::key::asymmetric::Role role,
        const VersionNumber version,
        const NymParameters& options,
        Space& params,
        const PasswordPrompt& reason)
    noexcept(false);
    RSA(const api::internal::Core& api,
        const crypto::AsymmetricProvider& engine,
        const proto::AsymmetricKey& serialized)
    noexcept(false);
    RSA(const RSA&) noexcept;

    ~RSA() final = default;

private:
    const OTData params_;

    static auto deserialize_key(
        const api::internal::Core& api,
        const proto::AsymmetricKey& serialized,
        Data& publicKey,
        Secret& privateKey) noexcept(false)
        -> std::unique_ptr<proto::Ciphertext>;

    auto clone() const noexcept -> RSA* final { return new RSA{*this}; }

    RSA() = delete;
    RSA(RSA&&) = delete;
    auto operator=(const RSA&) -> RSA& = delete;
    auto operator=(RSA&&) -> RSA& = delete;
};
}  // namespace opentxs::crypto::key::implementation
