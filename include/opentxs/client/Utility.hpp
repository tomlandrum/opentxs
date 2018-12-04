// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CLIENT_UTILITY_HPP
#define OPENTXS_CLIENT_UTILITY_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/util/Common.hpp"
#include "opentxs/Types.hpp"

#include <cstdint>
#include <array>
#include <string>

namespace opentxs
{

inline bool VerifyStringVal(const std::string& nValue)
{
    return 0 < nValue.length();
}

EXPORT std::int32_t InterpretTransactionMsgReply(
    const api::client::Manager& api,
    const std::string& NOTARY_ID,
    const std::string& NYM_ID,
    const std::string& ACCOUNT_ID,
    const std::string& strAttempt,
    const std::string& strResponse);
EXPORT bool VerifyMessage(const std::string& strMessage);
EXPORT NetworkOperationStatus VerifyMessageSuccess(
    const api::client::Manager& api,
    const std::string& message);
EXPORT std::int32_t VerifyMsgBalanceAgrmntSuccess(
    const api::client::Manager& api,
    const std::string& NOTARY_ID,
    const std::string& NYM_ID,
    const std::string& ACCOUNT_ID,
    const std::string& strMessage);
EXPORT std::int32_t VerifyMsgTrnxSuccess(
    const api::client::Manager& api,
    const std::string& NOTARY_ID,
    const std::string& NYM_ID,
    const std::string& ACCOUNT_ID,
    const std::string& strMessage);
}  // namespace opentxs

#endif
