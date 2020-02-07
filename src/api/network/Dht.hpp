// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs::api::network::implementation
{
class Dht final : virtual public opentxs::api::network::Dht
{
public:
    void GetPublicNym(const std::string& key) const final;
    void GetServerContract(const std::string& key) const final;
    void GetUnitDefinition(const std::string& key) const final;
    void Insert(const std::string& key, const std::string& value) const final;
    void Insert(const proto::Nym& nym) const final;
    void Insert(const proto::ServerContract& contract) const final;
    void Insert(const proto::UnitDefinition& contract) const final;
#if OT_DHT
    const opentxs::network::OpenDHT& OpenDHT() const final;
#endif
    void RegisterCallbacks(const CallbackMap& callbacks) const final;

    ~Dht() final = default;

private:
    friend opentxs::Factory;

    const api::internal::Core& api_;
    mutable CallbackMap callback_map_{};
    std::unique_ptr<const DhtConfig> config_{nullptr};
#if OT_DHT
    std::unique_ptr<opentxs::network::OpenDHT> node_{nullptr};
#endif
    OTZMQReplyCallback request_nym_callback_;
    OTZMQReplySocket request_nym_socket_;
    OTZMQReplyCallback request_server_callback_;
    OTZMQReplySocket request_server_socket_;
    OTZMQReplyCallback request_unit_callback_;
    OTZMQReplySocket request_unit_socket_;

#if OT_DHT
    static bool ProcessPublicNym(
        const api::internal::Core& api,
        const std::string key,
        const DhtResults& values,
        NotifyCB notifyCB);
    static bool ProcessServerContract(
        const api::internal::Core& api,
        const std::string key,
        const DhtResults& values,
        NotifyCB notifyCB);
    static bool ProcessUnitDefinition(
        const api::internal::Core& api,
        const std::string key,
        const DhtResults& values,
        NotifyCB notifyCB);
#endif

    OTZMQMessage process_request(
        const opentxs::network::zeromq::Message& incoming,
        void (Dht::*get)(const std::string&) const) const;

    Dht(DhtConfig& config, const api::internal::Core& api);
    Dht() = delete;
    Dht(const Dht&) = delete;
    Dht(Dht&&) = delete;
    Dht& operator=(const Dht&) = delete;
    Dht& operator=(Dht&&) = delete;
};
}  // namespace opentxs::api::network::implementation
