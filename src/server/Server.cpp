// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"       // IWYU pragma: associated
#include "1_Internal.hpp"     // IWYU pragma: associated
#include "server/Server.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <cstdint>
#include <list>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/OTStorage.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#if OT_CRYPTO_WITH_BIP32
#include "opentxs/api/HDSeed.hpp"
#endif  // OT_CRYPTO_WITH_BIP32
#include "opentxs/api/Settings.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/client/NymData.hpp"
#include "opentxs/core/AddressType.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Ledger.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/OTTransaction.hpp"
#include "opentxs/core/ProtocolVersion.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/cron/OTCron.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/crypto/Envelope.hpp"
#include "opentxs/crypto/Language.hpp"
#include "opentxs/crypto/SeedStyle.hpp"
#include "opentxs/ext/OTPayment.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"
#include "opentxs/network/zeromq/socket/Sender.tpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/OTXEnums.pb.h"
#include "opentxs/protobuf/OTXPush.pb.h"
#include "opentxs/protobuf/ServerContract.pb.h"
#include "server/ConfigLoader.hpp"
#include "server/MainFile.hpp"
#include "server/Transactor.hpp"

#define OTX_PUSH_VERSION 1
#define SEED_BACKUP_FILE "seed_backup.json"
#define SERVER_CONTRACT_FILE "NEW_SERVER_CONTRACT.otc"
#define SERVER_CONFIG_LISTEN_SECTION "listen"
#define SERVER_CONFIG_BIND_KEY "bindip"
#define SERVER_CONFIG_COMMAND_KEY "command"
#define SERVER_CONFIG_NOTIFY_KEY "notification"

#define OT_METHOD "opentxs::Server::"

namespace zmq = opentxs::network::zeromq;

namespace opentxs::server
{
Server::Server(
    const opentxs::api::server::internal::Manager& manager,
    const PasswordPrompt& reason)
    : manager_(manager)
    , reason_(reason)
    , mainFile_(*this, reason_)
    , notary_(*this, reason_, manager_)
    , transactor_(*this, reason_)
    , userCommandProcessor_(*this, reason_, manager_)
    , m_strWalletFilename(String::Factory())
    , m_bReadOnly(false)
    , m_bShutdownFlag(false)
    , m_notaryID(manager_.Factory().ServerID())
    , m_strServerNymID()
    , m_nymServer(nullptr)
    , m_Cron(manager.Factory().Cron())
    , notification_socket_(
          manager_.ZeroMQ().PushSocket(zmq::socket::Socket::Direction::Connect))
{
    const auto bound = notification_socket_->Start(
        manager_.Endpoints().InternalPushNotification());

    OT_ASSERT(bound);
}

void Server::ActivateCron()
{
    if (m_Cron->ActivateCron()) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Activate Cron. (STARTED)")
            .Flush();
    } else {
        LogNormal(OT_METHOD)(__FUNCTION__)(": Activate Cron. (FAILED)").Flush();
    }
}

/// Currently the test server calls this 10 times per second.
/// It also processes all the input/output at the same rate.
/// It sleeps in between. (See testserver.cpp for the call
/// and OTSleep() for the sleep code.)
///
void Server::ProcessCron()
{
    if (!m_Cron->IsActivated()) return;

    bool bAddedNumbers = false;

    // Cron requires transaction numbers in order to process.
    // So every time before I call Cron.Process(), I make sure to replenish
    // first.
    while (m_Cron->GetTransactionCount() < OTCron::GetCronRefillAmount()) {
        std::int64_t lTransNum = 0;
        bool bSuccess = transactor_.issueNextTransactionNumber(lTransNum);

        if (bSuccess) {
            m_Cron->AddTransactionNumber(lTransNum);
            bAddedNumbers = true;
        } else
            break;
    }

    if (bAddedNumbers) { m_Cron->SaveCron(); }

    m_Cron->ProcessCronItems();  // This needs to be called regularly for
                                 // trades, markets, payment plans, etc to
                                 // process.

    // NOTE:  TODO:  OTHER RE-OCCURRING SERVER FUNCTIONS CAN GO HERE AS WELL!!
    //
    // Such as sweeping server accounts after expiration dates, etc.
}

