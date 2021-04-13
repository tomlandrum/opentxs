// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                          // IWYU pragma: associated
#include "1_Internal.hpp"                        // IWYU pragma: associated
#include "ui/accountsummary/AccountSummary.hpp"  // IWYU pragma: associated

#include <map>
#include <memory>
#include <set>
#include <thread>
#include <utility>

#include "internal/api/client/Client.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Issuer.hpp"
#include "opentxs/api/network/ZMQ.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#if OT_QT
#include "opentxs/ui/qt/AccountSummary.hpp"
#endif  // OT_QT
#include "ui/base/List.hpp"

#define OT_METHOD "opentxs::ui::implementation::AccountSummary::"

namespace opentxs::factory
{
auto AccountSummaryModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const contact::ContactItemType currency,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::AccountSummary>
{
    using ReturnType = ui::implementation::AccountSummary;

    return std::make_unique<ReturnType>(api, nymID, currency, cb);
}

#if OT_QT
auto AccountSummaryQtModel(ui::implementation::AccountSummary& parent) noexcept
    -> std::unique_ptr<ui::AccountSummaryQt>
{
    using ReturnType = ui::AccountSummaryQt;

    return std::make_unique<ReturnType>(parent);
}
#endif  // OT_QT
}  // namespace opentxs::factory

#if OT_QT
namespace opentxs::ui
{
QT_PROXY_MODEL_WRAPPER(AccountSummaryQt, implementation::AccountSummary)
}  // namespace opentxs::ui
#endif

namespace opentxs::ui::implementation
{
AccountSummary::AccountSummary(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const contact::ContactItemType currency,
    const SimpleCallback& cb) noexcept
    : AccountSummaryList(
          api,
          nymID,
          cb,
          false
#if OT_QT
          ,
          Roles{
              {AccountSummaryQt::NotaryIDRole, "notary"},
              {AccountSummaryQt::AccountIDRole, "account"},
              {AccountSummaryQt::BalanceRole, "balance"}},
          5
#endif
          )
    , listeners_({
          {api_.Endpoints().IssuerUpdate(),
           new MessageProcessor<AccountSummary>(
               &AccountSummary::process_issuer)},
          {api_.Endpoints().ServerUpdate(),
           new MessageProcessor<AccountSummary>(
               &AccountSummary::process_server)},
          {api_.Endpoints().ConnectionStatus(),
           new MessageProcessor<AccountSummary>(
               &AccountSummary::process_connection)},
          {api_.Endpoints().NymDownload(),
           new MessageProcessor<AccountSummary>(&AccountSummary::process_nym)},
      })
    , currency_{currency}
    , issuers_{}
    , server_issuer_map_{}
    , nym_server_map_{}
{
    init();
    setup_listeners(listeners_);
    startup_.reset(new std::thread(&AccountSummary::startup, this));

    OT_ASSERT(startup_)
}

auto AccountSummary::construct_row(
    const AccountSummaryRowID& id,
    const AccountSummarySortKey& index,
    CustomData& custom) const noexcept -> RowPointer
{
    return factory::IssuerItem(*this, api_, id, index, custom, currency_);
}

auto AccountSummary::extract_key(
    const identifier::Nym& nymID,
    const identifier::Nym& issuerID) noexcept -> AccountSummarySortKey
{
    AccountSummarySortKey output{false, "opentxs notary"};
    auto& [state, name] = output;

    const auto issuer = api_.Wallet().Issuer(nymID, issuerID);

    if (false == bool(issuer)) { return output; }

    const auto serverID = issuer->PrimaryServer();

    if (serverID->empty()) { return output; }

    try {
        const auto server = api_.Wallet().Server(serverID);
        name = server->Alias();
        const auto& serverNymID = server->Nym()->ID();
        eLock lock(shared_lock_);
        nym_server_map_.emplace(serverNymID, serverID);
        server_issuer_map_.emplace(serverID, issuerID);
    } catch (...) {
        return output;
    }

    switch (api_.ZMQ().Status(serverID->str())) {
        case ConnectionState::ACTIVE: {
            state = true;
        } break;
        case ConnectionState::NOT_ESTABLISHED:
        case ConnectionState::STALLED:
        default: {
        }
    }

    return output;
}

void AccountSummary::process_connection(const Message& message) noexcept
{
    wait_for_startup();
    const auto body = message.Body();

    OT_ASSERT(2 < body.size());

    const auto id = [&] {
        auto output = api_.Factory().ServerID();
        output->Assign(body.at(1).Bytes());

        return output;
    }();
    process_server(id);
}

void AccountSummary::process_issuer(const identifier::Nym& issuerID) noexcept
{
    issuers_.emplace(issuerID);
    CustomData custom{};
    add_item(issuerID, extract_key(primary_id_, issuerID), custom);
}

void AccountSummary::process_issuer(const Message& message) noexcept
{
    wait_for_startup();
    const auto body = message.Body();

    OT_ASSERT(2 < body.size());

    const auto nymID = [&] {
        auto output = api_.Factory().NymID();
        output->Assign(body.at(1).Bytes());

        return output;
    }();
    const auto issuerID = [&] {
        auto output = api_.Factory().NymID();
        output->Assign(body.at(2).Bytes());

        return output;
    }();

    OT_ASSERT(false == nymID->empty())
    OT_ASSERT(false == issuerID->empty())

    if (nymID != primary_id_) { return; }

    process_issuer(issuerID);
}

void AccountSummary::process_nym(const Message& message) noexcept
{
    wait_for_startup();

    OT_ASSERT(1 < message.Body().size());

    auto nymID = api_.Factory().NymID();
    nymID->Assign(message.Body_at(1).Bytes());
    sLock lock(shared_lock_);
    const auto it = nym_server_map_.find(nymID);

    if (nym_server_map_.end() == it) { return; }

    const auto serverID = it->second;
    lock.unlock();

    process_server(serverID);
}

void AccountSummary::process_server(const Message& message) noexcept
{
    wait_for_startup();
    const auto body = message.Body();

    OT_ASSERT(1 < body.size());

    const auto serverID = [&] {
        auto output = api_.Factory().ServerID();
        output->Assign(body.at(1).Bytes());

        return output;
    }();

    OT_ASSERT(false == serverID->empty())

    process_server(serverID);
}

void AccountSummary::process_server(const identifier::Server& serverID) noexcept
{
    sLock lock(shared_lock_);
    const auto it = server_issuer_map_.find(serverID);

    if (server_issuer_map_.end() == it) { return; }

    const auto issuerID = it->second;
    lock.unlock();
    CustomData custom{};
    add_item(issuerID, extract_key(primary_id_, issuerID), custom);
}

void AccountSummary::startup() noexcept
{
    const auto issuers = api_.Wallet().IssuerList(primary_id_);
    LogDetail(OT_METHOD)(__FUNCTION__)(": Loading ")(issuers.size())(
        " issuers.")
        .Flush();

    for (const auto& id : issuers) { process_issuer(id); }

    finish_startup();
}

AccountSummary::~AccountSummary()
{
    for (auto& it : listeners_) { delete it.second; }
}
}  // namespace opentxs::ui::implementation
