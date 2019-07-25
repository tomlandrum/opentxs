// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "opentxs/api/client/Activity.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/OTX.hpp"
#include "opentxs/consensus/ServerContext.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/UniqueQueue.hpp"

#include "internal/api/Api.hpp"
#include "internal/core/identifier/Identifier.hpp"
#include "internal/core/Core.hpp"

#include <future>
#include <string>
#include <tuple>

namespace opentxs::api::client::internal
{
struct Activity : virtual public api::client::Activity {
    virtual void MigrateLegacyThreads(const PasswordPrompt& reason) const = 0;

    virtual ~Activity() = default;
};
struct Blockchain : virtual public api::client::Blockchain {
    struct TxoDB {
        using Status = std::pair<blockchain::Coin, bool>;

        virtual bool AddSpent(
            const identifier::Nym& nym,
            const blockchain::Coin txo,
            const std::string txid) const noexcept = 0;
        virtual bool AddUnspent(
            const identifier::Nym& nym,
            const blockchain::Coin txo,
            const std::vector<OTData>& elements) const noexcept = 0;
        virtual bool Claim(
            const identifier::Nym& nym,
            const blockchain::Coin txo) const noexcept = 0;
        virtual std::vector<Status> Lookup(
            const identifier::Nym& nym,
            const Data& element) const noexcept = 0;
    };

    virtual const api::Core& API() const noexcept = 0;
    virtual std::string CalculateAddress(
        const opentxs::blockchain::Type chain,
        const blockchain::AddressStyle format,
        const Data& pubkey) const noexcept = 0;
    virtual const api::client::Contacts& Contacts() const noexcept = 0;
    virtual const TxoDB& DB() const noexcept = 0;
    virtual OTData PubkeyHash(
        const opentxs::blockchain::Type chain,
        const Data& pubkey) const noexcept(false) = 0;

    virtual ~Blockchain() = default;
};
struct Contacts : virtual public api::client::Contacts {
    virtual void start(const PasswordPrompt& reason) = 0;

    virtual ~Contacts() = default;
};
struct Manager : virtual public api::client::Manager,
                 virtual public api::internal::Core {
    virtual void StartActivity(const PasswordPrompt& reason) = 0;
    virtual void StartContacts(const PasswordPrompt& reason) = 0;

    virtual ~Manager() = default;
};

struct OTX : virtual public api::client::OTX {
    virtual void associate_message_id(
        const Identifier& messageID,
        const TaskID taskID) const = 0;
    virtual Depositability can_deposit(
        const OTPayment& payment,
        const identifier::Nym& recipient,
        const Identifier& accountIDHint,
        identifier::Server& depositServer,
        identifier::UnitDefinition& unitID,
        Identifier& depositAccount) const = 0;
    virtual bool finish_task(
        const TaskID taskID,
        const bool success,
        Result&& result) const = 0;
    virtual UniqueQueue<OTNymID>& get_nym_fetch(
        const identifier::Server& serverID) const = 0;
    virtual BackgroundTask start_task(const TaskID taskID, bool success)
        const = 0;
};
}  // namespace opentxs::api::client::internal