auto Server::GetServerID() const -> const identifier::Server&
{
    return m_notaryID;
}

auto Server::GetServerNym() const -> const identity::Nym&
{
    return *m_nymServer;
}

auto Server::IsFlaggedForShutdown() const -> bool { return m_bShutdownFlag; }

auto Server::parse_seed_backup(const std::string& input) const
    -> std::pair<std::string, std::string>
{
    std::pair<std::string, std::string> output{};
    auto& phrase = output.first;
    auto& words = output.second;

    std::regex reg("\"passphrase\": \"(.*)\", \"words\": \"(.*)\"");
    std::cmatch match{};

    if (std::regex_search(input.c_str(), match, reg)) {
        phrase = match[1];
        words = match[2];
    }

    return output;
}

void Server::CreateMainFile(bool& mainFileExists)
{
#if OT_CRYPTO_WITH_BIP32
    const auto backup = OTDB::QueryPlainString(
        manager_, manager_.DataFolder(), SEED_BACKUP_FILE, "", "", "");
    std::string seed{};

    if (false == backup.empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Seed backup found. Restoring.")
            .Flush();
        auto parsed = parse_seed_backup(backup);
        auto phrase = manager_.Factory().SecretFromText(parsed.first);
        auto words = manager_.Factory().SecretFromText(parsed.second);
        seed = manager_.Seeds().ImportSeed(
            words,
            phrase,
            crypto::SeedStyle::BIP39,
            crypto::Language::en,
            reason_);

        if (seed.empty()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Seed restoration failed.")
                .Flush();
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Seed ")(seed)(" restored.")
                .Flush();
        }
    }
#endif

    const std::string defaultName = DEFAULT_NAME;
    const std::string& userName = manager_.GetUserName();
    std::string name = userName;

    if (1 > name.size()) { name = defaultName; }

    auto nymParameters = NymParameters{};
#if OT_CRYPTO_WITH_BIP32
    nymParameters.SetSeed(seed);
    nymParameters.SetNym(0);
    nymParameters.SetDefault(false);
#endif
    m_nymServer = manager_.Wallet().Nym(
        reason_, name, nymParameters, proto::CITEMTYPE_SERVER);

    if (false == bool(m_nymServer)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error: Failed to create server nym.")
            .Flush();
        OT_FAIL;
    }

    if (!m_nymServer->VerifyPseudonym()) { OT_FAIL; }

    const auto& nymID = m_nymServer->ID();
    const std::string defaultTerms = "This is an example server contract.";
    const std::string& userTerms = manager_.GetUserTerms();
    std::string terms = userTerms;

    if (1 > userTerms.size()) { terms = defaultTerms; }

    const std::string defaultExternalIP = DEFAULT_EXTERNAL_IP;
    const std::string& userExternalIP = manager_.GetExternalIP();
    std::string hostname = userExternalIP;

    if (5 > hostname.size()) { hostname = defaultExternalIP; }

    const std::string defaultBindIP = DEFAULT_BIND_IP;
    const std::string& userBindIP = manager_.GetDefaultBindIP();
    std::string bindIP = userBindIP;

    if (5 > bindIP.size()) { bindIP = defaultBindIP; }

    bool notUsed = false;
    manager_.Config().Set_str(
        String::Factory(SERVER_CONFIG_LISTEN_SECTION),
        String::Factory(SERVER_CONFIG_BIND_KEY),
        String::Factory(bindIP),
        notUsed);

    const std::uint32_t defaultCommandPort = DEFAULT_COMMAND_PORT;
    const std::string& userCommandPort = manager_.GetCommandPort();
    std::uint32_t commandPort = 0;
    bool needPort = true;

    while (needPort) {
        try {
            commandPort = std::stoi(userCommandPort.c_str());
        } catch (const std::invalid_argument&) {
            commandPort = defaultCommandPort;
            needPort = false;
        } catch (const std::out_of_range&) {
            commandPort = defaultCommandPort;
            needPort = false;
        }
        commandPort =
            (MAX_TCP_PORT < commandPort) ? defaultCommandPort : commandPort;
        commandPort =
            (MIN_TCP_PORT > commandPort) ? defaultCommandPort : commandPort;
        needPort = false;
    }

    const std::string& userListenCommand = manager_.GetListenCommand();
    std::uint32_t listenCommand = 0;
    bool needListenCommand = true;

    while (needListenCommand) {
        try {
            listenCommand = std::stoi(userListenCommand.c_str());
        } catch (const std::invalid_argument&) {
            listenCommand = defaultCommandPort;
            needListenCommand = false;
        } catch (const std::out_of_range&) {
            listenCommand = defaultCommandPort;
            needListenCommand = false;
        }
        listenCommand =
            (MAX_TCP_PORT < listenCommand) ? defaultCommandPort : listenCommand;
        listenCommand =
            (MIN_TCP_PORT > listenCommand) ? defaultCommandPort : listenCommand;
        needListenCommand = false;
    }

    manager_.Config().Set_str(
        String::Factory(SERVER_CONFIG_LISTEN_SECTION),
        String::Factory(SERVER_CONFIG_COMMAND_KEY),
        String::Factory(std::to_string(listenCommand)),
        notUsed);

    const std::uint32_t defaultNotificationPort = DEFAULT_NOTIFY_PORT;

    const std::string& userListenNotification = manager_.GetListenNotify();
    std::uint32_t listenNotification = 0;
    bool needListenNotification = true;

    while (needListenNotification) {
        try {
            listenNotification = std::stoi(userListenNotification.c_str());
        } catch (const std::invalid_argument&) {
            listenNotification = defaultNotificationPort;
            needListenNotification = false;
        } catch (const std::out_of_range&) {
            listenNotification = defaultNotificationPort;
            needListenNotification = false;
        }
        listenNotification = (MAX_TCP_PORT < listenNotification)
                                 ? defaultNotificationPort
                                 : listenNotification;
        listenNotification = (MIN_TCP_PORT > listenNotification)
                                 ? defaultNotificationPort
                                 : listenNotification;
        needListenNotification = false;
    }

    manager_.Config().Set_str(
        String::Factory(SERVER_CONFIG_LISTEN_SECTION),
        String::Factory(SERVER_CONFIG_NOTIFY_KEY),
        String::Factory(std::to_string(listenNotification)),
        notUsed);
    auto endpoints = std::list<contract::Server::Endpoint>{};
    const bool useInproc = false == manager_.GetInproc().empty();

    if (useInproc) {
        LogNormal("Creating inproc contract for instance ")(
            manager_.GetInproc())
            .Flush();
        contract::Server::Endpoint inproc{
            core::AddressType::Inproc,
            core::ProtocolVersion::Legacy,
            manager_.GetInproc(),
            commandPort,
            2};
        endpoints.push_back(inproc);
    } else {
        LogNormal("Creating standard contract.").Flush();
        contract::Server::Endpoint ipv4{
            core::AddressType::IPV4,
            core::ProtocolVersion::Legacy,
            hostname,
            commandPort,
            1};
        endpoints.push_back(ipv4);
        const std::string& onion = manager_.GetOnion();

        if (0 < onion.size()) {
            contract::Server::Endpoint tor{
                core::AddressType::Onion,
                core::ProtocolVersion::Legacy,
                onion,
                commandPort,
                1};
            endpoints.push_back(tor);
        }

        const std::string& eep = manager_.GetEEP();

        if (0 < eep.size()) {
            contract::Server::Endpoint i2p{
                core::AddressType::EEP,
                core::ProtocolVersion::Legacy,
                eep,
                commandPort,
                1};
            endpoints.push_back(i2p);
        }
    }

    auto& wallet = manager_.Wallet();
    const auto existing = String::Factory(
        OTDB::QueryPlainString(
            manager_, manager_.DataFolder(), SERVER_CONTRACT_FILE, "", "", "")
            .data());

    if (false == existing->empty()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Existing contract found. Restoring.")
            .Flush();
    }

    const auto serialized =
        existing->empty()
            ? proto::ServerContract{}
            : proto::StringToProto<proto::ServerContract>(existing);
    const auto contract =
        existing->empty()
            ? wallet.Server(
                  nymID.str(),
                  name,
                  terms,
                  endpoints,
                  reason_,
                  (useInproc) ? std::max(2u, contract::Server::DefaultVersion)
                              : contract::Server::DefaultVersion)
            : wallet.Server(serialized);
    std::string strNotaryID{};
    std::string strHostname{};
    std::uint32_t nPort{0};
    core::AddressType type{};

    if (!contract->ConnectInfo(strHostname, nPort, type, type)) {
        LogNormal(OT_METHOD)(__FUNCTION__)(__FUNCTION__)(
            ": Unable to retrieve connection info from this contract.")
            .Flush();

        OT_FAIL;
    }

    strNotaryID = String::Factory(contract->ID())->Get();

    OT_ASSERT(m_nymServer)

    {
        auto nymData = manager_.Wallet().mutable_Nym(nymID, reason_);

        if (false == nymData.SetCommonName(contract->ID()->str(), reason_)) {
            OT_FAIL
        }
    }

    m_nymServer = manager_.Wallet().Nym(nymID);

    OT_ASSERT(m_nymServer)

    const auto signedContract =
        manager_.Factory().Data(contract->PublicContract());
    auto ascContract = manager_.Factory().Armored(signedContract);
    auto strBookended = String::Factory();
    ascContract->WriteArmoredString(strBookended, "SERVER CONTRACT");
    OTDB::StorePlainString(
        manager_,
        strBookended->Get(),
        manager_.DataFolder(),
        SERVER_CONTRACT_FILE,
        "",
        "",
        "");

    LogNormal("Your new server contract has been saved as ")(
        SERVER_CONTRACT_FILE)(" in the server data directory.")
        .Flush();

#if OT_CRYPTO_WITH_BIP32
    const auto seedID = manager_.Storage().DefaultSeed();
    const auto words = manager_.Seeds().Words(seedID, reason_);
    const auto passphrase = manager_.Seeds().Passphrase(seedID, reason_);
#else
    const std::string words;
    const std::string passphrase;
#endif

    std::string json;
    json += "{ \"passphrase\": \"";
    json += passphrase;
    json += "\", \"words\": \"";
    json += words;
    json += "\" }\n";

    OTDB::StorePlainString(
        manager_, json, manager_.DataFolder(), SEED_BACKUP_FILE, "", "", "");

    mainFileExists =
        mainFile_.CreateMainFile(strBookended->Get(), strNotaryID, nymID.str());

    manager_.Config().Save();
}

