// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                        // IWYU pragma: associated
#include "1_Internal.hpp"                      // IWYU pragma: associated
#include "ui/accountactivity/BalanceItem.hpp"  // IWYU pragma: associated

#if OT_QT
#include <QDateTime>
#include <QObject>
#include <QStringList>
#endif  // OT_QT
#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "internal/api/client/Client.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/PaymentWorkflowType.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/blockchain/block/bitcoin/Transaction.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/protobuf/PaymentWorkflow.pb.h"
#include "opentxs/protobuf/PaymentWorkflowEnums.pb.h"
#if OT_QT
#include "opentxs/ui/qt/AccountActivity.hpp"
#endif  // OT_QT
#if OT_BLOCKCHAIN
#include "ui/accountactivity/BlockchainBalanceItem.hpp"
#endif  // OT_QT
#include "ui/accountactivity/ChequeBalanceItem.hpp"
#include "ui/accountactivity/TransferBalanceItem.hpp"
#include "ui/base/Widget.hpp"
#if OT_QT
#include "util/Polarity.hpp"  // IWYU pragma: keep
#endif                        // OT_QT

#define OT_METHOD "opentxs::ui::implementation::BalanceItem::"

namespace opentxs::factory
{
auto BalanceItem(
    const ui::implementation::AccountActivityInternalInterface& parent,
    const api::client::internal::Manager& api,
    const ui::implementation::AccountActivityRowID& rowID,
    const ui::implementation::AccountActivitySortKey& sortKey,
    ui::implementation::CustomData& custom,
    const identifier::Nym& nymID,
    const Identifier& accountID) noexcept
    -> std::shared_ptr<ui::implementation::AccountActivityRowInternal>
{
#if OT_BLOCKCHAIN
    if (2 < custom.size()) {
        using Transaction = opentxs::blockchain::block::bitcoin::Transaction;

        auto pTx =
            ui::implementation::extract_custom_ptr<Transaction>(custom, 2);

        OT_ASSERT(pTx);

        const auto& tx = *pTx;

        return std::make_shared<ui::implementation::BlockchainBalanceItem>(
            parent,
            api,
            rowID,
            sortKey,
            custom,
            nymID,
            accountID,
            ui::implementation::extract_custom<blockchain::Type>(custom, 3),
            ui::implementation::extract_custom<OTData>(custom, 5),
            tx.NetBalanceChange(api.Blockchain(), nymID),
            tx.Memo(api.Blockchain()),
            ui::implementation::extract_custom<std::string>(custom, 4));
    }
#endif  // OT_BLOCKCHAIN

    const auto type =
        ui::implementation::BalanceItem::recover_workflow(custom).type();

    switch (opentxs::api::client::internal::translate(type)) {
        case api::client::PaymentWorkflowType::OutgoingCheque:
        case api::client::PaymentWorkflowType::IncomingCheque:
        case api::client::PaymentWorkflowType::OutgoingInvoice:
        case api::client::PaymentWorkflowType::IncomingInvoice: {
            return std::make_shared<ui::implementation::ChequeBalanceItem>(
                parent, api, rowID, sortKey, custom, nymID, accountID);
        }
        case api::client::PaymentWorkflowType::OutgoingTransfer:
        case api::client::PaymentWorkflowType::IncomingTransfer:
        case api::client::PaymentWorkflowType::InternalTransfer: {
            return std::make_shared<ui::implementation::TransferBalanceItem>(
                parent, api, rowID, sortKey, custom, nymID, accountID);
        }
        case api::client::PaymentWorkflowType::Error:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unhandled workflow type (")(
                type)(")")
                .Flush();
        }
    }

    return nullptr;
}
}  // namespace opentxs::factory

