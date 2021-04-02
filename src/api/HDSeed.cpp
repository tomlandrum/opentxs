// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "api/HDSeed.hpp"  // IWYU pragma: associated

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "internal/api/Api.hpp"
#include "internal/api/Factory.hpp"
#include "internal/api/crypto/Crypto.hpp"
#include "internal/crypto/key/Factory.hpp"
#include "internal/identity/credential/Credential.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/api/Primitives.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/Bip32Child.hpp"
#include "opentxs/crypto/Bip39.hpp"
#include "opentxs/crypto/Bip43Purpose.hpp"
#include "opentxs/crypto/Bip44Type.hpp"
#include "opentxs/crypto/HashType.hpp"
#include "opentxs/crypto/Language.hpp"      // IWYU pragma: keep
#include "opentxs/crypto/SeedStrength.hpp"  // IWYU pragma: keep
#include "opentxs/crypto/SeedStyle.hpp"     // IWYU pragma: keep
#include "opentxs/crypto/SymmetricMode.hpp"
#include "opentxs/crypto/key/HD.hpp"
#include "opentxs/crypto/key/Secp256k1.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/HDPath.pb.h"
#include "util/HDIndex.hpp"  // IWYU pragma: keep

#define OT_METHOD "opentxs::api::implementation::HDSeed::"

namespace opentxs::factory
{
auto HDSeed(
    const api::Factory& factory,
    const api::crypto::internal::Asymmetric& asymmetric,
    const api::crypto::Symmetric& symmetric,
    const api::storage::Storage& storage,
    const crypto::Bip32& bip32,
    const crypto::Bip39& bip39) noexcept -> std::unique_ptr<api::HDSeed>
{
    using ReturnType = api::implementation::HDSeed;

    return std::make_unique<ReturnType>(
        factory, asymmetric, symmetric, storage, bip32, bip39);
}
}  // namespace opentxs::factory

