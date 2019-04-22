// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "opentxs/opentxs.hpp"

#include <gtest/gtest.h>

#include <mutex>

using namespace opentxs;

#define ALICE "Alice"
#define BOB "Bob"
#define ISSUER "Issuer"
#define ACCOUNT_ACTIVITY_USD "ACCOUNT_ACTIVITY_USD"
#define ACCOUNT_LIST "ACCOUNT_LIST"
#define ACCOUNT_SUMMARY_BTC "ACCOUNT_SUMMARY_BTC"
#define ACCOUNT_SUMMARY_BCH "ACCOUNT_SUMMARY_BCH"
#define ACCOUNT_SUMMARY_USD "ACCOUNT_SUMMARY_USD"
#define ACTIVITY_SUMMARY "ACTIVITY_SUMMARY"
#define ACTIVITY_THREAD_ALICE_BOB "ACTIVITY_THREAD_ALICE_BOB"
#define ACTIVITY_THREAD_ALICE_ISSUER "ACTIVITY_THREAD_ALICE_ISSUER"
#define ACTIVITY_THREAD_BOB_ALICE "ACTIVITY_THREAD_BOB_ALICE"
#define CONTACT_LIST "CONTACT_LIST"
#define CONTACT_ALICE_ISSUER "CONTACT_ALICE_ISSUER"
#define MESSAGABLE_LIST "MESSAGAGABLE_LIST"
#define PAYABLE_LIST_BTC "PAYABLE_LIST_BTC"
#define PAYABLE_LIST_BCH "PAYABLE_LIST_BCH"
#define PROFILE "PROFILE"

#define UNIT_DEFINITION_CONTRACT_NAME "Mt Gox USD"
#define UNIT_DEFINITION_TERMS "YOLO"
#define UNIT_DEFINITION_PRIMARY_UNIT_NAME "dollars"
#define UNIT_DEFINITION_SYMBOL "$"
#define UNIT_DEFINITION_TLA "USD"
#define UNIT_DEFINITION_POWER 2
#define UNIT_DEFINITION_FRACTIONAL_UNIT_NAME "cents"
#define CHEQUE_AMOUNT_1 100
#define CHEQUE_AMOUNT_2 75
#define CHEQUE_MEMO "memo"

#define OT_METHOD "::Test_Basic::"

namespace
{
bool init_{false};

class Test_Basic : public ::testing::Test
{
public:
    using WidgetCallback = std::function<bool()>;
    // target counter value, callback
    using WidgetCallbackData =
        std::tuple<int, WidgetCallback, std::promise<bool>>;
    // name, counter
    using WidgetData = std::tuple<std::string, int, WidgetCallbackData>;
    using WidgetMap = std::map<OTIdentifier, WidgetData>;
    using WidgetNameMap = std::map<std::string, OTIdentifier>;
    using StateMap = std::
        map<std::string, std::map<std::string, std::map<int, WidgetCallback>>>;

    static const opentxs::ArgList args_;
    static const std::string SeedA_;
    static const std::string SeedB_;
    static const std::string SeedC_;
    static const std::string Alice_;
    static const std::string Bob_;
    static const std::string Issuer_;
    static const OTNymID alice_nym_id_;
    static const OTNymID bob_nym_id_;
    static const OTNymID issuer_nym_id_;
    static OTIdentifier contact_id_alice_bob_;
    static OTIdentifier contact_id_alice_issuer_;
    static OTIdentifier contact_id_bob_alice_;
    static OTIdentifier contact_id_issuer_alice_;
    static const std::shared_ptr<const ServerContract> server_contract_;
    static const OTServerID server_1_id_;
    static const StateMap state_;

    static WidgetMap alice_widget_map_;
    static WidgetNameMap alice_ui_names_;
    static WidgetMap bob_widget_map_;
    static WidgetNameMap bob_ui_names_;

    static OTZMQListenCallback alice_ui_update_callback_;
    static OTZMQListenCallback bob_ui_update_callback_;

    static const opentxs::api::client::Manager* alice_;
    static const opentxs::api::client::Manager* bob_;
    static std::mutex callback_lock_;

    static std::string alice_payment_code_;
    static std::string bob_payment_code_;
    static std::string issuer_payment_code_;

    static int msg_count_;
    static std::map<int, std::string> message_;

    static OTUnitID unit_id_;
    static OTIdentifier alice_account_id_;
    static OTIdentifier issuer_account_id_;

    const opentxs::api::client::Manager& alice_client_;
    const opentxs::api::client::Manager& bob_client_;
    const opentxs::api::server::Manager& server_1_;
    const opentxs::api::client::Manager& issuer_client_;
    OTZMQSubscribeSocket alice_ui_update_listener_;
    OTZMQSubscribeSocket bob_ui_update_listener_;

    Test_Basic()
        : alice_client_(OT::App().StartClient(args_, 0))
        , bob_client_(OT::App().StartClient(args_, 1))
        , server_1_(OT::App().StartServer(args_, 0, true))
        , issuer_client_(OT::App().StartClient(args_, 2))
        , alice_ui_update_listener_(
              alice_client_.ZeroMQ().SubscribeSocket(alice_ui_update_callback_))
        , bob_ui_update_listener_(
              bob_client_.ZeroMQ().SubscribeSocket(bob_ui_update_callback_))
    {
#if OT_CASH
        server_1_.SetMintKeySize(OT_MINT_KEY_SIZE_TEST);
#endif
        subscribe_sockets();

        if (false == init_) { init(); }
    }

    std::future<bool> add_ui_widget(
        const std::string& name,
        const Identifier& id,
        WidgetMap& map,
        WidgetNameMap& nameMap,
        int counter = 0,
        WidgetCallback callback = {})
    {
        LogDetail(OT_METHOD)(__FUNCTION__)(": Name: ")(name)(" ID: ")(id)
            .Flush();
        WidgetData data{};
        std::get<0>(data) = name;
        auto& [limit, cb, promise] = std::get<2>(data);
        limit = counter, cb = callback, promise = {};
        auto output = promise.get_future();
        map.emplace(id, std::move(data));
        nameMap.emplace(name, id);

        return output;
    }

    std::future<bool> add_ui_widget_alice(
        const std::string& name,
        const Identifier& id,
        int counter = 0,
        WidgetCallback callback = {})
    {
        return add_ui_widget(
            name, id, alice_widget_map_, alice_ui_names_, counter, callback);
    }

    std::future<bool> add_ui_widget_bob(
        const std::string& name,
        const Identifier& id,
        int counter = 0,
        WidgetCallback callback = {})
    {
        return add_ui_widget(
            name, id, bob_widget_map_, bob_ui_names_, counter, callback);
    }

    void import_server_contract(
        const ServerContract& contract,
        const opentxs::api::client::Manager& client)
    {
        auto clientVersion =
            client.Wallet().Server(server_contract_->PublicContract());

        OT_ASSERT(clientVersion)

        client.OTX().SetIntroductionServer(*clientVersion);
    }

