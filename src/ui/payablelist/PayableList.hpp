// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/ui/PayableList.cpp"

#pragma once

#if OT_QT
#include <QHash>
#endif  // OT_QT
#include <map>
#include <utility>

#include "1_Internal.hpp"
#include "core/Worker.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/ui/PayableList.hpp"
#include "opentxs/util/WorkType.hpp"
#include "ui/base/List.hpp"
#include "ui/base/Widget.hpp"
#include "util/Work.hpp"

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

namespace identifier
{
class Nym;
}  // namespace identifier

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
}  // namespace opentxs

namespace opentxs::ui::implementation
{
using PayableListList = List<
    PayableExternalInterface,
    PayableInternalInterface,
    PayableListRowID,
    PayableListRowInterface,
    PayableListRowInternal,
    PayableListRowBlank,
    PayableListSortKey,
    PayablePrimaryID>;

class PayableList final : public PayableListList, Worker<PayableList>
{
public:
    auto ID() const noexcept -> const Identifier& final
    {
        return owner_contact_id_;
    }

    PayableList(
        const api::client::internal::Manager& api,
        const identifier::Nym& nymID,
        const contact::ContactItemType& currency,
        const SimpleCallback& cb) noexcept;
    ~PayableList() final;

private:
    friend Worker<PayableList>;

    enum class Work : OTZMQWorkType {
        contact = value(WorkType::ContactUpdated),
        nym = value(WorkType::NymUpdated),
        init = OT_ZMQ_INIT_SIGNAL,
        statemachine = OT_ZMQ_STATE_MACHINE_SIGNAL,
        shutdown = value(WorkType::Shutdown),
    };

    const OTIdentifier owner_contact_id_;
    const contact::ContactItemType currency_;

    auto construct_row(
        const PayableListRowID& id,
        const PayableListSortKey& index,
        CustomData& custom) const noexcept -> RowPointer final;
    auto last(const PayableListRowID& id) const noexcept -> bool final
    {
        return PayableListList::last(id);
    }

    auto pipeline(const Message& in) noexcept -> void;
    auto process_contact(
        const PayableListRowID& id,
        const PayableListSortKey& key) noexcept -> void;
    auto process_contact(const Message& message) noexcept -> void;
    auto process_nym(const Message& message) noexcept -> void;
    auto startup() noexcept -> void;

    PayableList() = delete;
    PayableList(const PayableList&) = delete;
    PayableList(PayableList&&) = delete;
    auto operator=(const PayableList&) -> PayableList& = delete;
    auto operator=(PayableList&&) -> PayableList& = delete;
};
}  // namespace opentxs::ui::implementation
