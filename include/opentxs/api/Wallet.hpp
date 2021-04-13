// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_WALLET_HPP
#define OPENTXS_API_WALLET_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <chrono>
#include <cstdint>
#include <ctime>
#include <list>
#include <memory>
#include <set>
#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Editor.hpp"
#include "opentxs/blind/CashType.hpp"
#include "opentxs/contact/ContactItemType.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/contract/basket/BasketContract.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
class Issuer;
}  // namespace client
}  // namespace api

namespace blind
{
class Purse;
}  // namespace blind

namespace otx
{
namespace context
{
class Base;
class Client;
class Server;
}  // namespace context
}  // namespace otx

namespace proto
{
class PeerReply;
class PeerRequest;
}  // namespace proto

class NymData;
class PeerObject;
}  // namespace opentxs

namespace opentxs
{
/** AccountInfo: accountID, nymID, serverID, unitID*/
using AccountInfo = std::tuple<OTIdentifier, OTNymID, OTServerID, OTUnitID>;

namespace api
{
/** \brief This class manages instantiated contracts and provides easy access
 *  to them.
 *
 * \ingroup native
 *
 *  It includes functionality which was previously found in OTWallet, and adds
 *  new capabilities such as the ability to (optionally) automatically perform
 *  remote lookups for contracts which are not already present in the local
 *  database.
 */
class Wallet
{
public:
    using AccountCallback = std::function<void(const Account&)>;

