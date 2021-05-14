// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_KEY_SYMMETRIC_HPP
#define OPENTXS_CRYPTO_KEY_SYMMETRIC_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <string>

#include "opentxs/Bytes.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/crypto/key/symmetric/Algorithm.hpp"

namespace opentxs
{
namespace api
{
class Core;
}  // namespace api

namespace crypto
{
namespace key
{
class Symmetric;
}  // namespace key
}  // namespace crypto

namespace proto
{
class Ciphertext;
class SymmetricKey;
}  // namespace proto

class PasswordPrompt;
class Secret;

using OTSymmetricKey = Pimpl<crypto::key::Symmetric>;
}  // namespace opentxs

namespace opentxs
{
namespace crypto
{
namespace key
{
class OPENTXS_EXPORT Symmetric
{
public:
    /** Generate a blank, invalid key */
    static OTSymmetricKey Factory();

    virtual operator bool() const = 0;

    virtual const api::Core& api() const = 0;

    /** Decrypt ciphertext using the symmetric key
     *
     *  \param[in] ciphertext The data to be decrypted
     *  \param[out] plaintext The encrypted output
     */
    OPENTXS_NO_EXPORT virtual bool Decrypt(
        const proto::Ciphertext& ciphertext,
        const PasswordPrompt& reason,
        const AllocateOutput plaintext) const = 0;
    virtual bool Decrypt(
        const ReadView& ciphertext,
        const PasswordPrompt& reason,
        const AllocateOutput plaintext) const = 0;
    /** Encrypt plaintext using the symmetric key
     *
     *  \param[in] plaintext The data to be encrypted
     *  \param[in] iv Nonce for the encryption operation
     *  \param[out] ciphertext The encrypted output
     *  \param[in] attachKey Set to true if the serialized key should be
     *                       embedded in the ciphertext
     *  \param[in] mode The symmetric algorithm to use for encryption
     */
    OPENTXS_NO_EXPORT virtual bool Encrypt(
        const ReadView plaintext,
        const PasswordPrompt& reason,
        proto::Ciphertext& ciphertext,
        const bool attachKey = true,
        const opentxs::crypto::key::symmetric::Algorithm mode =
            opentxs::crypto::key::symmetric::Algorithm::Error,
        const ReadView iv = {}) const = 0;
    virtual bool Encrypt(
        const ReadView plaintext,
        const PasswordPrompt& reason,
        AllocateOutput ciphertext,
        const bool attachKey = true,
        const opentxs::crypto::key::symmetric::Algorithm mode =
            opentxs::crypto::key::symmetric::Algorithm::Error,
        const ReadView iv = {}) const = 0;
    virtual OTIdentifier ID(const PasswordPrompt& reason) const = 0;
    virtual bool RawKey(const PasswordPrompt& reason, Secret& output) const = 0;
    OPENTXS_NO_EXPORT virtual bool Serialize(
        proto::SymmetricKey& output) const = 0;
    virtual bool Unlock(const PasswordPrompt& reason) const = 0;

    virtual bool ChangePassword(
        const PasswordPrompt& reason,
        const Secret& newPassword) = 0;

    virtual ~Symmetric() = default;

protected:
    Symmetric() = default;

private:
    friend OTSymmetricKey;

    virtual Symmetric* clone() const = 0;

    Symmetric(const Symmetric&) = delete;
    Symmetric(Symmetric&&) = delete;
    Symmetric& operator=(const Symmetric&) = delete;
    Symmetric& operator=(Symmetric&&) = delete;
};
}  // namespace key
}  // namespace crypto
}  // namespace opentxs
#endif
