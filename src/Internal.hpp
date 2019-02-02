// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "opentxs/Forward.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
namespace internal
{
struct Activity;
struct Contacts;
struct Manager;
struct Operation;
}  // namespace internal
}  // namespace client

namespace implementation
{
class Core;
class Native;
class Storage;
}  // namespace implementation

namespace network
{
namespace implementation
{
class Context;
class Dht;
class ZMQ;
}  // namespace implementation
}  // namespace network

namespace internal
{
struct Log;
struct Native;
}  // namespace internal

namespace server
{
namespace implementation
{
class Manager;
}  // namespace implementation
}  // namespace server

namespace storage
{
class Driver;
class Multiplex;
class Plugin;
class StorageInternal;
}  // namespace storage
}  // namespace api

namespace blind
{
namespace implementation
{
class Purse;
}  // namespace implementation

namespace token
{
namespace implementation
{
class Token;
}  // namespace implementation
}  // namespace token
}  // namespace blind

namespace crypto
{
class Ripemd160;

namespace implementation
{
class OpenSSL;
}  // namespace implementation
}  // namespace crypto

namespace internal
{
struct ClientContext;
struct Context;
struct NymFile;
struct ServerContext;
}  // namespace internal

namespace network
{
namespace zeromq
{
namespace implementation
{
class Context;
class Proxy;
}  // namespace implementation
}  // namespace zeromq
}  // namespace network

namespace rpc
{
namespace internal
{
struct RPC;
}  // namespace internal
}  // namespace rpc

namespace server
{
class ReplyMessage;
}  // namespace server

namespace storage
{
namespace implementation
{
class StorageMultiplex;
}  // namespace implementation

class Accounts;
class Bip47Channels;
class BlockchainTransactions;
class Contacts;
class Contexts;
class Credentials;
class Issuers;
class Mailbox;
class Notary;
class Nym;
class Nyms;
class PaymentWorkflows;
class PeerReplies;
class PeerRequests;
class Seeds;
class Servers;
class Thread;
class Threads;
class Tree;
class Units;
}  // namespace storage

