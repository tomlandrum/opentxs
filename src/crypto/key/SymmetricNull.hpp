// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs::crypto::key::implementation
{
class SymmetricNull final : virtual public key::Symmetric
{
public:
    auto api() const -> const api::internal::Core& final { throw; }

    auto ChangePassword(const PasswordPrompt&, const Secret&) -> bool final
    {
        return false;
    }
    auto Decrypt(
        const proto::Ciphertext&,
        const PasswordPrompt&,
        const AllocateOutput) const -> bool final
    {
        return false;
    }
    auto Encrypt(
        const ReadView,
        const PasswordPrompt&,
        proto::Ciphertext&,
        const bool,
        const opentxs::crypto::key::symmetric::Algorithm,
        const ReadView) const -> bool final
    {
        return false;
    }
    auto ID(const PasswordPrompt&) const -> OTIdentifier final
    {
        return Identifier::Factory();
    }
    auto RawKey(const PasswordPrompt&, Secret&) const -> bool final
    {
        return false;
    }
    auto Serialize(proto::SymmetricKey&) const -> bool final { return false; }
    auto Unlock(const PasswordPrompt&) const -> bool final { return false; }

    operator bool() const final { return false; }

    SymmetricNull() = default;
    ~SymmetricNull() final = default;

private:
    auto clone() const -> SymmetricNull* final { return nullptr; }

    SymmetricNull(const SymmetricNull&) = delete;
    SymmetricNull(SymmetricNull&&) = delete;
    auto operator=(const SymmetricNull&) -> SymmetricNull& = delete;
    auto operator=(SymmetricNull&&) -> SymmetricNull& = delete;
};
}  // namespace opentxs::crypto::key::implementation
