// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_ACCOUNTACTIVITY_HPP
#define OPENTXS_UI_ACCOUNTACTIVITY_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <tuple>

#include "opentxs/SharedPimpl.hpp"
#if OT_BLOCKCHAIN
#include "opentxs/blockchain/Types.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/ui/List.hpp"

#ifdef SWIG
// clang-format off
%rename(UIAccountActivity) opentxs::ui::AccountActivity;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class AccountActivity;
class BalanceItem;
}  // namespace ui
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class AccountActivity : virtual public List
{
public:
    OPENTXS_EXPORT virtual std::string AccountID() const noexcept = 0;
    OPENTXS_EXPORT virtual Amount Balance() const noexcept = 0;
    OPENTXS_EXPORT virtual int BalancePolarity() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string ContractID() const noexcept = 0;
#if OT_BLOCKCHAIN
    OPENTXS_EXPORT virtual std::string DepositAddress() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string DepositAddress(
        const blockchain::Type chain) const noexcept = 0;
    OPENTXS_EXPORT virtual std::vector<blockchain::Type> DepositChains()
        const noexcept = 0;
#endif  // OT_BLOCKCHAIN
    OPENTXS_EXPORT virtual std::string DisplayBalance() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string DisplayUnit() const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::BalanceItem>
    First() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Name() const noexcept = 0;
    OPENTXS_EXPORT virtual opentxs::SharedPimpl<opentxs::ui::BalanceItem> Next()
        const noexcept = 0;
    OPENTXS_EXPORT virtual std::string NotaryID() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string NotaryName() const noexcept = 0;
    OPENTXS_EXPORT virtual bool Send(
        const Identifier& contact,
        const Amount amount,
        const std::string& memo = {}) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Send(
        const Identifier& contact,
        const std::string& amount,
        const std::string& memo = {}) const noexcept = 0;
#if OT_BLOCKCHAIN
    OPENTXS_EXPORT virtual bool Send(
        const std::string& address,
        const Amount amount,
        const std::string& memo = {}) const noexcept = 0;
    OPENTXS_EXPORT virtual bool Send(
        const std::string& address,
        const std::string& amount,
        const std::string& memo = {}) const noexcept = 0;
    OPENTXS_EXPORT virtual double SyncPercentage() const noexcept = 0;
    OPENTXS_EXPORT virtual std::pair<int, int> SyncProgress()
        const noexcept = 0;
#endif  // OT_BLOCKCHAIN
    OPENTXS_EXPORT virtual AccountType Type() const noexcept = 0;
    OPENTXS_EXPORT virtual contact::ContactItemType Unit() const noexcept = 0;
#if OT_BLOCKCHAIN
    OPENTXS_EXPORT virtual bool ValidateAddress(
        const std::string& text) const noexcept = 0;
#endif  // OT_BLOCKCHAIN
    OPENTXS_EXPORT virtual std::string ValidateAmount(
        const std::string& text) const noexcept = 0;

    OPENTXS_EXPORT ~AccountActivity() override = default;

protected:
    AccountActivity() noexcept = default;

private:
    AccountActivity(const AccountActivity&) = delete;
    AccountActivity(AccountActivity&&) = delete;
    AccountActivity& operator=(const AccountActivity&) = delete;
    AccountActivity& operator=(AccountActivity&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
