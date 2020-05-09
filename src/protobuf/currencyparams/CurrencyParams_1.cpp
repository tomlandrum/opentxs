// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "opentxs/Proto.hpp"  // IWYU pragma: associated

#include <string>

#include "opentxs/protobuf/verify/CurrencyParams.hpp"
#include "protobuf/Check.hpp"

#define PROTO_NAME "currency params"

namespace opentxs
{
namespace proto
{

bool CheckProto_1(const CurrencyParams& input, const bool silent)
{
    if (!input.has_tla()) { FAIL_1("missing TLA") }

    if (3 != input.tla().size()) { FAIL_2("invalid TLA", input.tla()) }

    if (!input.has_fraction()) { FAIL_1("missing fraction") }

    if (1 > input.fraction().size()) {
        FAIL_2("invalid fraction", input.fraction())
    }

    if (!input.has_power()) { FAIL_1("missing power") }

    return true;
}
bool CheckProto_2(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(2)
}

bool CheckProto_3(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(3)
}

bool CheckProto_4(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(4)
}

bool CheckProto_5(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(5)
}

bool CheckProto_6(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(6)
}

bool CheckProto_7(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(7)
}

bool CheckProto_8(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(8)
}

bool CheckProto_9(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(9)
}

bool CheckProto_10(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(10)
}

bool CheckProto_11(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(11)
}

bool CheckProto_12(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(12)
}

bool CheckProto_13(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(13)
}

bool CheckProto_14(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(14)
}

bool CheckProto_15(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(15)
}

bool CheckProto_16(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(16)
}

bool CheckProto_17(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(17)
}

bool CheckProto_18(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(18)
}

bool CheckProto_19(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(19)
}

bool CheckProto_20(const CurrencyParams& input, const bool silent)
{
    UNDEFINED_VERSION(20)
}

}  // namespace proto
}  // namespace opentxs