namespace ui
{
namespace internal
{
struct AccountActivity;
struct AccountSummary;
struct AccountSummaryItem;
struct ActivityThread;
struct ActivityThreadItem;
struct ActivitySummary;
struct ActivitySummaryItem;
struct BalanceItem;
struct Contact;
struct ContactItem;
struct ContactList;
struct ContactListItem;
struct ContactSection;
struct ContactSubsection;
struct IssuerItem;
struct PayableListItem;
struct Profile;
struct ProfileItem;
struct ProfileSection;
struct ProfileSubsection;
}  // namespace internal

namespace implementation
{
class AccountActivity;
class AccountSummary;
class AccountSummaryItemBlank;
class ActivitySummary;
class ActivitySummaryItemBlank;
class ActivityThread;
class ActivityThreadItemBlank;
class BalanceItemBlank;
class Contact;
class ContactItemBlank;
class ContactListItemBlank;
class ContactSection;
class ContactSectionBlank;
class ContactSubsection;
class ContactSubsectionBlank;
class IssuerItemBlank;
class MessagableList;
class PayableList;
class PayableListItemBlank;
class Profile;
class ProfileItemBlank;
class ProfileSection;
class ProfileSectionBlank;
class ProfileSubsection;
class ProfileSubsectionBlank;

using CustomData = std::vector<const void*>;

// Account activity
using AccountActivityExternalInterface = ui::AccountActivity;
using AccountActivityInternalInterface = ui::internal::AccountActivity;
using AccountActivityRowID = std::pair<OTIdentifier, proto::PaymentEventType>;
using AccountActivityRowInterface = ui::BalanceItem;
using AccountActivityRowInternal = ui::internal::BalanceItem;
using AccountActivityRowBlank = BalanceItemBlank;
/** WorkflowID, state */
using AccountActivitySortKey = std::chrono::system_clock::time_point;

// Account summary
using AccountSummaryExternalInterface = ui::AccountSummary;
using AccountSummaryInternalInterface = ui::internal::AccountSummary;
using AccountSummaryRowID = OTIdentifier;
using AccountSummaryRowInterface = ui::IssuerItem;
using AccountSummaryRowInternal = ui::internal::IssuerItem;
using AccountSummaryRowBlank = IssuerItemBlank;
using AccountSummarySortKey = std::pair<bool, std::string>;

using IssuerItemExternalInterface = AccountSummaryRowInterface;
using IssuerItemInternalInterface = ui::internal::IssuerItem;
using IssuerItemRowID = std::pair<OTIdentifier, proto::ContactItemType>;
using IssuerItemRowInterface = ui::AccountSummaryItem;
using IssuerItemRowInternal = ui::internal::AccountSummaryItem;
using IssuerItemRowBlank = AccountSummaryItemBlank;
using IssuerItemSortKey = std::string;

// Activity summary
using ActivitySummaryExternalInterface = ui::ActivitySummary;
using ActivitySummaryInternalInterface = ui::internal::ActivitySummary;
using ActivitySummaryRowID = OTIdentifier;
using ActivitySummaryRowInterface = ui::ActivitySummaryItem;
using ActivitySummaryRowInternal = ui::internal::ActivitySummaryItem;
using ActivitySummaryRowBlank = ActivitySummaryItemBlank;
using ActivitySummarySortKey =
    std::pair<std::chrono::system_clock::time_point, std::string>;

// Activity thread
using ActivityThreadExternalInterface = ui::ActivityThread;
using ActivityThreadInternalInterface = ui::internal::ActivityThread;
/** item id, box, accountID, taskID */
using ActivityThreadRowID =
    std::tuple<OTIdentifier, StorageBox, OTIdentifier, int>;
using ActivityThreadRowInterface = ui::ActivityThreadItem;
using ActivityThreadRowInternal = ui::internal::ActivityThreadItem;
using ActivityThreadRowBlank = ActivityThreadItemBlank;
/** timestamp, index */
using ActivityThreadSortKey =
    std::pair<std::chrono::system_clock::time_point, std::uint64_t>;

// Contact
using ContactExternalInterface = ui::Contact;
using ContactInternalInterface = ui::internal::Contact;
using ContactRowID = proto::ContactSectionName;
using ContactRowInterface = ui::ContactSection;
using ContactRowInternal = ui::internal::ContactSection;
using ContactRowBlank = ContactSectionBlank;
using ContactSortKey = int;

using ContactSectionExternalInterface = ContactRowInterface;
using ContactSectionInternalInterface = ui::internal::ContactSection;
using ContactSectionRowID =
    std::pair<proto::ContactSectionName, proto::ContactItemType>;
using ContactSectionRowInterface = ui::ContactSubsection;
using ContactSectionRowInternal = ui::internal::ContactSubsection;
using ContactSectionRowBlank = ContactSubsectionBlank;
using ContactSectionSortKey = int;

using ContactSubsectionExternalInterface = ContactSectionRowInterface;
using ContactSubsectionInternalInterface = ui::internal::ContactSubsection;
using ContactSubsectionRowID = OTIdentifier;
using ContactSubsectionRowInterface = ui::ContactItem;
using ContactSubsectionRowInternal = ui::internal::ContactItem;
using ContactSubsectionRowBlank = ContactItemBlank;
using ContactSubsectionSortKey = int;

// Contact list
using ContactListExternalInterface = ui::ContactList;
using ContactListInternalInterface = ui::internal::ContactList;
using ContactListRowID = OTIdentifier;
using ContactListRowInterface = ui::ContactListItem;
using ContactListRowInternal = ui::internal::ContactListItem;
using ContactListRowBlank = ContactListItemBlank;
using ContactListSortKey = std::string;

// Messagable list
using MessagableExternalInterface = ui::MessagableList;
using MessagableInternalInterface = ContactListInternalInterface;
using MessagableListRowID = ContactListRowID;
using MessagableListRowInterface = ContactListRowInterface;
using MessagableListRowInternal = ContactListRowInternal;
using MessagableListRowBlank = ContactListRowBlank;
using MessagableListSortKey = std::string;

// Payable list
using PayableExternalInterface = ui::PayableList;
using PayableInternalInterface = ContactListInternalInterface;
using PayableListRowID = ContactListRowID;
using PayableListRowInterface = ui::PayableListItem;
using PayableListRowInternal = ui::internal::PayableListItem;
using PayableListRowBlank = PayableListItemBlank;
using PayableListSortKey = std::string;

// Profile
using ProfileExternalInterface = ui::Profile;
using ProfileInternalInterface = ui::internal::Profile;
using ProfileRowID = proto::ContactSectionName;
using ProfileRowInterface = ui::ProfileSection;
using ProfileRowInternal = ui::internal::ProfileSection;
using ProfileRowBlank = ProfileSectionBlank;
using ProfileSortKey = int;

using ProfileSectionExternalInterface = ProfileRowInterface;
using ProfileSectionInternalInterface = ui::internal::ProfileSection;
using ProfileSectionRowID =
    std::pair<proto::ContactSectionName, proto::ContactItemType>;
using ProfileSectionRowInterface = ui::ProfileSubsection;
using ProfileSectionRowInternal = ui::internal::ProfileSubsection;
using ProfileSectionRowBlank = ProfileSubsectionBlank;
using ProfileSectionSortKey = int;

using ProfileSubsectionExternalInterface = ProfileSectionRowInterface;
using ProfileSubsectionInternalInterface = ui::internal::ProfileSubsection;
using ProfileSubsectionRowID = OTIdentifier;
using ProfileSubsectionRowInterface = ui::ProfileItem;
using ProfileSubsectionRowInternal = ui::internal::ProfileItem;
using ProfileSubsectionRowBlank = ProfileItemBlank;
using ProfileSubsectionSortKey = int;
}  // namespace implementation
}  // namespace ui

class DhtConfig;
#if OT_CRYPTO_USING_LIBSECP256K1
class Libsecp256k1;
#endif
class Libsodium;
class LowLevelKeyGenerator;
#if OT_CRYPTO_USING_OPENSSL
class OpenSSL;
#endif
class StorageConfig;
#if OT_CRYPTO_USING_TREZOR
class TrezorCrypto;
#endif
}  // namespace opentxs

#include "Factory.hpp"
