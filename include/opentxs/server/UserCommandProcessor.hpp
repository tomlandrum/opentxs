/************************************************************
 *
 *  UserCommandProcessor.hpp
 *
 */

/************************************************************
 -----BEGIN PGP SIGNED MESSAGE-----
 Hash: SHA1

 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  Copyright (C) 2010-2013 by "Fellow Traveler" (A pseudonym)
 *
 *  EMAIL:
 *  FellowTraveler@rayservers.net
 *
 *  BITCOIN:  1NtTPVVjDsUfDWybS4BwvHpG2pdS9RnYyQ
 *
 *  KEY FINGERPRINT (PGP Key in license file):
 *  9DD5 90EB 9292 4B48 0484  7910 0308 00ED F951 BB8E
 *
 *  OFFICIAL PROJECT WIKI(s):
 *  https://github.com/FellowTraveler/Moneychanger
 *  https://github.com/FellowTraveler/Open-Transactions/wiki
 *
 *  WEBSITE:
 *  http://www.OpenTransactions.org/
 *
 *  Components and licensing:
 *   -- Moneychanger..A Java client GUI.....LICENSE:.....GPLv3
 *   -- otlib.........A class library.......LICENSE:...LAGPLv3
 *   -- otapi.........A client API..........LICENSE:...LAGPLv3
 *   -- opentxs/ot....Command-line client...LICENSE:...LAGPLv3
 *   -- otserver......Server Application....LICENSE:....AGPLv3
 *  Github.com/FellowTraveler/Open-Transactions/wiki/Components
 *
 *  All of the above OT components were designed and written by
 *  Fellow Traveler, with the exception of Moneychanger, which
 *  was contracted out to Vicky C (bitcointrader4@gmail.com).
 *  The open-source community has since actively contributed.
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This program is free software: you can redistribute it
 *   and/or modify it under the terms of the GNU Affero
 *   General Public License as published by the Free Software
 *   Foundation, either version 3 of the License, or (at your
 *   option) any later version.
 *
 *   ADDITIONAL PERMISSION under the GNU Affero GPL version 3
 *   section 7: (This paragraph applies only to the LAGPLv3
 *   components listed above.) If you modify this Program, or
 *   any covered work, by linking or combining it with other
 *   code, such other code is not for that reason alone subject
 *   to any of the requirements of the GNU Affero GPL version 3.
 *   (==> This means if you are only using the OT API, then you
 *   don't have to open-source your code--only your changes to
 *   Open-Transactions itself must be open source. Similar to
 *   LGPLv3, except it applies to software-as-a-service, not
 *   just to distributing binaries.)
 *
 *   Extra WAIVER for OpenSSL, Lucre, and all other libraries
 *   used by Open Transactions: This program is released under
 *   the AGPL with the additional exemption that compiling,
 *   linking, and/or using OpenSSL is allowed. The same is true
 *   for any other open source libraries included in this
 *   project: complete waiver from the AGPL is hereby granted to
 *   compile, link, and/or use them with Open-Transactions,
 *   according to their own terms, as long as the rest of the
 *   Open-Transactions terms remain respected, with regard to
 *   the Open-Transactions code itself.
 *
 *   Lucre License:
 *   This code is also "dual-license", meaning that Ben Lau-
 *   rie's license must also be included and respected, since
 *   the code for Lucre is also included with Open Transactions.
 *   See Open-Transactions/src/otlib/lucre/LUCRE_LICENSE.txt
 *   The Laurie requirements are light, but if there is any
 *   problem with his license, simply remove the Lucre code.
 *   Although there are no other blind token algorithms in Open
 *   Transactions (yet. credlib is coming), the other functions
 *   will continue to operate.
 *   See Lucre on Github:  https://github.com/benlaurie/lucre
 *   -----------------------------------------------------
 *   You should have received a copy of the GNU Affero General
 *   Public License along with this program.  If not, see:
 *   http://www.gnu.org/licenses/
 *
 *   If you would like to use this software outside of the free
 *   software license, please contact FellowTraveler.
 *   (Unfortunately many will run anonymously and untraceably,
 *   so who could really stop them?)
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will be
 *   useful, but WITHOUT ANY WARRANTY; without even the implied
 *   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *   PURPOSE.  See the GNU Affero General Public License for
 *   more details.

 -----BEGIN PGP SIGNATURE-----
 Version: GnuPG v1.4.9 (Darwin)

 iQIcBAEBAgAGBQJRSsfJAAoJEAMIAO35UbuOQT8P/RJbka8etf7wbxdHQNAY+2cC
 vDf8J3X8VI+pwMqv6wgTVy17venMZJa4I4ikXD/MRyWV1XbTG0mBXk/7AZk7Rexk
 KTvL/U1kWiez6+8XXLye+k2JNM6v7eej8xMrqEcO0ZArh/DsLoIn1y8p8qjBI7+m
 aE7lhstDiD0z8mwRRLKFLN2IH5rAFaZZUvj5ERJaoYUKdn4c+RcQVei2YOl4T0FU
 LWND3YLoH8naqJXkaOKEN4UfJINCwxhe5Ke9wyfLWLUO7NamRkWD2T7CJ0xocnD1
 sjAzlVGNgaFDRflfIF4QhBx1Ddl6wwhJfw+d08bjqblSq8aXDkmFA7HeunSFKkdn
 oIEOEgyj+veuOMRJC5pnBJ9vV+7qRdDKQWaCKotynt4sWJDGQ9kWGWm74SsNaduN
 TPMyr9kNmGsfR69Q2Zq/FLcLX/j8ESxU+HYUB4vaARw2xEOu2xwDDv6jt0j3Vqsg
 x7rWv4S/Eh18FDNDkVRChiNoOIilLYLL6c38uMf1pnItBuxP3uhgY6COm59kVaRh
 nyGTYCDYD2TK+fI9o89F1297uDCwEJ62U0Q7iTDp5QuXCoxkPfv8/kX6lS6T3y9G
 M9mqIoLbIQ1EDntFv7/t6fUTS2+46uCrdZWbQ5RjYXdrzjij02nDmJAm2BngnZvd
 kamH0Y/n11lCvo1oQxM+
 =uSzz
 -----END PGP SIGNATURE-----
**************************************************************/

