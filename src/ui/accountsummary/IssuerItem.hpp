// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/ui/IssuerItem.cpp"

#pragma once

#if OT_QT
#include <QHash>
#include <QVariant>
#endif  // OT_QT
#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "1_Internal.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/client/Issuer.hpp"
#include "opentxs/core/Identifier.hpp"
#include "ui/base/Combined.hpp"
#include "ui/base/List.hpp"
#include "ui/base/RowType.hpp"
#include "ui/base/Widget.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
namespace internal
{
struct Manager;
}  // namespace internal
}  // namespace client
}  // namespace api

namespace network
{
namespace zeromq
{
namespace socket
{
class Publish;
}  // namespace socket

class Message;
}  // namespace zeromq
}  // namespace network

namespace ui
{
class IssuerItem;
}  // namespace ui
}  // namespace opentxs

namespace opentxs::ui::implementation
{
using IssuerItemList = List<
    IssuerItemExternalInterface,
    IssuerItemInternalInterface,
    IssuerItemRowID,
    IssuerItemRowInterface,
    IssuerItemRowInternal,
    IssuerItemRowBlank,
    IssuerItemSortKey,
    IssuerItemPrimaryID>;
using IssuerItemRow = RowType<
    AccountSummaryRowInternal,
    AccountSummaryInternalInterface,
    AccountSummaryRowID>;

class IssuerItem final
    : public Combined<IssuerItemList, IssuerItemRow, AccountSummarySortKey>
{
public:
    auto ConnectionState() const noexcept -> bool final
    {
        return connection_.load();
    }
    auto Debug() const noexcept -> std::string final;
    auto Name() const noexcept -> std::string final;
    auto Trusted() const noexcept -> bool final { return issuer_->Paired(); }

#if OT_QT
    QVariant qt_data(const int column, const int role) const noexcept final;
#endif

    auto reindex(const AccountSummarySortKey& key, CustomData& custom) noexcept
        -> bool final;

    IssuerItem(
        const AccountSummaryInternalInterface& parent,
        const api::client::internal::Manager& api,
        const AccountSummaryRowID& rowID,
        const AccountSummarySortKey& sortKey,
        CustomData& custom,
        const contact::ContactItemType currency) noexcept;
    ~IssuerItem() final;

private:
    const ListenerDefinitions listeners_;
    const std::string& name_;
    std::atomic<bool> connection_;
    const std::shared_ptr<const api::client::Issuer> issuer_;
    const contact::ContactItemType currency_;

    auto construct_row(
        const IssuerItemRowID& id,
        const IssuerItemSortKey& index,
        CustomData& custom) const noexcept -> RowPointer final;

    void process_account(const Identifier& accountID) noexcept;
    void process_account(const Message& message) noexcept;
    void refresh_accounts() noexcept;
    void startup() noexcept;

    IssuerItem() = delete;
    IssuerItem(const IssuerItem&) = delete;
    IssuerItem(IssuerItem&&) = delete;
    auto operator=(const IssuerItem&) -> IssuerItem& = delete;
    auto operator=(IssuerItem&&) -> IssuerItem& = delete;
};
}  // namespace opentxs::ui::implementation

template class opentxs::SharedPimpl<opentxs::ui::IssuerItem>;
