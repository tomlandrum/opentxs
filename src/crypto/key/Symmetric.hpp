// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "internal/api/Api.hpp"
#include "internal/crypto/key/Factory.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/crypto/SymmetricKeyType.hpp"
#include "opentxs/crypto/SymmetricMode.hpp"
#include "opentxs/protobuf/Ciphertext.pb.h"

namespace opentxs
{
namespace crypto
{
class SymmetricProvider;
}  // namespace crypto

class Data;
class Factory;
class PasswordPrompt;
class String;
}  // namespace opentxs

namespace opentxs::crypto::key::implementation
{
class Symmetric final : virtual public key::Symmetric
{
public:
    operator bool() const final { return true; }

    auto api() const -> const api::Core& final { return api_; }

    auto Decrypt(
        const proto::Ciphertext& ciphertext,
        const PasswordPrompt& reason,
        const AllocateOutput plaintext) const -> bool final;
    auto Encrypt(
        const ReadView plaintext,
        const PasswordPrompt& reason,
        proto::Ciphertext& ciphertext,
        const bool attachKey = true,
        const opentxs::crypto::SymmetricMode mode =
            opentxs::crypto::SymmetricMode::Error,
        const ReadView iv = {}) const -> bool final;
    auto ID(const PasswordPrompt& reason) const -> OTIdentifier final;
    auto RawKey(const PasswordPrompt& reason, Secret& output) const
        -> bool final;
    auto Serialize(proto::SymmetricKey& output) const -> bool final;
    auto Unlock(const PasswordPrompt& reason) const -> bool final;

    auto ChangePassword(const PasswordPrompt& reason, const Secret& newPassword)
        -> bool final;

    Symmetric(
        const api::internal::Core& api,
        const crypto::SymmetricProvider& engine);
    Symmetric(
        const api::internal::Core& api,
        const crypto::SymmetricProvider& engine,
        const proto::SymmetricKey serialized);
    Symmetric(
        const api::internal::Core& api,
        const crypto::SymmetricProvider& engine,
        const Secret& seed,
        const std::string& salt,
        const std::size_t size,
        const std::uint64_t operations,
        const std::uint64_t difficulty,
        const crypto::SymmetricKeyType type = crypto::SymmetricKeyType::Argon2);

    ~Symmetric() final = default;

private:
    using SymmetricKeyTypeMap =
        std::map<crypto::SymmetricKeyType, proto::SymmetricKeyType>;
    using SymmetricKeyTypeReverseMap =
        std::map<proto::SymmetricKeyType, crypto::SymmetricKeyType>;
    using SymmetricModeMap =
        std::map<opentxs::crypto::SymmetricMode, proto::SymmetricMode>;
    using SymmetricModeReverseMap =
        std::map<proto::SymmetricMode, opentxs::crypto::SymmetricMode>;

    static const SymmetricKeyTypeMap symmetrickeytype_map_;
    static const SymmetricKeyTypeReverseMap symmetrickeytype_reverse_map_;
    static const SymmetricModeMap symmetricmode_map_;
    static const SymmetricModeReverseMap symmetricmode_reverse_map_;

    friend std::unique_ptr<crypto::key::Symmetric> opentxs::factory::
        SymmetricKey(
            const api::internal::Core&,
            const crypto::SymmetricProvider&,
            const opentxs::PasswordPrompt&,
            const crypto::SymmetricMode) noexcept;
    friend std::unique_ptr<crypto::key::Symmetric> opentxs::factory::
        SymmetricKey(
            const api::internal::Core&,
            const crypto::SymmetricProvider&,
            const Secret&,
            const std::uint64_t,
            const std::uint64_t,
            const std::size_t,
            const crypto::SymmetricKeyType) noexcept;
    friend std::unique_ptr<crypto::key::Symmetric> opentxs::factory::
        SymmetricKey(
            const api::internal::Core&,
            const crypto::SymmetricProvider&,
            const Secret&,
            const opentxs::PasswordPrompt&) noexcept;
    friend key::Symmetric;

    const api::internal::Core& api_;
    /// The library providing the underlying crypto algorithms
    const crypto::SymmetricProvider& engine_;
    const VersionNumber version_{0};
    const crypto::SymmetricKeyType type_{crypto::SymmetricKeyType::Error};
    /// Size of the plaintext key in bytes;
    std::size_t key_size_{0};
    std::uint64_t operations_{0};
    std::uint64_t difficulty_{0};
    mutable std::unique_ptr<std::string> salt_;
    /// The unencrypted, fully-derived version of the key which is provided to
    /// encryption functions.
    mutable std::optional<OTSecret> plaintext_key_;
    /// The encrypted form of the plaintext key
    mutable std::unique_ptr<proto::Ciphertext> encrypted_key_;
    mutable std::mutex lock_;

    auto clone() const -> Symmetric* final;

    static auto Allocate(const std::size_t size, String& container) -> bool;
    static auto Allocate(const std::size_t size, Data& container) -> bool;
    static auto Allocate(
        const api::Core& api,
        const std::size_t size,
        std::string& container,
        const bool random) -> bool;

    auto allocate(const Lock& lock, const std::size_t size, Secret& container)
        const -> bool;
    auto decrypt(
        const Lock& lock,
        const proto::Ciphertext& input,
        const PasswordPrompt& reason,
        std::uint8_t* plaintext) const -> bool;
    auto encrypt(
        const Lock& lock,
        const std::uint8_t* input,
        const std::size_t inputSize,
        const std::uint8_t* iv,
        const std::size_t ivSize,
        const opentxs::crypto::SymmetricMode mode,
        const PasswordPrompt& reason,
        proto::Ciphertext& ciphertext,
        const bool text = false) const -> bool;
    auto encrypt_key(
        const Lock& lock,
        const Secret& plaintextKey,
        const PasswordPrompt& reason,
        const crypto::SymmetricKeyType type =
            crypto::SymmetricKeyType::Argon2) const -> bool;
    auto get_encrypted(const Lock& lock) const
        -> std::unique_ptr<proto::Ciphertext>&;
    auto get_password(
        const Lock& lock,
        const PasswordPrompt& keyPassword,
        Secret& password) const -> bool;
    auto get_plaintext(const Lock& lock) const -> std::optional<OTSecret>&;
    auto get_salt(const Lock& lock) const -> std::unique_ptr<std::string>&;
    auto serialize(const Lock& lock, proto::SymmetricKey& output) const -> bool;
    auto unlock(const Lock& lock, const PasswordPrompt& reason) const -> bool;

    Symmetric(
        const api::internal::Core& api,
        const crypto::SymmetricProvider& engine,
        const VersionNumber version,
        const crypto::SymmetricKeyType type,
        const std::size_t keySize,
        std::string* salt,
        const std::uint64_t operations,
        const std::uint64_t difficulty,
        std::optional<OTSecret> plaintextKey,
        proto::Ciphertext* encryptedKey) noexcept(false);
    Symmetric() = delete;
    Symmetric(const Symmetric&);
    auto operator=(const Symmetric&) -> Symmetric& = delete;
};
}  // namespace opentxs::crypto::key::implementation
