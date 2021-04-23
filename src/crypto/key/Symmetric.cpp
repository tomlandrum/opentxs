// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"              // IWYU pragma: associated
#include "1_Internal.hpp"            // IWYU pragma: associated
#include "crypto/key/Symmetric.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "crypto/key/SymmetricNull.hpp"
#include "internal/api/Api.hpp"
#include "internal/crypto/key/Factory.hpp"
#include "internal/crypto/key/Key.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/crypto/library/SymmetricProvider.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/Ciphertext.pb.h"
#include "opentxs/protobuf/verify/SymmetricKey.hpp"

template class opentxs::Pimpl<opentxs::crypto::key::Symmetric>;

#define OT_SYMMETRIC_KEY_DEFAULT_OPERATIONS 3
#define OT_SYMMETRIC_KEY_DEFAULT_DIFFICULTY 8388608

#define OT_METHOD "opentxs::crypto::key::implementation::Symmetric::"

namespace opentxs::factory
{
auto SymmetricKey() noexcept -> std::unique_ptr<crypto::key::Symmetric>
{
    return std::make_unique<crypto::key::implementation::SymmetricNull>();
}

using ReturnType = crypto::key::implementation::Symmetric;

auto SymmetricKey(
    const api::internal::Core& api,
    const crypto::SymmetricProvider& engine,
    const opentxs::PasswordPrompt& reason,
    const opentxs::crypto::key::symmetric::Algorithm mode) noexcept
    -> std::unique_ptr<crypto::key::Symmetric>
{
    auto output = std::make_unique<ReturnType>(api, engine);

    if (false == bool(output)) { return nullptr; }

    const auto realMode{
        mode == opentxs::crypto::key::symmetric::Algorithm::Error
            ? engine.DefaultMode()
            : mode};
    Lock lock(output->lock_);
    const auto size = output->engine_.KeySize(realMode);
    output->key_size_ = size;
    output->plaintext_key_ = api.Factory().Secret(0);

    OT_ASSERT(output->plaintext_key_.has_value());

    auto& key = output->plaintext_key_.value();

    if (false == output->allocate(lock, size, key)) { return nullptr; }

    output->encrypt_key(lock, key, reason);

    return std::move(output);
}

auto SymmetricKey(
    const api::internal::Core& api,
    const crypto::SymmetricProvider& engine,
    const proto::SymmetricKey serialized) noexcept
    -> std::unique_ptr<crypto::key::Symmetric>
{
    if (proto::Validate(serialized, VERBOSE)) {

        return std::make_unique<ReturnType>(api, engine, serialized);
    }

    return {};
}

auto SymmetricKey(
    const api::internal::Core& api,
    const crypto::SymmetricProvider& engine,
    const Secret& seed,
    const std::uint64_t operations,
    const std::uint64_t difficulty,
    const std::size_t size,
    const crypto::key::symmetric::Source type) noexcept
    -> std::unique_ptr<crypto::key::Symmetric>
{
    auto salt = std::string{};
    ReturnType::Allocate(api, engine.SaltSize(type), salt, false);
    const std::uint64_t ops =
        (0 == operations) ? OT_SYMMETRIC_KEY_DEFAULT_OPERATIONS : operations;
    const std::uint64_t mem =
        (0 == difficulty) ? OT_SYMMETRIC_KEY_DEFAULT_DIFFICULTY : difficulty;

    return std::make_unique<ReturnType>(
        api, engine, seed, salt, size, ops, mem, type);
}

auto SymmetricKey(
    const api::internal::Core& api,
    const crypto::SymmetricProvider& engine,
    const Secret& raw,
    const opentxs::PasswordPrompt& reason) noexcept
    -> std::unique_ptr<crypto::key::Symmetric>
{
    auto output = std::make_unique<ReturnType>(api, engine);

    if (!output) { return {}; }

    Lock lock(output->lock_);
    output->encrypt_key(lock, raw, reason);
    output->key_size_ = raw.size();

    return std::move(output);
}
}  // namespace opentxs::factory

namespace opentxs::crypto::key
{
auto Symmetric::Factory() -> OTSymmetricKey
{
    return OTSymmetricKey{new implementation::SymmetricNull};
}
}  // namespace opentxs::crypto::key