    OPENTXS_EXPORT virtual SharedAccount Account(
        const Identifier& accountID) const = 0;
    OPENTXS_EXPORT virtual OTIdentifier AccountPartialMatch(
        const std::string& hint) const = 0;
    OPENTXS_EXPORT virtual ExclusiveAccount CreateAccount(
        const identifier::Nym& ownerNymID,
        const identifier::Server& notaryID,
        const identifier::UnitDefinition& instrumentDefinitionID,
        const identity::Nym& signer,
        Account::AccountType type,
        TransactionNumber stash,
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual bool DeleteAccount(
        const Identifier& accountID) const = 0;
    OPENTXS_EXPORT virtual SharedAccount IssuerAccount(
        const identifier::UnitDefinition& unitID) const = 0;
    OPENTXS_EXPORT virtual ExclusiveAccount mutable_Account(
        const Identifier& accountID,
        const PasswordPrompt& reason,
        const AccountCallback callback = nullptr) const = 0;
    OPENTXS_EXPORT virtual bool UpdateAccount(
        const Identifier& accountID,
        const otx::context::Server&,
        const String& serialized,
        const PasswordPrompt& reason) const = 0;
    OPENTXS_EXPORT virtual bool UpdateAccount(
        const Identifier& accountID,
        const otx::context::Server&,
        const String& serialized,
        const std::string& label,
        const PasswordPrompt& reason) const = 0;
    [[deprecated]] virtual bool ImportAccount(
        std::unique_ptr<opentxs::Account>& imported) const = 0;

    /**   Load a read-only copy of a Context object
     *
     *    This method should only be called if the specific client or server
     *    version is not available (such as by classes common to client and
     *    server).
     *
     *    \param[in] notaryID
     *    \param[in] clientNymID
     *    \returns A smart pointer to the object. The smart pointer will not be
     *             instantiated if the object does not exist or is invalid.
     */
    OPENTXS_EXPORT virtual std::shared_ptr<const otx::context::Base> Context(
        const identifier::Server& notaryID,
        const identifier::Nym& clientNymID) const = 0;

    /**   Load a read-only copy of a ClientContext object
     *
     *    \param[in] remoteNymID context identifier (usually the other party's
     *                           nym id)
     *    \returns A smart pointer to the object. The smart pointer will not be
     *             instantiated if the object does not exist or is invalid.
     */
    OPENTXS_EXPORT virtual std::shared_ptr<const otx::context::Client>
    ClientContext(const identifier::Nym& remoteNymID) const = 0;

    /**   Load a read-only copy of a ServerContext object
     *
     *    \param[in] localNymID the identifier of the nym who owns the context
     *    \param[in] remoteID context identifier (usually the other party's nym
     *                       id)
     *    \returns A smart pointer to the object. The smart pointer will not be
     *             instantiated if the object does not exist or is invalid.
     */
    OPENTXS_EXPORT virtual std::shared_ptr<const otx::context::Server>
    ServerContext(const identifier::Nym& localNymID, const Identifier& remoteID)
        const = 0;

    /**   Load an existing Context object
     *
     *    This method should only be called if the specific client or server
     *    version is not available (such as by classes common to client and
     *    server).
     *
     *    WARNING: The context being loaded via this function must exist or else
     *    the function will assert.
     *
     *    \param[in] notaryID the identifier of the nym who owns the context
     *    \param[in] clientNymID context identifier (usually the other party's
     *                           nym id)
     */
    OPENTXS_EXPORT virtual Editor<otx::context::Base> mutable_Context(
        const identifier::Server& notaryID,
        const identifier::Nym& clientNymID,
        const PasswordPrompt& reason) const = 0;

    /**   Load or create a ClientContext object
     *
     *    \param[in] remoteNymID context identifier (usually the other party's
     *                           nym id)
     */
    OPENTXS_EXPORT virtual Editor<otx::context::Client> mutable_ClientContext(
        const identifier::Nym& remoteNymID,
        const PasswordPrompt& reason) const = 0;

    /**   Load or create a ServerContext object
     *
     *    \param[in] localNymID the identifier of the nym who owns the context
     *    \param[in] remoteID context identifier (usually the other party's nym
     *                        id)
     */
    OPENTXS_EXPORT virtual Editor<otx::context::Server> mutable_ServerContext(
        const identifier::Nym& localNymID,
        const Identifier& remoteID,
        const PasswordPrompt& reason) const = 0;

    /**   Returns a list of all issuers associated with a local nym */
    OPENTXS_EXPORT virtual std::set<OTNymID> IssuerList(
        const identifier::Nym& nymID) const = 0;

    /**   Load a read-only copy of an Issuer object
     *
     *    \param[in] nymID the identifier of the local nym
     *    \param[in] issuerID the identifier of the issuer nym
     *    \returns A smart pointer to the object. The smart pointer will not be
     *             instantiated if the object does not exist or is invalid.
     */
    OPENTXS_EXPORT virtual std::shared_ptr<const client::Issuer> Issuer(
        const identifier::Nym& nymID,
        const identifier::Nym& issuerID) const = 0;

    /**   Load or create an Issuer object
     *
     *    \param[in] nymID the identifier of the local nym
     *    \param[in] issuerID the identifier of the issuer nym
     */
    OPENTXS_EXPORT virtual Editor<client::Issuer> mutable_Issuer(
        const identifier::Nym& nymID,
        const identifier::Nym& issuerID) const = 0;

    OPENTXS_EXPORT virtual bool IsLocalNym(const std::string& id) const = 0;

    OPENTXS_EXPORT virtual std::size_t LocalNymCount() const = 0;

    OPENTXS_EXPORT virtual std::set<OTNymID> LocalNyms() const = 0;

    /**   Obtain a smart pointer to an instantiated nym.
     *
     *    The smart pointer will not be initialized if the object does not
     *    exist or is invalid.
     *
     *    If the caller is willing to accept a network lookup delay, it can
     *    specify a timeout to be used in the event that the contract can not
     *    be located in local storage and must be queried from a remote
     *    location.
     *
     *    If no timeout is specified, the remote query will still happen in the
     *    background, but this method will return immediately with a null
     *    result.
     *
     *    \param[in] id the identifier of the nym to be returned
     *    \param[in] timeout The caller can set a non-zero value here if it's
     *                       willing to wait for a network lookup. The default
     *                       value of 0 will return immediately.
     */
    OPENTXS_EXPORT virtual Nym_p Nym(
        const identifier::Nym& id,
        const std::chrono::milliseconds& timeout =
            std::chrono::milliseconds(0)) const = 0;

    /**   Instantiate a nym from serialized form
     *
     *    The smart pointer will not be initialized if the provided serialized
     *    contract is invalid.
     *
     *    \param[in] nym the serialized version of the contract
     */
    OPENTXS_EXPORT virtual Nym_p Nym(
        const identity::Nym::Serialized& nym) const = 0;

    OPENTXS_EXPORT virtual Nym_p Nym(
        const PasswordPrompt& reason,
        const std::string name = "",
        const NymParameters& parameters = {},
        const contact::ContactItemType type =
            contact::ContactItemType::Individual) const = 0;

    OPENTXS_EXPORT virtual NymData mutable_Nym(
        const identifier::Nym& id,
        const PasswordPrompt& reason) const = 0;

    OPENTXS_EXPORT virtual std::unique_ptr<const opentxs::NymFile> Nymfile(
        const identifier::Nym& id,
        const PasswordPrompt& reason) const = 0;

    OPENTXS_EXPORT virtual Editor<opentxs::NymFile> mutable_Nymfile(
        const identifier::Nym& id,
        const PasswordPrompt& reason) const = 0;

    OPENTXS_EXPORT virtual Nym_p NymByIDPartialMatch(
        const std::string& partialId) const = 0;

    /**   Returns a list of all known nyms and their aliases
     */
    OPENTXS_EXPORT virtual ObjectList NymList() const = 0;

    OPENTXS_EXPORT virtual bool NymNameByIndex(
        const std::size_t index,
        String& name) const = 0;

    /**   Load a peer reply object
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the peer reply object
     *    \param[in] box the box from which to retrive the peer object
     *    \returns A smart pointer to the object. The smart pointer will not be
     *             instantiated if the object does not exist or is invalid.
     */
    OPENTXS_EXPORT virtual std::shared_ptr<proto::PeerReply> PeerReply(
        const identifier::Nym& nym,
        const Identifier& reply,
        const StorageBox& box) const = 0;

    /**   Clean up the recipient's copy of a peer reply
     *
     *    The peer reply is moved from the nym's SentPeerReply
     *    box to the FinishedPeerReply box.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] replyOrRequest the identifier of the peer reply object, or
     *               the id of its corresponding request
     *    \returns true if the request is successfully stored
     */
    OPENTXS_EXPORT virtual bool PeerReplyComplete(
        const identifier::Nym& nym,
        const Identifier& replyOrRequest) const = 0;

    /**   Store the recipient's copy of a peer reply
     *
     *    The peer reply is stored in the SendPeerReply box for the
     *    specified nym.
     *
     *    The corresponding request is moved from the nym's IncomingPeerRequest
     *    box to the ProcessedPeerRequest box.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the corresponding request
     *    \param[in] reply the serialized peer reply object
     *    \returns true if the request is successfully stored
     */
    OPENTXS_EXPORT virtual bool PeerReplyCreate(
        const identifier::Nym& nym,
        const proto::PeerRequest& request,
        const proto::PeerReply& reply) const = 0;

    /**   Rollback a PeerReplyCreate call
     *
     *    The original request is returned to IncomingPeerRequest box
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the corresponding request
     *    \param[in] reply the identifier of the peer reply object
     *    \returns true if the rollback is successful
     */
    OPENTXS_EXPORT virtual bool PeerReplyCreateRollback(
        const identifier::Nym& nym,
        const Identifier& request,
        const Identifier& reply) const = 0;

    /**   Obtain a list of sent peer replies
     *
     *    \param[in] nym the identifier of the nym whose box is returned
     */
    OPENTXS_EXPORT virtual ObjectList PeerReplySent(
        const identifier::Nym& nym) const = 0;

    /**   Obtain a list of incoming peer replies
     *
     *    \param[in] nym the identifier of the nym whose box is returned
     */
    OPENTXS_EXPORT virtual ObjectList PeerReplyIncoming(
        const identifier::Nym& nym) const = 0;

    /**   Obtain a list of finished peer replies
     *
     *    \param[in] nym the identifier of the nym whose box is returned
     */
    OPENTXS_EXPORT virtual ObjectList PeerReplyFinished(
        const identifier::Nym& nym) const = 0;

    /**   Obtain a list of processed peer replies
     *
     *    \param[in] nym the identifier of the nym whose box is returned
     */
    OPENTXS_EXPORT virtual ObjectList PeerReplyProcessed(
        const identifier::Nym& nym) const = 0;

    /**   Store the senders's copy of a peer reply
     *
     *    The peer reply is stored in the IncomingPeerReply box for the
     *    specified nym.
     *
     *    The corresponding request is moved from the nym's SentPeerRequest
     *    box to the FinishedPeerRequest box.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the corresponding request
     *    \param[in] reply the serialized peer reply object
     *    \returns true if the request is successfully stored
     */
    OPENTXS_EXPORT virtual bool PeerReplyReceive(
        const identifier::Nym& nym,
        const PeerObject& reply) const = 0;

    /**   Load a peer reply object
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the peer reply object
     *    \param[in] box the box from which to retrive the peer object
     *    \returns A smart pointer to the object. The smart pointer will not be
     *             instantiated if the object does not exist or is invalid.
     */
    OPENTXS_EXPORT virtual std::shared_ptr<proto::PeerRequest> PeerRequest(
        const identifier::Nym& nym,
        const Identifier& request,
        const StorageBox& box,
        std::time_t& time) const = 0;

    /**   Clean up the sender's copy of a peer reply
     *
     *    The peer reply is moved from the nym's IncomingPeerReply
     *    box to the ProcessedPeerReply box.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] reply the identifier of the peer reply object
     *    \returns true if the request is successfully moved
     */
    OPENTXS_EXPORT virtual bool PeerRequestComplete(
        const identifier::Nym& nym,
        const Identifier& reply) const = 0;

    /**   Store the initiator's copy of a peer request
     *
     *    The peer request is stored in the SentPeerRequest box for the
     *    specified nym.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the serialized peer request object
     *    \returns true if the request is successfully stored
     */
    OPENTXS_EXPORT virtual bool PeerRequestCreate(
        const identifier::Nym& nym,
        const proto::PeerRequest& request) const = 0;

    /**   Rollback a PeerRequestCreate call
     *
     *    The request is deleted from to SentPeerRequest box
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the peer request
     *    \returns true if the rollback is successful
     */
    OPENTXS_EXPORT virtual bool PeerRequestCreateRollback(
        const identifier::Nym& nym,
        const Identifier& request) const = 0;

    /**   Delete a peer reply object
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the peer reply object
     *    \param[in] box the box from which the peer object will be deleted
     */
    OPENTXS_EXPORT virtual bool PeerRequestDelete(
        const identifier::Nym& nym,
        const Identifier& request,
        const StorageBox& box) const = 0;

    /**   Obtain a list of sent peer requests
     *
     *    \param[in] nym the identifier of the nym whose box is returned
     */
    OPENTXS_EXPORT virtual ObjectList PeerRequestSent(
        const identifier::Nym& nym) const = 0;

    /**   Obtain a list of incoming peer requests
     *
     *    \param[in] nym the identifier of the nym whose box is returned
     */
    OPENTXS_EXPORT virtual ObjectList PeerRequestIncoming(
        const identifier::Nym& nym) const = 0;

    /**   Obtain a list of finished peer requests
     *
     *    \param[in] nym the identifier of the nym whose box is returned
     */
    OPENTXS_EXPORT virtual ObjectList PeerRequestFinished(
        const identifier::Nym& nym) const = 0;

    /**   Obtain a list of processed peer requests
     *
     *    \param[in] nym the identifier of the nym whose box is returned
     */
    OPENTXS_EXPORT virtual ObjectList PeerRequestProcessed(
        const identifier::Nym& nym) const = 0;

    /**   Store the recipient's copy of a peer request
     *
     *    The peer request is stored in the IncomingPeerRequest box for the
     *    specified nym.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the serialized peer request object
     *    \returns true if the request is successfully stored
     */
    OPENTXS_EXPORT virtual bool PeerRequestReceive(
        const identifier::Nym& nym,
        const PeerObject& request) const = 0;

    /**   Update the timestamp of a peer request object
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the peer request object
     *    \param[in] box the box from which the peer object will be deleted
     */
    OPENTXS_EXPORT virtual bool PeerRequestUpdate(
        const identifier::Nym& nym,
        const Identifier& request,
        const StorageBox& box) const = 0;

#if OT_CASH
    OPENTXS_EXPORT virtual std::unique_ptr<const blind::Purse> Purse(
        const identifier::Nym& nym,
        const identifier::Server& server,
        const identifier::UnitDefinition& unit,
        const bool checking = false) const = 0;
    OPENTXS_EXPORT virtual Editor<blind::Purse> mutable_Purse(
        const identifier::Nym& nym,
        const identifier::Server& server,
        const identifier::UnitDefinition& unit,
        const PasswordPrompt& reason,
        const blind::CashType = blind::CashType::Lucre) const = 0;
#endif

    /**   Unload and delete a server contract
     *
     *    This method destroys the contract object, removes it from the
     *    in-memory map, and deletes it from local storage.
     *    \param[in]  id the indentifier of the contract to be removed
     *    \returns true if successful, false if the contract did not exist
     *
     */
    OPENTXS_EXPORT virtual bool RemoveServer(
        const identifier::Server& id) const = 0;

    /**   Unload and delete a unit definition contract
     *
     *    This method destroys the contract object, removes it from the
     *    in-memory map, and deletes it from local storage.
     *    \param[in]  id the indentifier of the contract to be removed
     *    \returns true if successful, false if the contract did not exist
     *
     */
    OPENTXS_EXPORT virtual bool RemoveUnitDefinition(
        const identifier::UnitDefinition& id) const = 0;

    /**   Obtain an instantiated server contract.
     *
     *    If the caller is willing to accept a network lookup delay, it can
     *    specify a timeout to be used in the event that the contract can not
     *    be located in local storage and must be queried from a remote
     *    location.
     *
     *    If no timeout is specified, the remote query will still happen in the
     *    background, but this method will return immediately with a null
     *    result.
     *
     *    \param[in] id the identifier of the contract to be returned
     *    \param[in] timeout The caller can set a non-zero value here if it's
     *                       willing to wait for a network lookup. The default
     *                       value of 0 will return immediately.
     *    \throw std::runtime_error the specified contract does not exist in the
     *                              wallet
     */
    OPENTXS_EXPORT virtual OTServerContract Server(
        const identifier::Server& id,
        const std::chrono::milliseconds& timeout =
            std::chrono::milliseconds(0)) const noexcept(false) = 0;

    /**   Instantiate a server contract from serialized form
     *
     *    \param[in] contract the serialized version of the contract
     *    \throw std::runtime_error the provided contract is not valid
     */
    OPENTXS_EXPORT virtual OTServerContract Server(
        const proto::ServerContract& contract) const noexcept(false) = 0;

    /**   Create a new server contract
     *
     *    \param[in] nymid the identifier of nym which will create the contract
     *    \param[in] name the official name of the server
     *    \param[in] terms human-readable server description & terms of use
     *    \param[in] url externally-reachable IP address or hostname
     *    \param[in] port externally-reachable listen port
     *    \throw std::runtime_error the contract can not be created
     */
    OPENTXS_EXPORT virtual OTServerContract Server(
        const std::string& nymid,
        const std::string& name,
        const std::string& terms,
        const std::list<contract::Server::Endpoint>& endpoints,
        const PasswordPrompt& reason,
        const VersionNumber version) const noexcept(false) = 0;

    /**   Returns a list of all available server contracts and their aliases
     */
    OPENTXS_EXPORT virtual ObjectList ServerList() const = 0;

    /**   Updates the alias for the specified nym.
     *
     *    An alias is a local label which is not part of the nym credentials
     *    itself.
     *
     *    \param[in] id the identifier of the nym whose alias is to be set
     *    \param[in] alias the alias to set or update for the specified nym
     *    \returns true if successful, false if the nym can not be located
     */
    OPENTXS_EXPORT virtual bool SetNymAlias(
        const identifier::Nym& id,
        const std::string& alias) const = 0;

    /**   Updates the alias for the specified server contract.
     *
     *    An alias is a local label which is not part of the server contract
     *    itself.
     *
     *    \param[in] id the identifier of the contract whose alias is to be set
     *    \param[in] alias the alias to set or update for the specified contract
     *    \returns true if successful, false if the contract can not be located
     */
    OPENTXS_EXPORT virtual bool SetServerAlias(
        const identifier::Server& id,
        const std::string& alias) const = 0;

    /**   Updates the alias for the specified unit definition contract.
     *
     *    An alias is a local label which is not part of the unit definition
     *    contract itself.
     *
     *    \param[in] id the identifier of the contract whose alias is to be set
     *    \param[in] alias the alias to set or update for the specified contract
     *    \returns true if successful, false if the contract can not be located
     */
    OPENTXS_EXPORT virtual bool SetUnitDefinitionAlias(
        const identifier::UnitDefinition& id,
        const std::string& alias) const = 0;

    /**   Obtain a list of all available unit definition contracts and their
     *    aliases
     */
    OPENTXS_EXPORT virtual ObjectList UnitDefinitionList() const = 0;

    /**   Obtain an instantiated unit definition contract.
     *
     *    If the caller is willing to accept a network lookup delay, it can
     *    specify a timeout to be used in the event that the contract can not
     *    be located in local storage and must be queried from a remote
     *    location.
     *
     *    If no timeout is specified, the remote query will still happen in the
     *    background, but this method will return immediately with a null
     *    result.
     *
     *    \param[in] id the identifier of the contract to be returned
     *    \param[in] timeout The caller can set a non-zero value here if it's
     *                     willing to wait for a network lookup. The default
     *                     value of 0 will return immediately.
     *    \throw std::runtime_error the specified contract does not exist in the
     *                              wallet
     */
    OPENTXS_EXPORT virtual OTUnitDefinition UnitDefinition(
        const identifier::UnitDefinition& id,
        const std::chrono::milliseconds& timeout =
            std::chrono::milliseconds(0)) const noexcept(false) = 0;
    OPENTXS_EXPORT virtual OTBasketContract BasketContract(
        const identifier::UnitDefinition& id,
        const std::chrono::milliseconds& timeout =
            std::chrono::milliseconds(0)) const noexcept(false) = 0;

    /**   Instantiate a unit definition contract from serialized form
     *
     *    \param[in] contract the serialized version of the contract
     *    \throw std::runtime_error the provided contract is invalid
     */
    OPENTXS_EXPORT virtual OTUnitDefinition UnitDefinition(
        const proto::UnitDefinition& contract) const noexcept(false) = 0;

    /**   Create a new currency contract
     *
     *    \param[in] nymid the identifier of nym which will create the contract
     *    \param[in] shortname a short human-readable identifier for the
     *                         contract
     *    \param[in] name the official name of the unit of account
     *    \param[in] symbol symbol for the unit of account
     *    \param[in] terms human-readable terms and conditions
     *    \param[in] tla three-letter acronym abbreviation of the unit of
     *                   account
     *    \param[in] power the number of decimal places to shift to display
     *                     fractional units
     *    \param[in] fraction the name of the fractional unit
     *    \throw std::runtime_error the contract can not be created
     */
    OPENTXS_EXPORT virtual OTUnitDefinition UnitDefinition(
        const std::string& nymid,
        const std::string& shortname,
        const std::string& name,
        const std::string& symbol,
        const std::string& terms,
        const std::string& tla,
        const std::uint32_t power,
        const std::string& fraction,
        const contact::ContactItemType unitOfAccount,
        const PasswordPrompt& reason,
        const VersionNumber version = contract::Unit::DefaultVersion) const
        noexcept(false) = 0;

    /**   Create a new security contract
     *
     *    \param[in] nymid the identifier of nym which will create the contract
     *    \param[in] shortname a short human-readable identifier for the
     *                         contract
     *    \param[in] name the official name of the unit of account
     *    \param[in] symbol symbol for the unit of account
     *    \param[in] terms human-readable terms and conditions
     *    \throw std::runtime_error the contract can not be created
     */
    OPENTXS_EXPORT virtual OTUnitDefinition UnitDefinition(
        const std::string& nymid,
        const std::string& shortname,
        const std::string& name,
        const std::string& symbol,
        const std::string& terms,
        const contact::ContactItemType unitOfAccount,
        const PasswordPrompt& reason,
        const VersionNumber version = contract::Unit::DefaultVersion) const
        noexcept(false) = 0;

    OPENTXS_EXPORT virtual contact::ContactItemType CurrencyTypeBasedOnUnitType(
        const identifier::UnitDefinition& contractID) const = 0;

    OPENTXS_EXPORT virtual bool LoadCredential(
        const std::string& id,
        std::shared_ptr<proto::Credential>& credential) const = 0;
    OPENTXS_EXPORT virtual bool SaveCredential(
        const proto::Credential& credential) const = 0;

    OPENTXS_EXPORT virtual ~Wallet() = default;

protected:
    Wallet() = default;

private:
    Wallet(const Wallet&) = delete;
    Wallet(Wallet&&) = delete;
    Wallet& operator=(const Wallet&) = delete;
    Wallet& operator=(Wallet&&) = delete;
};
}  // namespace api
}  // namespace opentxs
#endif
