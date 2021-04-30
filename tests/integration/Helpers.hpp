// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Settings.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/OTX.hpp"
#include "opentxs/api/server/Manager.hpp"
#include "opentxs/client/OTAPI_Exec.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"

namespace
{
enum class Widget : int {
    AccountActivityUSD,
    AccountList,
    AccountSummaryBTC,
    AccountSummaryBCH,
    AccountSummaryUSD,
    ActivitySummary,
    ContactList,
    MessagableList,
    Profile,
    PayableListBTC,
    PayableListBCH,
    ActivityThreadAlice,
    ActivityThreadBob,
    ActivityThreadIssuer,
    ContactIssuer,
    AccountSummary,
};

using WidgetCallback = std::function<bool()>;
// target counter value, callback
using WidgetCallbackData = std::tuple<int, WidgetCallback, std::promise<bool>>;
// name, counter
using WidgetData = std::tuple<Widget, int, WidgetCallbackData>;
using WidgetMap = std::map<ot::OTIdentifier, WidgetData>;
using WidgetTypeMap = std::map<Widget, ot::OTIdentifier>;
using StateMap =
    std::map<std::string, std::map<Widget, std::map<int, WidgetCallback>>>;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-member-function"
struct Server {
    const ot::api::server::Manager* api_{nullptr};
    bool init_{false};
    const ot::OTServerID id_{ot::identifier::Server::Factory()};
    const std::string password_;

    ot::OTServerContract Contract() const { return api_->Wallet().Server(id_); }

    ot::OTPasswordPrompt Reason() const
    {
        OT_ASSERT(nullptr != api_);

        return api_->Factory().PasswordPrompt(__FUNCTION__);
    }

    void init(const ot::api::server::Manager& api)
    {
        if (init_) { return; }

#if OT_CASH
        api.SetMintKeySize(OT_MINT_KEY_SIZE_TEST);
#endif
        api_ = &api;
        const_cast<ot::OTServerID&>(id_) = api.ID();

        {
            const auto section = ot::String::Factory("permissions");
            const auto key = ot::String::Factory("admin_password");
            auto value = ot::String::Factory();
            auto exists{false};
            api.Config().Check_str(section, key, value, exists);

            OT_ASSERT(exists);

            const_cast<std::string&>(password_) = value->Get();
        }

        OT_ASSERT(false == id_->empty());
        OT_ASSERT(false == password_.empty());

        init_ = true;
    }
};

struct User {
    const std::string words_;
    const std::string passphrase_;
    const std::string name_;
    const ot::api::client::Manager* api_;
    bool init_;
    std::string seed_id_;
    std::uint32_t index_;
    ot::OTNymID nym_id_;
    std::string payment_code_;

    const ot::Identifier& Account(const std::string& type) const
    {
        ot::Lock lock(lock_);

        return accounts_.at(type).get();
    }

    const ot::Identifier& Contact(const std::string& contact) const
    {
        ot::Lock lock(lock_);

        return contacts_.at(contact).get();
    }

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
    ot::OTPaymentCode PaymentCode() const
    {
        OT_ASSERT(nullptr != api_);

        return api_->Factory().PaymentCode(payment_code_);
    }
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32

    ot::OTPasswordPrompt Reason() const
    {
        OT_ASSERT(nullptr != api_);

        return api_->Factory().PasswordPrompt(__FUNCTION__);
    }

    bool SetAccount(const std::string& type, const std::string& id) const
    {
        OT_ASSERT(nullptr != api_);

        return SetAccount(type, api_->Factory().Identifier(id));
    }

    bool SetAccount(const std::string& type, const ot::Identifier& id) const
    {
        OT_ASSERT(nullptr != api_);

        ot::Lock lock(lock_);
        const auto [it, added] = accounts_.emplace(type, id);

        return added;
    }

    bool SetContact(const std::string& contact, const std::string& id) const
    {
        OT_ASSERT(nullptr != api_);

        return SetContact(contact, api_->Factory().Identifier(id));
    }

    bool SetContact(const std::string& contact, const ot::Identifier& id) const
    {
        OT_ASSERT(nullptr != api_);

        ot::Lock lock(lock_);
        const auto [it, added] = contacts_.emplace(contact, id);

        return added;
    }

    void set_introduction_server(
        const ot::api::client::Manager& api,
        const Server& server)
    {
        auto bytes = ot::Space{};
        server.Contract()->PublicContract(ot::writer(bytes));
        auto clientVersion = api.Wallet().Server(ot::reader(bytes));
        api.OTX().SetIntroductionServer(clientVersion);
    }

    void init(
        const ot::api::client::Manager& api,
        const Server& server,
        const ot::contact::ContactItemType type =
            ot::contact::ContactItemType::Individual,
        const std::uint32_t index = 0)
    {
        if (init_) { return; }

        api_ = &api;
        seed_id_ = api.Exec().Wallet_ImportSeed(words_, passphrase_);
        index_ = index;
        nym_id_ =
            api.Wallet()
                .Nym(
                    Reason(), name_, {seed_id_, static_cast<int>(index_)}, type)
                ->ID();
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
        payment_code_ =
            api.Factory()
                .PaymentCode(
                    seed_id_, index_, ot::PaymentCode::DefaultVersion, Reason())
                ->asBase58();
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1 && OT_CRYPTO_WITH_BIP32
        set_introduction_server(api, server);
        init_ = true;
    }