namespace opentxs::crypto::key::implementation
{
Symmetric::Symmetric(
    const api::internal::Core& api,
    const crypto::SymmetricProvider& engine,
    const VersionNumber version,
    const crypto::key::symmetric::Source type,
    const std::size_t keySize,
    std::string* salt,
    const std::uint64_t operations,
    const std::uint64_t difficulty,
    std::optional<OTSecret> plaintextKey,
    proto::Ciphertext* encryptedKey)
    : key::Symmetric()
    , api_(api)
    , engine_(engine)
    , version_(version)
    , type_(type)
    , key_size_(keySize)
    , operations_(operations)
    , difficulty_(difficulty)
    , salt_(salt)
    , plaintext_key_(plaintextKey)
    , encrypted_key_(encryptedKey)
    , lock_()
{
}

Symmetric::Symmetric(
    const api::internal::Core& api,
    const crypto::SymmetricProvider& engine)
    : Symmetric(
          api,
          engine,
          1,
          crypto::key::symmetric::Source::Raw,
          0,
          nullptr,
          0,
          0,
          {},
          nullptr)
{
}

Symmetric::Symmetric(
    const api::internal::Core& api,
    const crypto::SymmetricProvider& engine,
    const proto::SymmetricKey serialized)
    : Symmetric(
          api,
          engine,
          serialized.version(),
          opentxs::crypto::key::internal::translate(serialized.type()),
          serialized.size(),
          new std::string(serialized.salt()),
          serialized.operations(),
          serialized.difficulty(),
          {},
          new proto::Ciphertext(serialized.key()))
{
}

Symmetric::Symmetric(
    const api::internal::Core& api,
    const crypto::SymmetricProvider& engine,
    const Secret& seed,
    const std::string& salt,
    const std::size_t size,
    const std::uint64_t operations,
    const std::uint64_t difficulty,
    const crypto::key::symmetric::Source type)
    : Symmetric(
          api,
          engine,
          1,
          type,
          size,
          new std::string(salt),
          operations,
          difficulty,
          api.Factory().Secret(0),
          nullptr)
{
    OT_ASSERT(salt_);
    OT_ASSERT(plaintext_key_);
    OT_ASSERT(0 != operations);
    OT_ASSERT(0 != difficulty);
    OT_ASSERT(0 != size);

    Lock lock(lock_);
    auto& plain = get_plaintext(lock);

    OT_ASSERT(plain.has_value());

    auto& salt_m = get_salt(lock);
    allocate(lock, key_size_, *plain);
    const auto bytes = seed.Bytes();

    const bool derived = engine.Derive(
        reinterpret_cast<const std::uint8_t*>(bytes.data()),
        bytes.size(),
        reinterpret_cast<const std::uint8_t*>(salt_m->data()),
        salt_m->size(),
        operations_,
        difficulty_,
        type_,
        reinterpret_cast<std::uint8_t*>(plain.value()->data()),
        plain.value()->size());

    OT_ASSERT(derived);
}

Symmetric::Symmetric(const Symmetric& rhs)
    : Symmetric(
          rhs.api_,
          rhs.engine_,
          rhs.version_,
          rhs.type_,
          rhs.key_size_,
          nullptr,
          rhs.operations_,
          rhs.difficulty_,
          {},
          nullptr)
{
    if (rhs.salt_) { salt_.reset(new std::string(*rhs.salt_)); }

    if (rhs.plaintext_key_.has_value()) {
        plaintext_key_ = rhs.plaintext_key_.value();
    }

    if (rhs.encrypted_key_) {
        encrypted_key_.reset(new proto::Ciphertext(*rhs.encrypted_key_));
    }
}

auto Symmetric::Allocate(const std::size_t size, Data& container) -> bool
{
    container.SetSize(size);

    return (size == container.size());
}

auto Symmetric::Allocate(const std::size_t size, String& container) -> bool
{
    if (std::numeric_limits<std::uint32_t>::max() < size) { return false; }

    auto blank = std::vector<char>{};
    blank.assign(size, 0x7f);

    OT_ASSERT(blank.size() == size);

    container.Set(blank.data(), static_cast<std::uint32_t>(blank.size()));

    return (size == container.GetLength());
}

auto Symmetric::Allocate(
    const api::Core& api,
    const std::size_t size,
    std::string& container,
    const bool random) -> bool
{
    container.resize(size, 0x0);

    if (random) {
        auto secret = api.Factory().Secret(0);
        secret->Randomize(size);
        container.assign(secret->Bytes());
    }

    return (size == container.size());
}

auto Symmetric::allocate(
    const Lock& lock,
    const std::size_t size,
    Secret& container) const -> bool
{
    return size == container.Randomize(size);
}

auto Symmetric::ChangePassword(
    const opentxs::PasswordPrompt& reason,
    const Secret& newPassword) -> bool
{
    Lock lock(lock_);
    auto& plain = get_plaintext(lock);

    OT_ASSERT(plain.has_value());

    if (unlock(lock, reason)) {
        OTPasswordPrompt copy{reason};
        copy->SetPassword(newPassword);

        return encrypt_key(lock, plain.value(), copy);
    }

    LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to unlock master key.")
        .Flush();

    return false;
}

auto Symmetric::clone() const -> Symmetric* { return new Symmetric(*this); }

auto Symmetric::decrypt(
    const Lock& lock,
    const proto::Ciphertext& input,
    const opentxs::PasswordPrompt& reason,
    std::uint8_t* plaintext) const -> bool
{
    auto& plain = get_plaintext(lock);

    if (false == plain.has_value()) {
        if (false == unlock(lock, reason)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to unlock master key.")
                .Flush();

            return false;
        }
    }

    OT_ASSERT(plain.has_value());

    const bool output = engine_.Decrypt(
        input,
        reinterpret_cast<const std::uint8_t*>(plain.value()->data()),
        plain.value()->size(),
        plaintext);

    if (false == output) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to decrypt key.").Flush();

        return false;
    }

