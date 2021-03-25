// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "api/Core.hpp"    // IWYU pragma: associated

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>

#include "2_Factory.hpp"
#include "api/Scheduler.hpp"
#include "api/StorageParent.hpp"
#include "api/ZMQ.hpp"
#include "internal/api/Factory.hpp"
#include "internal/api/storage/Storage.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/HDSeed.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"
#include "opentxs/api/network/Dht.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/crypto/OTCaller.hpp"
#include "opentxs/crypto/key/Symmetric.hpp"
#include "opentxs/protobuf/Ciphertext.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "util/ScopeGuard.hpp"

//#define OT_METHOD "opentxs::api::implementation::Core::"

namespace
{
opentxs::OTCaller* external_password_callback_{nullptr};

extern "C" auto internal_password_cb(
    char* output,
    std::int32_t size,
    std::int32_t rwflag,
    void* userdata) -> std::int32_t
{
    OT_ASSERT(nullptr != userdata);
    OT_ASSERT(nullptr != external_password_callback_);

    const bool askTwice = (1 == rwflag);
    const auto& reason = *static_cast<opentxs::PasswordPrompt*>(userdata);
    const auto& api = opentxs::api::implementation::Core::get_api(reason);
    opentxs::Lock lock(api.Lock());
    auto secret = api.Factory().Secret(0);

    if (false == api.GetSecret(lock, secret, reason, askTwice)) {
        opentxs::LogOutput(__FUNCTION__)(": Callback error").Flush();

        return 0;
    }

    if (static_cast<std::uint64_t>(secret->size()) >
        static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) {
        opentxs::LogOutput(__FUNCTION__)(": Secret too big").Flush();

        return 0;
    }

    const auto len = std::min(static_cast<std::int32_t>(secret->size()), size);

    if (len <= 0) {
        opentxs::LogOutput(__FUNCTION__)(": Callback error").Flush();

        return 0;
    }

    std::memcpy(output, secret->data(), len);

    return len;
}
}  // namespace

