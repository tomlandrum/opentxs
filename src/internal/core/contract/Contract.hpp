// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_include "opentxs/core/AddressType.hpp"
// IWYU pragma: no_include "opentxs/core/contract/ProtocolVersion.hpp"
// IWYU pragma: no_include "opentxs/core/contract/UnitType.hpp"

#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "opentxs/Bytes.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/contact/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/Types.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/core/contract/Types.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/contract/peer/BailmentNotice.hpp"
#include "opentxs/core/contract/peer/BailmentReply.hpp"
#include "opentxs/core/contract/peer/BailmentRequest.hpp"
#include "opentxs/core/contract/peer/ConnectionReply.hpp"
#include "opentxs/core/contract/peer/ConnectionRequest.hpp"
#include "opentxs/core/contract/peer/NoticeAcknowledgement.hpp"
#include "opentxs/core/contract/peer/OutBailmentReply.hpp"
#include "opentxs/core/contract/peer/OutBailmentRequest.hpp"
#include "opentxs/core/contract/peer/PeerReply.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"
#include "opentxs/core/contract/peer/PeerRequestType.hpp"
#include "opentxs/core/contract/peer/StoreSecret.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/protobuf/ContractEnums.pb.h"
#include "opentxs/protobuf/PeerReply.pb.h"
#include "opentxs/protobuf/PeerRequest.pb.h"
#include "opentxs/protobuf/ServerContract.pb.h"
#include "opentxs/protobuf/UnitDefinition.pb.h"

namespace opentxs
{
namespace contract
{
namespace peer
{
namespace reply
{
namespace blank
{
struct Acknowledgement;
struct Bailment;
struct Connection;
struct Outbailment;
}  // namespace blank
}  // namespace reply

namespace request
{
namespace blank
{
struct Bailment;
struct BailmentNotice;
struct Connection;
struct Outbailment;
struct StoreSecret;
}  // namespace blank
}  // namespace request
}  // namespace peer
}  // namespace contract

class Account;
class AccountVisitor;
class PasswordPrompt;
class String;
}  // namespace opentxs

namespace opentxs::contract::internal
{
using ProtocolVersionMap = std::map<ProtocolVersion, proto::ProtocolVersion>;
using ProtocolVersionReverseMap =
    std::map<proto::ProtocolVersion, ProtocolVersion>;
using UnitTypeMap = std::map<UnitType, proto::UnitType>;
using UnitTypeReverseMap = std::map<proto::UnitType, UnitType>;

auto protocolversion_map() noexcept -> const ProtocolVersionMap&;
auto unittype_map() noexcept -> const UnitTypeMap&;
auto translate(const ProtocolVersion in) noexcept -> proto::ProtocolVersion;
auto translate(const UnitType in) noexcept -> proto::UnitType;
auto translate(const proto::ProtocolVersion in) noexcept -> ProtocolVersion;
auto translate(const proto::UnitType in) noexcept -> UnitType;
}  // namespace opentxs::contract::internal

namespace opentxs::contract::blank
{
struct Signable : virtual public opentxs::contract::Signable {
    auto Alias() const -> std::string final { return {}; }
    auto ID() const -> OTIdentifier final { return id_; }
    auto Name() const -> std::string final { return {}; }
    auto Nym() const -> Nym_p final { return {}; }
    auto Terms() const -> const std::string& final { return terms_; }
    auto Serialize() const -> OTData final { return OTData{id_}; }
    auto Validate() const -> bool final { return {}; }
    auto Version() const -> VersionNumber final { return 0; }

    void SetAlias(const std::string&) final {}

    Signable(const api::Core& api)
        : api_(api)
        , id_(api.Factory().Identifier())
        , terms_()
    {
    }

    ~Signable() override = default;

protected:
    const api::Core& api_;
    const OTIdentifier id_;
    const std::string terms_;