    return output;
}

auto Symmetric::Decrypt(
    const proto::Ciphertext& ciphertext,
    const opentxs::PasswordPrompt& reason,
    const AllocateOutput plaintext) const -> bool
{
    Lock lock(lock_);

    if (false == bool(plaintext)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Missing output allocator")
            .Flush();

        return false;
    }

    auto output = plaintext(ciphertext.data().size());

    if (false == output.valid(ciphertext.data().size())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to allocate space for decryption.")
            .Flush();

        return false;
    }

    return decrypt(lock, ciphertext, reason, output.as<std::uint8_t>());
}

auto Symmetric::DecryptFromBytes(
    const Space& ciphertext,
    const opentxs::PasswordPrompt& reason,
    const AllocateOutput plaintext) const -> bool
{
    return Decrypt(
        proto::Factory<proto::Ciphertext>(ciphertext.data(), ciphertext.size()),
        reason,
        plaintext);
}

auto Symmetric::encrypt(
    const Lock& lock,
    const std::uint8_t* input,
    const std::size_t inputSize,
    const std::uint8_t* iv,
    const std::size_t ivSize,
    const opentxs::crypto::key::symmetric::Algorithm mode,
    const opentxs::PasswordPrompt& reason,
    proto::Ciphertext& ciphertext,
    const bool text) const -> bool
{
    if (nullptr == input) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Null input.").Flush();

        return false;
    }

    auto& plain = get_plaintext(lock);

    if (false == plain.has_value()) {
        if (false == unlock(lock, reason)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to unlock master key.")
                .Flush();

            return false;
        }
    }

    OT_ASSERT(plain.has_value());

    ciphertext.set_version(1);

    if (opentxs::crypto::key::symmetric::Algorithm::Error == mode) {
        ciphertext.set_mode(
            opentxs::crypto::key::internal::translate(engine_.DefaultMode()));
    } else {
        ciphertext.set_mode(opentxs::crypto::key::internal::translate(mode));
    }

    if ((0u == ivSize) || (nullptr == iv)) {
        const auto random = [&] {
            auto out = api_.Factory().Secret(0);
            const auto size = engine_.IvSize(
                opentxs::crypto::key::internal::translate(ciphertext.mode()));
            out->Randomize(size);

            OT_ASSERT(out->size() == size);

            return out;
        }();

        ciphertext.set_iv(random->data(), random->size());
    } else {
        ciphertext.set_iv(iv, ivSize);
    }

    ciphertext.set_text(text);

    OT_ASSERT(nullptr != plain.value()->data());

    return engine_.Encrypt(
        input,
        inputSize,
        reinterpret_cast<const std::uint8_t*>(plain.value()->data()),
        plain.value()->size(),
        ciphertext);
}

