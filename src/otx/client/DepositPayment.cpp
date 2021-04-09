// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                   // IWYU pragma: associated
#include "1_Internal.hpp"                 // IWYU pragma: associated
#include "otx/client/DepositPayment.hpp"  // IWYU pragma: associated

#include <functional>
#include <future>
#include <memory>
#include <set>
#include <tuple>
#include <type_traits>
#include <utility>

#include "internal/api/Api.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/otx/LastReplyStatus.hpp"
#include "otx/client/PaymentTasks.hpp"

#define OT_METHOD "opentxs::otx::client::implementation::DepositPayment::"

namespace opentxs::otx::client::implementation
{
DepositPayment::DepositPayment(
    client::internal::StateMachine& parent,
    const TaskID taskID,
    const DepositPaymentTask& payment,
    PaymentTasks& paymenttasks)
    : StateMachine(std::bind(&DepositPayment::deposit, this))
    , parent_(parent)
    , task_id_(taskID)
    , payment_(payment)
    , state_(Depositability::UNKNOWN)
    , result_(parent.error_result())
    , payment_tasks_(paymenttasks)
{
}

auto DepositPayment::deposit() -> bool
{
    bool error{false};
    bool repeat{true};
    auto& [unitID, accountID, pPayment] = payment_;

    if (false == bool(pPayment)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid payment").Flush();
        error = true;
        repeat = false;

        goto exit;
    }

    switch (state_) {
        case Depositability::UNKNOWN: {
            if (accountID->empty()) {
                state_ = Depositability::NO_ACCOUNT;
            } else {
                state_ = Depositability::READY;
            }
        } break;
        case Depositability::NO_ACCOUNT: {
            accountID = get_account_id(unitID);

            if (accountID->empty()) {
                error = true;
                repeat = true;

                goto exit;
            }

            state_ = Depositability::READY;
            [[fallthrough]];
        }
        case Depositability::READY: {
            auto [taskid, future] = parent_.DepositPayment(payment_);

            if (0 == taskid) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to schedule deposit payment")
                    .Flush();
                error = true;
                repeat = true;

                goto exit;
            }

            auto value = future.get();
            const auto [result, pMessage] = value;

            if (otx::LastReplyStatus::MessageSuccess == result) {
                LogVerbose(OT_METHOD)(__FUNCTION__)(": Deposit success")
                    .Flush();
                result_ = std::move(value);
                error = false;
                repeat = false;

                goto exit;
            } else {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Deposit failed").Flush();
                error = true;
                repeat = false;
            }
        } break;
        case Depositability::ACCOUNT_NOT_SPECIFIED:
        case Depositability::WRONG_ACCOUNT:
        case Depositability::INVALID_INSTRUMENT:
        case Depositability::WRONG_RECIPIENT:
        case Depositability::NOT_REGISTERED:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid state").Flush();
            error = true;
            repeat = false;

            goto exit;
        }
    }

exit:
    if (false == repeat) {
        parent_.finish_task(task_id_, !error, std::move(result_));
    }

    return repeat;
}

auto DepositPayment::get_account_id(const identifier::UnitDefinition& unit)
    -> OTIdentifier
{
    Lock lock(payment_tasks_.GetAccountLock(unit));
    const auto accounts = parent_.api().Storage().AccountsByContract(unit);

    if (1 < accounts.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Too many accounts to automatically deposit payment")
            .Flush();

        return parent_.api().Factory().Identifier();
    }

    if (1 == accounts.size()) { return *accounts.begin(); }

    try {
        parent_.api().Wallet().UnitDefinition(unit);
    } catch (...) {
        LogTrace(OT_METHOD)(__FUNCTION__)(": Downloading unit definition")
            .Flush();
        parent_.DownloadUnitDefinition(unit);

        return parent_.api().Factory().Identifier();
    }

    LogVerbose(OT_METHOD)(__FUNCTION__)(": Registering account for deposit")
        .Flush();

    auto [taskid, future] = parent_.RegisterAccount({"", unit});

    if (0 == taskid) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to schedule register account")
            .Flush();

        return parent_.api().Factory().Identifier();
    }

    result_ = future.get();
    const auto [result, pMessage] = result_;

    if (otx::LastReplyStatus::MessageSuccess != result) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to send register account message")
            .Flush();

        return parent_.api().Factory().Identifier();
    }

    if (false == bool(pMessage)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid register account reply")
            .Flush();

        return parent_.api().Factory().Identifier();
    }

    const auto& message = *pMessage;
    const auto accountID =
        parent_.api().Factory().Identifier(message.m_strAcctID);

    if (accountID->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to get account id")
            .Flush();
    } else {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Registered new account ")(
            accountID)
            .Flush();
    }

    return accountID;
}

DepositPayment::~DepositPayment() { Stop(); }
}  // namespace opentxs::otx::client::implementation
