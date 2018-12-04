// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CRYPTO_LETTER_HPP
#define OPENTXS_CORE_CRYPTO_LETTER_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/String.hpp"
#include "opentxs/crypto/library/SymmetricProvider.hpp"
#include "opentxs/Proto.hpp"

#include <list>
#include <map>
#include <string>

namespace opentxs
{
typedef std::multimap<std::string, crypto::key::Asymmetric*>
    mapOfAsymmetricKeys;
typedef std::
    tuple<OTString, OTString, OTString, OTString, std::shared_ptr<OTEnvelope>>
        symmetricEnvelope;
typedef std::list<symmetricEnvelope> listOfSessionKeys;
typedef std::map<proto::AsymmetricKeyType, std::string> listOfEphemeralKeys;
typedef std::multimap<std::string, const crypto::key::EllipticCurve*>
    mapOfECKeys;

/** A letter is a contract that contains the contents of an OTEnvelope along
 *  with some necessary metadata.
 */
class Letter
{
private:
    static bool AddRSARecipients(
        const mapOfAsymmetricKeys& recipients,
        const crypto::key::Symmetric& sessionKey,
        proto::Envelope& envelope);
    static bool DefaultPassword(OTPasswordData& password);
    static bool SortRecipients(
        const mapOfAsymmetricKeys& recipients,
        mapOfAsymmetricKeys& RSARecipients,
        mapOfECKeys& secp256k1Recipients,
        mapOfECKeys& ed25519Recipients);

    Letter() = default;

public:
    static bool Seal(
        const mapOfAsymmetricKeys& RecipPubKeys,
        const String& theInput,
        Data& dataOutput);
    static bool Open(
        const Data& dataInput,
        const Nym& theRecipient,
        const OTPasswordData& keyPassword,
        String& theOutput);

    ~Letter() = default;
};

}  // namespace opentxs

#endif