namespace opentxs::api::implementation
{
HDSeed::HDSeed(
    const api::Factory& factory,
    [[maybe_unused]] const api::crypto::internal::Asymmetric& asymmetric,
    const api::crypto::Symmetric& symmetric,
    const api::storage::Storage& storage,
    const opentxs::crypto::Bip32& bip32,
    const opentxs::crypto::Bip39& bip39)
    : factory_(factory)
    , symmetric_(symmetric)
#if OT_CRYPTO_WITH_BIP32
    , asymmetric_(asymmetric)
#endif  // OT_CRYPTO_WITH_BIP32
    , storage_(storage)
    , bip32_(bip32)
    , bip39_(bip39)
    , seed_lock_()
    , seeds_()
{
}

auto HDSeed::AccountChildKey(
    const proto::HDPath& rootPath,
    const BIP44Chain internal,
    const Bip32Index index,
    const PasswordPrompt& reason) const
    -> std::unique_ptr<opentxs::crypto::key::HD>
{
#if OT_CRYPTO_WITH_BIP32
    auto parent = AccountKey(rootPath, internal, reason);

    if (!parent) { return {}; }

    return parent->ChildKey(index, reason);
#else

    return {};
#endif  // OT_CRYPTO_WITH_BIP32
}

auto HDSeed::AccountKey(
    const proto::HDPath& rootPath,
    const BIP44Chain internal,
    const PasswordPrompt& reason) const
    -> std::unique_ptr<opentxs::crypto::key::HD>
{
#if OT_CRYPTO_WITH_BIP32
    std::string fingerprint{rootPath.root()};
    const auto change =
        (INTERNAL_CHAIN == internal) ? Bip32Index{1u} : Bip32Index{0u};
    Path path{};

    for (const auto& child : rootPath.child()) { path.emplace_back(child); }

    path.emplace_back(change);

    return GetHDKey(fingerprint, EcdsaCurve::secp256k1, path, reason);
#else

    return {};
#endif  // OT_CRYPTO_WITH_BIP32
}

auto HDSeed::AllowedSeedTypes() const noexcept -> const SupportedSeeds&
{
    static const auto map = SupportedSeeds{
        {Style::BIP39, "BIP-39"},
    };

    return map;
}

auto HDSeed::AllowedLanguages(const Style type) const noexcept
    -> const SupportedLanguages&
{
    static const auto null = SupportedLanguages{};
    static const auto map = std::map<Style, SupportedLanguages>{
        {Style::BIP39,
         {
             {Language::en, "English"},
         }},
    };

    try {
        return map.at(type);
    } catch (...) {

        return null;
    }
}

auto HDSeed::AllowedSeedStrength(const Style type) const noexcept
    -> const SupportedStrengths&
{
    static const auto null = SupportedStrengths{};
    static const auto map = std::map<Style, SupportedStrengths>{
        {Style::BIP39,
         {
             {Strength::Twelve, "12"},
             {Strength::Fifteen, "15"},
             {Strength::Eighteen, "18"},
             {Strength::TwentyOne, "21"},
             {Strength::TwentyFour, "24"},
         }},
    };

    try {
        return map.at(type);
    } catch (...) {

        return null;
    }
}

auto HDSeed::Bip32Root(const std::string& seedID, const PasswordPrompt& reason)
    const -> std::string
{
    auto lock = Lock{seed_lock_};

    try {
        const auto& seed = get_seed(lock, seedID, reason);
        const auto entropy = factory_.Data(seed.Entropy().Bytes());

        return entropy->asHex();
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto HDSeed::DefaultSeed() const -> std::string
{
    auto lock = Lock{seed_lock_};

    return storage_.DefaultSeed();
}

auto HDSeed::GetHDKey(
    const std::string& fingerprint,
    const EcdsaCurve& curve,
    const Path& path,
    const PasswordPrompt& reason,
    const identity::KeyRole role,
    const VersionNumber version) const
    -> std::unique_ptr<opentxs::crypto::key::HD>
{
#if OT_CRYPTO_WITH_BIP32
    Bip32Index notUsed{0};
    auto seed = Seed(fingerprint, notUsed, reason);

    if (seed->empty()) { return {}; }

    return asymmetric_.NewHDKey(
        fingerprint, seed, curve, path, reason, role, version);
#else

    return {};
#endif  // OT_CRYPTO_WITH_BIP32
}

auto HDSeed::GetOrCreateDefaultSeed(
    std::string& seedID,
    Style& type,
    Language& lang,
    Bip32Index& index,
    const Strength strength,
    const PasswordPrompt& reason) const -> OTSecret
{
    auto lock = Lock{seed_lock_};
    seedID = storage_.DefaultSeed();

    if (seedID.empty()) {
        seedID = new_seed(lock, type, lang, strength, reason);
    }

    try {
        if (seedID.empty()) {
            throw std::runtime_error{"Failed to create seed"};
        }

        const auto& seed = get_seed(lock, seedID, reason);

        return seed.Entropy();
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return factory_.Secret(0);
    }
}

auto HDSeed::GetPaymentCode(
    const std::string& fingerprint,
    const Bip32Index nym,
    const std::uint8_t version,
    const PasswordPrompt& reason) const
    -> std::unique_ptr<opentxs::crypto::key::Secp256k1>
{
#if OT_CRYPTO_WITH_BIP32 && OT_CRYPTO_SUPPORTED_KEY_SECP256K1
    Bip32Index notUsed{0};
    auto seed = Seed(fingerprint, notUsed, reason);

    if (seed->empty()) { return {}; }

    auto pKey = asymmetric_.NewSecp256k1Key(
        fingerprint,
        seed,
        {HDIndex{Bip43Purpose::PAYCODE, Bip32Child::HARDENED},
         HDIndex{Bip44Type::BITCOIN, Bip32Child::HARDENED},
         HDIndex{nym, Bip32Child::HARDENED}},
        reason);

    if (!pKey) { return pKey; }

    const auto& key = *pKey;

    switch (version) {
        case 3: {
        } break;
        case 1:
        case 2:
        default: {
            return pKey;
        }
    }

    const auto& api = asymmetric_.API();
    const auto code = [&] {
        auto out = api.Factory().Secret(0);
        api.Crypto().Hash().Digest(
            opentxs::crypto::HashType::Sha256D,
            key.PublicKey(),
            out->WriteInto());

        return out;
    }();
    const auto path = [&] {
        auto out = proto::HDPath{};
        key.Path(out);

        return out;
    }();

    return factory::Secp256k1Key(
        api,
        api.Crypto().SECP256K1(),
        api.Factory().SecretFromBytes(key.PrivateKey(reason)),
        code,
        api.Factory().Data(key.PublicKey()),
        path,
        key.Parent(),
        opentxs::identity::credential::internal::translate(key.Role()),
        key.Version(),
        reason);
#else

    return {};
#endif  // OT_CRYPTO_WITH_BIP32 && OT_CRYPTO_SUPPORTED_KEY_SECP256K1
}

auto HDSeed::GetStorageKey(
    const std::string& fingerprint,
    const PasswordPrompt& reason) const -> OTSymmetricKey
{
#if OT_CRYPTO_WITH_BIP32
    auto pKey = GetHDKey(
        fingerprint,
        EcdsaCurve::secp256k1,
        {HDIndex{Bip43Purpose::FS, Bip32Child::HARDENED},
         HDIndex{Bip32Child::ENCRYPT_KEY, Bip32Child::HARDENED}},
        reason);

    if (false == bool(pKey)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to derive storage key.")
            .Flush();

        return OTSymmetricKey{opentxs::factory::SymmetricKey()};
    }

    const auto& key = *pKey;

    return symmetric_.Key(
        opentxs::Context().Factory().SecretFromBytes(key.PrivateKey(reason)));
#else

    return factory_.SymmetricKey();
#endif  // OT_CRYPTO_WITH_BIP32
}

auto HDSeed::get_seed(
    const Lock& lock,
    const std::string& seedID,
    const PasswordPrompt& reason) const noexcept(false)
    -> opentxs::crypto::Seed&
{
    auto proto = std::shared_ptr<proto::Seed>{};

    if (auto it{seeds_.find(seedID)}; it != seeds_.end()) { return it->second; }

    if (false == storage_.Load(seedID, proto) || (false == bool(proto))) {
        throw std::runtime_error{std::string{"Failed to load seed "} + seedID};
    }

    auto seed = opentxs::crypto::Seed{
        bip39_, symmetric_, factory_, storage_, *proto, reason};
    auto id = seed.ID().str();

    OT_ASSERT(id == seedID);

    const auto [it, added] = seeds_.try_emplace(std::move(id), std::move(seed));

    if (false == added) {
        throw std::runtime_error{"failed to instantiate seed"};
    }

    return it->second;
}

auto HDSeed::ImportRaw(const Secret& entropy, const PasswordPrompt& reason)
    const -> std::string
{
    auto lock = Lock{seed_lock_};

    try {
        auto seed = opentxs::crypto::Seed{
            bip32_, bip39_, symmetric_, factory_, storage_, entropy, reason};
        const auto id = seed.ID().str();
        seeds_.try_emplace(id, std::move(seed));

        return id;
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto HDSeed::ImportSeed(
    const Secret& words,
    const Secret& passphrase,
    const Style type,
    const Language lang,
    const PasswordPrompt& reason) const -> std::string
{
    switch (type) {
        case Style::BIP39: {
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported seed type")
                .Flush();

            return {};
        }
    }

    auto lock = Lock{seed_lock_};

    try {
        auto seed = opentxs::crypto::Seed{
            bip32_,
            bip39_,
            symmetric_,
            factory_,
            storage_,
            type,
            lang,
            words,
            passphrase,
            reason};
        const auto id = seed.ID().str();
        seeds_.try_emplace(id, std::move(seed));

        return id;
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto HDSeed::LongestWord(const Style type, const Language lang) const noexcept
    -> std::size_t
{
    switch (type) {
        case Style::BIP39: {

            return bip39_.LongestWord(lang);
        }
        case Style::BIP32: {

            return 130u;
        }
        default: {

            return {};
        }
    }
}

auto HDSeed::NewSeed(
    const Style type,
    const Language lang,
    const Strength strength,
    const PasswordPrompt& reason) const -> std::string
{
    auto lock = Lock{seed_lock_};

    return new_seed(lock, type, lang, strength, reason);
}

auto HDSeed::new_seed(
    const Lock&,
    const Style type,
    const Language lang,
    const Strength strength,
    const PasswordPrompt& reason) const noexcept -> std::string
{
    switch (type) {
        case Style::BIP39: {
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported seed type")
                .Flush();

            return {};
        }
    }

    try {
        auto seed = opentxs::crypto::Seed{
            bip32_,
            bip39_,
            symmetric_,
            factory_,
            storage_,
            lang,
            strength,
            reason};
        const auto id = seed.ID().str();
        seeds_.try_emplace(id, std::move(seed));

        return id;
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto HDSeed::Passphrase(const std::string& seedID, const PasswordPrompt& reason)
    const -> std::string
{
    auto lock = Lock{seed_lock_};

    try {
        const auto& seed = get_seed(lock, seedID, reason);

        return std::string{seed.Phrase().Bytes()};
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto HDSeed::Seed(
    const std::string& seedID,
    Bip32Index& index,
    const PasswordPrompt& reason) const -> OTSecret
{
    auto lock = Lock{seed_lock_};

    try {
        const auto& seed = get_seed(lock, seedID, reason);
        index = seed.Index();

        return seed.Entropy();
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return factory_.Secret(0);
    }
}

auto HDSeed::UpdateIndex(
    const std::string& seedID,
    const Bip32Index index,
    const PasswordPrompt& reason) const -> bool
{
    auto lock = Lock{seed_lock_};

    try {
        auto& seed = get_seed(lock, seedID, reason);

        return seed.IncrementIndex(index);
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return false;
    }
}

auto HDSeed::ValidateWord(
    const Style type,
    const Language lang,
    const std::string_view word) const noexcept -> Matches
{
    if (Style::BIP39 != type) { return {}; }

    return bip39_.GetSuggestions(lang, word);
}

auto HDSeed::WordCount(const Style type, const Strength strength) const noexcept
    -> std::size_t
{
    if (Style::BIP39 != type) { return {}; }

    static const auto map = std::map<Strength, std::size_t>{
        {Strength::Twelve, 12},
        {Strength::Fifteen, 15},
        {Strength::Eighteen, 18},
        {Strength::TwentyOne, 21},
        {Strength::TwentyFour, 24},
    };

    try {

        return map.at(strength);
    } catch (...) {

        return {};
    }
}

auto HDSeed::Words(const std::string& seedID, const PasswordPrompt& reason)
    const -> std::string
{
    auto lock = Lock{seed_lock_};

    try {
        const auto& seed = get_seed(lock, seedID, reason);

        return std::string{seed.Words().Bytes()};
    } catch (const std::exception& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}
}  // namespace opentxs::api::implementation
