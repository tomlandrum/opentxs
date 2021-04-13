// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                      // IWYU pragma: associated
#include "1_Internal.hpp"                    // IWYU pragma: associated
#include "ui/accountsummary/IssuerItem.hpp"  // IWYU pragma: associated

#if OT_QT
#include <QObject>
#endif  // OT_QT
#include <algorithm>
#include <atomic>
#include <iterator>
#include <map>
#include <set>
#include <thread>
#include <utility>

#include "internal/api/client/Client.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Shared.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Issuer.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/UnitDefinition.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#if OT_QT
#include "opentxs/ui/qt/AccountSummary.hpp"
#endif  // OT_QT
#include "ui/base/Combined.hpp"

#define OT_METHOD "opentxs::ui::implementation::IssuerItem::"

namespace opentxs::factory
{
auto IssuerItem(
    const ui::implementation::AccountSummaryInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::AccountSummaryRowID& rowID,
    const ui::implementation::AccountSummarySortKey& sortKey,
    ui::implementation::CustomData& custom,
    const contact::ContactItemType currency) noexcept
    -> std::shared_ptr<ui::implementation::AccountSummaryRowInternal>
{
    using ReturnType = ui::implementation::IssuerItem;

    return std::make_shared<ReturnType>(
        parent, api, rowID, sortKey, custom, currency);
}
}  // namespace opentxs::factory

namespace opentxs::ui::implementation
{
IssuerItem::IssuerItem(
    const AccountSummaryInternalInterface& parent,
    const api::client::internal::Manager& api,
    const AccountSummaryRowID& rowID,
    const AccountSummarySortKey& key,
    [[maybe_unused]] CustomData& custom,
    const contact::ContactItemType currency) noexcept
    : Combined(
          api,
          parent.NymID(),
          parent.WidgetID(),
          parent,
          rowID,
          key,
          false
#if OT_QT
          ,
          Roles{},
          3
#endif
          )
    , listeners_({
          {api_.Endpoints().AccountUpdate(),
           new MessageProcessor<IssuerItem>(&IssuerItem::process_account)},
      })
    , name_{std::get<1>(key_)}
    , connection_{std::get<0>(key_)}
    , issuer_{api_.Wallet().Issuer(parent.NymID(), rowID)}
    , currency_{currency}
{
    OT_ASSERT(issuer_)

    init();
    setup_listeners(listeners_);
    startup_.reset(new std::thread(&IssuerItem::startup, this));

    OT_ASSERT(startup_)
}

auto IssuerItem::Debug() const noexcept -> std::string
{
    return issuer_->toString();
}

auto IssuerItem::construct_row(
    const IssuerItemRowID& id,
    const IssuerItemSortKey& index,
    CustomData& custom) const noexcept -> RowPointer
{
    return factory::AccountSummaryItem(*this, api_, id, index, custom);
}

auto IssuerItem::Name() const noexcept -> std::string
{
    sLock lock(shared_lock_);

    return name_;
}

void IssuerItem::process_account(const Identifier& accountID) noexcept
{
    const auto account = api_.Wallet().Account(accountID);

    if (false == bool(account)) { return; }

    auto name = String::Factory();
    account.get().GetName(name);
    const IssuerItemRowID rowID{accountID, currency_};
    const IssuerItemSortKey sortKey{name->Get()};
    CustomData custom{};
    custom.emplace_back(new Amount(account.get().GetBalance()));
    add_item(rowID, sortKey, custom);
}

void IssuerItem::process_account(const Message& message) noexcept
{
    wait_for_startup();
    const auto body = message.Body();

    OT_ASSERT(2 < message.Body().size())

    auto accountID = api_.Factory().Identifier();
    accountID->Assign(body.at(1).Bytes());

    OT_ASSERT(false == accountID->empty());

    const auto rowID =
        IssuerItemRowID{accountID, {api_.Storage().AccountUnit(accountID)}};
    const auto issuerID = api_.Storage().AccountIssuer(accountID);

    if (issuerID == issuer_->IssuerID()) { process_account(accountID); }
}

#if OT_QT
QVariant IssuerItem::qt_data(const int column, int role) const noexcept
{
    switch (role) {
        case Qt::DisplayRole: {
            switch (column) {
                case AccountSummaryQt::IssuerNameColumn: {
                    return Name().c_str();
                }
                default: {
                    return {};
                }
            }
        }
        case Qt::ToolTipRole: {
            return Debug().c_str();
        }
        case Qt::CheckStateRole: {
            switch (column) {
                case AccountSummaryQt::ConnectionStateColumn: {
                    return ConnectionState();
                }
                case AccountSummaryQt::TrustedColumn: {
                    return Trusted();
                }
                default: {
                }
            }

            [[fallthrough]];
        }
        default: {
            return {};
        }
    }
}
#endif

void IssuerItem::refresh_accounts() noexcept
{
    const auto blank = identifier::UnitDefinition::Factory();
    const auto accounts = issuer_->AccountList(currency_, blank);
    LogDetail(OT_METHOD)(__FUNCTION__)(": Loading ")(accounts.size())(
        " accounts.")
        .Flush();

    for (const auto& id : accounts) { process_account(id); }

    std::set<IssuerItemRowID> active{};
    std::transform(
        accounts.begin(),
        accounts.end(),
        std::inserter(active, active.end()),
        [&](const auto& in) -> IssuerItemRowID {
            return {in, currency_};
        });
    delete_inactive(active);
}

auto IssuerItem::reindex(const AccountSummarySortKey& key, CustomData&) noexcept
    -> bool
{
    eLock lock(shared_lock_);
    key_ = key;
    connection_.store(std::get<0>(key_));
    lock.unlock();
    refresh_accounts();

    return true;
}

void IssuerItem::startup() noexcept
{
    refresh_accounts();
    finish_startup();
}

IssuerItem::~IssuerItem()
{
    for (auto& it : listeners_) { delete it.second; }
}
}  // namespace opentxs::ui::implementation
