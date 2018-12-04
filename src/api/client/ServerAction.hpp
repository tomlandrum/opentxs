// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

namespace opentxs::api::client::implementation
{
class ServerAction : virtual public opentxs::api::client::ServerAction
{
public:
    Action ActivateSmartContract(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& accountID,
        const std::string& agentName,
        std::unique_ptr<OTSmartContract>& contract) const override;
    Action AdjustUsageCredits(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& targetNymID,
        const Amount adjustment) const override;
    Action CancelPaymentPlan(
        const Identifier& localNymID,
        const Identifier& serverID,
        std::unique_ptr<OTPaymentPlan>& plan) const override;
    Action CreateMarketOffer(
        const Identifier& assetAccountID,
        const Identifier& currencyAccountID,
        const Amount scale,
        const Amount increment,
        const Amount quantity,
        const Amount price,
        const bool selling,
        const std::chrono::seconds lifetime,
        const std::string& stopSign,
        const Amount activationPrice) const override;
    Action DepositPaymentPlan(
        const Identifier& localNymID,
        const Identifier& serverID,
        std::unique_ptr<OTPaymentPlan>& plan) const override;
    Action DownloadMarketList(
        const Identifier& localNymID,
        const Identifier& serverID) const override;
    Action DownloadMarketOffers(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& marketID,
        const Amount depth) const override;
    Action DownloadMarketRecentTrades(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& marketID) const override;
    Action DownloadNymMarketOffers(
        const Identifier& localNymID,
        const Identifier& serverID) const override;
    Action ExchangeBasketCurrency(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& instrumentDefinitionID,
        const Identifier& accountID,
        const Identifier& basketID,
        const bool direction) const override;
    Action IssueBasketCurrency(
        const Identifier& localNymID,
        const Identifier& serverID,
        const proto::UnitDefinition& basket,
        const std::string& label) const override;
    Action KillMarketOffer(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& accountID,
        const TransactionNumber number) const override;
    Action KillPaymentPlan(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& accountID,
        const TransactionNumber number) const override;
    Action PayDividend(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& instrumentDefinitionID,
        const Identifier& accountID,
        const std::string& memo,
        const Amount amountPerShare) const override;
    Action TriggerClause(
        const Identifier& localNymID,
        const Identifier& serverID,
        const TransactionNumber transactionNumber,
        const std::string& clause,
        const std::string& parameter) const override;
    Action UnregisterAccount(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& accountID) const override;
    Action UnregisterNym(
        const Identifier& localNymID,
        const Identifier& serverID) const override;
    Action WithdrawVoucher(
        const Identifier& localNymID,
        const Identifier& serverID,
        const Identifier& accountID,
        const Identifier& recipientNymID,
        const Amount amount,
        const std::string& memo) const override;

    ~ServerAction() = default;

private:
    friend opentxs::Factory;

    const api::client::Manager& api_;
    ContextLockCallback lock_callback_;

    ServerAction(
        const api::client::Manager& api,
        const ContextLockCallback& lockCallback);
    ServerAction() = delete;
    ServerAction(const ServerAction&) = delete;
    ServerAction(ServerAction&&) = delete;
    ServerAction& operator=(const ServerAction&) = delete;
    ServerAction& operator=(ServerAction&&) = delete;
};
}  // namespace opentxs::api::client::implementation
