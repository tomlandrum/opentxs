// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_SCRIPT_OTPARTYACCOUNT_HPP
#define OPENTXS_CORE_SCRIPT_OTPARTYACCOUNT_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/String.hpp"
#include "opentxs/Types.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace opentxs
{
// Each party has a list of accounts. Just as the agent, depending on context,
// MAY
// have an unowned-but-useful pointer to its active Nym, similarly a
// partyaccount
// MAY have an unowned-but-useful point to its actual Account.
//
// (Normally, an OTPartyAccount is loaded with the party, as part of an
// OTScriptable-
// derived object, and it doesn't actually load a copy of the actual Nyms and
// Accounts
// at that time, it just stores their IDs. But times may also come when the
// actual pointer
// is passed in for use by the Parties, who are not otherwise expected to do
// anything
// with it, or to clean it up or rely on it for their normal data storage.)
//
// Represents an account (to a party.) This is because ACTUALLY loading the REAL
// account is a pain in the ass,
// and we just want to store its ID, etc.  This way, the party can OWN this
// information about this list of
// accounts, and store it, load it, destroy it, etc, often without involving the
// actual account file at all.
//
// (Even if a this isn't currently doing anything with any of the party's
// accounts, the contract still
// has to have a LIST of them, so it knows what to do in those cases when it
// DOES need to do something.)
//
// For example, this is where we store the CLOSING TRANSACTION # for that
// account (for this agreement.)
// That number can't be found in the account itself, but all smart contracts
// involving asset accounts must
// provide a closing number for each asset account.
//
class OTPartyAccount
{
public:
    EXPORT void RegisterForExecution(OTScript& theScript);

    OTParty* GetParty() const { return m_pForParty; }
    void SetParty(OTParty& theOwnerParty);  // This happens when the
                                            // partyaccount
                                            // is added to the party. (so I have
                                            // a ptr back)

    EXPORT const String& GetName() const
    {
        return m_strName;
    }  // account's name as used in a script.
    const String& GetAgentName() const
    {
        return m_strAgentName;
    }  // agent's name as used in a script.
    const String& GetAcctID() const
    {
        return m_strAcctID;
    }  // account's ID as used internal to OT.
    const String& GetInstrumentDefinitionID() const
    {
        return m_strInstrumentDefinitionID;
    }  // instrument definition ID for the account.

    void SetAgentName(const String& strAgentName)
    {
        m_strAgentName = strAgentName;
    }
    void SetAcctID(const String& strAccountID) { m_strAcctID = strAccountID; }
    EXPORT OTAgent* GetAuthorizedAgent();
    SharedAccount LoadAccount();
    bool IsAccount(const Account& theAccount);
    bool IsAccountByID(const Identifier& theAcctID) const;
    bool VerifyOwnership() const;  // I have a ptr to my owner (party), as well
    // as to the actual account. I will ask him to
    // verify whether he actually owns it.
    bool VerifyAgency();  // I can get a ptr to my agent, and I have one to the
                          // actual account. I will ask him to verify whether he
                          // actually has agency over it.
    std::int64_t GetClosingTransNo() const { return m_lClosingTransNo; }
    void SetClosingTransNo(std::int64_t lTransNo)
    {
        m_lClosingTransNo = lTransNo;
    }
    bool Compare(const OTPartyAccount& rhs) const;
    bool DropFinalReceiptToInbox(
        const String& strNotaryID,
        OTSmartContract& theSmartContract,
        const std::int64_t& lNewTransactionNumber,
        const String& strOrigCronItem,
        OTString pstrNote = String::Factory(),
        OTString pstrAttachment = String::Factory());
    void Serialize(
        Tag& parent,
        bool bCalculatingID = false,
        bool bSpecifyInstrumentDefinitionID = false) const;

    OTPartyAccount(const api::Wallet& wallet, const std::string& dataFolder);
    OTPartyAccount(
        const api::Wallet& wallet,
        const std::string& dataFolder,
        const std::string& str_account_name,
        const String& strAgentName,
        Account& theAccount,
        std::int64_t lClosingTransNo);
    OTPartyAccount(
        const api::Wallet& wallet,
        const std::string& dataFolder,
        const String& strName,
        const String& strAgentName,
        const String& strAcctID,
        const String& strInstrumentDefinitionID,
        std::int64_t lClosingTransNo);

    virtual ~OTPartyAccount();

private:
    const api::Wallet& wallet_;
    const std::string data_folder_{""};
    OTParty* m_pForParty;  // When being added to a party, this pointer will be
                           // set.
    // NOTE: each party needs to have a list of partyaccounts, AND each account
    // on that list needs to have a CLOSING #!!! Ahh...
    std::int64_t m_lClosingTransNo;  // Any account that is party to an
                                     // agreement,
                                     // must have a closing transaction # for
                                     // finalReceipt.
    // account name (inside the script language, "gold_acct_A" could be used to
    // reference this acct.)
    //
    OTString m_strName;    // Name of the account (for use in scripts.)
    OTString m_strAcctID;  // The Account ID itself.
    OTString m_strInstrumentDefinitionID;  // The instrument definition ID for
                                           // the account. Stored
    // because parties agree on this even before the
    // account ID is selected. Compare() uses this
    // even when the account ID is blank, and when
    // acct ID *is* added, its instrument definition must match
    // this.
    OTString m_strAgentName;  // The name of the agent who has rights to this
                              // account.
    // Entity, role, and Nym information are not stored here.
    // Entity is already known on the party who owns this account (and I should
    // have a ptr to him.)
    // Role is already known on the agent who is presumably on the party's list
    // of agents.
    // Nym is known on the party (for owner) and on the agent.

    // "GetOwnerID()" for a partyaccount (if it were to store NymID, EntityIsD,
    // and a bool to choose
    // between them) should be logically the same as
    // m_pOwnerParty->GetPartyID().
    //

    SharedAccount get_account() const;

    OTPartyAccount() = delete;
};
}  // namespace opentxs
#endif