void Server::Init(bool readOnly)
{
    m_bReadOnly = readOnly;

    if (!ConfigLoader::load(manager_, manager_.Config(), WalletFilename())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to Load Config File!")
            .Flush();
        OT_FAIL;
    }

    OTDB::InitDefaultStorage(OTDB_DEFAULT_STORAGE, OTDB_DEFAULT_PACKER);

    // Load up the transaction number and other Server data members.
    bool mainFileExists = WalletFilename().Exists()
                              ? OTDB::Exists(
                                    manager_,
                                    manager_.DataFolder(),
                                    ".",
                                    WalletFilename().Get(),
                                    "",
                                    "")
                              : false;

    if (false == mainFileExists) {
        if (readOnly) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Error: Main file non-existent (")(WalletFilename().Get())(
                "). Plus, unable to create, since read-only flag is set.")
                .Flush();
            OT_FAIL;
        } else {
            CreateMainFile(mainFileExists);
        }
    }

    if (mainFileExists) {
        if (false == mainFile_.LoadMainFile(readOnly)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Error in Loading Main File, re-creating.")
                .Flush();
            OTDB::EraseValueByKey(
                manager_,
                manager_.DataFolder(),
                ".",
                WalletFilename().Get(),
                "",
                "");
            CreateMainFile(mainFileExists);

            OT_ASSERT(mainFileExists);

            if (!mainFile_.LoadMainFile(readOnly)) { OT_FAIL; }
        }
    }

    auto password = manager_.Crypto().Encode().Nonce(16);
    auto notUsed = String::Factory();
    bool ignored;
    manager_.Config().CheckSet_str(
        String::Factory("permissions"),
        String::Factory("admin_password"),
        password,
        notUsed,
        ignored);
    manager_.Config().Save();

    // With the Server's private key loaded, and the latest transaction number
    // loaded, and all the various other data (contracts, etc) the server is now
    // ready for operation!
}

