// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                      // IWYU pragma: associated
#include "1_Internal.hpp"                    // IWYU pragma: associated
#include "core/contract/ServerContract.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "2_Factory.hpp"
#include "core/contract/Signable.hpp"
#include "internal/api/Api.hpp"
#include "internal/core/Core.hpp"
#include "internal/core/contract/Contract.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/Types.hpp"
#include "opentxs/core/contract/Types.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/crypto/SignatureRole.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/ListenAddress.pb.h"
#include "opentxs/protobuf/Nym.pb.h"
#include "opentxs/protobuf/ServerContract.pb.h"
#include "opentxs/protobuf/Signature.pb.h"
#include "opentxs/protobuf/verify/ServerContract.hpp"

#define OT_METHOD "opentxs::contract::implementation::Server::"

namespace opentxs
{
using ReturnType = contract::implementation::Server;

auto Factory::ServerContract(const api::Core& api) noexcept
    -> std::unique_ptr<contract::Server>
{
    return std::make_unique<contract::blank::Server>(api);
}

auto Factory::ServerContract(
    const api::internal::Core& api,
    const Nym_p& nym,
    const std::list<Endpoint>& endpoints,
    const std::string& terms,
    const std::string& name,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::unique_ptr<contract::Server>
{
    if (false == bool(nym)) { return {}; }
    if (false == nym->HasCapability(NymCapability::AUTHENTICATE_CONNECTION)) {
        return {};
    }

    auto list = std::list<contract::Server::Endpoint>{};
    std::transform(
        std::begin(endpoints),
        std::end(endpoints),
        std::back_inserter(list),
        [](const auto& in) -> contract::Server::Endpoint {
            return {
                static_cast<core::AddressType>(std::get<0>(in)),
                static_cast<contract::ProtocolVersion>(std::get<1>(in)),
                std::get<2>(in),
                std::get<3>(in),
                std::get<4>(in)};
        });

    try {
        auto key = api.Factory().Data();
        nym->TransportKey(key, reason);
        auto output = std::make_unique<ReturnType>(
            api, nym, version, terms, name, std::move(list), std::move(key));

        OT_ASSERT(output);

        auto& contract = *output;
        Lock lock(contract.lock_);

        if (false == contract.update_signature(lock, reason)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to sign contract")
                .Flush();

            return nullptr;
        }

        if (!contract.validate(lock)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid contract").Flush();

            return nullptr;
        }

        return std::move(output);
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto Factory::ServerContract(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::ServerContract& serialized) noexcept
    -> std::unique_ptr<contract::Server>
{
    if (false == proto::Validate<proto::ServerContract>(serialized, VERBOSE)) {
        return nullptr;
    }

    auto contract = std::make_unique<ReturnType>(api, nym, serialized);

    if (!contract) { return nullptr; }

    Lock lock(contract->lock_);

    if (!contract->validate(lock)) { return nullptr; }

    contract->alias_ = contract->name_;

    return std::move(contract);
}
}  // namespace opentxs

namespace opentxs::contract
{
const VersionNumber Server::DefaultVersion{2};
}  // namespace opentxs::contract

namespace opentxs::contract::implementation
{
Server::Server(
    const api::internal::Core& api,
    const Nym_p& nym,
    const VersionNumber version,
    const std::string& terms,
    const std::string& name,
    std::list<contract::Server::Endpoint>&& endpoints,
    OTData&& key,
    const std::string& id,
    Signatures&& signatures)
    : Signable(
          api,
          nym,
          version,
          terms,
          "",
          api.Factory().Identifier(id),
          std::move(signatures))
    , listen_params_(std::move(endpoints))
    , name_(name)
    , transport_key_(std::move(key))
{
    Lock lock(lock_);
    first_time_init(lock);
}

Server::Server(
    const api::internal::Core& api,
    const Nym_p& nym,
    const proto::ServerContract& serialized)
    : Server(
          api,
          nym,
          serialized.version(),
          serialized.terms(),
          serialized.name(),
          extract_endpoints(serialized),
          api.Factory().Data(serialized.transportkey(), StringStyle::Raw),
          serialized.id(),
          serialized.has_signature()
              ? Signatures{std::make_shared<proto::Signature>(
                    serialized.signature())}
              : Signatures{})
{
    Lock lock(lock_);
    init_serialized(lock);
}

Server::Server(const Server& rhs)
    : Signable(rhs)
    , listen_params_(rhs.listen_params_)
    , name_(rhs.name_)
    , transport_key_(rhs.transport_key_)
{
}
auto Server::EffectiveName() const -> std::string
{
    OT_ASSERT(nym_)

    // TODO The version stored in nym_ might be out of date so load it from the
    // wallet. This can be fixed correctly by implementing in-place updates of
    // Nym credentials
    const auto nym = api_.Wallet().Nym(nym_->ID());
    const auto output = nym->Name();

    if (output.empty()) { return name_; }

    return output;
}

auto Server::extract_endpoints(const proto::ServerContract& serialized) noexcept
    -> std::list<contract::Server::Endpoint>
{
    auto output = std::list<contract::Server::Endpoint>{};

    for (auto& listen : serialized.address()) {
        // WARNING: preserve the order of this list, or signature verfication
        // will fail!
        output.emplace_back(contract::Server::Endpoint{
            core::internal::translate(listen.type()),
            contract::internal::translate(listen.protocol()),
            listen.host(),
            listen.port(),
            listen.version()});
    }

    return output;
}

auto Server::GetID(const Lock& lock) const -> OTIdentifier
{
    return api_.Factory().Identifier(IDVersion(lock));
}

auto Server::ConnectInfo(
    std::string& strHostname,
    std::uint32_t& nPort,
    core::AddressType& actual,
    const core::AddressType& preferred) const -> bool
{
    if (0 < listen_params_.size()) {
        for (auto& endpoint : listen_params_) {
            const auto& type = std::get<0>(endpoint);
            const auto& url = std::get<2>(endpoint);
            const auto& port = std::get<3>(endpoint);

            if (preferred == type) {
                strHostname = url;
                nPort = port;
                actual = type;

                return true;
            }
        }

        // If we didn't find the preferred type, return the first result
        const auto& endpoint = listen_params_.front();
        const auto& type = std::get<0>(endpoint);
        const auto& url = std::get<2>(endpoint);
        const auto& port = std::get<3>(endpoint);
        strHostname = url;
        nPort = port;
        actual = type;

        return true;
    }

    return false;
}

auto Server::contract(const Lock& lock) const -> proto::ServerContract
{
    auto contract = SigVersion(lock);
    if (0 < signatures_.size()) {
        *(contract.mutable_signature()) = *(signatures_.front());
    }

    return contract;
}

auto Server::IDVersion(const Lock& lock) const -> proto::ServerContract
{
    OT_ASSERT(verify_write_lock(lock));

    proto::ServerContract contract;
    contract.set_version(version_);
    contract.clear_id();         // reinforcing that this field must be blank.
    contract.clear_signature();  // reinforcing that this field must be blank.
    contract.clear_publicnym();  // reinforcing that this field must be blank.

    if (nym_) {
        auto nymID = String::Factory();
        nym_->GetIdentifier(nymID);
        contract.set_nymid(nymID->Get());
    }

    contract.set_name(name_);

    for (const auto& endpoint : listen_params_) {
        auto& addr = *contract.add_address();
        const auto& version = std::get<4>(endpoint);
        const auto& type = std::get<0>(endpoint);
        const auto& protocol = std::get<1>(endpoint);
        const auto& url = std::get<2>(endpoint);
        const auto& port = std::get<3>(endpoint);
        addr.set_version(version);
        addr.set_type(core::internal::translate(type));
        addr.set_protocol(contract::internal::translate(protocol));
        addr.set_host(url);
        addr.set_port(port);
    }

    contract.set_terms(conditions_);
    contract.set_transportkey(transport_key_->data(), transport_key_->size());

    return contract;
}

void Server::SetAlias(const std::string& alias)
{
    InitAlias(alias);
    api_.Wallet().SetServerAlias(
        identifier::Server::Factory(id_->str()), alias);  // TODO conversion
}

auto Server::SigVersion(const Lock& lock) const -> proto::ServerContract
{
    auto contract = IDVersion(lock);
    contract.set_id(String::Factory(id(lock))->Get());

    return contract;
}

auto Server::Serialize() const -> OTData
{
    Lock lock(lock_);

    return api_.Factory().Data(contract(lock));
}

auto Server::Serialize(proto::ServerContract& serialized, bool includeNym) const
    -> bool
{
    Lock lock(lock_);

    serialized = contract(lock);

    if (includeNym && nym_) {
        auto publicNym = nym_->asPublicNym();
        *(serialized.mutable_publicnym()) = publicNym;
    }

    return true;
}

auto Server::Serialize(AllocateOutput destination, bool includeNym) const
    -> bool
{
    auto serialized = proto::ServerContract{};
    if (false == Serialize(serialized, includeNym)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to serialize server.")
            .Flush();
        return false;
    }

    write(serialized, destination);

    return true;
}

auto Server::Statistics(String& strContents) const -> bool
{
    const auto strID = String::Factory(id_);

    strContents.Concatenate(
        " Notary Provider: %s\n"
        " NotaryID: %s\n"
        "\n",
        nym_->Alias().c_str(),
        strID->Get());

    return true;
}

auto Server::TransportKey() const -> const Data&
{
    return transport_key_.get();
}

auto Server::TransportKey(Data& pubkey, const PasswordPrompt& reason) const
    -> OTSecret
{
    OT_ASSERT(nym_);

    return nym_->TransportKey(pubkey, reason);
}

auto Server::update_signature(const Lock& lock, const PasswordPrompt& reason)
    -> bool
{
    if (!Signable::update_signature(lock, reason)) { return false; }

    bool success = false;
    signatures_.clear();
    auto serialized = SigVersion(lock);
    auto& signature = *serialized.mutable_signature();
    success = nym_->Sign(
        serialized, crypto::SignatureRole::ServerContract, signature, reason);

    if (success) {
        signatures_.emplace_front(new proto::Signature(signature));
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": failed to create signature.")
            .Flush();
    }

    return success;
}

auto Server::validate(const Lock& lock) const -> bool
{
    bool validNym = false;

    if (nym_) { validNym = nym_->VerifyPseudonym(); }

    if (!validNym) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid nym.").Flush();

        return false;
    }

    const bool validSyntax = proto::Validate(contract(lock), VERBOSE);

    if (!validSyntax) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid syntax.").Flush();

        return false;
    }

    if (1 > signatures_.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing signature.").Flush();

        return false;
    }

    bool validSig = false;
    auto& signature = *signatures_.cbegin();

    if (signature) { validSig = verify_signature(lock, *signature); }

    if (!validSig) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid signature.").Flush();

        return false;
    }

    return true;
}

auto Server::verify_signature(
    const Lock& lock,
    const proto::Signature& signature) const -> bool
{
    if (!Signable::verify_signature(lock, signature)) { return false; }

    auto serialized = SigVersion(lock);
    auto& sigProto = *serialized.mutable_signature();
    sigProto.CopyFrom(signature);

    return nym_->Verify(serialized, sigProto);
}
}  // namespace opentxs::contract::implementation
