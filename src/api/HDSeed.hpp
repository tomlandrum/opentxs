// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include "crypto/Seed.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/key/EllipticCurve.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/protobuf/Enums.pb.h"

namespace opentxs
{
namespace api
{
namespace crypto
{
namespace internal
{
struct Asymmetric;
}  // namespace internal

class Symmetric;
}  // namespace crypto

namespace storage
{
class Storage;
}  // namespace storage

class Factory;
}  // namespace api

namespace crypto
{
namespace key
{
class HD;
class Secp256k1;
}  // namespace key

class Bip32;
class Bip39;
}  // namespace crypto

namespace proto
{
class HDPath;
class Seed;
}  // namespace proto

class PasswordPrompt;
}  // namespace opentxs

namespace opentxs::api::implementation
{
class HDSeed final : public api::HDSeed
{
public:
    auto AccountChildKey(
        const proto::HDPath& path,
        const BIP44Chain internal,
        const Bip32Index index,
        const PasswordPrompt& reason) const
        -> std::unique_ptr<opentxs::crypto::key::HD> final;
    auto AccountKey(
        const proto::HDPath& path,
        const BIP44Chain internal,
        const PasswordPrompt& reason) const
        -> std::unique_ptr<opentxs::crypto::key::HD> final;
    auto AllowedSeedTypes() const noexcept -> const SupportedSeeds& final;
    auto AllowedLanguages(const Style type) const noexcept
        -> const SupportedLanguages& final;
    auto AllowedSeedStrength(const Style type) const noexcept
        -> const SupportedStrengths& final;
    auto Bip32Root(const std::string& seedID, const PasswordPrompt& reason)
        const -> std::string final;
    auto DefaultSeed() const -> std::string final;
    auto GetHDKey(
        const std::string& seedID,
        const EcdsaCurve& curve,
        const Path& path,
        const PasswordPrompt& reason,
        const identity::KeyRole role = identity::KeyRole::Sign,
        const VersionNumber version =
            opentxs::crypto::key::EllipticCurve::DefaultVersion) const
        -> std::unique_ptr<opentxs::crypto::key::HD> final;
    auto GetOrCreateDefaultSeed(
        std::string& seedID,
        Style& type,
        Language& lang,
        Bip32Index& index,
        const Strength strength,
        const PasswordPrompt& reason) const -> OTSecret final;
    auto GetPaymentCode(
        const std::string& seedID,
        const Bip32Index nym,
        const std::uint8_t version,
        const PasswordPrompt& reason) const
        -> std::unique_ptr<opentxs::crypto::key::Secp256k1> final;
    auto GetStorageKey(const std::string& seedID, const PasswordPrompt& reason)
        const -> OTSymmetricKey final;
    auto ImportRaw(const Secret& entropy, const PasswordPrompt& reason) const
        -> std::string final;
    auto ImportSeed(
        const Secret& words,
        const Secret& passphrase,
        const Style type,
        const Language lang,
        const PasswordPrompt& reason) const -> std::string final;
    auto LongestWord(const Style type, const Language lang) const noexcept
        -> std::size_t final;
    auto NewSeed(
        const Style type,
        const Language lang,
        const Strength strength,
        const PasswordPrompt& reason) const -> std::string final;
    auto Passphrase(const std::string& seedID, const PasswordPrompt& reason)
        const -> std::string final;
    auto Seed(
        const std::string& seedID,
        Bip32Index& index,
        const PasswordPrompt& reason) const -> OTSecret final;
    auto UpdateIndex(
        const std::string& seedID,
        const Bip32Index index,
        const PasswordPrompt& reason) const -> bool final;
    auto ValidateWord(
        const Style type,
        const Language lang,
        const std::string_view word) const noexcept -> Matches final;
    auto WordCount(const Style type, const Strength strength) const noexcept
        -> std::size_t final;
    auto Words(const std::string& seedID, const PasswordPrompt& reason) const
        -> std::string final;

    HDSeed(
        const api::Factory& factory,
        const api::crypto::internal::Asymmetric& asymmetric,
        const api::crypto::Symmetric& symmetric,
        const api::storage::Storage& storage,
        const opentxs::crypto::Bip32& bip32,
        const opentxs::crypto::Bip39& bip39);

    ~HDSeed() final = default;

private:
    using KeyRoleMap = std::map<identity::KeyRole, proto::KeyRole>;
    using KeyRoleReverseMap = std::map<proto::KeyRole, identity::KeyRole>;
    using SeedMap = std::map<std::string, opentxs::crypto::Seed>;
    using SymmetricModeMap =
        std::map<opentxs::crypto::SymmetricMode, proto::SymmetricMode>;
    using SymmetricModeReverseMap =
        std::map<proto::SymmetricMode, opentxs::crypto::SymmetricMode>;

    static const KeyRoleMap keyrole_map_;
    static const KeyRoleReverseMap keyrole_reverse_map_;
    static const SymmetricModeMap symmetricmode_map_;
    static const SymmetricModeReverseMap symmetricmode_reverse_map_;

    const api::Factory& factory_;
    const api::crypto::Symmetric& symmetric_;
#if OT_CRYPTO_WITH_BIP32
    const api::crypto::internal::Asymmetric& asymmetric_;
#endif  // OT_CRYPTO_WITH_BIP32
    const api::storage::Storage& storage_;
    const opentxs::crypto::Bip32& bip32_;
    const opentxs::crypto::Bip39& bip39_;
    mutable std::mutex seed_lock_;
    mutable SeedMap seeds_;

    auto get_seed(
        const Lock& lock,
        const std::string& seedID,
        const PasswordPrompt& reason) const noexcept(false)
        -> opentxs::crypto::Seed&;
    auto new_seed(
        const Lock& lock,
        const Style type,
        const Language lang,
        const Strength strength,
        const PasswordPrompt& reason) const noexcept -> std::string;

    HDSeed() = delete;
    HDSeed(const HDSeed&) = delete;
    HDSeed(HDSeed&&) = delete;
    auto operator=(const HDSeed&) -> HDSeed& = delete;
    auto operator=(HDSeed&&) -> HDSeed& = delete;
};
}  // namespace opentxs::api::implementation