    User(
        const std::string words,
        const std::string name,
        const std::string passphrase = "")
        : words_(words)
        , passphrase_(passphrase)
        , name_(name)
        , api_(nullptr)
        , init_(false)
        , seed_id_()
        , index_()
        , nym_id_(ot::identifier::Nym::Factory())
        , payment_code_()
        , lock_()
        , contacts_()
        , accounts_()
    {
    }

private:
    mutable std::mutex lock_;
    mutable std::map<std::string, ot::OTIdentifier> contacts_;
    mutable std::map<std::string, ot::OTIdentifier> accounts_;

    User(const User&) = delete;
    User(User&&) = delete;
    User& operator=(const User&) = delete;
    User& operator=(User&&) = delete;
};

struct Callbacks {
    mutable std::mutex callback_lock_;
    ot::OTZMQListenCallback callback_;

    std::size_t Count() const
    {
        ot::Lock lock(map_lock_);

        return widget_map_.size();
    }

    std::future<bool> RegisterWidget(
        const ot::Lock& callbackLock,
        const Widget type,
        const ot::Identifier& id,
        int counter = 0,
        WidgetCallback callback = {})
    {
        ot::LogDetail("::Callbacks::")(__FUNCTION__)(": Name: ")(name_)(
            " ID: ")(id)
            .Flush();
        WidgetData data{};
        std::get<0>(data) = type;
        auto& [limit, cb, promise] = std::get<2>(data);
        limit = counter, cb = callback, promise = {};
        auto output = promise.get_future();
        widget_map_.emplace(id, std::move(data));
        ui_names_.emplace(type, id);

        OT_ASSERT(widget_map_.size() == ui_names_.size());

        return output;
    }

    std::future<bool> SetCallback(
        const Widget type,
        int limit,
        WidgetCallback callback)
    {
        ot::Lock lock(map_lock_);
        auto& [counter, cb, promise] =
            std::get<2>(widget_map_.at(ui_names_.at(type)));
        counter += limit;
        cb = callback;
        promise = {};

        return promise.get_future();
    }

    Callbacks(const std::string& name)
        : callback_lock_()
        , callback_(ot::network::zeromq::ListenCallback::Factory(
              [this](const auto& incoming) -> void { callback(incoming); }))
        , map_lock_()
        , name_(name)
        , widget_map_()
        , ui_names_()
    {
    }

private:
    mutable std::mutex map_lock_;
    const std::string name_;
    WidgetMap widget_map_;
    WidgetTypeMap ui_names_;

    void callback(const ot::network::zeromq::Message& incoming)
    {
        ot::Lock lock(callback_lock_);
        const auto widgetID = ot::Identifier::Factory(incoming.Body().at(0));

        ASSERT_NE("", widgetID->str().c_str());

        auto& [type, counter, callbackData] = widget_map_.at(widgetID);
        auto& [limit, callback, future] = callbackData;
        ++counter;

        if (counter >= limit) {
            if (callback) {
                future.set_value(callback());
                callback = {};
                future = {};
                limit = 0;
            } else {
                ot::LogOutput("::Callbacks::")(__FUNCTION__)(": ")(name_)(
                    " missing callback for ")(static_cast<int>(type))
                    .Flush();
            }
        } else {
            ot::LogVerbose("::Callbacks::")(__FUNCTION__)(": Skipping update ")(
                counter)(" to ")(static_cast<int>(type))
                .Flush();
        }
    }

    Callbacks() = delete;
};

struct Issuer {
    static const int expected_bailments_{3};
    static const std::string new_notary_name_;

    int bailment_counter_;
    std::promise<bool> bailment_promise_;
    std::shared_future<bool> bailment_;
    std::promise<bool> store_secret_promise_;
    std::shared_future<bool> store_secret_;

    Issuer() noexcept
        : bailment_counter_(0)
        , bailment_promise_()
        , bailment_(bailment_promise_.get_future())
        , store_secret_promise_()
        , store_secret_(store_secret_promise_.get_future())
    {
    }
};
#pragma GCC diagnostic pop

[[maybe_unused]] bool test_future(
    std::future<bool>& future,
    const unsigned int seconds = 60) noexcept
{
    const auto status =
        future.wait_until(ot::Clock::now() + std::chrono::seconds{seconds});

    if (std::future_status::ready == status) { return future.get(); }

    return false;
}

static const User alex_{
    "spike nominee miss inquiry fee nothing belt list other daughter leave "
    "valley twelve gossip paper",
    "Alex"};
static const User bob_{
    "trim thunder unveil reduce crop cradle zone inquiry anchor skate property "
    "fringe obey butter text tank drama palm guilt pudding laundry stay axis "
    "prosper",
    "Bob"};
static const User issuer_{
    "abandon abandon abandon abandon abandon abandon abandon abandon abandon "
    "abandon abandon about",
    "Issuer"};
static const User chris_{
    "abandon abandon abandon abandon abandon abandon abandon abandon abandon "
    "abandon abandon prosper",
    "Chris"};
static const Server server_1_{};
}  // namespace