    Signable(const Signable& rhs)
        : api_(rhs.api_)
        , id_(rhs.id_)
        , terms_(rhs.terms_)
    {
    }
};

struct Unit final : virtual public opentxs::contract::Unit, public Signable {
    auto AddAccountRecord(const std::string&, const Account&) const
        -> bool final
    {
        return {};
    }
    auto Contract() const -> SerializedType final { return {}; }
    auto DecimalPower() const -> std::int32_t final { return {}; }
    auto DisplayStatistics(String&) const -> bool final { return {}; }
    auto EraseAccountRecord(const std::string&, const Identifier&) const
        -> bool final
    {
        return {};
    }
    auto FormatAmountLocale(
        Amount,
        std::string&,
        const std::string&,
        const std::string&) const -> bool final
    {
        return {};
    }
    auto FormatAmountLocale(Amount, std::string&) const -> bool final
    {
        return {};
    }
    auto FormatAmountWithoutSymbolLocale(
        Amount amount,
        std::string&,
        const std::string&,
        const std::string&) const -> bool final
    {
        return {};
    }
    auto FormatAmountWithoutSymbolLocale(Amount amount, std::string&) const
        -> bool final
    {
        return {};
    }
    auto FractionalUnitName() const -> std::string final { return {}; }
    auto GetCurrencyName() const -> const std::string& final { return terms_; }
    auto GetCurrencySymbol() const -> const std::string& final
    {
        return terms_;
    }
    auto PublicContract() const -> SerializedType final { return {}; }
    auto PublicContract(AllocateOutput) const -> bool final { return false; }
    auto StringToAmountLocale(
        Amount&,
        const std::string&,
        const std::string&,
        const std::string&) const -> bool final
    {
        return {};
    }
    auto TLA() const -> std::string final { return {}; }
    auto Type() const -> contract::UnitType final { return {}; }
    auto UnitOfAccount() const -> contact::ContactItemType final { return {}; }
    auto VisitAccountRecords(
        const std::string&,
        AccountVisitor&,
        const PasswordPrompt&) const -> bool final
    {
        return {};
    }

    void InitAlias(const std::string&) final {}

    Unit(const api::Core& api)
        : Signable(api)
    {
    }

    ~Unit() override = default;

private:
    auto clone() const noexcept -> Unit* override { return new Unit(*this); }

    Unit(const Unit& rhs)
        : Signable(rhs)
    {
    }
};

struct Server final : virtual public opentxs::contract::Server,
                      public blank::Signable {
    auto ConnectInfo(
        std::string&,
        std::uint32_t&,
        core::AddressType&,
        const core::AddressType&) const -> bool final
    {
        return {};
    }
    auto EffectiveName() const -> std::string final { return {}; }
    using Signable::Serialize;
    auto Serialize(AllocateOutput destination, bool includeNym = false) const
        -> bool final
    {
        return write(proto::ServerContract{}, destination);
    }
    auto Serialize(proto::ServerContract& output, bool includeNym = false) const
        -> bool final;
    auto Statistics(String&) const -> bool final { return {}; }
    auto TransportKey() const -> const Data& final { return id_; }
    auto TransportKey(Data&, const PasswordPrompt&) const -> OTSecret final
    {
        return api_.Factory().Secret(0);
    }

    void InitAlias(const std::string&) final {}

    Server(const api::Core& api)
        : Signable(api)
    {
    }

    ~Server() final = default;

private:
    auto clone() const noexcept -> Server* final { return new Server(*this); }

    Server(const Server& rhs)
        : Signable(rhs)
    {
    }
};
}  // namespace opentxs::contract::blank

namespace opentxs::contract::peer::blank
{
struct Reply : virtual public opentxs::contract::peer::Reply,
               public contract::blank::Signable {
    auto asAcknowledgement() const noexcept
        -> const reply::Acknowledgement& final;
    auto asBailment() const noexcept -> const reply::Bailment& final;
    auto asConnection() const noexcept -> const reply::Connection& final;
    auto asOutbailment() const noexcept -> const reply::Outbailment& final;

    using Signable::Serialize;
    auto Serialize(SerializedType& output) const -> bool final;
    auto Server() const -> const identifier::Server& final { return server_; }
    auto Type() const -> PeerRequestType final
    {
        return PeerRequestType::Error;
    }

    Reply(const api::Core& api)
        : Signable(api)
        , server_(api.Factory().ServerID())
    {
    }

    ~Reply() override = default;

protected:
    const identifier::Server& server_;

    auto clone() const noexcept -> Reply* override { return new Reply(*this); }

    Reply(const Reply& rhs)
        : Signable(rhs)
        , server_(rhs.server_)
    {
    }
};

struct Request : virtual public opentxs::contract::peer::Request,
                 public contract::blank::Signable {
    auto asBailment() const noexcept -> const peer::request::Bailment& final;
    auto asBailmentNotice() const noexcept
        -> const peer::request::BailmentNotice& final;
    auto asConnection() const noexcept
        -> const peer::request::Connection& final;
    auto asOutbailment() const noexcept
        -> const peer::request::Outbailment& final;
    auto asStoreSecret() const noexcept
        -> const peer::request::StoreSecret& final;

    auto Initiator() const -> const identifier::Nym& final { return nym_; }
    auto Recipient() const -> const identifier::Nym& final { return nym_; }
    using Signable::Serialize;
    auto Serialize(SerializedType& output) const -> bool final;
    auto Server() const -> const identifier::Server& final { return server_; }
    auto Type() const -> PeerRequestType final
    {
        return PeerRequestType::Error;
    }

    Request(const api::Core& api)
        : Signable(api)
        , nym_(api.Factory().NymID())
        , server_(api.Factory().ServerID())
    {
    }

    ~Request() override = default;

protected:
    const identifier::Nym& nym_;
    const identifier::Server& server_;

    auto clone() const noexcept -> Request* override
    {
        return new Request(*this);
    }

    Request(const Request& rhs)
        : Signable(rhs)
        , nym_(rhs.nym_)
        , server_(rhs.server_)
    {
    }
};
}  // namespace opentxs::contract::peer::blank

