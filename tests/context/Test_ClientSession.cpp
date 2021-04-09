// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <utility>

#include "OTLowLevelTestEnvironment.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/client/OTX.hpp"
#include "opentxs/api/server/Manager.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/otx/LastReplyStatus.hpp"

std::string nym_id_{};
std::string server_id_{};

TEST(ClientSession, create)
{
    const auto& otx = ot::InitContext(OTLowLevelTestEnvironment::test_args_);
    const auto& client = otx.StartClient({}, 0);
    const auto reason = client.Factory().PasswordPrompt(__FUNCTION__);
    const auto nym = client.Wallet().Nym(reason);

    ASSERT_TRUE(nym);

    nym_id_ = nym->ID().str();

    EXPECT_FALSE(nym_id_.empty());

    ot::Cleanup();
}

TEST(ClientSession, restart)
{
    const auto& otx = ot::InitContext(OTLowLevelTestEnvironment::test_args_);
    const auto& client = otx.StartClient({}, 0);
    const auto reason = client.Factory().PasswordPrompt(__FUNCTION__);
    const auto nym = client.Wallet().Nym(client.Factory().NymID(nym_id_));

    ASSERT_TRUE(nym);
    EXPECT_EQ(nym_id_, nym->ID().str());

    ot::Cleanup();
}

TEST(ClientSession, introduction_server)
{
    const auto& otx = ot::InitContext(OTLowLevelTestEnvironment::test_args_);
    const auto& server = otx.StartServer({}, 0);
    const auto& client = otx.StartClient({}, 0);
    const auto reasonS = server.Factory().PasswordPrompt(__FUNCTION__);
    const auto reasonC = client.Factory().PasswordPrompt(__FUNCTION__);
    const auto& serverID = server.ID();
    const auto nymID = client.Factory().NymID(nym_id_);
    server_id_ = serverID.str();

    EXPECT_FALSE(server_id_.empty());

    {
        const auto contract = server.Wallet().Server(serverID);
        const auto id = client.OTX().SetIntroductionServer(contract);

        EXPECT_EQ(serverID, id);
    }

    {
        auto [id, future] =
            client.OTX().RegisterNymPublic(nymID, serverID, true);

        ASSERT_NE(0, id);
        ASSERT_EQ(
            std::future_status::ready,
            future.wait_for(std::chrono::minutes(1)));

        const auto [status, reply] = future.get();

        EXPECT_EQ(ot::otx::LastReplyStatus::MessageSuccess, status);
        EXPECT_TRUE(reply);
    }

    client.OTX().ContextIdle(nymID, serverID).get();
    ot::Cleanup();
}

TEST(ClientSession, restart_after_registering)
{
    const auto& otx = ot::InitContext(OTLowLevelTestEnvironment::test_args_);
    const auto& server = otx.StartServer({}, 0);
    const auto& client = otx.StartClient({}, 0);
    const auto& serverID = server.ID();
    const auto nymID = client.Factory().NymID(nym_id_);
    client.OTX().DownloadNymbox(nymID, serverID);
    client.OTX().ContextIdle(nymID, serverID).get();
    ot::Cleanup();
}
