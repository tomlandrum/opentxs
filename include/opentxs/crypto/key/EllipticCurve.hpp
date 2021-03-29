// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_KEY_ELLIPTICCURVE_HPP
#define OPENTXS_CRYPTO_KEY_ELLIPTICCURVE_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <memory>

#include "opentxs/crypto/key/Asymmetric.hpp"
#include "opentxs/crypto/key/Keypair.hpp"

namespace opentxs
{
namespace crypto
{
class EcdsaProvider;
}  // namespace crypto
}  // namespace opentxs

namespace opentxs
{
namespace crypto
{
namespace key
{
class EllipticCurve : virtual public Asymmetric
{
public:
    OPENTXS_EXPORT static const VersionNumber DefaultVersion;
    OPENTXS_EXPORT static const VersionNumber MaxVersion;

    OPENTXS_EXPORT virtual std::unique_ptr<EllipticCurve> asPublicEC()
        const noexcept = 0;
    OPENTXS_EXPORT virtual std::unique_ptr<EllipticCurve> CloneEC()
        const noexcept = 0;
    OPENTXS_EXPORT virtual const crypto::EcdsaProvider& ECDSA()
        const noexcept = 0;
    OPENTXS_EXPORT virtual std::unique_ptr<EllipticCurve> IncrementPrivate(
        const Secret& scalar,
        const PasswordPrompt& reason) const noexcept = 0;
    OPENTXS_EXPORT virtual std::unique_ptr<EllipticCurve> IncrementPublic(
        const Secret& scalar) const noexcept = 0;
    OPENTXS_EXPORT virtual bool SignDER(
        const ReadView preimage,
        const crypto::HashType hash,
        Space& output,
        const PasswordPrompt& reason) const noexcept = 0;

    OPENTXS_EXPORT ~EllipticCurve() override = default;

protected:
    EllipticCurve() = default;

private:
    EllipticCurve(const EllipticCurve&) = delete;
    EllipticCurve(EllipticCurve&&) = delete;
    EllipticCurve& operator=(const EllipticCurve&) = delete;
    EllipticCurve& operator=(EllipticCurve&&) = delete;
};
}  // namespace key
}  // namespace crypto
}  // namespace opentxs
#endif
