// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/core/contract/ServerContract.cpp"

#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <string>

#include "core/contract/Signable.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/Types.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/protobuf/ServerContract.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal

class Core;
}  // namespace api

namespace proto
{
class Signature;
}  // namespace proto

class Factory;
class OTPassword;
class PasswordPrompt;
class String;
}  // namespace opentxs

namespace opentxs::contract::implementation
{
class Server final : public contract::Server,
                     public opentxs::contract::implementation::Signable
{
public:
    auto ConnectInfo(
        std::string& strHostname,
        std::uint32_t& nPort,
        core::AddressType& actual,
        const core::AddressType& preferred) const -> bool final;
    auto Contract() const -> proto::ServerContract final;
    auto EffectiveName() const -> std::string final;
    auto Name() const -> std::string final { return name_; }
    auto PublicContract() const -> proto::ServerContract final;
    auto PublicContract(AllocateOutput destination) const -> bool final;
    auto Statistics(String& strContents) const -> bool final;
    auto Serialize() const -> OTData final;
    auto TransportKey() const -> const Data& final;
    auto TransportKey(Data& pubkey, const PasswordPrompt& reason) const
        -> OTSecret final;

    void InitAlias(const std::string& alias) final
    {
        contract::implementation::Signable::SetAlias(alias);
    }
    void SetAlias(const std::string& alias) final;

    Server(
        const api::internal::Core& api,
        const Nym_p& nym,
        const VersionNumber version,
        const std::string& terms,
        const std::string& name,
        std::list<contract::Server::Endpoint>&& endpoints,
        OTData&& key,
        const std::string& id = {},
        Signatures&& signatures = {});
    Server(
        const api::internal::Core& api,
        const Nym_p& nym,
        const proto::ServerContract& serialized);

    ~Server() final = default;

private:
    friend opentxs::Factory;

    const std::list<contract::Server::Endpoint> listen_params_;
    const std::string name_;
    const OTData transport_key_;

    static auto extract_endpoints(
        const proto::ServerContract& serialized) noexcept
        -> std::list<contract::Server::Endpoint>;

    auto clone() const noexcept -> Server* final { return new Server(*this); }
    auto contract(const Lock& lock) const -> proto::ServerContract;
    auto GetID(const Lock& lock) const -> OTIdentifier final;
    auto IDVersion(const Lock& lock) const -> proto::ServerContract;
    auto SigVersion(const Lock& lock) const -> proto::ServerContract;
    auto validate(const Lock& lock) const -> bool final;
    auto verify_signature(const Lock& lock, const proto::Signature& signature)
        const -> bool final;

    auto update_signature(const Lock& lock, const PasswordPrompt& reason)
        -> bool final;

    Server() = delete;
    Server(const Server&);
    Server(Server&&) = delete;
    auto operator=(const Server&) -> Server& = delete;
    auto operator=(Server&&) -> Server& = delete;
};
}  // namespace opentxs::contract::implementation
