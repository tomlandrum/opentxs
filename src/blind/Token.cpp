// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"     // IWYU pragma: associated
#include "1_Internal.hpp"   // IWYU pragma: associated
#include "blind/Token.hpp"  // IWYU pragma: associated

#include <memory>
#include <stdexcept>

#include "2_Factory.hpp"
#if OT_CASH_USING_LUCRE
#include "blind/token/Lucre.hpp"
#endif  // OT_CASH_USING_LUCRE
#include "internal/blind/Blind.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/blind/Purse.hpp"
#include "opentxs/blind/CashType.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/crypto/SymmetricMode.hpp"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/Token.pb.h"
#include "util/Container.hpp"

#define OT_METHOD "opentxs::blind::token::implementation::Token::"

namespace opentxs
{
using ReturnType = blind::token::implementation::Lucre;

auto Factory::Token(const blind::Token& token, blind::Purse& purse) noexcept
    -> std::unique_ptr<blind::Token>
{
    switch (token.Type()) {
        case blind::CashType::Lucre: {

            return std::make_unique<ReturnType>(
                dynamic_cast<const ReturnType&>(token), purse);
        }
        default: {
            OT_FAIL;
        }
    }
}

auto Factory::Token(
    const api::internal::Core& api,
    blind::Purse& purse,
    const proto::Token& serialized) noexcept(false)
    -> std::unique_ptr<blind::Token>
{
    switch (opentxs::blind::internal::translate(serialized.type())) {
        case blind::CashType::Lucre: {

            return std::make_unique<ReturnType>(api, purse, serialized);
        }
        default: {
            throw std::runtime_error("Unknown token type");
        }
    }
}

auto Factory::Token(
    const api::internal::Core& api,
    const identity::Nym& owner,
    const blind::Mint& mint,
    const blind::Token::Denomination value,
    blind::Purse& purse,
    const opentxs::PasswordPrompt& reason) -> std::unique_ptr<blind::Token>
{
    switch (purse.Type()) {
        case blind::CashType::Lucre: {

            return std::make_unique<ReturnType>(
                api, owner, mint, value, purse, reason);
        }
        default: {
            throw std::runtime_error("Unknown token type");
        }
    }
}
}  // namespace opentxs

namespace opentxs::blind::token::implementation
{
const opentxs::crypto::SymmetricMode Token::mode_{
    opentxs::crypto::SymmetricMode::ChaCha20Poly1305};

Token::Token(
    const api::internal::Core& api,
    Purse& purse,
    const blind::TokenState state,
    const blind::CashType type,
    const identifier::Server& notary,
    const identifier::UnitDefinition& unit,
    const std::uint64_t series,
    const Denomination denomination,
    const Time validFrom,
    const Time validTo,
    const VersionNumber version)
    : api_(api)
    , purse_(purse)
    , state_(state)
    , notary_(notary)
    , unit_(unit)
    , series_(series)
    , denomination_(denomination)
    , valid_from_(validFrom)
    , valid_to_(validTo)
    , type_(type)
    , version_(version)
{
}

Token::Token(const Token& rhs)
    : Token(
          rhs.api_,
          rhs.purse_,
          rhs.state_,
          rhs.type_,
          rhs.notary_,
          rhs.unit_,
          rhs.series_,
          rhs.denomination_,
          rhs.valid_from_,
          rhs.valid_to_,
          rhs.version_)
{
}

Token::Token(
    const api::internal::Core& api,
    Purse& purse,
    const proto::Token& in)
    : Token(
          api,
          purse,
          opentxs::blind::internal::translate(in.state()),
          opentxs::blind::internal::translate(in.type()),
          identifier::Server::Factory(in.notary()),
          identifier::UnitDefinition::Factory(in.mint()),
          in.series(),
          in.denomination(),
          Clock::from_time_t(in.validfrom()),
          Clock::from_time_t(in.validto()),
          in.version())
{
}

Token::Token(
    const api::internal::Core& api,
    Purse& purse,
    const VersionNumber version,
    const blind::TokenState state,
    const std::uint64_t series,
    const Denomination denomination,
    const Time validFrom,
    const Time validTo)
    : Token(
          api,
          purse,
          state,
          purse.Type(),
          purse.Notary(),
          purse.Unit(),
          series,
          denomination,
          validFrom,
          validTo,
          version)
{
}

auto Token::reencrypt(
    const crypto::key::Symmetric& oldKey,
    const PasswordPrompt& oldPassword,
    const crypto::key::Symmetric& newKey,
    const PasswordPrompt& newPassword,
    proto::Ciphertext& ciphertext) -> bool
{
    auto plaintext = Data::Factory();
    auto output =
        oldKey.Decrypt(ciphertext, oldPassword, plaintext->WriteInto());

    if (false == output) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to decrypt ciphertext.")
            .Flush();

        return false;
    }

    output = newKey.Encrypt(
        plaintext->Bytes(),
        newPassword,
        ciphertext,
        false,
        opentxs::crypto::SymmetricMode::ChaCha20Poly1305);

    if (false == output) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to encrypt ciphertext.")
            .Flush();

        return false;
    }

    return output;
}

auto Token::Serialize() const -> proto::Token
{
    proto::Token output{};
    output.set_version(version_);
    output.set_type(opentxs::blind::internal::translate(type_));
    output.set_state(opentxs::blind::internal::translate(state_));
    output.set_notary(notary_->str());
    output.set_mint(unit_->str());
    output.set_series(series_);
    output.set_denomination(denomination_);
    output.set_validfrom(Clock::to_time_t(valid_from_));
    output.set_validto(Clock::to_time_t(valid_to_));

    return output;
}
}  // namespace opentxs::blind::token::implementation
