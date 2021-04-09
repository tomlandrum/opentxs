// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <atomic>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <set>
#include <string>

#include "core/contract/Signable.hpp"
#include "internal/api/Api.hpp"
#include "internal/otx/consensus/Consensus.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/protobuf/Context.pb.h"
#include "opentxs/otx/Types.hpp"

namespace opentxs
{
namespace api
{
class Core;

namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace identifier
{
class Nym;
}  // namespace identifier

namespace proto
{
class Signature;
}  // namespace proto

class Factory;
class NymFile;
class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::otx::context::implementation
{
class Base : virtual public internal::Base,
             public opentxs::contract::implementation::Signable
{
public:
    auto AcknowledgedNumbers() const -> std::set<RequestNumber> final;
    auto AvailableNumbers() const -> std::size_t final;
    auto HaveLocalNymboxHash() const -> bool final;
    auto HaveRemoteNymboxHash() const -> bool final;
    auto IssuedNumbers() const -> std::set<TransactionNumber> final;
    auto Name() const -> std::string final;
    auto NymboxHashMatch() const -> bool final;
    auto LegacyDataFolder() const -> std::string final;
    auto LocalNymboxHash() const -> OTIdentifier final;
    auto Notary() const -> const identifier::Server& final
    {
        return server_id_;
    }
    auto Nymfile(const PasswordPrompt& reason) const
        -> std::unique_ptr<const opentxs::NymFile> final;
    auto RemoteNym() const -> const identity::Nym& final;
    auto RemoteNymboxHash() const -> OTIdentifier final;
    auto Request() const -> RequestNumber final;
    auto Serialize() const -> OTData final;
    auto Serialized() const -> proto::Context final;
    auto VerifyAcknowledgedNumber(const RequestNumber& req) const -> bool final;
    auto VerifyAvailableNumber(const TransactionNumber& number) const
        -> bool final;
    auto VerifyIssuedNumber(const TransactionNumber& number) const
        -> bool final;

    auto AddAcknowledgedNumber(const RequestNumber req) -> bool final;
    auto CloseCronItem(const TransactionNumber) -> bool override
    {
        return false;
    }
    auto ConsumeAvailable(const TransactionNumber& number) -> bool final;
    auto ConsumeIssued(const TransactionNumber& number) -> bool final;
    auto IncrementRequest() -> RequestNumber final;
    auto InitializeNymbox(const PasswordPrompt& reason) -> bool final;
    auto mutable_Nymfile(const PasswordPrompt& reason)
        -> Editor<opentxs::NymFile> final;
    auto OpenCronItem(const TransactionNumber) -> bool override
    {
        return false;
    }
    auto RecoverAvailableNumber(const TransactionNumber& number) -> bool final;
    auto Refresh(const PasswordPrompt& reason) -> proto::Context final;
    auto RemoveAcknowledgedNumber(const std::set<RequestNumber>& req)
        -> bool final;
    void Reset() final;
    void SetLocalNymboxHash(const Identifier& hash) final;
    void SetRemoteNymboxHash(const Identifier& hash) final;
    void SetRequest(const RequestNumber req) final;

    ~Base() override = default;

protected:
    const OTServerID server_id_;
    Nym_p remote_nym_{};
    std::set<TransactionNumber> available_transaction_numbers_{};
    std::set<TransactionNumber> issued_transaction_numbers_{};
    std::atomic<RequestNumber> request_number_{0};
    std::set<RequestNumber> acknowledged_request_numbers_{};
    OTIdentifier local_nymbox_hash_;
    OTIdentifier remote_nymbox_hash_;

    auto contract(const Lock& lock) const -> proto::Context;
    auto GetID(const Lock& lock) const -> OTIdentifier final;
    auto serialize(const Lock& lock, const otx::ConsensusType type) const
        -> proto::Context;
    virtual auto serialize(const Lock& lock) const -> proto::Context = 0;
    virtual auto type() const -> std::string = 0;
    auto validate(const Lock& lock) const -> bool final;

    auto add_acknowledged_number(const Lock& lock, const RequestNumber req)
        -> bool;
    auto consume_available(const Lock& lock, const TransactionNumber& number)
        -> bool;
    auto consume_issued(const Lock& lock, const TransactionNumber& number)
        -> bool;
    void finish_acknowledgements(
        const Lock& lock,
        const std::set<RequestNumber>& req);
    auto issue_number(const Lock& lock, const TransactionNumber& number)
        -> bool;
    auto recover_available_number(
        const Lock& lock,
        const TransactionNumber& number) -> bool;
    auto remove_acknowledged_number(
        const Lock& lock,
        const std::set<RequestNumber>& req) -> bool;
    auto save(const Lock& lock, const PasswordPrompt& reason) -> bool;
    void set_local_nymbox_hash(const Lock& lock, const Identifier& hash);
    void set_remote_nymbox_hash(const Lock& lock, const Identifier& hash);
    auto update_signature(const Lock& lock, const PasswordPrompt& reason)
        -> bool final;
    auto verify_available_number(const Lock& lock, const TransactionNumber& req)
        const -> bool;
    auto verify_acknowledged_number(const Lock& lock, const RequestNumber& req)
        const -> bool;
    auto verify_issued_number(const Lock& lock, const TransactionNumber& number)
        const -> bool;

    Base(
        const api::internal::Core& api,
        const VersionNumber targetVersion,
        const Nym_p& local,
        const Nym_p& remote,
        const identifier::Server& server);
    Base(
        const api::internal::Core& api,
        const VersionNumber targetVersion,
        const proto::Context& serialized,
        const Nym_p& local,
        const Nym_p& remote,
        const identifier::Server& server);

private:
    friend opentxs::Factory;

    const VersionNumber target_version_{0};

    static auto calculate_id(
        const api::Core& api,
        const Nym_p& client,
        const Nym_p& server) noexcept(false) -> OTIdentifier;

    virtual auto client_nym_id(const Lock& lock) const
        -> const identifier::Nym& = 0;
    auto clone() const noexcept -> Base* final { return nullptr; }
    auto IDVersion(const Lock& lock) const -> proto::Context;
    virtual auto server_nym_id(const Lock& lock) const
        -> const identifier::Nym& = 0;
    auto SigVersion(const Lock& lock) const -> proto::Context;
    auto verify_signature(const Lock& lock, const proto::Signature& signature)
        const -> bool final;

    // Transition method used for converting from Nym class
    auto insert_available_number(const TransactionNumber& number) -> bool;
    // Transition method used for converting from Nym class
    auto insert_issued_number(const TransactionNumber& number) -> bool;

    Base() = delete;
    Base(const Base&) = delete;
    Base(Base&&) = delete;
    auto operator=(const Base&) -> Base& = delete;
    auto operator=(Base&&) -> Base& = delete;
};
}  // namespace opentxs::otx::context::implementation