    void init()
    {
        const_cast<std::string&>(SeedA_) =
            alice_client_.Exec().Wallet_ImportSeed(
                "spike nominee miss inquiry fee nothing belt list other "
                "daughter leave valley twelve gossip paper",
                "");
        const_cast<std::string&>(SeedB_) = bob_client_.Exec().Wallet_ImportSeed(
            "trim thunder unveil reduce crop cradle zone inquiry "
            "anchor skate property fringe obey butter text tank drama "
            "palm guilt pudding laundry stay axis prosper",
            "");
        const_cast<std::string&>(SeedC_) =
            issuer_client_.Exec().Wallet_ImportSeed(
                "abandon abandon abandon abandon abandon abandon abandon "
                "abandon abandon abandon abandon about",
                "");
        const_cast<std::string&>(Alice_) = alice_client_.Exec().CreateNymHD(
            proto::CITEMTYPE_INDIVIDUAL, ALICE, SeedA_, 0);
        const_cast<std::string&>(Bob_) = bob_client_.Exec().CreateNymHD(
            proto::CITEMTYPE_INDIVIDUAL, BOB, SeedB_, 0);
        const_cast<std::string&>(Issuer_) = issuer_client_.Exec().CreateNymHD(
            proto::CITEMTYPE_INDIVIDUAL, ISSUER, SeedC_, 0);
        const_cast<OTNymID&>(alice_nym_id_) = identifier::Nym::Factory(Alice_);
        const_cast<OTNymID&>(bob_nym_id_) = identifier::Nym::Factory(Bob_);
        const_cast<OTNymID&>(issuer_nym_id_) =
            identifier::Nym::Factory(Issuer_);
        const_cast<OTServerID&>(server_1_id_) =
            identifier::Server::Factory(server_1_.ID().str());
        const_cast<std::shared_ptr<const ServerContract>&>(server_contract_) =
            server_1_.Wallet().Server(server_1_id_);

        OT_ASSERT(server_contract_);
        OT_ASSERT(false == server_1_id_->empty());

        import_server_contract(*server_contract_, alice_client_);
        import_server_contract(*server_contract_, bob_client_);
        import_server_contract(*server_contract_, issuer_client_);

        alice_ = &alice_client_;
        bob_ = &bob_client_;

        init_ = true;
    }

    std::future<bool> set_callback(
        const std::string& name,
        const WidgetNameMap& nameMap,
        int limit,
        WidgetCallback callback,
        WidgetMap& map)
    {
        Lock lock(callback_lock_);
        auto& [counter, cb, promise] = std::get<2>(map.at(nameMap.at(name)));
        counter += limit;
        cb = callback;
        promise = {};

        return promise.get_future();
    }

    std::future<bool> set_callback_alice(
        const std::string& name,
        int limit,
        WidgetCallback callback)
    {
        return set_callback(
            name, alice_ui_names_, limit, callback, alice_widget_map_);
    }

    std::future<bool> set_callback_bob(
        const std::string& name,
        int limit,
        WidgetCallback callback)
    {
        return set_callback(
            name, bob_ui_names_, limit, callback, bob_widget_map_);
    }

    void subscribe_sockets()
    {
        ASSERT_TRUE(alice_ui_update_listener_->Start(
            alice_client_.Endpoints().WidgetUpdate()));
        ASSERT_TRUE(bob_ui_update_listener_->Start(
            bob_client_.Endpoints().WidgetUpdate()));
    }

    static void widget_updated(
        const Lock& lock,
        const std::string nym,
        WidgetData& data)
    {
        auto& [name, counter, callbackData] = data;
        auto& [limit, callback, future] = callbackData;
        ++counter;

        if (counter >= limit) {
            if (callback) {
                future.set_value(callback());
                callback = {};
                future = {};
                limit = 0;
            } else {
                LogOutput(OT_METHOD)(__FUNCTION__)(": ")(nym)(
                    " missing callback for ")(name)
                    .Flush();
            }
        } else {
            LogVerbose(OT_METHOD)(__FUNCTION__)(": Skipping update ")(counter)(
                " to ")(name)
                .Flush();
        }
    }

    static void widget_updated_alice(
        const opentxs::network::zeromq::Message& incoming)
    {
        Lock lock(callback_lock_);
        const auto widgetID = Identifier::Factory(incoming.Body().at(0));

        ASSERT_NE("", widgetID->str().c_str());

        widget_updated(lock, "alice", alice_widget_map_.at(widgetID));
    }

