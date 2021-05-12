// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "internal/api/client/Client.hpp"
#include "internal/api/server/Server.hpp"
#include "internal/otx/OTX.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/OTX.hpp"
#include "opentxs/api/server/Manager.hpp"
#include "opentxs/client/OTAPI_Exec.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/otx/OTXPushType.hpp"
#include "opentxs/otx/Reply.hpp"
#include "opentxs/otx/Request.hpp"
#include "opentxs/otx/ServerReplyType.hpp"
#include "opentxs/otx/ServerRequestType.hpp"

using namespace opentxs;

namespace
{
bool init_{false};

class Test_Messages : public ::testing::Test
{
public:
    static const std::string SeedA_;
    static const std::string Alice_;
    static const OTNymID alice_nym_id_;

    const ot::api::client::internal::Manager& client_;
    const ot::api::server::internal::Manager& server_;
    ot::OTPasswordPrompt reason_c_;
    ot::OTPasswordPrompt reason_s_;
    const identifier::Server& server_id_;
    const OTServerContract server_contract_;

    Test_Messages()
        : client_(dynamic_cast<const ot::api::client::internal::Manager&>(
              Context().StartClient(OTTestEnvironment::test_args_, 0)))
        , server_(dynamic_cast<const ot::api::server::internal::Manager&>(
              Context().StartServer(OTTestEnvironment::test_args_, 0, true)))
        , reason_c_(client_.Factory().PasswordPrompt(__FUNCTION__))
        , reason_s_(server_.Factory().PasswordPrompt(__FUNCTION__))
        , server_id_(server_.ID())
        , server_contract_(server_.Wallet().Server(server_id_))
    {
        if (false == init_) { init(); }
    }

    void import_server_contract(
        const contract::Server& contract,
        const ot::api::client::Manager& client)
    {
        auto bytes = ot::Space{};
        server_contract_->Serialize(ot::writer(bytes), true);
        auto clientVersion = client.Wallet().Server(ot::reader(bytes));
        client.OTX().SetIntroductionServer(clientVersion);
    }

    void init()
    {
        const_cast<std::string&>(SeedA_) = client_.Exec().Wallet_ImportSeed(
            "spike nominee miss inquiry fee nothing belt list other "
            "daughter leave valley twelve gossip paper",
            "");
        const_cast<OTNymID&>(alice_nym_id_) =
            client_.Wallet().Nym(reason_c_, "Alice", {SeedA_, 0})->ID();
        const_cast<std::string&>(Alice_) = alice_nym_id_->str();

        OT_ASSERT(false == server_id_.empty());

        import_server_contract(server_contract_, client_);

        init_ = true;
    }
};

const std::string Test_Messages::SeedA_{""};
const std::string Test_Messages::Alice_{""};
const OTNymID Test_Messages::alice_nym_id_{identifier::Nym::Factory()};

TEST_F(Test_Messages, activateRequest)
{
    const otx::ServerRequestType type{otx::ServerRequestType::Activate};
    auto requestID = Identifier::Factory();
    const auto alice = client_.Wallet().Nym(alice_nym_id_);

    ASSERT_TRUE(alice);

    auto request = ot::otx::Request::Factory(
        client_, alice, server_id_, type, 1, reason_c_);

    ASSERT_TRUE(request->Nym());
    EXPECT_EQ(alice_nym_id_.get(), request->Nym()->ID());
    EXPECT_EQ(alice_nym_id_.get(), request->Initiator());
    EXPECT_EQ(server_id_, request->Server());
    EXPECT_EQ(type, request->Type());
    EXPECT_EQ(1, request->Number());

    requestID = request->ID();

    EXPECT_FALSE(requestID->empty());
    EXPECT_TRUE(request->Validate());

    EXPECT_EQ(ot::otx::Request::DefaultVersion, request->Version());
    EXPECT_EQ(requestID->str(), request->ID()->str());

    request->SetIncludeNym(true, reason_c_);

    EXPECT_TRUE(request->Validate());

    auto bytes = ot::Space{};
    EXPECT_TRUE(request->Contract(ot::writer(bytes)));

    const auto serverCopy =
        ot::otx::Request::Factory(server_, ot::reader(bytes));

    ASSERT_TRUE(serverCopy->Nym());
    EXPECT_EQ(alice_nym_id_.get(), serverCopy->Nym()->ID());
    EXPECT_EQ(alice_nym_id_.get(), serverCopy->Initiator());
    EXPECT_EQ(server_id_, serverCopy->Server());
    EXPECT_EQ(type, serverCopy->Type());
    EXPECT_EQ(1, serverCopy->Number());
    EXPECT_EQ(requestID.get(), serverCopy->ID());
    EXPECT_TRUE(serverCopy->Validate());
}

TEST_F(Test_Messages, pushReply)
{
    const std::string payload{"TEST PAYLOAD"};
    const otx::ServerReplyType type{otx::ServerReplyType::Push};
    auto replyID = Identifier::Factory();
    const auto server = server_.Wallet().Nym(server_.NymID());

    ASSERT_TRUE(server);

    auto reply = ot::otx::Reply::Factory(
        server_,
        server,
        alice_nym_id_,
        server_id_,
        type,
        true,
        1,
        reason_s_,
        ot::otx::OTXPushType::Nymbox,
        payload);

    ASSERT_TRUE(reply->Nym());
    EXPECT_EQ(server_.NymID(), reply->Nym()->ID());
    EXPECT_EQ(alice_nym_id_.get(), reply->Recipient());
    EXPECT_EQ(server_id_, reply->Server());
    EXPECT_EQ(type, reply->Type());
    EXPECT_EQ(1, reply->Number());
    EXPECT_TRUE(reply->Push());

    replyID = reply->ID();

    EXPECT_FALSE(replyID->empty());
    EXPECT_TRUE(reply->Validate());

    auto bytes = ot::Space{};
    EXPECT_TRUE(reply->Contract(ot::writer(bytes)));

    EXPECT_EQ(ot::otx::Reply::DefaultVersion, reply->Version());
    EXPECT_EQ(replyID->str(), reply->ID()->str());

    EXPECT_TRUE(reply->Validate());

    const auto aliceCopy = ot::otx::Reply::Factory(client_, ot::reader(bytes));

    ASSERT_TRUE(aliceCopy->Nym());
    EXPECT_EQ(server_.NymID(), aliceCopy->Nym()->ID());
    EXPECT_EQ(alice_nym_id_.get(), aliceCopy->Recipient());
    EXPECT_EQ(server_id_, aliceCopy->Server());
    EXPECT_EQ(type, aliceCopy->Type());
    EXPECT_EQ(1, aliceCopy->Number());
    EXPECT_EQ(replyID.get(), aliceCopy->ID());
    ASSERT_TRUE(aliceCopy->Push());
    EXPECT_TRUE(aliceCopy->Validate());
}
}  // namespace
