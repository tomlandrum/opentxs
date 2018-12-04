// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_HELPERS_HPP
#define OPENTXS_CORE_HELPERS_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/Log.hpp"

namespace
{

opentxs::Account::AccountType TranslateAccountTypeStringToEnum(
    const opentxs::String& acctTypeString)
{
    opentxs::Account::AccountType acctType = opentxs::Account::err_acct;

    if (acctTypeString.Compare("user"))
        acctType = opentxs::Account::user;
    else if (acctTypeString.Compare("issuer"))
        acctType = opentxs::Account::issuer;
    else if (acctTypeString.Compare("basket"))
        acctType = opentxs::Account::basket;
    else if (acctTypeString.Compare("basketsub"))
        acctType = opentxs::Account::basketsub;
    else if (acctTypeString.Compare("mint"))
        acctType = opentxs::Account::mint;
    else if (acctTypeString.Compare("voucher"))
        acctType = opentxs::Account::voucher;
    else if (acctTypeString.Compare("stash"))
        acctType = opentxs::Account::stash;
    else
        opentxs::LogOutput(": Error: Unknown account type: ")(acctTypeString)(
            ".")
            .Flush();

    return acctType;
}

void TranslateAccountTypeToString(
    opentxs::Account::AccountType type,
    opentxs::String& acctType)
{
    switch (type) {
        case opentxs::Account::user:
            acctType.Set("user");
            break;
        case opentxs::Account::issuer:
            acctType.Set("issuer");
            break;
        case opentxs::Account::basket:
            acctType.Set("basket");
            break;
        case opentxs::Account::basketsub:
            acctType.Set("basketsub");
            break;
        case opentxs::Account::mint:
            acctType.Set("mint");
            break;
        case opentxs::Account::voucher:
            acctType.Set("voucher");
            break;
        case opentxs::Account::stash:
            acctType.Set("stash");
            break;
        default:
            acctType.Set("err_acct");
            break;
    }
}
}  // namespace

#endif
