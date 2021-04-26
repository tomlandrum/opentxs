// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <memory>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "opentxs/OT.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/server/Manager.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Ledger.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/core/identifier/Server.hpp"
#include "opentxs/identity/Nym.hpp"

ot::OTNymID nym_id_{ot::identifier::Nym::Factory()};
ot::OTServerID server_id_{ot::identifier::Server::Factory()};

namespace
{
struct Ledger : public ::testing::Test {
    const ot::api::client::Manager& client_;
    const ot::api::server::Manager& server_;
    ot::OTPasswordPrompt reason_c_;
    ot::OTPasswordPrompt reason_s_;

    Ledger()
        : client_(ot::Context().StartClient(OTTestEnvironment::test_args_, 0))
        , server_(
              ot::Context().StartServer(OTTestEnvironment::test_args_, 0, true))
        , reason_c_(client_.Factory().PasswordPrompt(__FUNCTION__))
        , reason_s_(server_.Factory().PasswordPrompt(__FUNCTION__))
    {
    }
};
}  // namespace

TEST_F(Ledger, init)
{
    nym_id_ = client_.Wallet().Nym(reason_c_, "Alice")->ID();

    ASSERT_FALSE(nym_id_->empty());

    const auto serverContract = server_.Wallet().Server(server_.ID());
    auto bytes = ot::Space{};
    serverContract->PublicContractToBytes(ot::writer(bytes));
    client_.Wallet().Server(bytes);
    server_id_->SetString(serverContract->ID()->str());

    ASSERT_FALSE(server_id_->empty());
}

TEST_F(Ledger, create_nymbox)
{
    const auto nym = client_.Wallet().Nym(nym_id_);

    ASSERT_TRUE(nym);

    auto nymbox = client_.Factory().Ledger(
        nym_id_, nym_id_, server_id_, ot::ledgerType::nymbox, true);

    ASSERT_TRUE(nymbox);

    nymbox->ReleaseSignatures();

    EXPECT_TRUE(nymbox->SignContract(*nym, reason_c_));
    EXPECT_TRUE(nymbox->SaveContract());
    EXPECT_TRUE(nymbox->SaveNymbox());
}

TEST_F(Ledger, load_nymbox)
{
    auto nymbox = client_.Factory().Ledger(
        nym_id_, nym_id_, server_id_, ot::ledgerType::nymbox, false);

    ASSERT_TRUE(nymbox);
    EXPECT_TRUE(nymbox->LoadNymbox());
}
