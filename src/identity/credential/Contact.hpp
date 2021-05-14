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
#include "opentxs/protobuf/ContactData.pb.h"

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
class Contact final : virtual public credential::internal::Contact,
                      public credential::implementation::Base
{
public:
    auto GetContactData(proto::ContactData& contactData) const -> bool final;

    ~Contact() final = default;

private:
    friend opentxs::Factory;

    const proto::ContactData data_;

    auto serialize(
        const Lock& lock,
        const SerializationModeFlag asPrivate,
        const SerializationSignatureFlag asSigned) const
        -> std::shared_ptr<Base::SerializedType> final;

    Contact(
        const api::internal::Core& api,
        const identity::internal::Authority& parent,
        const identity::Source& source,
        const internal::Primary& master,
        const NymParameters& nymParameters,
        const VersionNumber version,
        const PasswordPrompt& reason) noexcept(false);
    Contact(
        const api::internal::Core& api,
        const identity::internal::Authority& parent,
        const identity::Source& source,
        const internal::Primary& master,
        const proto::Credential& credential) noexcept(false);
    Contact() = delete;
    Contact(const Contact&) = delete;
    Contact(Contact&&) = delete;
    auto operator=(const Contact&) -> Contact& = delete;
    auto operator=(Contact&&) -> Contact& = delete;
};
}  // namespace opentxs::identity::credential::implementation