namespace opentxs::api::implementation
{
Core::Core(
    const api::internal::Context& parent,
    Flag& running,
    const ArgList& args,
    const api::Crypto& crypto,
    const api::Settings& config,
    const opentxs::network::zeromq::Context& zmq,
    const std::string& dataFolder,
    const int instance,
    const bool dhtDefault,
    std::unique_ptr<api::internal::Factory> factory)
    : ZMQ(zmq, instance)
    , Scheduler(parent, running)
    , StorageParent(running, args, crypto, config, parent.Legacy(), dataFolder)
    , factory_p_(std::move(factory))
    , factory_(*factory_p_)
    , asymmetric_(factory_.Asymmetric())
    , symmetric_(factory_.Symmetric())
    , seeds_(factory::HDSeed(
          factory_,
          asymmetric_,
          symmetric_,
          *storage_,
          crypto_.BIP32(),
          crypto_.BIP39()))
    , wallet_(nullptr)
    , dht_(opentxs::Factory::Dht(
          dhtDefault,
          *this,
          nym_publish_interval_,
          nym_refresh_interval_,
          server_publish_interval_,
          server_refresh_interval_,
          unit_publish_interval_,
          unit_refresh_interval_))
    , encrypted_secret_()
    , master_key_lock_()
    , master_secret_()
    , master_key_(make_master_key(
          parent,
          factory_,
          encrypted_secret_,
          master_secret_,
          symmetric_,
          *storage_))
    , password_timeout_()
    , password_duration_(-1)
    , last_activity_()
    , timeout_thread_running_(false)
{
    OT_ASSERT(seeds_);
    OT_ASSERT(dht_);

    if (master_secret_) {
        opentxs::Lock lock(master_key_lock_);
        bump_password_timer(lock);
    }
}

void Core::bump_password_timer(const opentxs::Lock& lock) const
{
    last_activity_ = Clock::now();
    const auto running = timeout_thread_running_.exchange(true);

    if (running) { return; }

    if (password_timeout_.joinable()) { password_timeout_.join(); }

    password_timeout_ = std::thread{&Core::password_timeout, this};
}

void Core::cleanup()
{
    dht_.reset();
    wallet_.reset();
    seeds_.reset();
    factory_p_.reset();

    if (password_timeout_.joinable()) { password_timeout_.join(); }
}

auto Core::DHT() const -> const api::network::Dht&
{
    OT_ASSERT(dht_)

    return *dht_;
}

auto Core::get_api(const PasswordPrompt& reason) noexcept
    -> const api::internal::Core&
{
    return reason.api_;
}

INTERNAL_PASSWORD_CALLBACK* Core::GetInternalPasswordCallback() const
{
    return &internal_password_cb;
}

auto Core::GetSecret(
    const opentxs::Lock& lock,
    Secret& secret,
    const PasswordPrompt& reason,
    const bool twice,
    const std::string& key) const -> bool
{
    bump_password_timer(lock);

    if (master_secret_.has_value()) {
        secret.Assign(master_secret_.value());

        return true;
    }

    auto success{false};
    auto postcondition = ScopeGuard{[this, &success] {
        if (false == success) { master_secret_ = {}; }
    }};

    master_secret_ = factory_.Secret(0);

    OT_ASSERT(master_secret_.has_value());

    auto& callback = *external_password_callback_;
    auto masterPassword = factory_.Secret(256);
    const std::string password_key{key.empty() ? parent_.ProfileId() : key};

    if (twice) {
        callback.AskTwice(reason, masterPassword, password_key);
    } else {
        callback.AskOnce(reason, masterPassword, password_key);
    }

    auto prompt = factory_.PasswordPrompt(reason.GetDisplayString());
    prompt->SetPassword(masterPassword);

    if (false == master_key_->Unlock(prompt)) {
        opentxs::LogOutput(__FUNCTION__)(": Failed to unlock master key")
            .Flush();

        return success;
    }

    const auto decrypted = master_key_->Decrypt(
        encrypted_secret_,
        prompt,
        master_secret_.value()->WriteInto(Secret::Mode::Mem));

    if (false == decrypted) {
        opentxs::LogOutput(__FUNCTION__)(": Failed to decrypt master secret")
            .Flush();

        return success;
    }

    secret.Assign(master_secret_.value());
    success = true;

    return success;
}

auto Core::make_master_key(
    const api::internal::Context& parent,
    const api::Factory& factory,
    proto::Ciphertext& encrypted_secret,
    std::optional<OTSecret>& master_secret,
    const api::crypto::Symmetric& symmetric,
    const api::storage::Storage& storage) -> OTSymmetricKey
{
    auto& caller = parent.GetPasswordCaller();
    external_password_callback_ = &caller;

    OT_ASSERT(nullptr != external_password_callback_);

    auto& encrypted = encrypted_secret;
    auto existing = std::shared_ptr<proto::Ciphertext>{};
    const auto have = storage.Load(existing, true);

    if (have) {
        OT_ASSERT(existing);

        encrypted = *existing;

        return symmetric.Key(
            existing->key(), opentxs::crypto::SymmetricMode::ChaCha20Poly1305);
    }

    master_secret = factory.Secret(0);

    OT_ASSERT(master_secret.has_value());

    master_secret.value()->Randomize(32);

    auto reason = factory.PasswordPrompt("Choose a master password");
    auto masterPassword = factory.Secret(0);
    caller.AskTwice(reason, masterPassword, parent.ProfileId());
    reason->SetPassword(masterPassword);
    auto output =
        symmetric.Key(reason, opentxs::crypto::SymmetricMode::ChaCha20Poly1305);
    auto saved = output->Encrypt(
        master_secret.value()->Bytes(),
        reason,
        encrypted,
        true,
        opentxs::crypto::SymmetricMode::ChaCha20Poly1305);

    OT_ASSERT(saved);

    saved = storage.Store(encrypted);

    OT_ASSERT(saved);

    return output;
}

auto Core::MasterKey(const opentxs::Lock& lock) const
    -> const opentxs::crypto::key::Symmetric&
{
    return master_key_;
}

auto Core::Seeds() const -> const api::HDSeed&
{
    OT_ASSERT(seeds_);

    return *seeds_;
}

void Core::SetMasterKeyTimeout(const std::chrono::seconds& timeout) const
{
    opentxs::Lock lock(master_key_lock_);
    password_duration_ = timeout;
}

auto Core::Storage() const -> const api::storage::Storage&
{
    OT_ASSERT(storage_)

    return *storage_;
}

void Core::storage_gc_hook()
{
    if (storage_) { storage_->RunGC(); }
}

void Core::password_timeout() const
{
    struct Cleanup {
        std::atomic<bool>& running_;

        Cleanup(std::atomic<bool>& running)
            : running_(running)
        {
            running_.store(true);
        }

        ~Cleanup() { running_.store(false); }
    };

    opentxs::Lock lock(master_key_lock_, std::defer_lock);
    Cleanup cleanup(timeout_thread_running_);

    while (running_) {
        lock.lock();

        // Negative durations means never time out
        if (0 > password_duration_.count()) { return; }

        const auto now = Clock::now();
        const auto interval = now - last_activity_;

        if (interval > password_duration_) {
            master_secret_.reset();

            return;
        }

        lock.unlock();
        Sleep(std::chrono::milliseconds(250));
    }
}

auto Core::Wallet() const -> const api::Wallet&
{
    OT_ASSERT(wallet_);

    return *wallet_;
}

Core::~Core() { cleanup(); }
}  // namespace opentxs::api::implementation
