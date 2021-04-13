// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                    // IWYU pragma: associated
#include "1_Internal.hpp"                  // IWYU pragma: associated
#include "internal/api/client/Client.hpp"  // IWYU pragma: associated

#include <map>
#include <type_traits>

#include "internal/api/client/Client.hpp"
#include "internal/blockchain/Params.hpp"
#include "opentxs/api/client/PaymentWorkflowState.hpp"
#include "opentxs/api/client/PaymentWorkflowType.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/protobuf/PaymentWorkflowEnums.pb.h"
#include "util/Container.hpp"

namespace opentxs
{
namespace api::client::internal
{
auto paymentworkflowstate_map() noexcept -> const PaymentWorkflowStateMap&
{
    static const auto map = PaymentWorkflowStateMap{
        {PaymentWorkflowState::Error, proto::PAYMENTWORKFLOWSTATE_ERROR},
        {PaymentWorkflowState::Unsent, proto::PAYMENTWORKFLOWSTATE_UNSENT},
        {PaymentWorkflowState::Conveyed, proto::PAYMENTWORKFLOWSTATE_CONVEYED},
        {PaymentWorkflowState::Cancelled,
         proto::PAYMENTWORKFLOWSTATE_CANCELLED},
        {PaymentWorkflowState::Accepted, proto::PAYMENTWORKFLOWSTATE_ACCEPTED},
        {PaymentWorkflowState::Completed,
         proto::PAYMENTWORKFLOWSTATE_COMPLETED},
        {PaymentWorkflowState::Expired, proto::PAYMENTWORKFLOWSTATE_EXPIRED},
        {PaymentWorkflowState::Initiated,
         proto::PAYMENTWORKFLOWSTATE_INITIATED},
        {PaymentWorkflowState::Aborted, proto::PAYMENTWORKFLOWSTATE_ABORTED},
        {PaymentWorkflowState::Acknowledged,
         proto::PAYMENTWORKFLOWSTATE_ACKNOWLEDGED},
        {PaymentWorkflowState::Rejected, proto::PAYMENTWORKFLOWSTATE_REJECTED},
    };

    return map;
}

auto paymentworkflowtype_map() noexcept -> const PaymentWorkflowTypeMap&
{
    static const auto map = PaymentWorkflowTypeMap{
        {PaymentWorkflowType::Error, proto::PAYMENTWORKFLOWTYPE_ERROR},
        {PaymentWorkflowType::OutgoingCheque,
         proto::PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE},
        {PaymentWorkflowType::IncomingCheque,
         proto::PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE},
        {PaymentWorkflowType::OutgoingInvoice,
         proto::PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE},
        {PaymentWorkflowType::IncomingInvoice,
         proto::PAYMENTWORKFLOWTYPE_INCOMINGINVOICE},
        {PaymentWorkflowType::OutgoingTransfer,
         proto::PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER},
        {PaymentWorkflowType::IncomingTransfer,
         proto::PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER},
        {PaymentWorkflowType::InternalTransfer,
         proto::PAYMENTWORKFLOWTYPE_INTERNALTRANSFER},
        {PaymentWorkflowType::OutgoingCash,
         proto::PAYMENTWORKFLOWTYPE_OUTGOINGCASH},
        {PaymentWorkflowType::IncomingCash,
         proto::PAYMENTWORKFLOWTYPE_INCOMINGCASH},
    };

    return map;
}

auto translate(const PaymentWorkflowState in) noexcept
    -> proto::PaymentWorkflowState
{
    try {
        return paymentworkflowstate_map().at(in);
    } catch (...) {
        return proto::PAYMENTWORKFLOWSTATE_ERROR;
    }
}

auto translate(const PaymentWorkflowType in) noexcept
    -> proto::PaymentWorkflowType
{
    try {
        return paymentworkflowtype_map().at(in);
    } catch (...) {
        return proto::PAYMENTWORKFLOWTYPE_ERROR;
    }
}

auto translate(const proto::PaymentWorkflowState in) noexcept
    -> PaymentWorkflowState
{
    static const auto map = reverse_arbitrary_map<
        PaymentWorkflowState,
        proto::PaymentWorkflowState,
        PaymentWorkflowStateReverseMap>(paymentworkflowstate_map());

    try {
        return map.at(in);
    } catch (...) {
        return PaymentWorkflowState::Error;
    }
}

auto translate(const proto::PaymentWorkflowType in) noexcept
    -> PaymentWorkflowType
{
    static const auto map = reverse_arbitrary_map<
        PaymentWorkflowType,
        proto::PaymentWorkflowType,
        PaymentWorkflowTypeReverseMap>(paymentworkflowtype_map());

    try {
        return map.at(in);
    } catch (...) {
        return PaymentWorkflowType::Error;
    }
}
}  // namespace api::client::internal

auto Translate(const blockchain::Type type) noexcept -> contact::ContactItemType
{
    try {
        return blockchain::params::Data::Chains().at(type).itemtype_;
    } catch (...) {
        return contact::ContactItemType::Unknown;
    }
}

auto Translate(const contact::ContactItemType type) noexcept -> blockchain::Type
{
    using Map =
        std::map<opentxs::contact::ContactItemType, opentxs::blockchain::Type>;

    static const auto build = []() -> auto
    {
        auto output = Map{};

        for (const auto& [chain, data] : blockchain::params::Data::Chains()) {
            output.emplace(data.itemtype_, chain);
        }

        return output;
    };
    static const auto map{build()};

    try {
        return map.at(type);
    } catch (...) {
        return blockchain::Type::Unknown;
    }
}
}  // namespace opentxs