    static void widget_updated_bob(
        const opentxs::network::zeromq::Message& incoming)
    {
        Lock lock(callback_lock_);
        const auto widgetID = Identifier::Factory(incoming.Body().at(0));

        ASSERT_NE("", widgetID->str().c_str());

        widget_updated(lock, "bob", bob_widget_map_.at(widgetID));
    }
};

const opentxs::ArgList Test_Basic::args_{
    {{OPENTXS_ARG_STORAGE_PLUGIN, {"mem"}}}};
const std::string Test_Basic::SeedA_{""};
const std::string Test_Basic::SeedB_{""};
const std::string Test_Basic::SeedC_{""};
const std::string Test_Basic::Alice_{""};
const std::string Test_Basic::Bob_{""};
const std::string Test_Basic::Issuer_{""};
const OTNymID Test_Basic::alice_nym_id_{identifier::Nym::Factory()};
const OTNymID Test_Basic::bob_nym_id_{identifier::Nym::Factory()};
const OTNymID Test_Basic::issuer_nym_id_{identifier::Nym::Factory()};
OTIdentifier Test_Basic::contact_id_alice_bob_{Identifier::Factory()};
OTIdentifier Test_Basic::contact_id_alice_issuer_{Identifier::Factory()};
OTIdentifier Test_Basic::contact_id_bob_alice_{Identifier::Factory()};
OTIdentifier Test_Basic::contact_id_issuer_alice_{Identifier::Factory()};
const std::shared_ptr<const ServerContract> Test_Basic::server_contract_{
    nullptr};
const OTServerID Test_Basic::server_1_id_{identifier::Server::Factory()};
Test_Basic::WidgetMap Test_Basic::alice_widget_map_{};
Test_Basic::WidgetNameMap Test_Basic::alice_ui_names_{};
Test_Basic::WidgetMap Test_Basic::bob_widget_map_{};
Test_Basic::WidgetNameMap Test_Basic::bob_ui_names_{};
OTZMQListenCallback Test_Basic::alice_ui_update_callback_{
    opentxs::network::zeromq::ListenCallback::Factory(
        [](const opentxs::network::zeromq::Message& incoming) -> void {
            widget_updated_alice(incoming);
        })};
OTZMQListenCallback Test_Basic::bob_ui_update_callback_{
    opentxs::network::zeromq::ListenCallback::Factory(
        [](const opentxs::network::zeromq::Message& incoming) -> void {
            widget_updated_bob(incoming);
        })};
const opentxs::api::client::Manager* Test_Basic::alice_{nullptr};
const opentxs::api::client::Manager* Test_Basic::bob_{nullptr};
std::mutex Test_Basic::callback_lock_{};
std::string Test_Basic::alice_payment_code_;
std::string Test_Basic::bob_payment_code_;
std::string Test_Basic::issuer_payment_code_;
int Test_Basic::msg_count_ = 0;
std::map<int, std::string> Test_Basic::message_{};
OTUnitID Test_Basic::unit_id_{identifier::UnitDefinition::Factory()};
OTIdentifier Test_Basic::alice_account_id_{Identifier::Factory()};
OTIdentifier Test_Basic::issuer_account_id_{Identifier::Factory()};
const Test_Basic::StateMap Test_Basic::state_{
    {ALICE,
     {
         {PROFILE,
          {
              {0,
               []() -> bool {
                   const auto& widget = alice_->UI().Profile(alice_nym_id_);
                   auto paymentCode =
                       alice_->Factory().PaymentCode(SeedA_, 0, 1);

                   EXPECT_EQ(paymentCode->asBase58(), widget.PaymentCode());
                   EXPECT_STREQ(ALICE, widget.DisplayName().c_str());

                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
          }},
         {CONTACT_LIST,
          {
              {0,
               []() -> bool {
                   const auto& widget = alice_->UI().ContactList(alice_nym_id_);
                   const auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_TRUE(
                       row->DisplayName() == ALICE ||
                       row->DisplayName() == "Owner");
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("ME", row->Section().c_str());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& widget = alice_->UI().ContactList(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(row->DisplayName().c_str(), ALICE);
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("ME", row->Section().c_str());
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_STREQ(row->DisplayName().c_str(), BOB);
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("B", row->Section().c_str());
                   EXPECT_TRUE(row->Last());

                   // We need this later
                   contact_id_alice_bob_->SetString(row->ContactID());

                   EXPECT_FALSE(contact_id_alice_bob_->empty());

                   return true;
               }},
              {2,
               []() -> bool {
                   const auto& widget = alice_->UI().ContactList(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(row->DisplayName().c_str(), ALICE);
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("ME", row->Section().c_str());
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_STREQ(row->DisplayName().c_str(), BOB);
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("B", row->Section().c_str());
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_STREQ(row->DisplayName().c_str(), ISSUER);
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("I", row->Section().c_str());
                   EXPECT_TRUE(row->Last());

                   contact_id_alice_issuer_->SetString(row->ContactID());

                   EXPECT_FALSE(contact_id_alice_issuer_->empty());

                   return true;
               }},
          }},
         {MESSAGABLE_LIST,
          {
              {0,
               []() -> bool {
                   const auto& widget =
                       alice_->UI().MessagableList(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& widget =
                       alice_->UI().MessagableList(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(row->DisplayName().c_str(), BOB);
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("B", row->Section().c_str());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {2,
               []() -> bool {
                   const auto& widget =
                       alice_->UI().MessagableList(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(row->DisplayName().c_str(), BOB);
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("B", row->Section().c_str());
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_STREQ(row->DisplayName().c_str(), ISSUER);
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("I", row->Section().c_str());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
          }},
         {PAYABLE_LIST_BTC,
          {
              {0,
               []() -> bool {
                   const auto& widget = alice_->UI().PayableList(
                       alice_nym_id_, proto::CITEMTYPE_BTC);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(ALICE, row->DisplayName().c_str());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& widget = alice_->UI().PayableList(
                       alice_nym_id_, proto::CITEMTYPE_BTC);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(ALICE, row->DisplayName().c_str());
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_STREQ(BOB, row->DisplayName().c_str());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
          }},
         {PAYABLE_LIST_BCH,
          {
              {0,
               []() -> bool {
                   const auto& widget = alice_->UI().PayableList(
                       alice_nym_id_, proto::CITEMTYPE_BCH);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& widget = alice_->UI().PayableList(
                       alice_nym_id_, proto::CITEMTYPE_BCH);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(BOB, row->DisplayName().c_str());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {2,
               []() -> bool {
                   const auto& widget = alice_->UI().PayableList(
                       alice_nym_id_, proto::CITEMTYPE_BCH);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(ALICE, row->DisplayName().c_str());
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_STREQ(BOB, row->DisplayName().c_str());
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_STREQ(ISSUER, row->DisplayName().c_str());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
          }},
         {ACTIVITY_SUMMARY,
          {
              {0,
               []() -> bool {
                   const auto& widget =
                       alice_->UI().ActivitySummary(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& firstMessage = message_[msg_count_];
                   const auto& widget =
                       alice_->UI().ActivitySummary(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());
                   EXPECT_EQ(row->DisplayName(), BOB);
                   EXPECT_EQ(row->ImageURI(), "");
                   EXPECT_EQ(row->Text(), firstMessage);
                   EXPECT_FALSE(row->ThreadID().empty());
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILOUTBOX);
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {2,
               []() -> bool {
                   const auto& secondMessage = message_[msg_count_];
                   const auto& widget =
                       alice_->UI().ActivitySummary(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());
                   EXPECT_EQ(row->DisplayName(), BOB);
                   EXPECT_EQ(row->ImageURI(), "");
                   EXPECT_EQ(row->Text(), secondMessage);
                   EXPECT_FALSE(row->ThreadID().empty());
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILINBOX);
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {3,
               []() -> bool {
                   const auto& secondMessage = message_[msg_count_];
                   const auto& widget =
                       alice_->UI().ActivitySummary(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(ISSUER, row->DisplayName().c_str());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("Received cheque", row->Text().c_str());
                   EXPECT_FALSE(row->ThreadID().empty());
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(StorageBox::INCOMINGCHEQUE, row->Type());
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_EQ(row->DisplayName(), BOB);
                   EXPECT_EQ(row->ImageURI(), "");
                   EXPECT_EQ(row->Text(), secondMessage);
                   EXPECT_FALSE(row->ThreadID().empty());
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILINBOX);
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {4,
               []() -> bool {
                   const auto& widget =
                       alice_->UI().ActivitySummary(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_EQ(row->DisplayName(), BOB);
                   EXPECT_EQ(row->ImageURI(), "");
                   EXPECT_EQ(row->Text(), "Sent cheque for dollars 0.75");
                   EXPECT_FALSE(row->ThreadID().empty());
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::OUTGOINGCHEQUE);
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_STREQ(ISSUER, row->DisplayName().c_str());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("Received cheque", row->Text().c_str());
                   EXPECT_FALSE(row->ThreadID().empty());
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(StorageBox::INCOMINGCHEQUE, row->Type());

                   return true;
               }},
          }},
         {ACTIVITY_THREAD_ALICE_BOB,
          {
              {0,
               []() -> bool {
                   const auto& widget = alice_->UI().ActivityThread(
                       alice_nym_id_, contact_id_alice_bob_);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& firstMessage = message_[msg_count_];
                   const auto& widget = alice_->UI().ActivityThread(
                       alice_nym_id_, contact_id_alice_bob_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());
                   EXPECT_EQ(row->Amount(), 0);
                   EXPECT_EQ(row->DisplayAmount(), "");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), "");
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), firstMessage);
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILOUTBOX);
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {2,
               []() -> bool {
                   const auto& firstMessage = message_[msg_count_ - 1];
                   const auto& secondMessage = message_[msg_count_];
                   const auto& widget = alice_->UI().ActivityThread(
                       alice_nym_id_, contact_id_alice_bob_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_EQ(row->Amount(), 0);
                   EXPECT_EQ(row->DisplayAmount(), "");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), "");
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), firstMessage);
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILOUTBOX);
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_EQ(row->Amount(), 0);
                   EXPECT_EQ(row->DisplayAmount(), "");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), "");
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), secondMessage);
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILINBOX);
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {3,
               []() -> bool {
                   const auto& firstMessage = message_[msg_count_ - 1];
                   const auto& secondMessage = message_[msg_count_];
                   const auto& widget = alice_->UI().ActivityThread(
                       alice_nym_id_, contact_id_alice_bob_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_EQ(row->Amount(), 0);
                   EXPECT_EQ(row->DisplayAmount(), "");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), "");
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), firstMessage);
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILOUTBOX);
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_EQ(row->Amount(), 0);
                   EXPECT_EQ(row->DisplayAmount(), "");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), "");
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), secondMessage);
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILINBOX);
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_EQ(row->Amount(), CHEQUE_AMOUNT_2);
                   EXPECT_EQ(row->DisplayAmount(), "dollars 0.75");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), CHEQUE_MEMO);
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), "Sent cheque for dollars 0.75");
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::OUTGOINGCHEQUE);
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
          }},
         {ACTIVITY_THREAD_ALICE_ISSUER,
          {
              {0,
               []() -> bool {
                   const auto& widget = alice_->UI().ActivityThread(
                       alice_nym_id_, contact_id_alice_issuer_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   bool loading{true};

                   // This allows the test to work correctly in valgrind when
                   // loading is unusually slow
                   while (loading) {
                       row = widget.First();
                       loading = row->Loading();
                   }

                   EXPECT_EQ(CHEQUE_AMOUNT_1, row->Amount());
                   EXPECT_FALSE(row->Loading());
                   EXPECT_STREQ(CHEQUE_MEMO, row->Memo().c_str());
                   EXPECT_FALSE(row->Pending());
                   EXPECT_STREQ(
                       "Received cheque for dollars 1.00", row->Text().c_str());
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(StorageBox::INCOMINGCHEQUE, row->Type());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
          }},
         {ACCOUNT_SUMMARY_BTC,
          {
              {0,
               []() -> bool {
                   const auto& widget = alice_->UI().AccountSummary(
                       alice_nym_id_, proto::CITEMTYPE_BTC);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
          }},
         {ACCOUNT_SUMMARY_BCH,
          {
              {0,
               []() -> bool {
                   const auto& widget = alice_->UI().AccountSummary(
                       alice_nym_id_, proto::CITEMTYPE_BCH);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
          }},
         {ACCOUNT_SUMMARY_USD,
          {
              {0,
               []() -> bool {
                   const auto& widget = alice_->UI().AccountSummary(
                       alice_nym_id_, proto::CITEMTYPE_USD);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
          }},
         {ACCOUNT_ACTIVITY_USD,
          {
              {0,
               []() -> bool {
                   const auto& widget = alice_->UI().AccountActivity(
                       alice_nym_id_, alice_account_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_EQ(CHEQUE_AMOUNT_1, row->Amount());
                   EXPECT_EQ(1, row->Contacts().size());

                   if (0 < row->Contacts().size()) {
                       EXPECT_EQ(
                           contact_id_alice_issuer_->str(),
                           *row->Contacts().begin());
                   }

                   EXPECT_EQ("dollars 1.00", row->DisplayAmount());
                   EXPECT_EQ(CHEQUE_MEMO, row->Memo());
                   EXPECT_FALSE(row->Workflow().empty());
                   EXPECT_EQ("Received cheque #510 from Issuer", row->Text());
                   EXPECT_EQ(StorageBox::INCOMINGCHEQUE, row->Type());
                   EXPECT_FALSE(row->UUID().empty());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& widget = alice_->UI().AccountActivity(
                       alice_nym_id_, alice_account_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_EQ(-1 * CHEQUE_AMOUNT_2, row->Amount());
                   EXPECT_EQ(1, row->Contacts().size());

                   if (0 < row->Contacts().size()) {
                       EXPECT_EQ(
                           contact_id_alice_bob_->str(),
                           *row->Contacts().begin());
                   }

                   EXPECT_EQ("-dollars 0.75", row->DisplayAmount());
                   EXPECT_EQ(CHEQUE_MEMO, row->Memo());
                   EXPECT_FALSE(row->Workflow().empty());
                   EXPECT_EQ("Wrote cheque #721 for Bob", row->Text());
                   EXPECT_EQ(StorageBox::OUTGOINGCHEQUE, row->Type());
                   EXPECT_FALSE(row->UUID().empty());
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_EQ(CHEQUE_AMOUNT_1, row->Amount());
                   EXPECT_EQ(1, row->Contacts().size());

                   if (0 < row->Contacts().size()) {
                       EXPECT_EQ(
                           contact_id_alice_issuer_->str(),
                           *row->Contacts().begin());
                   }

                   EXPECT_EQ("dollars 1.00", row->DisplayAmount());
                   EXPECT_EQ(CHEQUE_MEMO, row->Memo());
                   EXPECT_FALSE(row->Workflow().empty());
                   EXPECT_EQ("Received cheque #510 from Issuer", row->Text());
                   EXPECT_EQ(StorageBox::INCOMINGCHEQUE, row->Type());
                   EXPECT_FALSE(row->UUID().empty());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
          }},
         {ACCOUNT_LIST,
          {
              {0,
               []() -> bool {
                   const auto& widget = alice_->UI().AccountList(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& widget = alice_->UI().AccountList(alice_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_TRUE(alice_account_id_->empty());

                   alice_account_id_->SetString(row->AccountID());

                   EXPECT_FALSE(alice_account_id_->empty());
                   EXPECT_EQ(unit_id_->str(), row->ContractID());
                   EXPECT_STREQ("dollars 1.00", row->DisplayBalance().c_str());
                   EXPECT_STREQ("", row->Name().c_str());
                   EXPECT_EQ(server_1_id_->str(), row->NotaryID());
                   EXPECT_EQ(
                       server_contract_->EffectiveName(), row->NotaryName());
                   EXPECT_EQ(AccountType::Custodial, row->Type());
                   EXPECT_EQ(proto::CITEMTYPE_USD, row->Unit());

                   return true;
               }},
          }},
         {CONTACT_ALICE_ISSUER,
          {
              {0,
               []() -> bool {
                   const auto& widget =
                       alice_->UI().Contact(contact_id_alice_issuer_);

                   EXPECT_EQ(
                       contact_id_alice_issuer_->str(), widget.ContactID());
                   EXPECT_EQ(std::string(ISSUER), widget.DisplayName());
                   EXPECT_EQ(issuer_payment_code_, widget.PaymentCode());

                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   return true;
               }},
          }},
     }},
    {BOB,
     {
         {PROFILE,
          {
              {0,
               []() -> bool {
                   const auto& widget = bob_->UI().Profile(bob_nym_id_);
                   auto paymentCode = bob_->Factory().PaymentCode(SeedB_, 0, 1);

                   EXPECT_EQ(paymentCode->asBase58(), widget.PaymentCode());
                   EXPECT_STREQ(BOB, widget.DisplayName().c_str());

                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
          }},
         {CONTACT_LIST,
          {
              {0,
               []() -> bool {
                   const auto& widget = bob_->UI().ContactList(bob_nym_id_);
                   const auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_TRUE(
                       row->DisplayName() == BOB ||
                       row->DisplayName() == "Owner");
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("ME", row->Section().c_str());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& widget = bob_->UI().ContactList(bob_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(row->DisplayName().c_str(), BOB);
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("ME", row->Section().c_str());
                   EXPECT_FALSE(row->Last());

                   row = widget.Next();

                   EXPECT_STREQ(row->DisplayName().c_str(), ALICE);
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("A", row->Section().c_str());
                   EXPECT_TRUE(row->Last());

                   contact_id_bob_alice_->SetString(row->ContactID());

                   EXPECT_FALSE(contact_id_bob_alice_->empty());

                   return true;
               }},
          }},
         {MESSAGABLE_LIST,
          {
              {0,
               []() -> bool {
                   const auto& widget = bob_->UI().MessagableList(bob_nym_id_);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& widget = bob_->UI().MessagableList(bob_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(row->DisplayName().c_str(), ALICE);
                   EXPECT_TRUE(row->Valid());
                   EXPECT_STREQ("", row->ImageURI().c_str());
                   EXPECT_STREQ("A", row->Section().c_str());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
          }},
         {PAYABLE_LIST_BTC,
          {
              {0,
               []() -> bool {
                   const auto& widget = bob_->UI().PayableList(
                       bob_nym_id_, proto::CITEMTYPE_BTC);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(BOB, row->DisplayName().c_str());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& widget = bob_->UI().PayableList(
                       bob_nym_id_, proto::CITEMTYPE_BTC);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(ALICE, row->DisplayName().c_str());
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   EXPECT_STREQ(BOB, row->DisplayName().c_str());
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
          }},
         {PAYABLE_LIST_BCH,
          {
              {0,
               []() -> bool {
                   const auto& widget = bob_->UI().PayableList(
                       bob_nym_id_, proto::CITEMTYPE_BCH);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& widget = bob_->UI().PayableList(
                       bob_nym_id_, proto::CITEMTYPE_BCH);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_STREQ(ALICE, row->DisplayName().c_str());
                   EXPECT_TRUE(row->Last());

                   // TODO why isn't Bob in this list?

                   return true;
               }},
          }},
         {ACTIVITY_SUMMARY,
          {
              {0,
               []() -> bool {
                   const auto& widget = bob_->UI().ActivitySummary(bob_nym_id_);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& firstMessage = message_[msg_count_];
                   const auto& widget = bob_->UI().ActivitySummary(bob_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());
                   EXPECT_EQ(row->DisplayName(), ALICE);
                   EXPECT_EQ(row->ImageURI(), "");
                   EXPECT_EQ(row->Text(), firstMessage);
                   EXPECT_FALSE(row->ThreadID().empty());
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILINBOX);
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {2,
               []() -> bool {
                   const auto& secondMessage = message_[msg_count_];
                   const auto& widget = bob_->UI().ActivitySummary(bob_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());
                   EXPECT_EQ(row->DisplayName(), ALICE);
                   EXPECT_EQ(row->ImageURI(), "");
                   EXPECT_EQ(row->Text(), secondMessage);
                   EXPECT_FALSE(row->ThreadID().empty());
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILOUTBOX);
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {3,
               []() -> bool {
                   const auto& widget = bob_->UI().ActivitySummary(bob_nym_id_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());
                   EXPECT_EQ(row->DisplayName(), ALICE);
                   EXPECT_EQ(row->ImageURI(), "");
                   EXPECT_EQ(row->Text(), "Received cheque");
                   EXPECT_FALSE(row->ThreadID().empty());
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::INCOMINGCHEQUE);
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
          }},
         {ACTIVITY_THREAD_BOB_ALICE,
          {
              {0,
               []() -> bool {
                   const auto& widget = bob_->UI().ActivityThread(
                       bob_nym_id_, contact_id_bob_alice_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());
                   EXPECT_EQ(row->Amount(), 0);
                   EXPECT_EQ(row->DisplayAmount(), "");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), "");
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), message_.at(msg_count_));
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILINBOX);

                   return true;
               }},
              {1,
               []() -> bool {
                   const auto& firstMessage = message_[msg_count_ - 1];
                   const auto& secondMessage = message_[msg_count_];
                   const auto& widget = bob_->UI().ActivityThread(
                       bob_nym_id_, contact_id_bob_alice_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_EQ(row->Amount(), 0);
                   EXPECT_EQ(row->DisplayAmount(), "");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), "");
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), firstMessage);
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILINBOX);
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   bool loading{true};

                   // This allows the test to work correctly in valgrind when
                   // loading is unusually slow
                   while (loading) {
                       row = widget.First();
                       row = widget.Next();
                       loading = row->Loading();
                   }

                   EXPECT_EQ(row->Amount(), 0);
                   EXPECT_EQ(row->DisplayAmount(), "");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), "");
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), secondMessage);
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILOUTBOX);
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
              {2,
               []() -> bool {
                   const auto& firstMessage = message_[msg_count_ - 1];
                   const auto& secondMessage = message_[msg_count_];
                   const auto& widget = bob_->UI().ActivityThread(
                       bob_nym_id_, contact_id_bob_alice_);
                   auto row = widget.First();

                   EXPECT_TRUE(row->Valid());

                   if (false == row->Valid()) { return false; }

                   EXPECT_EQ(row->Amount(), 0);
                   EXPECT_EQ(row->DisplayAmount(), "");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), "");
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), firstMessage);
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILINBOX);
                   EXPECT_FALSE(row->Last());

                   if (row->Last()) { return false; }

                   row = widget.Next();

                   bool loading{true};

                   EXPECT_EQ(row->Amount(), 0);
                   EXPECT_EQ(row->DisplayAmount(), "");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), "");
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), secondMessage);
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::MAILOUTBOX);
                   EXPECT_FALSE(row->Last());

                   // This allows the test to work correctly in valgrind when
                   // loading is unusually slow
                   while (loading) {
                       row = widget.First();
                       row = widget.Next();
                       row = widget.Next();
                       loading = row->Loading();
                   }

                   EXPECT_EQ(row->Amount(), CHEQUE_AMOUNT_2);
                   EXPECT_EQ(row->DisplayAmount(), "");
                   EXPECT_FALSE(row->Loading());
                   EXPECT_EQ(row->Memo(), CHEQUE_MEMO);
                   EXPECT_FALSE(row->Pending());
                   EXPECT_EQ(row->Text(), "Received cheque");
                   EXPECT_LT(0, Clock::to_time_t(row->Timestamp()));
                   EXPECT_EQ(row->Type(), StorageBox::INCOMINGCHEQUE);
                   EXPECT_TRUE(row->Last());

                   return true;
               }},
          }},
         {ACCOUNT_SUMMARY_BTC,
          {
              {0,
               []() -> bool {
                   const auto& widget = bob_->UI().AccountSummary(
                       bob_nym_id_, proto::CITEMTYPE_BTC);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
          }},
         {ACCOUNT_SUMMARY_BCH,
          {
              {0,
               []() -> bool {
                   const auto& widget = bob_->UI().AccountSummary(
                       bob_nym_id_, proto::CITEMTYPE_BCH);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
          }},
         {ACCOUNT_SUMMARY_USD,
          {
              {0,
               []() -> bool {
                   const auto& widget = bob_->UI().AccountSummary(
                       bob_nym_id_, proto::CITEMTYPE_USD);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
          }},
         {ACCOUNT_LIST,
          {
              {0,
               []() -> bool {
                   const auto& widget = bob_->UI().AccountList(bob_nym_id_);
                   auto row = widget.First();

                   EXPECT_FALSE(row->Valid());

                   return true;
               }},
          }},
     }},
};

TEST_F(Test_Basic, instantiate_ui_objects)
{
    Lock lock(callback_lock_);
    auto future1 = add_ui_widget_alice(
        PROFILE,
        alice_client_.UI().Profile(alice_nym_id_).WidgetID(),
        2,
        state_.at(ALICE).at(PROFILE).at(0));
    auto future2 = add_ui_widget_bob(
        PROFILE,
        bob_client_.UI().Profile(bob_nym_id_).WidgetID(),
        2,
        state_.at(BOB).at(PROFILE).at(0));
    add_ui_widget_alice(
        ACTIVITY_SUMMARY,
        alice_client_.UI().ActivitySummary(alice_nym_id_).WidgetID());
    add_ui_widget_bob(
        ACTIVITY_SUMMARY,
        bob_client_.UI().ActivitySummary(bob_nym_id_).WidgetID());
    auto future3 = add_ui_widget_alice(
        CONTACT_LIST,
        alice_client_.UI().ContactList(alice_nym_id_).WidgetID(),
        1,
        state_.at(ALICE).at(CONTACT_LIST).at(0));
    auto future4 = add_ui_widget_bob(
        CONTACT_LIST,
        bob_client_.UI().ContactList(bob_nym_id_).WidgetID(),
        1,
        state_.at(BOB).at(CONTACT_LIST).at(0));
    add_ui_widget_alice(
        PAYABLE_LIST_BCH,
        alice_client_.UI()
            .PayableList(alice_nym_id_, proto::CITEMTYPE_BCH)
            .WidgetID());
    add_ui_widget_bob(
        PAYABLE_LIST_BCH,
        bob_client_.UI()
            .PayableList(bob_nym_id_, proto::CITEMTYPE_BCH)
            .WidgetID());
    auto future5 = add_ui_widget_alice(
        PAYABLE_LIST_BTC,
        alice_client_.UI()
            .PayableList(alice_nym_id_, proto::CITEMTYPE_BTC)
            .WidgetID(),
        1,
        state_.at(ALICE).at(PAYABLE_LIST_BTC).at(0));
    auto future6 = add_ui_widget_bob(
        PAYABLE_LIST_BTC,
        bob_client_.UI()
            .PayableList(bob_nym_id_, proto::CITEMTYPE_BTC)
            .WidgetID(),
        1,
        state_.at(BOB).at(PAYABLE_LIST_BTC).at(0));
    add_ui_widget_alice(
        ACCOUNT_SUMMARY_BTC,
        alice_client_.UI()
            .AccountSummary(alice_nym_id_, proto::CITEMTYPE_BTC)
            .WidgetID());
    add_ui_widget_bob(
        ACCOUNT_SUMMARY_BTC,
        bob_client_.UI()
            .AccountSummary(bob_nym_id_, proto::CITEMTYPE_BTC)
            .WidgetID());
    add_ui_widget_alice(
        ACCOUNT_SUMMARY_BCH,
        alice_client_.UI()
            .AccountSummary(alice_nym_id_, proto::CITEMTYPE_BCH)
            .WidgetID());
    add_ui_widget_bob(
        ACCOUNT_SUMMARY_BCH,
        bob_client_.UI()
            .AccountSummary(bob_nym_id_, proto::CITEMTYPE_BCH)
            .WidgetID());
    add_ui_widget_alice(
        ACCOUNT_SUMMARY_USD,
        alice_client_.UI()
            .AccountSummary(alice_nym_id_, proto::CITEMTYPE_USD)
            .WidgetID());
    add_ui_widget_bob(
        ACCOUNT_SUMMARY_USD,
        bob_client_.UI()
            .AccountSummary(bob_nym_id_, proto::CITEMTYPE_USD)
            .WidgetID());
    add_ui_widget_alice(
        MESSAGABLE_LIST,
        alice_client_.UI().MessagableList(alice_nym_id_).WidgetID());
    add_ui_widget_bob(
        MESSAGABLE_LIST,
        bob_client_.UI().MessagableList(bob_nym_id_).WidgetID());
    add_ui_widget_alice(
        ACCOUNT_LIST, alice_client_.UI().AccountList(alice_nym_id_).WidgetID());
    add_ui_widget_bob(
        ACCOUNT_LIST, bob_client_.UI().AccountList(bob_nym_id_).WidgetID());

    EXPECT_EQ(10, alice_widget_map_.size());
    EXPECT_EQ(10, alice_ui_names_.size());
    EXPECT_EQ(10, bob_widget_map_.size());
    EXPECT_EQ(10, bob_ui_names_.size());

    lock.unlock();

    EXPECT_TRUE(future1.get());
    EXPECT_TRUE(future2.get());
    EXPECT_TRUE(future3.get());
    EXPECT_TRUE(future4.get());
    EXPECT_TRUE(future5.get());
    EXPECT_TRUE(future6.get());
}

TEST_F(Test_Basic, initial_state)
{
    EXPECT_TRUE(state_.at(ALICE).at(ACTIVITY_SUMMARY).at(0)());
    EXPECT_TRUE(state_.at(ALICE).at(MESSAGABLE_LIST).at(0)());
    EXPECT_TRUE(state_.at(ALICE).at(PAYABLE_LIST_BCH).at(0)());
    EXPECT_TRUE(state_.at(ALICE).at(ACCOUNT_SUMMARY_BTC).at(0)());
    EXPECT_TRUE(state_.at(ALICE).at(ACCOUNT_SUMMARY_BCH).at(0)());
    EXPECT_TRUE(state_.at(ALICE).at(ACCOUNT_SUMMARY_USD).at(0)());
    EXPECT_TRUE(state_.at(ALICE).at(ACCOUNT_LIST).at(0)());

    EXPECT_TRUE(state_.at(BOB).at(ACTIVITY_SUMMARY).at(0)());
    EXPECT_TRUE(state_.at(BOB).at(MESSAGABLE_LIST).at(0)());
    EXPECT_TRUE(state_.at(BOB).at(PAYABLE_LIST_BCH).at(0)());
    EXPECT_TRUE(state_.at(BOB).at(ACCOUNT_SUMMARY_BTC).at(0)());
    EXPECT_TRUE(state_.at(BOB).at(ACCOUNT_SUMMARY_BCH).at(0)());
    EXPECT_TRUE(state_.at(BOB).at(ACCOUNT_SUMMARY_USD).at(0)());
    EXPECT_TRUE(state_.at(BOB).at(ACCOUNT_LIST).at(0)());
}

TEST_F(Test_Basic, payment_codes)
{
    auto alice = alice_client_.Wallet().mutable_Nym(alice_nym_id_);
    auto bob = bob_client_.Wallet().mutable_Nym(bob_nym_id_);
    auto issuer = issuer_client_.Wallet().mutable_Nym(issuer_nym_id_);

    EXPECT_EQ(opentxs::proto::CITEMTYPE_INDIVIDUAL, alice.Type());
    EXPECT_EQ(opentxs::proto::CITEMTYPE_INDIVIDUAL, bob.Type());
    EXPECT_EQ(opentxs::proto::CITEMTYPE_INDIVIDUAL, issuer.Type());

    auto aliceScopeSet =
        alice.SetScope(opentxs::proto::CITEMTYPE_INDIVIDUAL, ALICE, true);
    auto bobScopeSet = bob.SetScope(proto::CITEMTYPE_INDIVIDUAL, BOB, true);
    auto issuerScopeSet =
        issuer.SetScope(proto::CITEMTYPE_INDIVIDUAL, ISSUER, true);

    EXPECT_TRUE(aliceScopeSet);
    EXPECT_TRUE(bobScopeSet);
    EXPECT_TRUE(issuerScopeSet);

    alice_payment_code_ =
        alice_client_.Factory().PaymentCode(SeedA_, 0, 1)->asBase58();
    bob_payment_code_ =
        bob_client_.Factory().PaymentCode(SeedB_, 0, 1)->asBase58();
    issuer_payment_code_ =
        issuer_client_.Factory().PaymentCode(SeedC_, 0, 1)->asBase58();

    EXPECT_FALSE(alice_payment_code_.empty());
    EXPECT_FALSE(bob_payment_code_.empty());
    EXPECT_FALSE(issuer_payment_code_.empty());

    alice.AddPaymentCode(
        alice_payment_code_, opentxs::proto::CITEMTYPE_BTC, true, true);
    bob.AddPaymentCode(
        bob_payment_code_, opentxs::proto::CITEMTYPE_BTC, true, true);
    issuer.AddPaymentCode(
        issuer_payment_code_, opentxs::proto::CITEMTYPE_BTC, true, true);
    alice.AddPaymentCode(
        alice_payment_code_, opentxs::proto::CITEMTYPE_BCH, true, true);
    bob.AddPaymentCode(
        bob_payment_code_, opentxs::proto::CITEMTYPE_BCH, true, true);
    issuer.AddPaymentCode(
        issuer_payment_code_, opentxs::proto::CITEMTYPE_BCH, true, true);

    EXPECT_FALSE(alice.PaymentCode(proto::CITEMTYPE_BTC).empty());
    EXPECT_FALSE(bob.PaymentCode(proto::CITEMTYPE_BTC).empty());
    EXPECT_FALSE(issuer.PaymentCode(proto::CITEMTYPE_BTC).empty());
    EXPECT_FALSE(alice.PaymentCode(proto::CITEMTYPE_BCH).empty());
    EXPECT_FALSE(bob.PaymentCode(proto::CITEMTYPE_BCH).empty());
    EXPECT_FALSE(issuer.PaymentCode(proto::CITEMTYPE_BCH).empty());

    alice.Release();
    bob.Release();
    issuer.Release();
}

TEST_F(Test_Basic, introduction_server)
{
    alice_client_.OTX().StartIntroductionServer(alice_nym_id_);
    bob_client_.OTX().StartIntroductionServer(bob_nym_id_);
    auto task1 = alice_client_.OTX().RegisterNymPublic(
        alice_nym_id_, server_1_id_, true);
    auto task2 =
        bob_client_.OTX().RegisterNymPublic(bob_nym_id_, server_1_id_, true);

    ASSERT_NE(0, task1.first);
    ASSERT_NE(0, task2.first);
    EXPECT_EQ(proto::LASTREPLYSTATUS_MESSAGESUCCESS, task1.second.get().first);
    EXPECT_EQ(proto::LASTREPLYSTATUS_MESSAGESUCCESS, task2.second.get().first);

    alice_client_.OTX().ContextIdle(alice_nym_id_, server_1_id_).get();
    bob_client_.OTX().ContextIdle(bob_nym_id_, server_1_id_).get();
}

TEST_F(Test_Basic, add_contact_preconditions)
{
    // Neither alice nor bob should know about each other yet
    auto alice = bob_client_.Wallet().Nym(alice_nym_id_);
    auto bob = alice_client_.Wallet().Nym(bob_nym_id_);

    EXPECT_FALSE(alice);
    EXPECT_FALSE(bob);
}

TEST_F(Test_Basic, add_contact_Bob_To_Alice)
{
    auto contactListDone = set_callback_alice(
        CONTACT_LIST, 2, state_.at(ALICE).at(CONTACT_LIST).at(1));
    auto payableBTCListDone = set_callback_alice(
        PAYABLE_LIST_BTC, 2, state_.at(ALICE).at(PAYABLE_LIST_BTC).at(1));
    auto payableBCHListDone = set_callback_alice(
        PAYABLE_LIST_BCH, 1, state_.at(ALICE).at(PAYABLE_LIST_BCH).at(1));
    auto messagableListDone = set_callback_alice(
        MESSAGABLE_LIST, 1, state_.at(ALICE).at(MESSAGABLE_LIST).at(1));

    // Add the contact
    alice_->UI()
        .ContactList(alice_nym_id_)
        .AddContact(BOB, bob_payment_code_, bob_nym_id_->str());

    EXPECT_TRUE(contactListDone.get());
    EXPECT_TRUE(payableBTCListDone.get());
    EXPECT_TRUE(payableBCHListDone.get());
    EXPECT_TRUE(messagableListDone.get());
}

TEST_F(Test_Basic, activity_thread_alice_bob)
{
    Lock lock(callback_lock_);
    auto done = add_ui_widget_alice(
        ACTIVITY_THREAD_ALICE_BOB,
        alice_client_.UI()
            .ActivityThread(alice_nym_id_, contact_id_alice_bob_)
            .WidgetID(),
        2,
        state_.at(ALICE).at(ACTIVITY_THREAD_ALICE_BOB).at(0));

    EXPECT_EQ(11, alice_widget_map_.size());
    EXPECT_EQ(11, alice_ui_names_.size());

    lock.unlock();

    EXPECT_TRUE(done.get());
}

TEST_F(Test_Basic, send_message_from_Alice_to_Bob_1)
{
    const auto& from_client = alice_client_;
    const auto& from_nym_id_ = alice_nym_id_;
    const auto messageID = ++msg_count_;
    std::stringstream text{};
    text << ALICE << " messaged " << BOB << " with message #"
         << std::to_string(messageID);
    auto& firstMessage = message_[messageID];
    firstMessage = text.str();

    auto aliceActivitySummaryDone = set_callback_alice(
        ACTIVITY_SUMMARY, 2, state_.at(ALICE).at(ACTIVITY_SUMMARY).at(1));
    auto bobActivitySummaryDone = set_callback_bob(
        ACTIVITY_SUMMARY, 2, state_.at(BOB).at(ACTIVITY_SUMMARY).at(1));
    auto aliceActivityThreadDone = set_callback_alice(
        ACTIVITY_THREAD_ALICE_BOB,
        7,
        state_.at(ALICE).at(ACTIVITY_THREAD_ALICE_BOB).at(1));
    auto bobContactListDone = set_callback_bob(
        CONTACT_LIST, 3, state_.at(BOB).at(CONTACT_LIST).at(1));
    auto bobMessagableListDone = set_callback_bob(
        MESSAGABLE_LIST, 2, state_.at(BOB).at(MESSAGABLE_LIST).at(1));
    auto bobPayableListBTCDone = set_callback_bob(
        PAYABLE_LIST_BTC, 1, state_.at(BOB).at(PAYABLE_LIST_BTC).at(1));
    auto bobPayableListBCHDone = set_callback_bob(
        PAYABLE_LIST_BCH, 1, state_.at(BOB).at(PAYABLE_LIST_BCH).at(1));

    const auto& conversation =
        from_client.UI().ActivityThread(alice_nym_id_, contact_id_alice_bob_);
    conversation.SetDraft(firstMessage);

    EXPECT_EQ(conversation.GetDraft(), firstMessage);

    conversation.SendDraft();

    EXPECT_EQ(conversation.GetDraft(), "");
    EXPECT_TRUE(aliceActivitySummaryDone.get());
    EXPECT_TRUE(aliceActivityThreadDone.get());
    EXPECT_TRUE(bobContactListDone.get());
    EXPECT_TRUE(bobPayableListBTCDone.get());
    EXPECT_TRUE(bobPayableListBCHDone.get());
    EXPECT_TRUE(bobMessagableListDone.get());
    EXPECT_TRUE(bobActivitySummaryDone.get());
}

TEST_F(Test_Basic, activity_thread_bob_alice)
{
    Lock lock(callback_lock_);
    auto done = add_ui_widget_bob(
        ACTIVITY_THREAD_BOB_ALICE,
        bob_client_.UI()
            .ActivityThread(bob_nym_id_, contact_id_bob_alice_)
            .WidgetID(),
        3,
        state_.at(BOB).at(ACTIVITY_THREAD_BOB_ALICE).at(0));

    EXPECT_EQ(11, bob_widget_map_.size());
    EXPECT_EQ(11, bob_ui_names_.size());

    lock.unlock();

    EXPECT_TRUE(done.get());
}

TEST_F(Test_Basic, send_message_from_Bob_to_Alice_2)
{
    const auto& firstMessage = message_.at(msg_count_);
    const auto& from_client = bob_client_;
    const auto& from_nym_id_ = bob_nym_id_;
    const auto messageID = ++msg_count_;
    std::stringstream text{};
    text << BOB << " messaged " << ALICE << " with message #"
         << std::to_string(messageID);
    auto& secondMessage = message_[messageID];
    secondMessage = text.str();

    auto aliceActivitySummaryDone = set_callback_alice(
        ACTIVITY_SUMMARY, 4, state_.at(ALICE).at(ACTIVITY_SUMMARY).at(2));
    auto aliceActivityThreadDone = set_callback_alice(
        ACTIVITY_THREAD_ALICE_BOB,
        10,
        state_.at(ALICE).at(ACTIVITY_THREAD_ALICE_BOB).at(2));
    auto bobActivitySummaryDone = set_callback_bob(
        ACTIVITY_SUMMARY, 4, state_.at(BOB).at(ACTIVITY_SUMMARY).at(2));
    auto bobActivityThreadDone = set_callback_bob(
        ACTIVITY_THREAD_BOB_ALICE,
        8,
        state_.at(BOB).at(ACTIVITY_THREAD_BOB_ALICE).at(1));

    const auto& conversation =
        from_client.UI().ActivityThread(bob_nym_id_, contact_id_bob_alice_);
    conversation.SetDraft(secondMessage);

    EXPECT_EQ(conversation.GetDraft(), secondMessage);

    conversation.SendDraft();

    EXPECT_EQ(conversation.GetDraft(), "");
    EXPECT_TRUE(bobActivitySummaryDone.get());
    EXPECT_TRUE(bobActivityThreadDone.get());
    EXPECT_TRUE(aliceActivitySummaryDone.get());
    EXPECT_TRUE(aliceActivityThreadDone.get());
}

TEST_F(Test_Basic, issue_dollars)
{
    const auto contract = issuer_client_.Wallet().UnitDefinition(
        issuer_nym_id_->str(),
        UNIT_DEFINITION_CONTRACT_NAME,
        UNIT_DEFINITION_TERMS,
        UNIT_DEFINITION_PRIMARY_UNIT_NAME,
        UNIT_DEFINITION_SYMBOL,
        UNIT_DEFINITION_TLA,
        UNIT_DEFINITION_POWER,
        UNIT_DEFINITION_FRACTIONAL_UNIT_NAME);

    ASSERT_TRUE(contract);
    EXPECT_EQ(proto::UNITTYPE_CURRENCY, contract->Type());
    EXPECT_TRUE(unit_id_->empty());

    unit_id_->Assign(contract->ID());

    EXPECT_FALSE(unit_id_->empty());

    {
        auto issuer = issuer_client_.Wallet().mutable_Nym(issuer_nym_id_);
        issuer.AddPreferredOTServer(server_1_id_->str(), true);
        issuer.AddContract(unit_id_->str(), proto::CITEMTYPE_USD, true, true);
    }

    auto task = issuer_client_.OTX().IssueUnitDefinition(
        issuer_nym_id_, server_1_id_, unit_id_);
    auto& [taskID, future] = task;
    const auto result = future.get();

    EXPECT_NE(0, taskID);
    EXPECT_EQ(proto::LASTREPLYSTATUS_MESSAGESUCCESS, result.first);
    ASSERT_TRUE(result.second);

    issuer_account_id_->SetString(result.second->m_strAcctID);

    EXPECT_FALSE(issuer_account_id_->empty());

    issuer_client_.OTX().ContextIdle(issuer_nym_id_, server_1_id_).get();
}

TEST_F(Test_Basic, add_alice_contact_to_issuer)
{
    const auto alice = alice_client_.Factory().PaymentCode(alice_payment_code_);
    contact_id_issuer_alice_ =
        issuer_client_.Contacts().NymToContact(alice->ID());

    EXPECT_FALSE(contact_id_issuer_alice_->empty());

    issuer_client_.OTX().Refresh();
    issuer_client_.OTX().ContextIdle(issuer_nym_id_, server_1_id_).get();
}

TEST_F(Test_Basic, pay_alice)
{
    auto contactListDone = set_callback_alice(
        CONTACT_LIST, 6, state_.at(ALICE).at(CONTACT_LIST).at(2));
    auto messagableListDone = set_callback_alice(
        MESSAGABLE_LIST, 3, state_.at(ALICE).at(MESSAGABLE_LIST).at(2));
    auto activitySummaryDone = set_callback_alice(
        ACTIVITY_SUMMARY, 6, state_.at(ALICE).at(ACTIVITY_SUMMARY).at(3));
    auto payableBCHListDone = set_callback_alice(
        PAYABLE_LIST_BCH, 3, state_.at(ALICE).at(PAYABLE_LIST_BCH).at(2));

    auto task = issuer_client_.OTX().SendCheque(
        issuer_nym_id_,
        issuer_account_id_,
        contact_id_issuer_alice_,
        CHEQUE_AMOUNT_1,
        CHEQUE_MEMO);
    auto& [taskID, future] = task;

    ASSERT_NE(0, taskID);
    EXPECT_EQ(proto::LASTREPLYSTATUS_MESSAGESUCCESS, future.get().first);

    alice_client_.OTX().Refresh();
    alice_client_.OTX().ContextIdle(alice_nym_id_, server_1_id_).get();

    EXPECT_TRUE(contactListDone.get());
    EXPECT_TRUE(messagableListDone.get());
    EXPECT_TRUE(activitySummaryDone.get());
    EXPECT_TRUE(payableBCHListDone.get());
}

TEST_F(Test_Basic, contact_alice_issuer)
{
    auto issuerContactDone = add_ui_widget_alice(
        CONTACT_ALICE_ISSUER,
        alice_client_.UI().Contact(contact_id_alice_issuer_).WidgetID(),
        3,
        state_.at(ALICE).at(CONTACT_ALICE_ISSUER).at(0));

    EXPECT_EQ(12, alice_widget_map_.size());
    EXPECT_EQ(12, alice_ui_names_.size());
    EXPECT_TRUE(issuerContactDone.get());
}

TEST_F(Test_Basic, deposit_cheque_alice)
{
    auto activityThreadDone = add_ui_widget_alice(
        ACTIVITY_THREAD_ALICE_ISSUER,
        alice_client_.UI()
            .ActivityThread(alice_nym_id_, contact_id_alice_issuer_)
            .WidgetID(),
        3,
        state_.at(ALICE).at(ACTIVITY_THREAD_ALICE_ISSUER).at(0));

    EXPECT_EQ(13, alice_widget_map_.size());
    EXPECT_EQ(13, alice_ui_names_.size());

    auto accountListDone = set_callback_alice(
        ACCOUNT_LIST, 4, state_.at(ALICE).at(ACCOUNT_LIST).at(1));
    auto issuerContactDone = set_callback_alice(
        CONTACT_ALICE_ISSUER,
        9,
        state_.at(ALICE).at(CONTACT_ALICE_ISSUER).at(0));

    EXPECT_TRUE(activityThreadDone.get());

    const auto& thread =
        alice_->UI().ActivityThread(alice_nym_id_, contact_id_alice_issuer_);
    auto row = thread.First();

    ASSERT_TRUE(row->Valid());
    EXPECT_TRUE(row->Deposit());
    EXPECT_TRUE(accountListDone.get());
    EXPECT_TRUE(issuerContactDone.get());

    alice_client_.OTX().ContextIdle(alice_nym_id_, server_1_id_).get();
}

TEST_F(Test_Basic, account_activity_alice)
{
    auto accountActivityDone = add_ui_widget_alice(
        ACCOUNT_ACTIVITY_USD,
        alice_client_.UI()
            .AccountActivity(alice_nym_id_, alice_account_id_)
            .WidgetID(),
        4,
        state_.at(ALICE).at(ACCOUNT_ACTIVITY_USD).at(0));

    EXPECT_EQ(14, alice_widget_map_.size());
    EXPECT_EQ(14, alice_ui_names_.size());

    EXPECT_TRUE(accountActivityDone.get());
}

TEST_F(Test_Basic, process_inbox_issuer)
{
    auto task = issuer_client_.OTX().ProcessInbox(
        issuer_nym_id_, server_1_id_, issuer_account_id_);
    auto& [id, future] = task;

    ASSERT_NE(0, id);

    const auto [status, message] = future.get();

    EXPECT_EQ(proto::LASTREPLYSTATUS_MESSAGESUCCESS, status);
    ASSERT_TRUE(message);

    const auto account = issuer_client_.Wallet().Account(issuer_account_id_);

    EXPECT_EQ(-1 * CHEQUE_AMOUNT_1, account.get().GetBalance());
}

TEST_F(Test_Basic, pay_bob)
{
    auto aliceActivityThreadDone = set_callback_alice(
        ACTIVITY_THREAD_ALICE_BOB,
        15,
        state_.at(ALICE).at(ACTIVITY_THREAD_ALICE_BOB).at(3));
    auto aliceAccountActivityDone = set_callback_alice(
        ACCOUNT_ACTIVITY_USD,
        8,
        state_.at(ALICE).at(ACCOUNT_ACTIVITY_USD).at(1));
    auto aliceActivitySummaryDone = set_callback_alice(
        ACTIVITY_SUMMARY, 8, state_.at(ALICE).at(ACTIVITY_SUMMARY).at(4));
    auto bobActivityThreadDone = set_callback_bob(
        ACTIVITY_THREAD_BOB_ALICE,
        11,
        state_.at(BOB).at(ACTIVITY_THREAD_BOB_ALICE).at(2));
    auto bobActivitySummaryDone = set_callback_bob(
        ACTIVITY_SUMMARY, 6, state_.at(BOB).at(ACTIVITY_SUMMARY).at(3));
    auto issuerContactDone = set_callback_alice(
        CONTACT_ALICE_ISSUER,
        13,
        state_.at(ALICE).at(CONTACT_ALICE_ISSUER).at(0));

    auto& thread =
        alice_client_.UI().ActivityThread(alice_nym_id_, contact_id_alice_bob_);
    const auto sent = thread.Pay(
        CHEQUE_AMOUNT_2, alice_account_id_, CHEQUE_MEMO, PaymentType::Cheque);

    EXPECT_TRUE(sent);
    EXPECT_TRUE(aliceActivityThreadDone.get());
    EXPECT_TRUE(aliceAccountActivityDone.get());
    EXPECT_TRUE(aliceActivitySummaryDone.get());
    EXPECT_TRUE(bobActivityThreadDone.get());
    EXPECT_TRUE(bobActivitySummaryDone.get());
    EXPECT_TRUE(issuerContactDone.get());

    alice_client_.OTX().ContextIdle(alice_nym_id_, server_1_id_).get();
    bob_client_.OTX().ContextIdle(bob_nym_id_, server_1_id_).get();
}

TEST_F(Test_Basic, shutdown)
{
    alice_client_.OTX().ContextIdle(alice_nym_id_, server_1_id_).get();
    bob_client_.OTX().ContextIdle(bob_nym_id_, server_1_id_).get();
    issuer_client_.OTX().ContextIdle(issuer_nym_id_, server_1_id_).get();
}
}  // namespace
