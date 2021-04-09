// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/otx/Request.cpp"

#pragma once

#include <string>

#include "core/contract/Signable.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/otx/Request.hpp"
#include "opentxs/otx/Types.hpp"
#include "opentxs/protobuf/OTXEnums.pb.h"
#include "opentxs/protobuf/ServerRequest.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace proto
{
class Signature;
}  // namespace proto

class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::otx::implementation
{
class Request final : public otx::Request,
                      public opentxs::contract::implementation::Signable
{
public:
    auto Contract() const -> proto::ServerRequest final;
    auto Initiator() const -> const identifier::Nym& final
    {
        return initiator_;
    }
    auto Number() const -> RequestNumber final;
    auto Server() const -> const identifier::Server& final { return server_; }
    auto Type() const -> otx::ServerRequestType final { return type_; }

    auto SetIncludeNym(const bool include, const PasswordPrompt& reason)
        -> bool final;

    ~Request() final = default;

private:
    friend otx::Request;

    const OTNymID initiator_;
    const OTServerID server_;
    const otx::ServerRequestType type_;
    const RequestNumber number_;
    OTFlag include_nym_;

    static auto extract_nym(
        const api::internal::Core& api,
        const proto::ServerRequest serialized) -> Nym_p;

    auto clone() const noexcept -> Request* final { return new Request(*this); }
    auto GetID(const Lock& lock) const -> OTIdentifier final;
    auto full_version(const Lock& lock) const -> proto::ServerRequest;
    auto id_version(const Lock& lock) const -> proto::ServerRequest;
    auto Name() const -> std::string final { return {}; }
    auto Serialize() const -> OTData final;
    auto signature_version(const Lock& lock) const -> proto::ServerRequest;
    auto update_signature(const Lock& lock, const PasswordPrompt& reason)
        -> bool final;
    auto validate(const Lock& lock) const -> bool final;
    auto verify_signature(const Lock& lock, const proto::Signature& signature)
        const -> bool final;

    Request(
        const api::internal::Core& api,
        const Nym_p signer,
        const identifier::Nym& initiator,
        const identifier::Server& server,
        const otx::ServerRequestType type,
        const RequestNumber number);
    Request(
        const api::internal::Core& api,
        const proto::ServerRequest serialized);
    Request() = delete;
    Request(const Request& rhs);
    Request(Request&& rhs) = delete;
    auto operator=(const Request& rhs) -> Request& = delete;
    auto operator=(Request&& rhs) -> Request& = delete;
};
}  // namespace opentxs::otx::implementation