auto Server::LoadServerNym(const identifier::Nym& nymID) -> bool
{
    auto nym = manager_.Wallet().Nym(nymID);

    if (false == bool(nym)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Server nym does not exist.")
            .Flush();

        return false;
    }

    m_nymServer = nym;

    OT_ASSERT(m_nymServer);

    return true;
}

// msg, the request msg from payer, which is attached WHOLE to the Nymbox
// receipt. contains payment already. or pass pPayment instead: we will create
// our own msg here (with payment inside) to be attached to the receipt.
// szCommand for passing payDividend (as the message command instead of
// sendNymInstrument, the default.)
auto Server::SendInstrumentToNym(
    const identifier::Server& NOTARY_ID,
    const identifier::Nym& SENDER_NYM_ID,
    const identifier::Nym& RECIPIENT_NYM_ID,
    const OTPayment& pPayment,
    const char* szCommand) -> bool
{
    OT_ASSERT(pPayment.IsValid());

    // If a payment was passed in (for us to use it to construct pMsg, which is
    // nullptr in the case where payment isn't nullptr)
    // Then we grab it in string form, so we can pass it on...
    auto strPayment = String::Factory();
    const bool bGotPaymentContents = pPayment.GetPaymentContents(strPayment);

    if (!bGotPaymentContents) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Error GetPaymentContents Failed!")
            .Flush();
    }

    const bool bDropped = DropMessageToNymbox(
        NOTARY_ID,
        SENDER_NYM_ID,
        RECIPIENT_NYM_ID,
        transactionType::instrumentNotice,
        nullptr,
        strPayment,
        szCommand);

    return bDropped;
}

