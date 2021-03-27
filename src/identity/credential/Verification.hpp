// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>

#include "identity/credential/Base.hpp"
#include "internal/identity/credential/Credential.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/protobuf/VerificationSet.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace identity
{
namespace internal
{
struct Authority;
}  // namespace internal

class Source;
}  // namespace identity

namespace proto
{
class Credential;
}  // namespace proto

class Factory;
class NymParameters;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::identity::credential::implementation
{
class Verification final : virtual public credential::internal::Verification,
                           public credential::implementation::Base
{
public:
    auto GetVerificationSet(
        std::unique_ptr<proto::VerificationSet>& verificationSet) const
        -> bool final;

    ~Verification() final = default;

private:
    using KeyModeMap = std::map<identity::KeyMode, proto::KeyMode>;

    static const KeyModeMap keymode_map_;

    friend opentxs::Factory;

    const proto::VerificationSet data_;

    auto serialize(
        const Lock& lock,
        const SerializationModeFlag asPrivate,
        const SerializationSignatureFlag asSigned) const
        -> std::shared_ptr<Base::SerializedType> final;
    auto verify_internally(const Lock& lock) const -> bool final;

    Verification(
        const api::internal::Core& api,
        const identity::internal::Authority& parent,
        const identity::Source& source,
        const internal::Primary& master,
        const NymParameters& nymParameters,
        const VersionNumber version,
        const PasswordPrompt& reason) noexcept(false);
    Verification(
        const api::internal::Core& api,
        const identity::internal::Authority& parent,
        const identity::Source& source,
        const internal::Primary& master,
        const proto::Credential& credential) noexcept(false);
    Verification() = delete;
    Verification(const Verification&) = delete;
    Verification(Verification&&) = delete;
    auto operator=(const Verification&) -> Verification& = delete;
    auto operator=(Verification&&) -> Verification& = delete;
};
}  // namespace opentxs::identity::credential::implementation