auto Symmetric::Encrypt(
    const ReadView plaintext,
    const opentxs::PasswordPrompt& reason,
    proto::Ciphertext& ciphertext,
    const bool attachKey,
    const opentxs::crypto::key::symmetric::Algorithm mode,
    const ReadView iv) const -> bool
{
    Lock lock(lock_);
    auto success = encrypt(
        lock,
        reinterpret_cast<const std::uint8_t*>(plaintext.data()),
        plaintext.size(),
        reinterpret_cast<const std::uint8_t*>(iv.data()),
        iv.size(),
        mode,
        reason,
        ciphertext,
        true);

    if (success && attachKey) {
        success &= serialize(lock, *ciphertext.mutable_key());
    }

    return success;
}

auto Symmetric::EncryptToBytes(
    const ReadView plaintext,
    const opentxs::PasswordPrompt& reason,
    AllocateOutput ciphertext,
    const bool attachKey,
    const opentxs::crypto::key::symmetric::Algorithm mode,
    const ReadView iv) const -> bool
{
    auto serialized = proto::Ciphertext{};

    if (false == Encrypt(plaintext, reason, serialized, attachKey, mode, iv)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to encrypt data.").Flush();

        return false;
    }

    auto view = ciphertext(serialized.ByteSizeLong());
    if (false == serialized.SerializeToArray(
                     view.data(), static_cast<int>(view.size()))) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to serialize encrypted data.")
            .Flush();

        return false;
    }

    return true;
}

auto Symmetric::encrypt_key(
    const Lock& lock,
    const Secret& plaintextKey,
    const opentxs::PasswordPrompt& reason,
    const crypto::key::symmetric::Source type) const -> bool
{
    auto& encrypted = get_encrypted(lock);
    encrypted.reset(new proto::Ciphertext);

    OT_ASSERT(encrypted);

    encrypted->set_mode(
        opentxs::crypto::key::internal::translate(engine_.DefaultMode()));
    auto blankIV = api_.Factory().Secret(0);
    blankIV->Randomize(engine_.IvSize(
        opentxs::crypto::key::internal::translate(encrypted->mode())));
    encrypted->set_iv(blankIV->data(), blankIV->size());
    encrypted->set_text(false);
    auto key = api_.Factory().Secret(0);
    get_password(lock, reason, key);
    const auto saltSize = engine_.SaltSize(type);
    auto& salt = get_salt(lock);

    if (!salt) { salt.reset(new std::string); }

    OT_ASSERT(salt);

    if (salt->size() != saltSize) {
        if (!Allocate(api_, saltSize, *salt, true)) { return false; }
    }

    auto secondaryKey = Symmetric{
        api_,
        engine_,
        key,
        *salt,
        engine_.KeySize(
            opentxs::crypto::key::internal::translate(encrypted->mode())),
        OT_SYMMETRIC_KEY_DEFAULT_OPERATIONS,
        OT_SYMMETRIC_KEY_DEFAULT_DIFFICULTY};

    OT_ASSERT(secondaryKey.plaintext_key_.has_value());

    return engine_.Encrypt(
        reinterpret_cast<const std::uint8_t*>(plaintextKey.data()),
        plaintextKey.size(),
        reinterpret_cast<const std::uint8_t*>(
            secondaryKey.plaintext_key_.value()->data()),
        secondaryKey.plaintext_key_.value()->size(),
        *encrypted);
}

auto Symmetric::get_encrypted(const Lock& lock) const
    -> std::unique_ptr<proto::Ciphertext>&
{
    return encrypted_key_;
}

auto Symmetric::get_password(
    const Lock& lock,
    const opentxs::PasswordPrompt& reason,
    Secret& password) const -> bool
{
    if (false == reason.Password().empty()) {
        password.Assign(reason.Password());

        return true;
    } else {
        auto buffer = api_.Factory().Secret(0);
        buffer->Randomize(1024);
        auto* callback = api_.GetInternalPasswordCallback();

        OT_ASSERT(nullptr != callback);

        auto bytes = buffer->Bytes();

        OT_ASSERT(std::numeric_limits<int>::max() >= bytes.size());

        const auto length = (*callback)(
            const_cast<char*>(bytes.data()),
            static_cast<int>(bytes.size()),
            0,
            const_cast<PasswordPrompt*>(&reason));
        bool result = false;

        if (0 < length) {
            password.Assign(bytes.data(), static_cast<std::size_t>(length));
            result = true;
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to obtain master password")
                .Flush();
        }

        return result;
    }
}