#ifndef OPENTXS_SERVER_USERCOMMANDPROCESSOR_HPP
#define OPENTXS_SERVER_USERCOMMANDPROCESSOR_HPP

#include <cstdint>

namespace opentxs
{
class String;
class Message;
class Nym;
class OTServer;
class Identifier;
class ClientConnection;

class UserCommandProcessor
{
public:
    UserCommandProcessor(OTServer* server);

    bool ProcessUserCommand(Message& msgIn, Message& msgOut,
                            ClientConnection* connection, Nym* nym);

private:
    bool SendMessageToNym(const Identifier& serverId,
                          const Identifier& senderUserId,
                          const Identifier& recipientUserId,
                          Message* msg = nullptr,
                          const String* messageString = nullptr);

    void DropReplyNoticeToNymbox(const Identifier& serverId,
                                 const Identifier& userId,
                                 const String& messageString,
                                 const int64_t& requestNum,
                                 const bool replyTransSuccess,
                                 Nym* actualNym = nullptr);

    void UserCmdCheckServerID(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdCheckUser(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdSendUserMessage(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdSendUserInstrument(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdGetRequest(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdGetTransactionNum(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdIssueAssetType(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdIssueBasket(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdGetBoxReceipt(Message& msgIn, Message& msgOut);
    void UserCmdDeleteUser(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdDeleteAssetAcct(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdCreateAccount(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdNotarizeTransactions(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdGetNymbox(Nym& nym, Message& msgIn, Message& msgOut);
    // Deprecated (replaced by UserCmdGetAccountFiles)
    void UserCmdGetInbox(Nym& nym, Message& msgIn, Message& msgOut);
    // Deprecated (replaced by UserCmdGetAccountFiles)
    void UserCmdGetOutbox(Nym& nym, Message& msgIn, Message& msgOut);
    // Deprecated (replaced by UserCmdGetAccountFiles)
    void UserCmdGetAccount(Nym& nym, Message& msgIn, Message& msgOut);
    // This combines GetInbox, GetOutbox, and GetAccount.
    void UserCmdGetAccountFiles(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdGetContract(Message& msgIn, Message& msgOut);
    void UserCmdGetMint(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdProcessInbox(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdProcessNymbox(Nym& nym, Message& msgIn, Message& msgOut);

    void UserCmdUsageCredits(Nym& nym, Message& msgIn, Message& msgOut);
    void UserCmdTriggerClause(Nym& nym, Message& msgIn, Message& msgOut);

    void UserCmdQueryAssetTypes(Nym& nym, Message& msgIn, Message& msgOut);

    // Get the list of markets on this server.
    void UserCmdGetMarketList(Nym& nym, Message& msgIn, Message& msgOut);

    // Get the publicly-available list of offers on a specific market.
    void UserCmdGetMarketOffers(Nym& nym, Message& msgIn, Message& msgOut);

    // Get a report of recent trades that have occurred on a specific market.
    void UserCmdGetMarketRecentTrades(Nym& nym, Message& msgIn,
                                      Message& msgOut);

    // Get the offers that a specific Nym has placed on a specific market.
    void UserCmdGetNym_MarketOffers(Nym& nym, Message& msgIn, Message& msgOut);

private:
    OTServer* server_;
};

} // namespace opentxs

#endif // OPENTXS_SERVER_USERCOMMANDPROCESSOR_HPP
