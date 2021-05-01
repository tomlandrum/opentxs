// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_OTX_REQUEST_HPP
#define OPENTXS_OTX_REQUEST_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <memory>

#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/otx/Types.hpp"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace otx
{
class Request;
}  // namespace otx

namespace proto
{
class ServerRequest;
}  // namespace proto

using OTXRequest = Pimpl<otx::Request>;
}  // namespace opentxs

namespace opentxs
{
namespace otx
{
class Request : virtual public opentxs::contract::Signable
{
public:
    OPENTXS_EXPORT static const VersionNumber DefaultVersion;
    OPENTXS_EXPORT static const VersionNumber MaxVersion;

    OPENTXS_EXPORT static Pimpl<opentxs::otx::Request> Factory(
        const api::internal::Core& api,
        const Nym_p signer,
        const identifier::Server& server,
        const otx::ServerRequestType type,
        const RequestNumber number,
        const PasswordPrompt& reason);
    OPENTXS_EXPORT static Pimpl<opentxs::otx::Request> Factory(
        const api::internal::Core& api,
        const proto::ServerRequest serialized);
    OPENTXS_EXPORT static Pimpl<opentxs::otx::Request> Factory(
        const api::internal::Core& api,
        const ReadView& view);

    OPENTXS_EXPORT virtual proto::ServerRequest Contract() const = 0;
    OPENTXS_EXPORT virtual bool Contract(AllocateOutput destination) const = 0;
    OPENTXS_EXPORT virtual const identifier::Nym& Initiator() const = 0;
    OPENTXS_EXPORT virtual RequestNumber Number() const = 0;
    OPENTXS_EXPORT virtual const identifier::Server& Server() const = 0;
    OPENTXS_EXPORT virtual otx::ServerRequestType Type() const = 0;

    OPENTXS_EXPORT virtual bool SetIncludeNym(
        const bool include,
        const PasswordPrompt& reason) = 0;

    OPENTXS_EXPORT ~Request() override = default;

protected:
    Request() = default;

private:
    friend OTXRequest;

#ifndef _WIN32
    Request* clone() const noexcept override = 0;
#endif

    Request(const Request&) = delete;
    Request(Request&&) = delete;
    Request& operator=(const Request&) = delete;
    Request& operator=(Request&&) = delete;
};
}  // namespace otx
}  // namespace opentxs
#endif
