// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CLIENT_BLOCKCHAIN_DETERMINISTIC_HPP
#define OPENTXS_API_CLIENT_BLOCKCHAIN_DETERMINISTIC_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <optional>
#include <string>
#include <vector>

#include "opentxs/Types.hpp"
#include "opentxs/api/client/blockchain/BalanceNode.hpp"
#include "opentxs/core/Identifier.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
namespace blockchain
{
class Deterministic : virtual public BalanceNode
{
public:
    using Batch = std::vector<Bip32Index>;

    OPENTXS_EXPORT virtual std::optional<Bip32Index> Floor(
        const Subchain type) const noexcept = 0;
    OPENTXS_EXPORT virtual std::optional<Bip32Index> GenerateNext(
        const Subchain type,
        const PasswordPrompt& reason) const noexcept = 0;
    OPENTXS_EXPORT virtual ECKey Key(
        const Subchain type,
        const Bip32Index index) const noexcept = 0;
    OPENTXS_EXPORT virtual std::optional<Bip32Index> LastGenerated(
        const Subchain type) const noexcept = 0;
    OPENTXS_EXPORT virtual std::size_t Lookahead() const noexcept = 0;
    OPENTXS_EXPORT virtual proto::HDPath Path() const noexcept = 0;
    OPENTXS_EXPORT virtual const std::string PathRoot() const noexcept = 0;
    OPENTXS_EXPORT virtual std::optional<Bip32Index> Reserve(
        const Subchain type,
        const PasswordPrompt& reason,
        const Identifier& contact = Identifier::Factory(),
        const std::string& label = {},
        const Time time = Clock::now()) const noexcept = 0;
    OPENTXS_EXPORT virtual Batch Reserve(
        const Subchain type,
        const std::size_t batch,
        const PasswordPrompt& reason,
        const Identifier& contact = Identifier::Factory(),
        const std::string& label = {},
        const Time time = Clock::now()) const noexcept = 0;
    OPENTXS_EXPORT virtual HDKey RootNode(
        const PasswordPrompt& reason) const noexcept = 0;

    OPENTXS_EXPORT ~Deterministic() override = default;

protected:
    Deterministic() noexcept = default;

private:
    Deterministic(const Deterministic&) = delete;
    Deterministic(Deterministic&&) = delete;
    Deterministic& operator=(const Deterministic&) = delete;
    Deterministic& operator=(Deterministic&&) = delete;
};
}  // namespace blockchain
}  // namespace client
}  // namespace api
}  // namespace opentxs
#endif  // OPENTXS_API_CLIENT_BLOCKCHAIN_DETERMINISTIC_HPP
