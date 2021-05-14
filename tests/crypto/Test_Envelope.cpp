// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <algorithm>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/crypto/Envelope.hpp"
#include "opentxs/identity/Nym.hpp"

namespace
{
bool init_{false};

class Test_Envelope : public ::testing::Test
{
public:
    using Nyms = std::vector<ot::Nym_p>;
    using Test = std::pair<bool, std::vector<int>>;
    using Expected = std::vector<Test>;

    static const Expected expected_;
    static Nyms nyms_;

    const ot::api::internal::Core& sender_;
    const ot::api::internal::Core& recipient_;
    const ot::OTPasswordPrompt reason_s_;
    const ot::OTPasswordPrompt reason_r_;
    const ot::OTString plaintext_;

    static bool can_seal(const std::size_t row)
    {
        return expected_.at(row).first;
    }
    static bool can_open(const std::size_t row, const std::size_t column)
    {
        return can_seal(row) && should_seal(row, column);
    }
    static bool is_active(const std::size_t row, const std::size_t column)
    {
        return 0 != (row & (std::size_t{1} << column));
    }
    static bool should_seal(const std::size_t row, const std::size_t column)
    {
        return expected_.at(row).second.at(column);
    }

    Test_Envelope()
        : sender_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient({}, 0)))
        , recipient_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient({}, 1)))
        , reason_s_(sender_.Factory().PasswordPrompt(__FUNCTION__))
        , reason_r_(recipient_.Factory().PasswordPrompt(__FUNCTION__))
        , plaintext_(ot::String::Factory(
              "The quick brown fox jumped over the lazy dog"))
    {
        if (false == init_) {
            init_ = true;
            {
                auto params = ot::NymParameters
                {
#if OT_CRYPTO_SUPPORTED_KEY_ED25519
                    ot::NymParameterType::ed25519
#elif OT_CRYPTO_SUPPORTED_KEY_SECP256K1
                    ot::NymParameterType::secp256k1
#elif OT_CRYPTO_SUPPORTED_KEY_RSA
                    ot::NymParameterType::rsa
#endif
                };
                auto rNym = recipient_.Wallet().Nym(reason_r_, "", params);

                OT_ASSERT(rNym);

                auto bytes = ot::Space{};
                OT_ASSERT(rNym->Serialize(ot::writer(bytes)));
                nyms_.emplace_back(sender_.Wallet().Nym(ot::reader(bytes)));

                OT_ASSERT(bool(*nyms_.crbegin()));
            }
            {
                auto params = ot::NymParameters
                {
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
                    ot::NymParameterType::secp256k1
#elif OT_CRYPTO_SUPPORTED_KEY_RSA
                    ot::NymParameterType::rsa
#elif OT_CRYPTO_SUPPORTED_KEY_ED25519
                    ot::NymParameterType::ed25519
#endif
                };
                auto rNym = recipient_.Wallet().Nym(reason_r_, "", params);

                OT_ASSERT(rNym);

                auto bytes = ot::Space{};
                OT_ASSERT(rNym->Serialize(ot::writer(bytes)));
                nyms_.emplace_back(sender_.Wallet().Nym(ot::reader(bytes)));

                OT_ASSERT(bool(*nyms_.crbegin()));
            }
            {
                auto params = ot::NymParameters
                {
#if OT_CRYPTO_SUPPORTED_KEY_RSA
                    ot::NymParameterType::rsa
#elif OT_CRYPTO_SUPPORTED_KEY_ED25519
                    ot::NymParameterType::ed25519
#elif OT_CRYPTO_SUPPORTED_KEY_SECP256K1
                    ot::NymParameterType::secp256k1
#endif
                };
                auto rNym = recipient_.Wallet().Nym(reason_r_, "", params);

                OT_ASSERT(rNym);

                auto bytes = ot::Space{};
                OT_ASSERT(rNym->Serialize(ot::writer(bytes)));
                nyms_.emplace_back(sender_.Wallet().Nym(ot::reader(bytes)));

                OT_ASSERT(bool(*nyms_.crbegin()));
            }
        }
    }
};

Test_Envelope::Nyms Test_Envelope::nyms_{};
const Test_Envelope::Expected Test_Envelope::expected_{
    {false, {false, false, false}},
    {true, {true, false, false}},
    {true, {false, true, false}},
    {true, {true, true, false}},
    {true, {false, false, true}},
    {true, {true, false, true}},
    {true, {false, true, true}},
    {true, {true, true, true}},
};

TEST_F(Test_Envelope, init_ot) {}

TEST_F(Test_Envelope, one_recipient)
{
    for (const auto& pNym : nyms_) {
        const auto& nym = *pNym;
        auto plaintext = ot::String::Factory();
        auto armored = sender_.Factory().Armored();
        auto sender = sender_.Factory().Envelope();
        const auto sealed = sender->Seal(nym, plaintext_->Bytes(), reason_s_);

        EXPECT_TRUE(sealed);

        if (false == sealed) { continue; }

        EXPECT_TRUE(sender->Armored(armored));

        try {
            auto recipient = recipient_.Factory().Envelope(armored);
            auto opened =
                recipient->Open(nym, plaintext->WriteInto(), reason_r_);

            EXPECT_FALSE(opened);

            auto rNym = recipient_.Wallet().Nym(nym.ID());

            OT_ASSERT(rNym);

            opened = recipient->Open(*rNym, plaintext->WriteInto(), reason_r_);

            EXPECT_TRUE(opened);

            if (opened) { EXPECT_STREQ(plaintext->Get(), plaintext_->Get()); }
        } catch (...) {
            EXPECT_TRUE(false);
        }
    }
}

TEST_F(Test_Envelope, multiple_recipients)
{
    constexpr auto one = std::size_t{1};

    for (auto row = std::size_t{0}; row < (one << nyms_.size()); ++row) {
        auto recipients = ot::crypto::Envelope::Recipients{};
        auto sender = sender_.Factory().Envelope();

        for (auto nym = nyms_.cbegin(); nym != nyms_.cend(); ++nym) {
            const auto column =
                static_cast<std::size_t>(std::distance(nyms_.cbegin(), nym));

            if (is_active(row, column)) { recipients.insert(*nym); }
        }

        const auto sealed =
            sender->Seal(recipients, plaintext_->Bytes(), reason_s_);

        EXPECT_EQ(sealed, can_seal(row));

        if (false == sealed) { continue; }

        auto bytes = ot::Space{};
        ASSERT_TRUE(sender->Serialize(ot::writer(bytes)));

        for (auto nym = nyms_.cbegin(); nym != nyms_.cend(); ++nym) {
            const auto column =
                static_cast<std::size_t>(std::distance(nyms_.cbegin(), nym));
            auto plaintext = ot::String::Factory();

            try {
                auto recipient = sender_.Factory().Envelope(ot::reader(bytes));
                auto rNym = recipient_.Wallet().Nym((*nym)->ID());

                OT_ASSERT(rNym);

                const auto opened =
                    recipient->Open(*rNym, plaintext->WriteInto(), reason_s_);

                EXPECT_EQ(opened, can_open(row, column));

                if (opened) {
                    EXPECT_STREQ(plaintext->Get(), plaintext_->Get());
                }
            } catch (...) {
                EXPECT_TRUE(false);
            }
        }
    }
}
}  // namespace