namespace opentxs::contract::peer::reply::blank
{
struct Acknowledgement final
    : virtual public opentxs::contract::peer::reply::Acknowledgement,
      public contract::peer::blank::Reply {

    Acknowledgement(const api::Core& api)
        : Reply(api)
    {
    }

    ~Acknowledgement() final = default;

private:
    auto clone() const noexcept -> Acknowledgement* final
    {
        return new Acknowledgement(*this);
    }

    Acknowledgement(const Acknowledgement& rhs)
        : Reply(rhs)
    {
    }
};

struct Bailment final : virtual public opentxs::contract::peer::reply::Bailment,
                        public contract::peer::blank::Reply {

    Bailment(const api::Core& api)
        : Reply(api)
    {
    }

    ~Bailment() final = default;

private:
    auto clone() const noexcept -> Bailment* final
    {
        return new Bailment(*this);
    }

    Bailment(const Bailment& rhs)
        : Reply(rhs)
    {
    }
};

struct Connection final
    : virtual public opentxs::contract::peer::reply::Connection,
      public contract::peer::blank::Reply {

    Connection(const api::Core& api)
        : Reply(api)
    {
    }

    ~Connection() final = default;

private:
    auto clone() const noexcept -> Connection* final
    {
        return new Connection(*this);
    }

    Connection(const Connection& rhs)
        : Reply(rhs)
    {
    }
};

struct Outbailment final
    : virtual public opentxs::contract::peer::reply::Outbailment,
      public contract::peer::blank::Reply {

    Outbailment(const api::Core& api)
        : Reply(api)
    {
    }

    ~Outbailment() final = default;

private:
    auto clone() const noexcept -> Outbailment* final
    {
        return new Outbailment(*this);
    }

    Outbailment(const Outbailment& rhs)
        : Reply(rhs)
    {
    }
};

}  // namespace opentxs::contract::peer::reply::blank

namespace opentxs::contract::peer::request::blank
{
struct Bailment final
    : virtual public opentxs::contract::peer::request::Bailment,
      public contract::peer::blank::Request {
    auto UnitID() const -> const identifier::UnitDefinition& final
    {
        return unit_;
    }
    auto ServerID() const -> const identifier::Server& final { return server_; }

    Bailment(const api::Core& api)
        : Request(api)
        , unit_(api.Factory().UnitID())
    {
    }

    ~Bailment() final = default;

private:
    const OTUnitID unit_;

    auto clone() const noexcept -> Bailment* final
    {
        return new Bailment(*this);
    }

    Bailment(const Bailment& rhs)
        : Request(rhs)
        , unit_(rhs.unit_)
    {
    }
};

struct BailmentNotice final
    : virtual public opentxs::contract::peer::request::BailmentNotice,
      public contract::peer::blank::Request {

    BailmentNotice(const api::Core& api)
        : Request(api)
    {
    }

    ~BailmentNotice() final = default;

private:
    auto clone() const noexcept -> BailmentNotice* final
    {
        return new BailmentNotice(*this);
    }

    BailmentNotice(const BailmentNotice& rhs)
        : Request(rhs)
    {
    }
};

struct Connection final
    : virtual public opentxs::contract::peer::request::Connection,
      public contract::peer::blank::Request {

    Connection(const api::Core& api)
        : Request(api)
    {
    }

    ~Connection() final = default;

private:
    auto clone() const noexcept -> Connection* final
    {
        return new Connection(*this);
    }

    Connection(const Connection& rhs)
        : Request(rhs)
    {
    }
};

struct Outbailment final
    : virtual public opentxs::contract::peer::request::Outbailment,
      public contract::peer::blank::Request {

    Outbailment(const api::Core& api)
        : Request(api)
    {
    }

    ~Outbailment() final = default;

private:
    auto clone() const noexcept -> Outbailment* final
    {
        return new Outbailment(*this);
    }

    Outbailment(const Outbailment& rhs)
        : Request(rhs)
    {
    }
};

struct StoreSecret final
    : virtual public opentxs::contract::peer::request::StoreSecret,
      public contract::peer::blank::Request {

    StoreSecret(const api::Core& api)
        : Request(api)
    {
    }

    ~StoreSecret() final = default;

private:
    auto clone() const noexcept -> StoreSecret* final
    {
        return new StoreSecret(*this);
    }

    StoreSecret(const StoreSecret& rhs)
        : Request(rhs)
    {
    }
};

}  // namespace opentxs::contract::peer::request::blank
