// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_BAILMENTREQUEST_HPP
#define OPENTXS_CORE_CONTRACT_PEER_BAILMENTREQUEST_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include "opentxs/SharedPimpl.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"

namespace opentxs
{
namespace identifier
{
class Server;
class UnitDefinition;
}  // namespace identifier
namespace contract
{
namespace peer
{
namespace request
{
class Bailment;
}  // namespace request
}  // namespace peer
}  // namespace contract

using OTBailmentRequest = SharedPimpl<contract::peer::request::Bailment>;
}  // namespace opentxs

namespace opentxs
{
namespace contract
{
namespace peer
{
namespace request
{
class OPENTXS_EXPORT Bailment : virtual public peer::Request
{
public:
    ~Bailment() override = default;

    OPENTXS_EXPORT virtual const identifier::Server& ServerID() const = 0;
    OPENTXS_EXPORT virtual const identifier::UnitDefinition& UnitID() const = 0;

protected:
    Bailment() noexcept = default;

private:
    friend OTBailmentRequest;

#ifndef _WIN32
    Bailment* clone() const noexcept override = 0;
#endif

    Bailment(const Bailment&) = delete;
    Bailment(Bailment&&) = delete;
    Bailment& operator=(const Bailment&) = delete;
    Bailment& operator=(Bailment&&) = delete;
};
}  // namespace request
}  // namespace peer
}  // namespace contract
}  // namespace opentxs
#endif
