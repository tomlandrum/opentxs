// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_UNITDEFINITION_HPP
#define OPENTXS_CORE_CONTRACT_UNITDEFINITION_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <string>

#include "opentxs/SharedPimpl.hpp"
#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/core/contract/Types.hpp"

namespace opentxs
{
namespace contract
{
class Unit;
}  // namespace contract

namespace proto
{
class UnitDefinition;
}  // namespace proto

class Account;
class AccountVisitor;

using OTUnitDefinition = SharedPimpl<contract::Unit>;
}  // namespace opentxs

namespace opentxs
{
namespace contract
{
class OPENTXS_EXPORT Unit : virtual public opentxs::contract::Signable
{
public:
    using SerializedType = proto::UnitDefinition;

    static const VersionNumber DefaultVersion;
    static const VersionNumber MaxVersion;

    static std::string formatLongAmount(
        const Amount lValue,
        const std::int32_t nFactor = 100,
        const std::int32_t nPower = 2,
        const char* szCurrencySymbol = "",
        const char* szThousandSeparator = ",",
        const char* szDecimalPoint = ".");
    static bool ParseFormatted(
        Amount& lResult,
        const std::string& str_input,
        const std::int32_t nFactor = 100,
        const std::int32_t nPower = 2,
        const char* szThousandSeparator = ",",
        const char* szDecimalPoint = ".");
    static std::set<contact::ContactItemType> ValidUnits(
        const VersionNumber version = DefaultVersion) noexcept;

    virtual bool AddAccountRecord(
        const std::string& dataFolder,
        const Account& theAccount) const = 0;
    virtual std::int32_t DecimalPower() const = 0;
    virtual bool DisplayStatistics(String& strContents) const = 0;
    virtual bool EraseAccountRecord(
        const std::string& dataFolder,
        const Identifier& theAcctID) const = 0;
    virtual bool FormatAmountLocale(
        Amount amount,
        std::string& str_output,
        const std::string& str_thousand,
        const std::string& str_decimal) const = 0;
    virtual bool FormatAmountLocale(Amount amount, std::string& str_output)
        const = 0;
    virtual bool FormatAmountWithoutSymbolLocale(
        Amount amount,
        std::string& str_output,
        const std::string& str_thousand,
        const std::string& str_decimal) const = 0;
    virtual bool FormatAmountWithoutSymbolLocale(
        Amount amount,
        std::string& str_output) const = 0;
    virtual std::string FractionalUnitName() const = 0;
    virtual const std::string& GetCurrencyName() const = 0;
    virtual const std::string& GetCurrencySymbol() const = 0;
    using Signable::Serialize;
    OPENTXS_NO_EXPORT virtual bool Serialize(
        SerializedType&,
        bool includeNym = false) const = 0;
    virtual bool Serialize(AllocateOutput destination, bool includeNym = false)
        const = 0;
    virtual bool StringToAmountLocale(
        Amount& amount,
        const std::string& str_input,
        const std::string& str_thousand,
        const std::string& str_decimal) const = 0;
    virtual std::string TLA() const = 0;
    virtual contract::UnitType Type() const = 0;
    virtual contact::ContactItemType UnitOfAccount() const = 0;
    virtual bool VisitAccountRecords(
        const std::string& dataFolder,
        AccountVisitor& visitor,
        const PasswordPrompt& reason) const = 0;

    virtual void InitAlias(const std::string& alias) = 0;

    ~Unit() override = default;

protected:
    Unit() noexcept = default;

private:
    friend OTUnitDefinition;

#ifndef _WIN32
    Unit* clone() const noexcept override = 0;
#endif

    Unit(const Unit&) = delete;
    Unit(Unit&&) = delete;
    Unit& operator=(const Unit&) = delete;
    Unit& operator=(Unit&&) = delete;
};
}  // namespace contract
}  // namespace opentxs
#endif
