// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "opentxs/Bytes.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/crypto/Asymmetric.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/crypto/Bip32Child.hpp"
#include "opentxs/crypto/Bip43Purpose.hpp"
#include "opentxs/crypto/Language.hpp"
#include "opentxs/crypto/SeedStyle.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"

namespace
{
struct User {
    static constexpr auto account_ = ot::Bip32Index{0};
    static constexpr auto index_ = ot::Bip32Index{0};

    [[maybe_unused]] auto blinding_key_public()
        -> const ot::crypto::key::EllipticCurve&
    {
        if (false == bool(blind_key_secret_)) {
            throw std::runtime_error("missing private key");
        }

        auto& var = blind_key_public_;

        if (false == bool(var)) { var = blind_key_secret_->asPublicEC(); }

        if (false == bool(var)) { throw std::runtime_error("Failed"); }

        return *var;
    }
    [[maybe_unused]] auto blinding_key_secret(
        const ot::api::client::Manager& api,
        const ot::blockchain::Type chain,
        const ot::PasswordPrompt& reason)
        -> const ot::crypto::key::EllipticCurve&
    {
        auto& var = blind_key_secret_;

        if (false == bool(var)) {
            auto bytes = ot::Space{};
            if (false == bip44_path(api, chain, ot::writer(bytes))) {
                throw std::runtime_error("Failed");
            }
            var = api.Seeds().AccountChildKey(
                ot::reader(bytes), ot::INTERNAL_CHAIN, index_, reason);
        }

        if (false == bool(var)) { throw std::runtime_error("Failed"); }

        return *var;
    }
    [[maybe_unused]] auto blinding_key_secret(
        const ot::api::client::Manager& api,
        const std::string& privateKey,
        const ot::PasswordPrompt& reason)
        -> const ot::crypto::key::EllipticCurve&
    {
        auto& var = blind_key_secret_;

        if (false == bool(var)) {
            const auto decoded =
                api.Factory().Data(privateKey, ot::StringStyle::Hex);
            const auto key = api.Factory().SecretFromBytes(decoded->Bytes());
            var = api.Asymmetric().InstantiateSecp256k1Key(key, reason);
        }

        if (false == bool(var)) { throw std::runtime_error("Failed"); }

        return *var;
    }
    auto payment_code_public(
        const ot::api::Core& api,
        const std::string& base58) -> const ot::PaymentCode&
    {
        auto& var = pc_public_;

        if (false == var.has_value()) {
            var.emplace(api.Factory().PaymentCode(base58));
        }

        if (false == var.has_value()) { throw std::runtime_error("Failed"); }

        return var.value().get();
    }
    auto payment_code_secret(
        const ot::api::Core& api,
        const std::uint8_t version,
        const ot::PasswordPrompt& reason) -> const ot::PaymentCode&
    {
        if (false == seed_.has_value()) {
            throw std::runtime_error("missing seed");
        }

        auto& var = pc_secret_;

        if (false == var.has_value()) {
            var.emplace(api.Factory().PaymentCode(
                seed_.value(), account_, version, reason));
        }

        if (false == var.has_value()) { throw std::runtime_error("Failed"); }

        return var.value().get();
    }
    auto seed(
        const ot::api::Core& api,
        const std::string_view wordList,
        const ot::PasswordPrompt& reason) -> const std::string&
    {
        auto& var = seed_;

        if (false == var.has_value()) {
            const auto words = api.Factory().SecretFromText(wordList);
            const auto phrase = api.Factory().Secret(0);
            var.emplace(api.Seeds().ImportSeed(
                words,
                phrase,
                ot::crypto::SeedStyle::BIP39,
                ot::crypto::Language::en,
                reason));
        }

        if (false == var.has_value()) { throw std::runtime_error("Failed"); }

        return var.value();
    }

    auto cleanup() -> void
    {
        blind_key_public_.reset();
        blind_key_secret_.reset();
        pc_public_ = std::nullopt;
        pc_secret_ = std::nullopt;
        seed_ = std::nullopt;
    }

    ~User() { cleanup(); }

private:
    std::optional<std::string> seed_{};
    std::optional<ot::OTPaymentCode> pc_secret_{};
    std::optional<ot::OTPaymentCode> pc_public_{};
    std::unique_ptr<const ot::crypto::key::EllipticCurve> blind_key_secret_{};
    std::unique_ptr<const ot::crypto::key::EllipticCurve> blind_key_public_{};

    auto bip44_path(
        const ot::api::client::Manager& api,
        const ot::blockchain::Type chain,
        ot::AllocateOutput destination) const -> bool
    {
        if (false == seed_.has_value()) {
            throw std::runtime_error("missing seed");
        }

        if (false ==
            api.Blockchain().Bip44Path(chain, seed_.value(), destination)) {
            throw std::runtime_error("missing path");
        }

        return true;
    }
};

class PC_Fixture_Base : virtual public ::testing::Test
{
public:
    static User user_1_;
    static User user_2_;

    const ot::api::client::Manager& api_;
    const ot::OTPasswordPrompt reason_;
    const std::string& alice_seed_;
    const std::string& bob_seed_;
    const ot::PaymentCode& alice_pc_secret_;
    const ot::PaymentCode& alice_pc_public_;
    const ot::PaymentCode& bob_pc_secret_;
    const ot::PaymentCode& bob_pc_public_;

    virtual auto Shutdown() noexcept -> void
    {
        user_1_.cleanup();
        user_2_.cleanup();
    }

    PC_Fixture_Base(
        const std::uint8_t aliceVersion,
        const std::uint8_t bobVersion,
        const std::string& aliceBip39,
        const std::string& bobBip39,
        const std::string& aliceExpectedPC,
        const std::string& bobExpectedPC)
        : api_(ot::Context().StartClient({}, 0))
        , reason_(api_.Factory().PasswordPrompt(__FUNCTION__))
        , alice_seed_(user_1_.seed(api_, aliceBip39, reason_))
        , bob_seed_(user_2_.seed(api_, bobBip39, reason_))
        , alice_pc_secret_(
              user_1_.payment_code_secret(api_, aliceVersion, reason_))
        , alice_pc_public_(user_1_.payment_code_public(api_, aliceExpectedPC))
        , bob_pc_secret_(user_2_.payment_code_secret(api_, bobVersion, reason_))
        , bob_pc_public_(user_2_.payment_code_public(api_, bobExpectedPC))
    {
    }
};

User PC_Fixture_Base::user_1_{};
User PC_Fixture_Base::user_2_{};
}  // namespace
