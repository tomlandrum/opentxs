// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_BLOCKCHAIN_P2P_ADDRESS_HPP
#define OPENTXS_BLOCKCHAIN_P2P_ADDRESS_HPP

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <set>

#include "opentxs/Pimpl.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/p2p/Types.hpp"

namespace opentxs
{
namespace blockchain
{
namespace p2p
{
class Address;
}  // namespace p2p
}  // namespace blockchain

namespace proto
{
class BlockchainPeerAddress;
}  // namespace proto

using OTBlockchainAddress = Pimpl<blockchain::p2p::Address>;
}  // namespace opentxs

namespace opentxs
{
namespace blockchain
{
namespace p2p
{
class OPENTXS_EXPORT Address
{
public:
    using SerializedType = proto::BlockchainPeerAddress;

    virtual OTData Bytes() const noexcept = 0;
    virtual blockchain::Type Chain() const noexcept = 0;
    virtual std::string Display() const noexcept = 0;
    virtual const Identifier& ID() const noexcept = 0;
    virtual Time LastConnected() const noexcept = 0;
    virtual std::uint16_t Port() const noexcept = 0;
    OPENTXS_NO_EXPORT virtual bool Serialize(
        SerializedType& out) const noexcept = 0;
    virtual std::set<Service> Services() const noexcept = 0;
    virtual Protocol Style() const noexcept = 0;
    virtual Network Type() const noexcept = 0;

    virtual void AddService(const Service service) noexcept = 0;
    virtual void RemoveService(const Service service) noexcept = 0;
    virtual void SetLastConnected(const Time& time) noexcept = 0;
    virtual void SetServices(const std::set<Service>& services) noexcept = 0;

    virtual ~Address() = default;

protected:
    Address() noexcept = default;

private:
    friend OTBlockchainAddress;

    virtual Address* clone() const noexcept = 0;

    Address(const Address&) = delete;
    Address(Address&&) = delete;
    Address& operator=(const Address&) = delete;
    Address& operator=(Address&&) = delete;
};
}  // namespace p2p
}  // namespace blockchain
}  // namespace opentxs
#endif