auto Server::SendInstrumentToNym(
    const identifier::Server& NOTARY_ID,
    const identifier::Nym& SENDER_NYM_ID,
    const identifier::Nym& RECIPIENT_NYM_ID,
    const Message& pMsg) -> bool
{
    return DropMessageToNymbox(
        NOTARY_ID,
        SENDER_NYM_ID,
        RECIPIENT_NYM_ID,
        transactionType::instrumentNotice,
        pMsg);
}

auto Server::DropMessageToNymbox(
    const identifier::Server& notaryID,
    const identifier::Nym& senderNymID,
    const identifier::Nym& recipientNymID,
    transactionType transactionType,
    const Message& msg) -> bool
{
    return DropMessageToNymbox(
        notaryID, senderNymID, recipientNymID, transactionType, &msg);
}

// Can't be static (transactor_.issueNextTransactionNumber is called...)
//
// About pMsg...
// (Normally) when you send a cheque to someone, you encrypt it inside an
// envelope, and that
// envelope is attached to a OTMessage (sendNymInstrument) and sent to the
// server. The server
// takes your entire OTMessage and attaches it to an instrumentNotice
// (OTTransaction) which is
// added to the recipient's Nymbox.
// In that case, just pass the pointer to the incoming message here as pMsg, and
// the OT Server
// will attach it as described.
// But let's say you are paying a dividend. The server can't just attach your
// dividend request in
// that case. Normally the recipient's cheque is already in the request. But
// with a dividend, there
// could be a thousand recipients, and their individual vouchers are only
// generated and sent AFTER
// the server receives the "pay dividend" request.
// Therefore in that case, nullptr would be passed for pMsg, meaning that inside
// this function we have
// to generate our own OTMessage "from the server" instead of "from the sender".
// After all, the server's
// private key is the only signer we have in here. And the recipient will be
// expecting to have to
// open a message, so we must create one to give him. So if pMsg is nullptr,
// then
// this function will
// create a message "from the server", containing the instrument, and drop it
// into the recipient's nymbox
// as though it were some incoming message from a normal user.
// This message, in the case of payDividend, should be an "payDividendResponse"
// message, "from" the server
// and "to" the recipient. The payment instrument must be attached to that new
// message, and therefore it
// must be passed into this function.
// Of course, if pMsg was not nullptr, that means the message (and instrument
// inside of it) already exist,
// so no instrument would need to be passed. But if pMsg IS nullptr, that means
// the
// msg must be generated,
// and thus the instrument MUST be passed in, so that that can be done.
// Therefore the instrument will sometimes be passed in, and sometimes not.
// Therefore the instrument must
// be passed as a pointer.
//
// Conclusion: if pMsg is passed in, then pass a null instrument. (Since the
// instrument is already on pMsg.)
//                (And since the instrument defaults to nullptr, this makes pMsg
// the final argument in the call.)
//         but if pMsg is nullptr, then you must pass the payment instrument as
// the
// next argument. (So pMsg can be created with it.)
// Note: you cannot pass BOTH, or the instrument will simply be ignored, since
// it's already assumed to be in pMsg.
// You might ask: what about the original request then, doesn't the recipient
// get a copy of that? Well, maybe we
// pass it in here and attach it to the new message. Or maybe we just set it as
// the voucher memo.
//
auto Server::DropMessageToNymbox(
    const identifier::Server& NOTARY_ID,
    const identifier::Nym& SENDER_NYM_ID,
    const identifier::Nym& RECIPIENT_NYM_ID,
    transactionType theType,
    const Message* pMsg,
    const String& pstrMessage,
    const char* szCommand) -> bool  // If you pass something here, it will
{                                   // replace pMsg->m_strCommand below.
    OT_ASSERT_MSG(
        !((nullptr == pMsg) && (pstrMessage.empty())),
        "pMsg and pstrMessage -- these can't BOTH be nullptr.\n");
    // ^^^ Must provde one or the other.
    OT_ASSERT_MSG(
        !((nullptr != pMsg) && (!pstrMessage.empty())),
        "pMsg and pstrMessage -- these can't BOTH be not-nullptr.\n");
    // ^^^ Can't provide both.
    std::int64_t lTransNum{0};
    const bool bGotNextTransNum =
        transactor_.issueNextTransactionNumber(lTransNum);

    if (!bGotNextTransNum) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Error: Failed trying to get next transaction number.")
            .Flush();
        return false;
    }
    switch (theType) {
        case transactionType::message:
            break;
        case transactionType::instrumentNotice:
            break;
        default:
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Unexpected transactionType passed here (Expected message "
                "or instrumentNotice).")
                .Flush();
            return false;
    }
    // If pMsg was not already passed in here, then
    // create pMsg using pstrMessage.
    //
    std::unique_ptr<Message> theMsgAngel;
    const Message* message{nullptr};

    if (nullptr == pMsg) {
        theMsgAngel.reset(manager_.Factory().Message().release());

        if (nullptr != szCommand)
            theMsgAngel->m_strCommand = String::Factory(szCommand);
        else {
            switch (theType) {
                case transactionType::message:
                    theMsgAngel->m_strCommand =
                        String::Factory("sendNymMessage");
                    break;
                case transactionType::instrumentNotice:
                    theMsgAngel->m_strCommand =
                        String::Factory("sendNymInstrument");
                    break;
                default:
                    break;  // should never happen.
            }
        }
        theMsgAngel->m_strNotaryID = String::Factory(m_notaryID);
        theMsgAngel->m_bSuccess = true;
        SENDER_NYM_ID.GetString(theMsgAngel->m_strNymID);
        RECIPIENT_NYM_ID.GetString(
            theMsgAngel->m_strNymID2);  // set the recipient ID
                                        // in theMsgAngel to match our
                                        // recipient ID.
        // Load up the recipient's public key (so we can encrypt the envelope
        // to him that will contain the payment instrument.)
        //
        auto nymRecipient = manager_.Wallet().Nym(RECIPIENT_NYM_ID);

        // Wrap the message up into an envelope and attach it to theMsgAngel.
        auto theEnvelope = manager_.Factory().Envelope();
        theMsgAngel->m_ascPayload->Release();

        if ((!pstrMessage.empty()) &&
            theEnvelope->Seal(
                *nymRecipient, pstrMessage.Bytes(), reason_) &&  // Seal
                                                                 // pstrMessage
                                                                 // into
                                                                 // theEnvelope,
            // using nymRecipient's
            // public key.
            theEnvelope->Armored(theMsgAngel->m_ascPayload))  // Grab the
                                                              // sealed
                                                              // version as
        // base64-encoded string, into
        // theMsgAngel->m_ascPayload.
        {
            theMsgAngel->SignContract(*m_nymServer, reason_);
            theMsgAngel->SaveContract();
        } else {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed trying to seal envelope containing theMsgAngel "
                "(or while grabbing the base64-encoded result).")
                .Flush();
            return false;
        }

        // By this point, pMsg is all set up, signed and saved. Its payload
        // contains
        // the envelope (as base64) containing the encrypted message.

        message = theMsgAngel.get();
    } else {
        message = pMsg;
    }
    //  else // pMsg was passed in, so it's not nullptr. No need to create it
    // ourselves like above. (pstrMessage should be nullptr anyway in this
    // case.)
    //  {
    //       // Apparently no need to do anything in here at all.
    //  }
    // Grab a string copy of message.
    //
    const auto strInMessage = String::Factory(*message);
    auto theLedger{manager_.Factory().Ledger(
        RECIPIENT_NYM_ID, RECIPIENT_NYM_ID, NOTARY_ID)};  // The
                                                          // recipient's
                                                          // Nymbox.
    // Drop in the Nymbox
    if ((theLedger->LoadNymbox() &&  // I think this loads the box
                                     // receipts too, since I didn't call
                                     // "LoadNymboxNoVerify"
         //          theLedger.VerifyAccount(m_nymServer)    &&    // This loads
         // all the Box Receipts, which is unnecessary.
         theLedger->VerifyContractID() &&  // Instead, we'll verify the IDs and
                                           // Signature only.
         theLedger->VerifySignature(*m_nymServer))) {
        // Create the instrumentNotice to put in the Nymbox.
        auto pTransaction{manager_.Factory().Transaction(
            *theLedger, theType, originType::not_applicable, lTransNum)};

        if (false != bool(pTransaction))  // The above has an OT_ASSERT within,
                                          // but I just like to check my
                                          // pointers.
        {
            // NOTE: todo: SHOULD this be "in reference to" itself? The reason,
            // I assume we are doing this
            // is because there is a reference STRING so "therefore" there must
            // be a reference # as well. Eh?
            // Anyway, it must be understood by those involved that a message is
            // stored inside. (Which has no transaction #.)

            pTransaction->SetReferenceToNum(lTransNum);  // <====== Recipient
                                                         // RECEIVES entire
                                                         // incoming message as
                                                         // string here, which
                                                         // includes the sender
                                                         // user ID,
            pTransaction->SetReferenceString(
                strInMessage);  // and has an OTEnvelope in the payload. Message
            // is signed by sender, and envelope is encrypted
            // to recipient.

            pTransaction->SignContract(*m_nymServer, reason_);
            pTransaction->SaveContract();
            std::shared_ptr<OTTransaction> transaction{pTransaction.release()};
            theLedger->AddTransaction(transaction);  // Add the message
                                                     // transaction to the
                                                     // nymbox. (It will
                                                     // cleanup.)

            theLedger->ReleaseSignatures();
            theLedger->SignContract(*m_nymServer, reason_);
            theLedger->SaveContract();
            theLedger->SaveNymbox(
                manager_.Factory().Identifier());  // We don't grab the
                                                   // Nymbox hash here,
                                                   // since
            // nothing important changed (just a message
            // was sent.)

            // Any inbox/nymbox/outbox ledger will only itself contain
            // abbreviated versions of the receipts, including their hashes.
            //
            // The rest is stored separately, in the box receipt, which is
            // created
            // whenever a receipt is added to a box, and deleted after a receipt
            // is removed from a box.
            //
            transaction->SaveBoxReceipt(*theLedger);
            notification_socket_->Send(
                nymbox_push(RECIPIENT_NYM_ID, *transaction));

            return true;
        } else  // should never happen
        {
            const auto strRecipientNymID = String::Factory(RECIPIENT_NYM_ID);
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed while trying to generate transaction in order to "
                "add a message to Nymbox: ")(strRecipientNymID->Get())(".")
                .Flush();
        }
    } else {
        const auto strRecipientNymID = String::Factory(RECIPIENT_NYM_ID);
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed while trying to load or verify Nymbox: ")(
            strRecipientNymID->Get())(".")
            .Flush();
    }

    return false;
}