namespace opentxs::ui::implementation
{
BalanceItem::BalanceItem(
    const AccountActivityInternalInterface& parent,
    const api::client::internal::Manager& api,
    const AccountActivityRowID& rowID,
    const AccountActivitySortKey& sortKey,
    CustomData& custom,
    const identifier::Nym& nymID,
    const Identifier& accountID,
    const std::string& text) noexcept
    : BalanceItemRow(parent, api, rowID, true)
    , nym_id_(nymID)
    , workflow_(recover_workflow(custom).id())
    , type_(extract_type(recover_workflow(custom)))
    , text_(text)
    , time_(sortKey)
    , account_id_(Identifier::Factory(accountID))
    , contacts_(extract_contacts(api_, recover_workflow(custom)))
{
}

auto BalanceItem::DisplayAmount() const noexcept -> std::string
{
    sLock lock(shared_lock_);
    const auto amount = effective_amount();
    std::string output{};
    const auto formatted =
        parent_.Contract().FormatAmountLocale(amount, output, ",", ".");

    if (formatted) { return output; }

    return std::to_string(amount);
}

auto BalanceItem::extract_contacts(
    const api::client::internal::Manager& api,
    const proto::PaymentWorkflow& workflow) noexcept -> std::vector<std::string>
{
    std::vector<std::string> output{};

    for (const auto& party : workflow.party()) {
        const auto contactID =
            api.Contacts().NymToContact(identifier::Nym::Factory(party));
        output.emplace_back(contactID->str());
    }

    return output;
}

auto BalanceItem::extract_type(const proto::PaymentWorkflow& workflow) noexcept
    -> StorageBox
{
    switch (opentxs::api::client::internal::translate(workflow.type())) {
        case api::client::PaymentWorkflowType::OutgoingCheque: {

            return StorageBox::OUTGOINGCHEQUE;
        }
        case api::client::PaymentWorkflowType::IncomingCheque: {

            return StorageBox::INCOMINGCHEQUE;
        }
        case api::client::PaymentWorkflowType::OutgoingTransfer: {

            return StorageBox::OUTGOINGTRANSFER;
        }
        case api::client::PaymentWorkflowType::IncomingTransfer: {

            return StorageBox::INCOMINGTRANSFER;
        }
        case api::client::PaymentWorkflowType::InternalTransfer: {

            return StorageBox::INTERNALTRANSFER;
        }
        case api::client::PaymentWorkflowType::Error:
        case api::client::PaymentWorkflowType::OutgoingInvoice:
        case api::client::PaymentWorkflowType::IncomingInvoice:
        default: {

            return StorageBox::UNKNOWN;
        }
    }
}

auto BalanceItem::get_contact_name(const identifier::Nym& nymID) const noexcept
    -> std::string
{
    if (nymID.empty()) { return {}; }

    std::string output{nymID.str()};
    const auto contactID = api_.Contacts().ContactID(nymID);

    if (false == contactID->empty()) {
        output = api_.Contacts().ContactName(contactID);
    }

    return output;
}

#if OT_QT
QVariant BalanceItem::qt_data(const int column, int role) const noexcept
{
    switch (role) {
        case Qt::DisplayRole: {
            switch (column) {
                case AccountActivityQt::AmountColumn: {
                    return DisplayAmount().c_str();
                }
                case AccountActivityQt::TextColumn: {
                    return Text().c_str();
                }
                case AccountActivityQt::MemoColumn: {
                    return Memo().c_str();
                }
                case AccountActivityQt::TimeColumn: {
                    auto qdatetime = QDateTime{};
                    qdatetime.setSecsSinceEpoch(Clock::to_time_t(Timestamp()));

                    return qdatetime;
                }
                case AccountActivityQt::UUIDColumn: {
                    return UUID().c_str();
                }
                default: {
                    return {};
                }
            }
        }
        case AccountActivityQt::PolarityRole: {
            return polarity(Amount());
        }
        case AccountActivityQt::ContactsRole: {
            auto output = QStringList{};

            for (const auto& contact : Contacts()) {
                output << contact.c_str();
            }

            return output;
        }
        case AccountActivityQt::WorkflowRole: {
            return Workflow().c_str();
        }
        case AccountActivityQt::TypeRole: {
            return static_cast<int>(Type());
        }
        default: {
            return {};
        }
    };
}
#endif

auto BalanceItem::recover_workflow(CustomData& custom) noexcept
    -> const proto::PaymentWorkflow&
{
    OT_ASSERT(2 <= custom.size())

    const auto& input = custom.at(0);

    OT_ASSERT(nullptr != input)

    return *static_cast<const proto::PaymentWorkflow*>(input);
}

auto BalanceItem::reindex(
    const implementation::AccountActivitySortKey& key,
    implementation::CustomData&) noexcept -> bool
{
    eLock lock(shared_lock_);

    if (key == time_) {

        return false;
    } else {
        time_ = key;

        return true;
    }
}

auto BalanceItem::Text() const noexcept -> std::string
{
    sLock lock(shared_lock_);

    return text_;
}

auto BalanceItem::Timestamp() const noexcept -> Time
{
    sLock lock(shared_lock_);

    return time_;
}

BalanceItem::~BalanceItem() = default;
}  // namespace opentxs::ui::implementation