auto Symmetric::get_plaintext(const Lock& lock) const
    -> std::optional<OTSecret>&
{
    return plaintext_key_;
}

auto Symmetric::get_salt(const Lock& lock) const
    -> std::unique_ptr<std::string>&
{
    return salt_;
}

auto Symmetric::ID(const opentxs::PasswordPrompt& reason) const -> OTIdentifier
{
    Lock lock(lock_);
    auto& plain = get_plaintext(lock);

    if (false == plain.has_value()) {
        if (false == unlock(lock, reason)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to unlock master key.")
                .Flush();

            return api_.Factory().Identifier();
        }
    }

    OT_ASSERT(plain.has_value());

    return api_.Factory().Identifier(plain.value()->Bytes());
}

auto Symmetric::RawKey(const opentxs::PasswordPrompt& reason, Secret& output)
    const -> bool
{
    Lock lock(lock_);
    auto& plain = get_plaintext(lock);

    if (false == plain.has_value()) {
        if (false == unlock(lock, reason)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to unlock master key.")
                .Flush();

            return false;
        }
    }

    OT_ASSERT(plain.has_value());

    output.Assign(plain.value());

    return true;
}

auto Symmetric::serialize(const Lock& lock, proto::SymmetricKey& output) const
    -> bool
{
    auto& encrypted = get_encrypted(lock);
    auto& salt = get_salt(lock);

    if (!encrypted) { return false; }

    OT_ASSERT(std::numeric_limits<std::uint32_t>::max() >= key_size_);

    output.set_version(version_);
    output.set_type(opentxs::crypto::key::internal::translate(type_));
    output.set_size(static_cast<std::uint32_t>(key_size_));
    *output.mutable_key() = *encrypted;

    if (salt) { output.set_salt(*salt); }

    output.set_operations(operations_);
    output.set_difficulty(difficulty_);

    return proto::Validate(output, VERBOSE);
}

auto Symmetric::Serialize(proto::SymmetricKey& output) const -> bool
{
    Lock lock(lock_);

    return serialize(lock, output);
}

auto Symmetric::unlock(const Lock& lock, const opentxs::PasswordPrompt& reason)
    const -> bool
{
    auto& encrypted = get_encrypted(lock);
    auto& plain = get_plaintext(lock);

    if (false == bool(encrypted)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Master key not loaded.").Flush();

        return false;
    }

    if (plain.has_value()) {
        if (0 < plain.value()->Bytes().size()) {
            LogDetail(OT_METHOD)(__FUNCTION__)(": Already unlocked").Flush();

            return true;
        }
    } else {
        plain = api_.Factory().Secret(0);

        OT_ASSERT(plain.has_value());

        // Allocate space for plaintext (same size as ciphertext)
        if (!allocate(lock, encrypted->data().size(), plain.value())) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Unable to allocate space for plaintext master key.")
                .Flush();

            return false;
        }
    }

    OT_ASSERT(plain.has_value());

    auto& salt = get_salt(lock);
    auto key = api_.Factory().Secret(0);

    if (false == get_password(lock, reason, key)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Unable to obtain master password.")
            .Flush();

        return false;
    }

    auto secondaryKey = Symmetric{
        api_,
        engine_,
        key,
        *salt,
        engine_.KeySize(
            opentxs::crypto::key::internal::translate(encrypted->mode())),
        OT_SYMMETRIC_KEY_DEFAULT_OPERATIONS,
        OT_SYMMETRIC_KEY_DEFAULT_DIFFICULTY};

    OT_ASSERT(secondaryKey.plaintext_key_.has_value());

    const auto output = engine_.Decrypt(
        *encrypted,
        reinterpret_cast<const std::uint8_t*>(
            secondaryKey.plaintext_key_.value()->data()),
        secondaryKey.plaintext_key_.value()->size(),
        reinterpret_cast<std::uint8_t*>(plain.value()->data()));

    if (output) {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Key unlocked").Flush();
    } else {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Failed to unlock key").Flush();
    }

    return output;
}

auto Symmetric::Unlock(const opentxs::PasswordPrompt& reason) const -> bool
{
    Lock lock(lock_);

    return unlock(lock, reason);
}
}  // namespace opentxs::crypto::key::implementation