auto Server::GetConnectInfo(
    core::AddressType& type,
    std::string& strHostname,
    std::uint32_t& nPort) const -> bool
{
    auto contract = manager_.Wallet().Server(m_notaryID);
    std::string contractHostname{};
    std::uint32_t contractPort{};
    const auto haveEndpoints =
        contract->ConnectInfo(contractHostname, contractPort, type, type);

    OT_ASSERT(haveEndpoints)

    bool notUsed = false;
    std::int64_t port = 0;
    const bool haveIP = manager_.Config().CheckSet_str(
        String::Factory(SERVER_CONFIG_LISTEN_SECTION),
        String::Factory("bindip"),
        String::Factory(DEFAULT_BIND_IP),
        strHostname,
        notUsed);
    const bool havePort = manager_.Config().CheckSet_long(
        String::Factory(SERVER_CONFIG_LISTEN_SECTION),
        String::Factory(SERVER_CONFIG_COMMAND_KEY),
        DEFAULT_COMMAND_PORT,
        port,
        notUsed);
    port = (MAX_TCP_PORT < port) ? DEFAULT_COMMAND_PORT : port;
    port = (MIN_TCP_PORT > port) ? DEFAULT_COMMAND_PORT : port;
    nPort = static_cast<std::uint32_t>(port);
    manager_.Config().Save();

    return (haveIP && havePort);
}

auto Server::nymbox_push(
    const identifier::Nym& nymID,
    const OTTransaction& item) const -> OTZMQMessage
{
    auto output = zmq::Message::Factory();
    output->AddFrame(nymID.str());
    proto::OTXPush push;
    push.set_version(OTX_PUSH_VERSION);
    push.set_type(proto::OTXPUSH_NYMBOX);
    push.set_item(String::Factory(item)->Get());
    output->AddFrame(push);

    return output;
}

auto Server::TransportKey(Data& pubkey) const -> OTSecret
{
    return manager_.Wallet().Server(m_notaryID)->TransportKey(pubkey, reason_);
}

Server::~Server() = default;
}  // namespace opentxs::server
