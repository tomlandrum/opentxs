// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>
#include <string>

#include "identity/credential/Base.hpp"
#include "identity/credential/Key.hpp"
#include "internal/identity/credential/Credential.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/identity/credential/Base.hpp"
#include "opentxs/protobuf/SourceProof.pb.h"

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
class HDPath;
class Signature;
}  // namespace proto

class Factory;
class Identifier;
class NymParameters;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::identity::credential::implementation
{
class Primary final : virtual public credential::internal::Primary,
                      credential::implementation::Key
{
public:
    auto hasCapability(const NymCapability& capability) const -> bool final;
    auto Path(proto::HDPath& output) const -> bool final;
    auto Path() const -> std::string final;
    using Base::Verify;
    auto Verify(
        const proto::Credential& credential,
        const identity::CredentialRole& role,
        const Identifier& masterID,
        const proto::Signature& masterSig) const -> bool final;

    ~Primary() final = default;

private:
    friend opentxs::Factory;

    using SourceProofTypeMap =
        std::map<identity::SourceProofType, proto::SourceProofType>;
    using SourceProofTypeReverseMap =
        std::map<proto::SourceProofType, identity::SourceProofType>;

    static const VersionConversionMap credential_to_master_params_;

    const proto::SourceProof source_proof_;

    static auto source_proof(const NymParameters& params) -> proto::SourceProof;
    static auto sourceprooftype_map() noexcept -> const SourceProofTypeMap&;
    static auto translate(const identity::SourceProofType in) noexcept
        -> proto::SourceProofType;
    static auto translate(const proto::SourceProofType in) noexcept
        -> identity::SourceProofType;

    auto serialize(
        const Lock& lock,
        const SerializationModeFlag asPrivate,
        const SerializationSignatureFlag asSigned) const
        -> std::shared_ptr<identity::credential::Base::SerializedType> final;
    auto verify_against_source(const Lock& lock) const -> bool;
    auto verify_internally(const Lock& lock) const -> bool final;

    void sign(
        const identity::credential::internal::Primary& master,
        const PasswordPrompt& reason) noexcept(false) final;

    Primary(
        const api::internal::Core& api,
        const identity::internal::Authority& parent,
        const identity::Source& source,
        const NymParameters& nymParameters,
        const VersionNumber version,
        const PasswordPrompt& reason) noexcept(false);
    Primary(
        const api::internal::Core& api,
        const identity::internal::Authority& parent,
        const identity::Source& source,
        const proto::Credential& serializedCred) noexcept(false);
    Primary() = delete;
    Primary(const Primary&) = delete;
    Primary(Primary&&) = delete;
    auto operator=(const Primary&) -> Primary& = delete;
    auto operator=(Primary&&) -> Primary& = delete;
};
}  // namespace opentxs::identity::credential::implementation
