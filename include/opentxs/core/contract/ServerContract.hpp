// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_SERVERCONTRACT_HPP
#define OPENTXS_CORE_CONTRACT_SERVERCONTRACT_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>

#include "opentxs/SharedPimpl.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/Types.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/core/contract/Types.hpp"

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
class OPENTXS_EXPORT Server : virtual public opentxs::contract::Signable
{
public:
    using Endpoint = std::tuple<
        core::AddressType,
        contract::ProtocolVersion,
        std::string,     // hostname / address
        std::uint32_t,   // port
        VersionNumber>;  // version

    static const VersionNumber DefaultVersion;

    virtual bool ConnectInfo(
        std::string& strHostname,
        std::uint32_t& nPort,
        core::AddressType& actual,
        const core::AddressType& preferred) const = 0;
    virtual std::string EffectiveName() const = 0;
    using Signable::Serialize;
    virtual bool Serialize(AllocateOutput destination, bool includeNym = false)
        const = 0;
    OPENTXS_NO_EXPORT virtual bool Serialize(
        proto::ServerContract&,
        bool includeNym = false) const = 0;
    virtual bool Statistics(String& strContents) const = 0;
    virtual const Data& TransportKey() const = 0;
    virtual OTSecret TransportKey(Data& pubkey, const PasswordPrompt& reason)
        const = 0;

    virtual void InitAlias(const std::string& alias) = 0;

    ~Server() override = default;

protected:
    Server() noexcept = default;

private:
    friend OTServerContract;

#ifndef _WIN32
    Server* clone() const noexcept override = 0;
#endif

    Server(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(const Server&) = delete;
    Server& operator=(Server&&) = delete;
};
}  // namespace contract
}  // namespace opentxs
#endif
