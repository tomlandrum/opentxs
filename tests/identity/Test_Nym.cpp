// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "2_Factory.hpp"
#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "internal/api/client/Client.hpp"
#include "internal/identity/Identity.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/contact/ContactData.hpp"
#include "opentxs/contact/ContactGroup.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/contact/ContactSection.hpp"
#include "opentxs/core/PasswordPrompt.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/identity/Source.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/Enums.pb.h"
#include "opentxs/protobuf/Nym.pb.h"
#include "opentxs/protobuf/verify/Nym.hpp"

namespace ot = opentxs;

namespace
{
class Test_Nym : public ::testing::Test
{
public:
    const ot::api::client::internal::Manager& client_;
#if OT_STORAGE_FS
    const ot::api::client::internal::Manager& client_fs_;
#endif  // OT_STORAGE_FS
#if OT_STORAGE_SQLITE
    const ot::api::client::internal::Manager& client_sqlite_;
#endif  // OT_STORAGE_SQLITE
#if OT_STORAGE_LMDB
    const ot::api::client::internal::Manager& client_lmdb_;
#endif  // OT_STORAGE_LMDB
    const ot::OTPasswordPrompt reason_;

    bool test_nym(
        const ot::NymParameterType type,
        const ot::identity::CredentialType cred,
        const ot::proto::SourceType source,
        const std::string& name = "Nym")
    {
        const auto params = ot::NymParameters{type, cred, source};
        const auto pNym = client_.Wallet().Nym(reason_, name, params);

        if (false == bool(pNym)) { return false; }

        const auto& nym = *pNym;

        {
            EXPECT_EQ(name, nym.Alias());
            EXPECT_TRUE(nym.HasCapability(ot::NymCapability::SIGN_MESSAGE));
            EXPECT_TRUE(nym.HasCapability(ot::NymCapability::ENCRYPT_MESSAGE));
            EXPECT_TRUE(
                nym.HasCapability(ot::NymCapability::AUTHENTICATE_CONNECTION));
            EXPECT_TRUE(nym.HasCapability(ot::NymCapability::SIGN_CHILDCRED));
            EXPECT_EQ(1, nym.Revision());
            EXPECT_EQ(name, nym.Name());
            EXPECT_EQ(source, nym.Source().Type());
        }

        {
            const auto& claims = nym.Claims();
            const auto pSection =
                claims.Section(ot::proto::CONTACTSECTION_SCOPE);

            EXPECT_TRUE(pSection);

            if (false == bool(pSection)) { return false; }

            const auto& section = *pSection;

            EXPECT_EQ(1, section.Size());

            const auto pGroup = section.Group(ot::proto::CITEMTYPE_INDIVIDUAL);

            EXPECT_TRUE(pGroup);

            if (false == bool(pGroup)) { return false; }

            const auto& group = *pGroup;
            const auto pItem = group.PrimaryClaim();

            EXPECT_TRUE(pItem);

            if (false == bool(pItem)) { return false; }

            const auto& item = *pItem;

            EXPECT_EQ(name, item.Value());
        }

        return true;
    }

    bool test_storage(const ot::api::client::internal::Manager& api)
    {
        const auto reason = api.Factory().PasswordPrompt(__FUNCTION__);
        const auto alias = std::string{"alias"};
        std::unique_ptr<ot::identity::internal::Nym> pNym(ot::Factory::Nym(
            api, {}, ot::proto::CITEMTYPE_INDIVIDUAL, alias, reason));

        EXPECT_TRUE(pNym);

        if (!pNym) { return false; }

        auto& nym = *pNym;
        nym.SetAlias(alias);
        const auto id = ot::OTNymID{nym.ID()};

        EXPECT_TRUE(nym.VerifyPseudonym());

        {
            const auto serialized = nym.SerializeCredentialIndex(
                ot::identity::internal::Nym::Mode::Abbreviated);

            EXPECT_TRUE(ot::proto::Validate(serialized, ot::VERBOSE));
            EXPECT_TRUE(api.Storage().Store(serialized, nym.Alias()));
        }

        {
            const auto nymList = api.Storage().NymList();

            EXPECT_EQ(1, nymList.size());

            if (1 > nymList.size()) { return false; }

            const auto& item = *nymList.begin();

            EXPECT_EQ(item.first, id->str());
            EXPECT_EQ(item.second, alias);
        }

        {
            auto pSerialized = std::shared_ptr<ot::proto::Nym>{};

            EXPECT_TRUE(api.Storage().Load(id->str(), pSerialized));
            EXPECT_TRUE(pSerialized);

            if (!pSerialized) { return false; }

            const auto& serialized = *pSerialized;
            pNym.reset(ot::Factory::Nym(api, serialized, alias));

            EXPECT_TRUE(pNym);

            if (!pNym) { return false; }

            const auto& loadedNym = *pNym;

            EXPECT_TRUE(loadedNym.CompareID(id));
            EXPECT_TRUE(loadedNym.VerifyPseudonym());
        }

        return true;
    }

    Test_Nym()
        : client_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient({}, 0)))
#if OT_STORAGE_FS
        , client_fs_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient(
                  {{OPENTXS_ARG_STORAGE_PLUGIN, {"fs"}}},
                  1)))
