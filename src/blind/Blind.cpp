// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"              // IWYU pragma: associated
#include "1_Internal.hpp"            // IWYU pragma: associated
#include "internal/blind/Blind.hpp"  // IWYU pragma: associated

#include "opentxs/blind/CashType.hpp"
#include "opentxs/blind/PurseType.hpp"
#include "opentxs/blind/TokenState.hpp"
#include "opentxs/protobuf/CashEnums.pb.h"
#include "util/Container.hpp"

namespace opentxs::blind::internal
{
auto cashtype_map() noexcept -> const CashTypeMap&
{
    static const auto map = CashTypeMap{
        {blind::CashType::Error, proto::CASHTYPE_ERROR},
        {blind::CashType::Lucre, proto::CASHTYPE_LUCRE},
    };

    return map;
}

auto pursetype_map() noexcept -> const PurseTypeMap&
{
    static const auto map = PurseTypeMap{
        {blind::PurseType::Error, proto::PURSETYPE_ERROR},
        {blind::PurseType::Request, proto::PURSETYPE_REQUEST},
        {blind::PurseType::Issue, proto::PURSETYPE_ISSUE},
        {blind::PurseType::Normal, proto::PURSETYPE_NORMAL},
    };

    return map;
}

auto tokenstate_map() noexcept -> const TokenStateMap&
{
    static const auto map = TokenStateMap{
        {blind::TokenState::Error, proto::TOKENSTATE_ERROR},
        {blind::TokenState::Blinded, proto::TOKENSTATE_BLINDED},
        {blind::TokenState::Signed, proto::TOKENSTATE_SIGNED},
        {blind::TokenState::Ready, proto::TOKENSTATE_READY},
        {blind::TokenState::Spent, proto::TOKENSTATE_SPENT},
        {blind::TokenState::Expired, proto::TOKENSTATE_EXPIRED},
    };

    return map;
}

auto translate(const blind::CashType in) noexcept -> proto::CashType
{
    try {
        return cashtype_map().at(in);
    } catch (...) {
        return proto::CASHTYPE_ERROR;
    }
}

auto translate(const blind::PurseType in) noexcept -> proto::PurseType
{
    try {
        return pursetype_map().at(in);
    } catch (...) {
        return proto::PURSETYPE_ERROR;
    }
}

auto translate(const blind::TokenState in) noexcept -> proto::TokenState
{
    try {
        return tokenstate_map().at(in);
    } catch (...) {
        return proto::TOKENSTATE_ERROR;
    }
}

auto translate(const proto::CashType in) noexcept -> blind::CashType
{
    static const auto map = reverse_arbitrary_map<
        blind::CashType,
        proto::CashType,
        CashTypeReverseMap>(cashtype_map());
    try {
        return map.at(in);
    } catch (...) {
        return blind::CashType::Error;
    }
}

auto translate(const proto::PurseType in) noexcept -> blind::PurseType
{
    static const auto map = reverse_arbitrary_map<
        blind::PurseType,
        proto::PurseType,
        PurseTypeReverseMap>(pursetype_map());
    try {
        return map.at(in);
    } catch (...) {
        return blind::PurseType::Error;
    }
}

auto translate(const proto::TokenState in) noexcept -> blind::TokenState
{
    static const auto map = reverse_arbitrary_map<
        blind::TokenState,
        proto::TokenState,
        TokenStateReverseMap>(tokenstate_map());
    try {
        return map.at(in);
    } catch (...) {
        return blind::TokenState::Error;
    }
}
}  // namespace opentxs::blind::internal
