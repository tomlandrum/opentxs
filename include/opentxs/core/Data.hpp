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

#ifndef OPENTXS_CORE_DATA_HPP
#define OPENTXS_CORE_DATA_HPP

#include "opentxs/Forward.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace opentxs
{
class Data
{
public:
    EXPORT static OTData Factory();
    EXPORT static OTData Factory(const Data& rhs);
    EXPORT static OTData Factory(const void* data, std::size_t size);
    EXPORT static OTData Factory(const OTASCIIArmor& source);
    EXPORT static OTData Factory(const std::vector<unsigned char>& source);

    EXPORT virtual bool operator==(const Data& rhs) const = 0;
    EXPORT virtual bool operator!=(const Data& rhs) const = 0;
    EXPORT virtual std::string asHex() const = 0;
    EXPORT virtual Data* clone() const = 0;
    EXPORT virtual bool empty() const = 0;
    EXPORT virtual const void* GetPointer() const = 0;
    EXPORT virtual std::size_t GetSize() const = 0;
    EXPORT virtual bool IsEmpty() const[[deprecated]] = 0;

    EXPORT virtual Data& operator+=(const Data& rhs) = 0;
    EXPORT virtual void Assign(const Data& source) = 0;
    EXPORT virtual void Assign(const void* data, const std::size_t& size) = 0;
    EXPORT virtual void Concatenate(
        const void* data,
        const std::size_t& size) = 0;
    EXPORT virtual std::size_t OTfread(
        std::uint8_t* data,
        const std::size_t& size) = 0;
    EXPORT virtual bool Randomize(const std::size_t& size) = 0;
    EXPORT virtual void Release() = 0;
    EXPORT virtual void reset() = 0;
    EXPORT virtual void SetSize(const std::size_t& size) = 0;
    EXPORT virtual void swap(Data&& rhs) = 0;
    EXPORT virtual void zeroMemory() = 0;

    EXPORT virtual ~Data() = default;

protected:
    Data() = default;

private:
    Data(const Data& rhs) = delete;
    Data(Data&& rhs) = delete;
    Data& operator=(const Data& rhs) = delete;
    Data& operator=(Data&& rhs) = delete;
};
}  // namespace opentxs

#endif  // OPENTXS_CORE_DATA_HPP
