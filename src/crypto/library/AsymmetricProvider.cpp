// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/core/crypto/Signature.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/String.hpp"

#include "AsymmetricProvider.hpp"

#define OT_METHOD "opentxs::crypto::AsymmetricProvider::"

namespace opentxs::crypto
{
proto::AsymmetricKeyType AsymmetricProvider::CurveToKeyType(
    const EcdsaCurve& curve)
{
    proto::AsymmetricKeyType output = proto::AKEYTYPE_ERROR;

    switch (curve) {
        case (EcdsaCurve::SECP256K1): {
            output = proto::AKEYTYPE_SECP256K1;

            break;
        }
        case (EcdsaCurve::ED25519): {
            output = proto::AKEYTYPE_ED25519;

            break;
        }
        default: {
        }
    }

    return output;
}

EcdsaCurve AsymmetricProvider::KeyTypeToCurve(
    const proto::AsymmetricKeyType& type)
{
    EcdsaCurve output = EcdsaCurve::ERROR;

    switch (type) {
        case (proto::AKEYTYPE_SECP256K1): {
            output = EcdsaCurve::SECP256K1;

            break;
        }
        case (proto::AKEYTYPE_ED25519): {
            output = EcdsaCurve::ED25519;

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
bool AsymmetricProvider::SignContract(
    const String& strContractUnsigned,
    const key::Asymmetric& theKey,
    Signature& theSignature,  // output
    const proto::HashType hashType,
    const OTPasswordData* pPWData) const
{
    auto plaintext = Data::Factory(
        strContractUnsigned.Get(),
        strContractUnsigned.GetLength() + 1);  // include null terminator
    auto signature = Data::Factory();
    bool success = Sign(plaintext, theKey, hashType, signature, pPWData);
    theSignature.SetData(signature, true);  // true means, "yes, with newlines
                                            // in the b64-encoded output,
                                            // please."

    if (false == success) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to sign contract").Flush();
    }

    return success;
}

bool AsymmetricProvider::VerifyContractSignature(
    const String& strContractToVerify,
    const key::Asymmetric& theKey,
    const Signature& theSignature,
    const proto::HashType hashType,
    const OTPasswordData* pPWData) const
{
    auto plaintext = Data::Factory(
        strContractToVerify.Get(),
        strContractToVerify.GetLength() + 1);  // include null terminator
    auto signature = Data::Factory();
    theSignature.GetData(signature);

    return Verify(plaintext, theKey, signature, hashType, pPWData);
}
}  // namespace opentxs::crypto::implementation
