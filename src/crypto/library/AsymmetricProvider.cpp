// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                           // IWYU pragma: associated
#include "1_Internal.hpp"                         // IWYU pragma: associated
#include "crypto/library/AsymmetricProvider.hpp"  // IWYU pragma: associated

extern "C" {
#include <sodium.h>
}

#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Primitives.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/crypto/Signature.hpp"
#include "opentxs/crypto/AsymmetricKeyType.hpp"
#include "util/Sodium.hpp"

#define OT_METHOD "opentxs::crypto::AsymmetricProvider::"

namespace opentxs::crypto
{
auto AsymmetricProvider::CurveToKeyType(const EcdsaCurve& curve)
    -> crypto::AsymmetricKeyType
{
    crypto::AsymmetricKeyType output = crypto::AsymmetricKeyType::Error;

    switch (curve) {
        case (EcdsaCurve::secp256k1): {
            output = AsymmetricKeyType::Secp256k1;

            break;
        }
        case (EcdsaCurve::ed25519): {
            output = AsymmetricKeyType::ED25519;

            break;
        }
        default: {
        }
    }

    return output;
}

auto AsymmetricProvider::KeyTypeToCurve(const crypto::AsymmetricKeyType& type)
    -> EcdsaCurve
{
    EcdsaCurve output = EcdsaCurve::invalid;

    switch (type) {
        case (AsymmetricKeyType::Secp256k1): {
            output = EcdsaCurve::secp256k1;

            break;
        }
        case (AsymmetricKeyType::ED25519): {
            output = EcdsaCurve::ed25519;

            break;
        }
        default: {
        }
    }

    return output;
}
}  // namespace opentxs::crypto

namespace opentxs::crypto::implementation
{
AsymmetricProvider::AsymmetricProvider() noexcept
{
    if (0 > ::sodium_init()) { OT_FAIL; }
}

auto AsymmetricProvider::SeedToCurveKey(
    const ReadView seed,
    const AllocateOutput privateKey,
    const AllocateOutput publicKey) const noexcept -> bool
{
    auto edPublic = Data::Factory();
    auto edPrivate = Context().Factory().Secret(0);

    if (false == sodium::ExpandSeed(
                     seed,
                     edPrivate->WriteInto(Secret::Mode::Mem),
                     edPublic->WriteInto())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to expand seed.").Flush();

        return false;
    }

    if (false == bool(privateKey) || false == bool(publicKey)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator")
            .Flush();

        return false;
    }

    return sodium::ToCurveKeypair(
        edPrivate->Bytes(), edPublic->Bytes(), privateKey, publicKey);
}

auto AsymmetricProvider::SignContract(
    const api::internal::Core& api,
    const String& strContractUnsigned,
    const key::Asymmetric& theKey,
    Signature& theSignature,  // output
    const proto::HashType hashType,
    const PasswordPrompt& reason) const -> bool
{
    auto plaintext = Data::Factory(
        strContractUnsigned.Get(),
        strContractUnsigned.GetLength() + 1);  // include null terminator
    auto signature = Data::Factory();
    bool success = Sign(
        api,
        plaintext->Bytes(),
        theKey,
        hashType,
        signature->WriteInto(),
        reason);
    theSignature.SetData(signature, true);  // true means, "yes, with newlines
                                            // in the b64-encoded output,
                                            // please."

    if (false == success) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to sign contract").Flush();
    }

    return success;
}

auto AsymmetricProvider::VerifyContractSignature(
    const String& strContractToVerify,
    const key::Asymmetric& theKey,
    const Signature& theSignature,
    const proto::HashType hashType) const -> bool
{
    auto plaintext = Data::Factory(
        strContractToVerify.Get(),
        strContractToVerify.GetLength() + 1);  // include null terminator
    auto signature = Data::Factory();
    theSignature.GetData(signature);

    return Verify(plaintext, theKey, signature, hashType);
}
}  // namespace opentxs::crypto::implementation
