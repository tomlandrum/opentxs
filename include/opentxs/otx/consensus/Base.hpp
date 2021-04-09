// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_OTX_CONSENSUS_BASE_HPP
#define OPENTXS_OTX_CONSENSUS_BASE_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <memory>
#include <set>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/otx/Types.hpp"

namespace opentxs
{
namespace proto
{
class Context;
}  // namespace proto

class PasswordPrompt;
}  // namespace opentxs

namespace opentxs
{
namespace otx
{
namespace context
{
class Base : virtual public opentxs::contract::Signable
{
public:
    using TransactionNumbers = std::set<TransactionNumber>;
    using RequestNumbers = std::set<RequestNumber>;

    OPENTXS_EXPORT virtual RequestNumbers AcknowledgedNumbers() const = 0;
    OPENTXS_EXPORT virtual std::size_t AvailableNumbers() const = 0;
    OPENTXS_EXPORT virtual bool HaveLocalNymboxHash() const = 0;
    OPENTXS_EXPORT virtual bool HaveRemoteNymboxHash() const = 0;
    OPENTXS_EXPORT virtual TransactionNumbers IssuedNumbers() const = 0;
    OPENTXS_EXPORT virtual std::string LegacyDataFolder() const = 0;
    OPENTXS_EXPORT virtual OTIdentifier LocalNymboxHash() const = 0;
    OPENTXS_EXPORT virtual const identifier::Server& Notary() const = 0;
    OPENTXS_EXPORT virtual bool NymboxHashMatch() const = 0;
    OPENTXS_EXPORT virtual std::unique_ptr<const opentxs::NymFile> Nymfile(
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual const identity::Nym& RemoteNym() const = 0;
    OPENTXS_EXPORT virtual OTIdentifier RemoteNymboxHash() const = 0;
    OPENTXS_EXPORT virtual RequestNumber Request() const = 0;
    OPENTXS_EXPORT virtual proto::Context Serialized() const = 0;
    OPENTXS_EXPORT virtual otx::ConsensusType Type() const = 0;
    OPENTXS_EXPORT virtual bool VerifyAcknowledgedNumber(
        const RequestNumber& req) const = 0;
    OPENTXS_EXPORT virtual bool VerifyAvailableNumber(
        const TransactionNumber& number) const = 0;
    OPENTXS_EXPORT virtual bool VerifyIssuedNumber(
        const TransactionNumber& number) const = 0;

    OPENTXS_EXPORT virtual bool AddAcknowledgedNumber(
        const RequestNumber req) = 0;
    OPENTXS_EXPORT virtual bool CloseCronItem(const TransactionNumber) = 0;
    OPENTXS_EXPORT virtual bool ConsumeAvailable(
        const TransactionNumber& number) = 0;
    OPENTXS_EXPORT virtual bool ConsumeIssued(
        const TransactionNumber& number) = 0;
    OPENTXS_EXPORT virtual RequestNumber IncrementRequest() = 0;
    OPENTXS_EXPORT virtual bool InitializeNymbox(
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual Editor<opentxs::NymFile> mutable_Nymfile(
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual bool OpenCronItem(const TransactionNumber) = 0;
    OPENTXS_EXPORT virtual bool RecoverAvailableNumber(
        const TransactionNumber& number) = 0;
    OPENTXS_EXPORT virtual bool RemoveAcknowledgedNumber(
        const RequestNumbers& req) = 0;
    OPENTXS_EXPORT virtual void Reset() = 0;
    OPENTXS_EXPORT virtual proto::Context Refresh(
        const PasswordPrompt& reason) = 0;
    OPENTXS_EXPORT virtual void SetLocalNymboxHash(const Identifier& hash) = 0;
    OPENTXS_EXPORT virtual void SetRemoteNymboxHash(const Identifier& hash) = 0;
    OPENTXS_EXPORT virtual void SetRequest(const RequestNumber req) = 0;

    OPENTXS_EXPORT ~Base() override = default;

protected:
    Base() = default;

private:
    Base(const Base&) = delete;
    Base(Base&&) = delete;
    Base& operator=(const Base&) = delete;
    Base& operator=(Base&&) = delete;
};
}  // namespace context
}  // namespace otx
}  // namespace opentxs
#endif
