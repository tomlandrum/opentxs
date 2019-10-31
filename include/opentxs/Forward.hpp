// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_FORWARD_HPP
#define OPENTXS_FORWARD_HPP

#include "opentxs/Version.hpp"

#include "opentxs/Exclusive.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Shared.hpp"
#include "opentxs/SharedPimpl.hpp"

#include <type_traits>

namespace opentxs
{
namespace api
{
namespace client
{
#if OT_CRYPTO_SUPPORTED_KEY_HD
namespace blockchain
{
class BalanceList;
class BalanceNode;
class BalanceTree;
class Deterministic;
class Ethereum;
class HD;
class Imported;
#if OT_CRYPTO_SUPPORTED_SOURCE_BIP47
class PaymentCode;
#endif
}  // namespace blockchain
#endif  // OT_CRYPTO_SUPPORTED_KEY_HD

class Activity;
class Blockchain;
class Contacts;
class Issuer;
class Manager;
class OTX;
class Pair;
class ServerAction;
class UI;
class Workflow;
}  // namespace client

namespace crypto
{
class Asymmetric;
class Config;
class Crypto;
class Encode;
class Hash;
class Symmetric;
class Util;
}  // namespace crypto

namespace internal
{
struct Core;
}

namespace network
{
class Dht;
class ZAP;
class ZMQ;
}  // namespace network

namespace server
{
class Manager;
}  // namespace server

namespace storage
{
class Storage;
}  // namespace storage

class Context;
class Core;
class Crypto;
class Endpoints;
class Factory;
class HDSeed;
class Legacy;
class Context;
class Periodic;
class Settings;
class Wallet;
}  // namespace api

#if OT_CASH
namespace blind
{
class Mint;
class Purse;
class Token;
}  // namespace blind
#endif

#if OT_BLOCKCHAIN
namespace blockchain
{
namespace block
{
namespace bitcoin
{
class Header;
}  // namespace bitcoin

class Header;
}  // namespace block

namespace client
{
class HeaderOracle;
}  // namespace client

namespace p2p
{
class Address;
class Peer;
}  // namespace p2p

class BloomFilter;
class Network;
class NumericHash;
class Work;
}  // namespace blockchain
#endif  // OT_BLOCKCHAIN

namespace client
{
class ServerAction;
class Wallet;
}  // namespace client

namespace contract
{
namespace peer
{
namespace reply
{
class Acknowledgement;
class Bailment;
class Connection;
class Outbailment;
}  // namespace reply

namespace request
{
class Bailment;
class BailmentNotice;
class Connection;
class Outbailment;
class StoreSecret;
}  // namespace request

class Reply;
class Request;
}  // namespace peer

namespace unit
{
class Basket;
class Currency;
class Security;
}  // namespace unit

class Server;
class Signable;
class Unit;
}  // namespace contract

namespace crypto
{
namespace key
{
class Asymmetric;
class Ed25519;
class EllipticCurve;
class Keypair;
class HD;
class RSA;
class Secp256k1;
class Symmetric;
}  // namespace key

class AsymmetricProvider;
class Bip32;
class Bip39;
class Bitcoin;
class EcdsaProvider;
class EncodingProvider;
class HashingProvider;
class OpenSSL;
class Secp256k1;
class Sodium;
class SymmetricProvider;
class Trezor;
}  // namespace crypto

namespace identifier
{
class Account;
class Contact;
class Nym;
class Server;
class UnitDefinition;
}  // namespace identifier

namespace identity
{
namespace credential
{
class Base;
class Contact;
class Key;
class Primary;
class Secondary;
class Verification;
}  // namespace credential

namespace wot
{
namespace verification
{
class Group;
class Item;
class Nym;
class Set;
}  // namespace verification
}  // namespace wot

class Authority;
class Nym;
class Source;
}  // namespace identity

namespace network
{
namespace zeromq
{
namespace curve
{
class Client;
class Server;
}  // namespace curve

namespace socket
{
class Dealer;
class Pair;
class Publish;
class Pull;
class Push;
class Reply;
class Request;
class Router;
class Sender;
class Socket;
class Subscribe;
}  // namespace socket

namespace zap
{
class Callback;
class Handler;
class Reply;
class Request;
}  // namespace zap

class Context;
class FrameIterator;
class FrameSection;
class ListenCallback;
class Frame;
class Message;
class PairEventCallback;
class Pipeline;
class Proxy;
class ReplyCallback;
}  // namespace zeromq

class Dht;
class OpenDHT;
class ServerConnection;
class ZMQ;
}  // namespace network

namespace OTDB
{
class OfferListNym;
class OTPacker;
class TradeListMarket;
}  // namespace OTDB

namespace otx
{
class Request;
class Reply;
}  // namespace otx

namespace server
{
class MessageProcessor;
class Notary;
class Server;
class UserCommandProcessor;
}  // namespace server

namespace storage
{
class Root;
}  // namespace storage

namespace ui
{
class AccountList;
class AccountListItem;
class AccountActivity;
class AccountSummary;
class AccountSummaryItem;
class ActivityThread;
class ActivityThreadItem;
class ActivitySummary;
class ActivitySummaryItem;
class BalanceItem;
class Contact;
class ContactItem;
class ContactSection;
class ContactSubsection;
class ContactList;
class ContactListItem;
class IssuerItem;
class ListRow;
class MessagableList;
class PayableList;
class PayableListItem;
class Profile;
class ProfileItem;
class ProfileSection;
class ProfileSubsection;

#if OT_QT
class AccountActivityQt;
class AccountListQt;
class AccountSummaryQt;
class ActivitySummaryQt;
class ActivityThreadQt;
class ContactQt;
class ContactListQt;
class MessagableListQt;
class PayableListQt;
class ProfileQt;
#endif
}  // namespace ui

class Account;
class AccountList;
class AccountVisitor;
class Armored;
class Basket;
class BasketItem;
class Cheque;
class ClientContext;
class Contact;
class ContactData;
class ContactGroup;
class ContactItem;
class ContactSection;
class Context;
class Contract;
class Data;
class Factory;
class Flag;
class Identifier;
class Item;
class Ledger;
class Letter;
class ListenCallbackSwig;
class ManagedNumber;
class Message;
class NumList;
class NymData;
class NymFile;
class NymParameters;
class OT_API;
class OTAgent;
class OTAgreement;
class OTAPI_Exec;
class OTBylaw;
class OTCallback;
class OTCaller;
class OTClause;
class OTClient;
class OTCron;
class OTCronItem;
class OTDataFolder;
class OTEnvelope;
class OTMarket;
class OTNym_or_SymmetricKey;
class OTOffer;
class OTParty;
class OTPartyAccount;
class OTPassword;
class OTPayment;
class OTPaymentPlan;
class OTScript;
class OTScriptable;
class Signature;
class OTSignatureMetadata;
class OTSignedFile;
class OTSmartContract;
class OTStash;
class OTTrackable;
class OTTrade;
class OTTransaction;
class OTTransactionType;
class OTVariable;
class OTWallet;
class PairEventCallbackSwig;
class PasswordPrompt;
class PayDividendVisitor;
#if OT_CRYPTO_SUPPORTED_SOURCE_BIP47
class PaymentCode;
#endif
class PeerObject;
class PIDFile;
class ServerContext;
class Signals;
class StorageDriver;
class StoragePlugin;
class String;
class StringXML;
class Tag;
class TransactionStatement;
class UserCommandProcessor;

using OTArmored = Pimpl<Armored>;
using OTAsymmetricKey = Pimpl<crypto::key::Asymmetric>;
using OTBailmentNotice = SharedPimpl<contract::peer::request::BailmentNotice>;
using OTBailmentReply = SharedPimpl<contract::peer::reply::Bailment>;
using OTBailmentRequest = SharedPimpl<contract::peer::request::Bailment>;
using OTBasketContract = SharedPimpl<contract::unit::Basket>;
using OTConnectionReply = SharedPimpl<contract::peer::reply::Connection>;
using OTConnectionRequest = SharedPimpl<contract::peer::request::Connection>;
using OTCurrencyContract = SharedPimpl<contract::unit::Currency>;
using OTData = Pimpl<Data>;
using OTKeypair = Pimpl<crypto::key::Keypair>;
using OTFlag = Pimpl<Flag>;
using OTHDKey = Pimpl<crypto::key::HD>;
using OTIdentifier = Pimpl<Identifier>;
using OTManagedNumber = Pimpl<ManagedNumber>;
using OTNymID = Pimpl<identifier::Nym>;
using OTOutbailmentReply = SharedPimpl<contract::peer::reply::Outbailment>;
using OTOutbailmentRequest = SharedPimpl<contract::peer::request::Outbailment>;
using OTPasswordPrompt = Pimpl<PasswordPrompt>;
#if OT_CRYPTO_SUPPORTED_SOURCE_BIP47
using OTPaymentCode = Pimpl<PaymentCode>;
#endif
using OTPeerObject = Pimpl<PeerObject>;
using OTPeerReply = SharedPimpl<contract::peer::Reply>;
using OTPeerRequest = SharedPimpl<contract::peer::Request>;
using OTReplyAcknowledgement =
    SharedPimpl<contract::peer::reply::Acknowledgement>;
using OTSecurityContract = SharedPimpl<contract::unit::Security>;
using OTServerConnection = Pimpl<network::ServerConnection>;
using OTServerContract = SharedPimpl<contract::Server>;
using OTServerID = Pimpl<identifier::Server>;
using OTSignature = Pimpl<Signature>;
using OTStoreSecret = SharedPimpl<contract::peer::request::StoreSecret>;
using OTString = Pimpl<String>;
using OTStringXML = Pimpl<StringXML>;
using OTSymmetricKey = Pimpl<crypto::key::Symmetric>;
using OTUnitDefinition = SharedPimpl<contract::Unit>;
using OTUnitID = Pimpl<identifier::UnitDefinition>;
using OTXReply = Pimpl<otx::Reply>;
using OTXRequest = Pimpl<otx::Request>;
using OTZMQContext = Pimpl<network::zeromq::Context>;
using OTZMQDealerSocket = Pimpl<network::zeromq::socket::Dealer>;
using OTZMQListenCallback = Pimpl<network::zeromq::ListenCallback>;
using OTZMQFrame = Pimpl<network::zeromq::Frame>;
using OTZMQMessage = Pimpl<network::zeromq::Message>;
using OTZMQPairEventCallback = Pimpl<network::zeromq::PairEventCallback>;
using OTZMQPairSocket = Pimpl<network::zeromq::socket::Pair>;
using OTZMQPipeline = Pimpl<network::zeromq::Pipeline>;
using OTZMQProxy = Pimpl<network::zeromq::Proxy>;
using OTZMQPublishSocket = Pimpl<network::zeromq::socket::Publish>;
using OTZMQPullSocket = Pimpl<network::zeromq::socket::Pull>;
using OTZMQPushSocket = Pimpl<network::zeromq::socket::Push>;
using OTZMQReplyCallback = Pimpl<network::zeromq::ReplyCallback>;
using OTZMQReplySocket = Pimpl<network::zeromq::socket::Reply>;
using OTZMQRequestSocket = Pimpl<network::zeromq::socket::Request>;
using OTZMQRouterSocket = Pimpl<network::zeromq::socket::Router>;
using OTZMQSubscribeSocket = Pimpl<network::zeromq::socket::Subscribe>;
using OTZMQZAPCallback = Pimpl<network::zeromq::zap::Callback>;
using OTZMQZAPHandler = Pimpl<network::zeromq::zap::Handler>;
using OTZMQZAPReply = Pimpl<network::zeromq::zap::Reply>;
using OTZMQZAPRequest = Pimpl<network::zeromq::zap::Request>;

#if OT_BLOCKCHAIN
using OTBlockchainAddress = Pimpl<blockchain::p2p::Address>;
using OTBloomFilter = Pimpl<blockchain::BloomFilter>;
using OTNumericHash = Pimpl<blockchain::NumericHash>;
using OTWork = Pimpl<blockchain::Work>;
#endif  // OT_BLOCKCHAIN

#if OT_CASH
using OTPurse = Pimpl<blind::Purse>;
using OTToken = Pimpl<blind::Token>;
#endif

using ExclusiveAccount = Exclusive<Account>;

using SharedAccount = Shared<Account>;

using OTUIAccountListItem = SharedPimpl<ui::AccountListItem>;
using OTUIAccountSummaryItem = SharedPimpl<ui::AccountSummaryItem>;
using OTUIActivitySummaryItem = SharedPimpl<ui::ActivitySummaryItem>;
using OTUIActivityThreadItem = SharedPimpl<ui::ActivityThreadItem>;
using OTUIBalanceItem = SharedPimpl<ui::BalanceItem>;
using OTUIContactItem = SharedPimpl<ui::ContactItem>;
using OTUIContactListItem = SharedPimpl<ui::ContactListItem>;
using OTUIContactSection = SharedPimpl<ui::ContactSection>;
using OTUIContactSubsection = SharedPimpl<ui::ContactSubsection>;
using OTUIIssuerItem = SharedPimpl<ui::IssuerItem>;
using OTUIPayableListItem = SharedPimpl<ui::PayableListItem>;
using OTUIProfileItem = SharedPimpl<ui::ProfileItem>;
using OTUIProfileSection = SharedPimpl<ui::ProfileSection>;
using OTUIProfileSubsection = SharedPimpl<ui::ProfileSubsection>;
}  // namespace opentxs

namespace std
{
template <>
struct less<opentxs::OTData> {
    OPENTXS_EXPORT bool operator()(
        const opentxs::OTData& lhs,
        const opentxs::OTData& rhs) const;
};
template <>
struct less<opentxs::OTIdentifier> {
    OPENTXS_EXPORT bool operator()(
        const opentxs::OTIdentifier& lhs,
        const opentxs::OTIdentifier& rhs) const;
};
}  // namespace std
#endif
