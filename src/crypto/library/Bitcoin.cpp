/************************************************************
 *
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
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/
#include "stdafx.hpp"

#include "Internal.hpp"

#if OT_WITH_BLOCKCHAIN
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/api/crypto/Util.hpp"
#include "opentxs/api/Native.hpp"
#include "opentxs/core/crypto/OTPassword.hpp"
#include "opentxs/core/crypto/OTPasswordData.hpp"
#include "opentxs/core/util/Assert.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/Secp256k1.hpp"
#include "opentxs/crypto/library/AsymmetricProvider.hpp"
#include "opentxs/crypto/library/Bitcoin.hpp"
#include "opentxs/crypto/library/EcdsaProvider.hpp"
#include "opentxs/crypto/library/LegacySymmetricProvider.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"

#if OT_CRYPTO_WITH_BIP32
#include "crypto/Bip32.hpp"
#endif
#include "AsymmetricProvider.hpp"
#include "EcdsaProvider.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include "Bitcoin.hpp"

//#define OT_METHOD "opentxs::crypto::implementation::Bitcoin::"

namespace opentxs
{
crypto::Bitcoin* Factory::Bitcoin(const api::Crypto& crypto)
{
    return new crypto::implementation::Bitcoin(crypto);
}
}  // namespace opentxs

namespace opentxs::crypto::implementation
{
Bitcoin::Bitcoin(const api::Crypto& crypto)
#if OT_CRYPTO_WITH_BIP32
    : Bip32()
#endif
#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
#if OT_CRYPTO_WITH_BIP32
    ,
#else
    :
#endif
    EcdsaProvider(crypto)
#endif
{
}

#if OT_CRYPTO_WITH_BIP39
bool Bitcoin::SeedToWords(
    [[maybe_unused]] const OTPassword& seed,
    [[maybe_unused]] OTPassword& words) const
{
    // TODO

    return false;
}

void Bitcoin::WordsToSeed(
    [[maybe_unused]] const OTPassword& words,
    [[maybe_unused]] OTPassword& seed,
    [[maybe_unused]] const OTPassword& passphrase) const
{
    OT_ASSERT(words.isPassword());
    OT_ASSERT(passphrase.isPassword());

    seed.SetSize(512 / 8);

    // TODO
}
#endif  // OT_CRYPTO_WITH_BIP39

#if OT_CRYPTO_WITH_BIP32
std::string Bitcoin::SeedToFingerprint(
    [[maybe_unused]] const EcdsaCurve& curve,
    [[maybe_unused]] const OTPassword& seed) const
{
    // TODO

    return {};
}

std::shared_ptr<proto::AsymmetricKey> Bitcoin::SeedToPrivateKey(
    [[maybe_unused]] const EcdsaCurve& curve,
    [[maybe_unused]] const OTPassword& seed) const
{
    // TODO

    return {};
}

std::shared_ptr<proto::AsymmetricKey> Bitcoin::GetChild(
    [[maybe_unused]] const proto::AsymmetricKey& parent,
    [[maybe_unused]] const std::uint32_t index) const
{
    // TODO

    return {};
}

std::shared_ptr<proto::AsymmetricKey> Bitcoin::GetHDKey(
    [[maybe_unused]] const EcdsaCurve& curve,
    [[maybe_unused]] const OTPassword& seed,
    [[maybe_unused]] proto::HDPath& path) const
{
    // TODO

    return {};
}

bool Bitcoin::RandomKeypair(
    [[maybe_unused]] OTPassword& privateKey,
    [[maybe_unused]] Data& publicKey) const
{
    // TODO

    return {};
}
#endif  // OT_CRYPTO_WITH_BIP32

#if OT_CRYPTO_SUPPORTED_KEY_SECP256K1
bool Bitcoin::ECDH(
    [[maybe_unused]] const Data& publicKey,
    [[maybe_unused]] const OTPassword& privateKey,
    [[maybe_unused]] OTPassword& secret) const
{
    // TODO

    return {};
}

bool Bitcoin::ScalarBaseMultiply(
    [[maybe_unused]] const OTPassword& privateKey,
    [[maybe_unused]] Data& publicKey) const
{
    // TODO

    return {};
}
#endif  // OT_CRYPTO_SUPPORTED_KEY_SECP256K1

std::string Bitcoin::Base58CheckEncode(
    [[maybe_unused]] const std::uint8_t* inputStart,
    [[maybe_unused]] const std::size_t& inputSize) const
{
    // TODO

    return {};
}

bool Bitcoin::Base58CheckDecode(
    [[maybe_unused]] const std::string&& input,
    [[maybe_unused]] RawData& output) const
{
    // TODO

    return {};
}

bool Bitcoin::RIPEMD160(
    [[maybe_unused]] const std::uint8_t* input,
    [[maybe_unused]] const size_t inputSize,
    [[maybe_unused]] std::uint8_t* output) const
{
    // TODO

    return {};
}

bool Bitcoin::Sign(
    [[maybe_unused]] const Data& plaintext,
    [[maybe_unused]] const key::Asymmetric& theKey,
    [[maybe_unused]] const proto::HashType hashType,
    [[maybe_unused]] Data& signature,
    [[maybe_unused]] const OTPasswordData* pPWData,
    [[maybe_unused]] const OTPassword* exportPassword) const
{
    // TODO

    return {};
}

bool Bitcoin::Verify(
    [[maybe_unused]] const Data& plaintext,
    [[maybe_unused]] const key::Asymmetric& theKey,
    [[maybe_unused]] const Data& signature,
    [[maybe_unused]] const proto::HashType hashType,
    [[maybe_unused]] const OTPasswordData* pPWData) const
{
    // TODO

    return {};
}
}  // namespace opentxs::crypto::implementation
#endif  // OT_WITH_BLOCKCHAIN
