// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"              // IWYU pragma: associated
#include "1_Internal.hpp"            // IWYU pragma: associated
#include "ui/unitlist/UnitList.hpp"  // IWYU pragma: associated

#include <memory>
#include <set>
#include <thread>
#include <utility>

#include "internal/api/client/Client.hpp"
#include "internal/contact/Contact.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"
#include "opentxs/protobuf/verify/VerifyContacts.hpp"
#if OT_QT
#include "opentxs/ui/qt/UnitList.hpp"
#endif  // OT_QT
#include "opentxs/util/WorkType.hpp"

#define OT_METHOD "opentxs::ui::implementation::UnitList::"

namespace zmq = opentxs::network::zeromq;

namespace opentxs::factory
{
auto UnitListModel(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::implementation::UnitList>
{
    using ReturnType = ui::implementation::UnitList;

    return std::make_unique<ReturnType>(api, nymID, cb);
}

#if OT_QT
auto UnitListQtModel(ui::implementation::UnitList& parent) noexcept
    -> std::unique_ptr<ui::UnitListQt>
{
    using ReturnType = ui::UnitListQt;

    return std::make_unique<ReturnType>(parent);
}
#endif  // OT_QT
}  // namespace opentxs::factory

#if OT_QT
namespace opentxs::ui
{
QT_PROXY_MODEL_WRAPPER(UnitListQt, implementation::UnitList)
}  // namespace opentxs::ui
#endif

namespace opentxs::ui::implementation
{
UnitList::UnitList(
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const SimpleCallback& cb) noexcept
    : UnitListList(
          api,
          nymID,
          cb,
          false
#if OT_QT
          ,
          Roles{
              {UnitListQt::UnitIDRole, "unit"},
          },
          1
#endif
          )
#if OT_BLOCKCHAIN
    , blockchain_balance_cb_(zmq::ListenCallback::Factory(
          [this](const auto& in) { process_blockchain_balance(in); }))
    , blockchain_balance_(api_.ZeroMQ().DealerSocket(
          blockchain_balance_cb_,
          zmq::socket::Socket::Direction::Connect))
#endif  // OT_BLOCKCHAIN
    , listeners_{
          {api_.Endpoints().AccountUpdate(),
           new MessageProcessor<UnitList>(&UnitList::process_account)}}
{
    init();
    setup_listeners(listeners_);
    startup_.reset(new std::thread(&UnitList::startup, this));

    OT_ASSERT(startup_)
}

auto UnitList::construct_row(
    const UnitListRowID& id,
    const UnitListSortKey& index,
    CustomData& custom) const noexcept -> RowPointer
{
    return factory::UnitListItem(*this, api_, id, index, custom);
}

auto UnitList::process_account(const Message& message) noexcept -> void
{
    wait_for_startup();
    const auto body = message.Body();

    OT_ASSERT(2 < body.size());

    auto accountID = api_.Factory().Identifier();
    accountID->Assign(body.at(1).Bytes());

    OT_ASSERT(false == accountID->empty());

    process_account(accountID);
}

auto UnitList::process_account(const Identifier& id) noexcept -> void
{
    process_unit(api_.Storage().AccountUnit(id));
}

#if OT_BLOCKCHAIN
auto UnitList::process_blockchain_balance(const Message& message) noexcept
    -> void
{
    wait_for_startup();
    const auto body = message.Body();

    OT_ASSERT(3 < body.size());

    const auto& chainFrame = message.Body_at(1);

    try {
        process_unit(Translate(chainFrame.as<blockchain::Type>()));
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid chain").Flush();

        return;
    }
}
#endif  // OT_BLOCKCHAIN

auto UnitList::process_unit(const UnitListRowID& id) noexcept -> void
{
    auto custom = CustomData{};
    add_item(
        id, proto::TranslateItemType(contact::internal::translate(id)), custom);
}

#if OT_BLOCKCHAIN
auto UnitList::setup_listeners(const ListenerDefinitions& definitions) noexcept
    -> void
{
    Widget::setup_listeners(definitions);
    const auto connected =
        blockchain_balance_->Start(api_.Endpoints().BlockchainBalance());

    OT_ASSERT(connected);
}
#endif  // OT_BLOCKCHAIN

auto UnitList::startup() noexcept -> void
{
    const auto accounts = api_.Storage().AccountsByOwner(primary_id_);
    LogDetail(OT_METHOD)(__FUNCTION__)(": Loading ")(accounts.size())(
        " accounts.")
        .Flush();

    for (const auto& id : accounts) { process_account(id); }

#if OT_BLOCKCHAIN
    for (const auto& chain : blockchain::SupportedChains()) {
        if (0 < api_.Blockchain().AccountList(primary_id_, chain).size()) {
            auto out = api_.ZeroMQ().TaggedMessage(WorkType::BlockchainBalance);
            out->AddFrame(chain);
            blockchain_balance_->Send(out);
        }
    }
#endif  // OT_BLOCKCHAIN

    finish_startup();
}

UnitList::~UnitList()
{
    for (auto& it : listeners_) { delete it.second; }
}
}  // namespace opentxs::ui::implementation