#endif  // OT_STORAGE_FS
#if OT_STORAGE_SQLITE
        , client_sqlite_(
              dynamic_cast<const ot::api::client::internal::Manager&>(
                  ot::Context().StartClient(
                      {{OPENTXS_ARG_STORAGE_PLUGIN, {"sqlite"}}},
                      2)))
#endif  // OT_STORAGE_SQLITE
#if OT_STORAGE_LMDB
        , client_lmdb_(dynamic_cast<const ot::api::client::internal::Manager&>(
              ot::Context().StartClient(
                  {{OPENTXS_ARG_STORAGE_PLUGIN, {"lmdb"}}},
                  3)))
#endif  // OT_STORAGE_LMDB
        , reason_(client_.Factory().PasswordPrompt(__FUNCTION__))
    {
    }
};

TEST_F(Test_Nym, init_ot) {}

TEST_F(Test_Nym, storage_memdb) { EXPECT_TRUE(test_storage(client_)); }

#if OT_STORAGE_FS
TEST_F(Test_Nym, storage_fs) { EXPECT_TRUE(test_storage(client_fs_)); }
#endif  // OT_STORAGE_FS
#if OT_STORAGE_SQLITE
TEST_F(Test_Nym, storage_sqlite) { EXPECT_TRUE(test_storage(client_sqlite_)); }
#endif  // OT_STORAGE_SQLITE
#if OT_STORAGE_LMDB
TEST_F(Test_Nym, storage_lmdb) { EXPECT_TRUE(test_storage(client_lmdb_)); }
#endif  // OT_STORAGE_LMDB

TEST_F(Test_Nym, default_params)
{
    const auto pNym = client_.Wallet().Nym(reason_);

    ASSERT_TRUE(pNym);

    const auto& nym = *pNym;
    const auto& claims = nym.Claims();

    EXPECT_TRUE(nym.Alias().empty());
    EXPECT_TRUE(nym.HasCapability(ot::NymCapability::SIGN_MESSAGE));
    EXPECT_TRUE(nym.HasCapability(ot::NymCapability::ENCRYPT_MESSAGE));
    EXPECT_TRUE(nym.HasCapability(ot::NymCapability::AUTHENTICATE_CONNECTION));
    EXPECT_TRUE(nym.HasCapability(ot::NymCapability::SIGN_CHILDCRED));
    EXPECT_EQ(1, nym.Revision());
    EXPECT_TRUE(nym.Name().empty());

    const auto pSection = claims.Section(ot::proto::CONTACTSECTION_SCOPE);

    EXPECT_FALSE(pSection);
}

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#if OT_CRYPTO_WITH_BIP32
TEST_F(Test_Nym, secp256k1_hd_bip47)
{
    EXPECT_TRUE(test_nym(
        ot::NymParameterType::secp256k1,
        ot::identity::CredentialType::HD,
        ot::proto::SOURCETYPE_BIP47));
}

TEST_F(Test_Nym, secp256k1_hd_self_signed)
{
    EXPECT_TRUE(test_nym(
        ot::NymParameterType::secp256k1,
        ot::identity::CredentialType::HD,
        ot::proto::SOURCETYPE_PUBKEY));
}

TEST_F(Test_Nym, secp256k1_legacy_bip47)
{
    EXPECT_FALSE(test_nym(
        ot::NymParameterType::secp256k1,
        ot::identity::CredentialType::Legacy,
        ot::proto::SOURCETYPE_BIP47));
}
#endif  // OT_CRYPTO_WITH_BIP32

TEST_F(Test_Nym, secp256k1_legacy_self_signed)
{
    EXPECT_TRUE(test_nym(
        ot::NymParameterType::secp256k1,
        ot::identity::CredentialType::Legacy,
        ot::proto::SOURCETYPE_PUBKEY));
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1

#if OT_CRYPTO_SUPPORTED_KEY_ED25519
#if OT_CRYPTO_WITH_BIP32
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
TEST_F(Test_Nym, ed25519_hd_bip47)
{
    EXPECT_TRUE(test_nym(
        ot::NymParameterType::ed25519,
        ot::identity::CredentialType::HD,
        ot::proto::SOURCETYPE_BIP47));
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1

TEST_F(Test_Nym, ed25519_hd_self_signed)
{
    EXPECT_TRUE(test_nym(
        ot::NymParameterType::ed25519,
        ot::identity::CredentialType::HD,
        ot::proto::SOURCETYPE_PUBKEY));
}

TEST_F(Test_Nym, ed25519_legacy_bip47)
{
    EXPECT_FALSE(test_nym(
        ot::NymParameterType::ed25519,
        ot::identity::CredentialType::Legacy,
        ot::proto::SOURCETYPE_BIP47));
}
#endif  // OT_CRYPTO_WITH_BIP32

TEST_F(Test_Nym, ed25519_legacy_self_signed)
{
    EXPECT_TRUE(test_nym(
        ot::NymParameterType::ed25519,
        ot::identity::CredentialType::Legacy,
        ot::proto::SOURCETYPE_PUBKEY));
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519

#if OT_CRYPTO_SUPPORTED_KEY_RSA
TEST_F(Test_Nym, rsa_legacy_self_signed)
{
    EXPECT_TRUE(test_nym(
        ot::NymParameterType::rsa,
        ot::identity::CredentialType::Legacy,
        ot::proto::SOURCETYPE_PUBKEY));
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_RSA
}  // namespace
