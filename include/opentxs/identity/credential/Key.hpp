// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_IDENTITY_CREDENTIAL_KEY_HPP
#define OPENTXS_IDENTITY_CREDENTIAL_KEY_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <memory>

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/crypto/NymParameters.hpp"
#include "opentxs/crypto/HashType.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/key/Keypair.hpp"
#include "opentxs/identity/credential/Base.hpp"

namespace opentxs
{
namespace identity
{
namespace credential
{
class Key : virtual public Base
{
public:
    OPENTXS_EXPORT virtual const crypto::key::Keypair& GetKeypair(
        const crypto::AsymmetricKeyType type,
        const identity::KeyRole role) const = 0;
    OPENTXS_EXPORT virtual const crypto::key::Keypair& GetKeypair(
        const identity::KeyRole role) const = 0;
    OPENTXS_EXPORT virtual std::int32_t GetPublicKeysBySignature(
        crypto::key::Keypair::Keys& listOutput,
        const opentxs::Signature& theSignature,
        char cKeyType = '0') const = 0;
    OPENTXS_EXPORT virtual bool Sign(
        const GetPreimage input,
        const crypto::SignatureRole role,
        proto::Signature& signature,
        const PasswordPrompt& reason,
        identity::KeyRole key = identity::KeyRole::Sign,
        const crypto::HashType hash = crypto::HashType::Error) const = 0;

    OPENTXS_EXPORT ~Key() override = default;

protected:
    Key() noexcept {}  // TODO Signable

private:
    Key(const Key&) = delete;
    Key(Key&&) = delete;
    Key& operator=(const Key&) = delete;
    Key& operator=(Key&&) = delete;
};
}  // namespace credential
}  // namespace identity
}  // namespace opentxs
#endif
