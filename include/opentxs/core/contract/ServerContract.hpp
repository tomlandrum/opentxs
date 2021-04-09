// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_SERVERCONTRACT_HPP
#define OPENTXS_CORE_CONTRACT_SERVERCONTRACT_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>

#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/core/contract/Types.hpp"
#include "opentxs/core/Types.hpp"

namespace opentxs
{
namespace contract
{
class Server;
}  // namespace contract

namespace proto
{
class ServerContract;
}  // namespace proto

class PasswordPrompt;

using OTServerContract = SharedPimpl<contract::Server>;
}  // namespace opentxs

namespace opentxs
{
namespace contract
{
class Server : virtual public opentxs::contract::Signable
{
public:
    using Endpoint = std::tuple<
        core::AddressType,
        contract::ProtocolVersion,
        std::string,     // hostname / address
        std::uint32_t,   // port
        VersionNumber>;  // version

    OPENTXS_EXPORT static const VersionNumber DefaultVersion;

    OPENTXS_EXPORT virtual bool ConnectInfo(
        std::string& strHostname,
        std::uint32_t& nPort,
        core::AddressType& actual,
        const core::AddressType& preferred) const = 0;
    OPENTXS_EXPORT virtual proto::ServerContract Contract() const = 0;
    OPENTXS_EXPORT virtual std::string EffectiveName() const = 0;
    OPENTXS_EXPORT virtual proto::ServerContract PublicContract() const = 0;
    OPENTXS_EXPORT virtual bool Statistics(String& strContents) const = 0;
    OPENTXS_EXPORT virtual const Data& TransportKey() const = 0;
    OPENTXS_EXPORT virtual OTSecret TransportKey(
        Data& pubkey,
        const PasswordPrompt& reason) const = 0;

    OPENTXS_EXPORT virtual void InitAlias(const std::string& alias) = 0;

    OPENTXS_EXPORT ~Server() override = default;

protected:
    Server() noexcept = default;

private:
    friend OTServerContract;

#ifndef _WIN32
    OPENTXS_EXPORT Server* clone() const noexcept override = 0;
#endif

    Server(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&) = delete;
};
}  // namespace contract
}  // namespace opentxs
#endif
