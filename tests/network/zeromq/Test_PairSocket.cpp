// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "opentxs/opentxs.hpp"

#include <gtest/gtest.h>

using namespace opentxs;

#define TEST_ENDPOINT "inproc://opentxs/pairsocket_endpoint"

namespace
{
class Test_PairSocket : public ::testing::Test
{
public:
    static OTZMQContext context_;

    const std::string testMessage_{"zeromq test message"};
    const std::string testMessage2_{"zeromq test message 2"};

    OTZMQPairSocket* pairSocket_;

    void pairSocketThread(const std::string& msg);

    Test_PairSocket()
        : pairSocket_(nullptr)
    {
    }
};

OTZMQContext Test_PairSocket::context_{network::zeromq::Context::Factory()};

void Test_PairSocket::pairSocketThread(const std::string& message)
{
    ASSERT_NE(nullptr, &Test_PairSocket::context_.get());

    bool callbackFinished = false;

    auto listenCallback = network::zeromq::ListenCallback::Factory(
        [this, &callbackFinished, &message](
            network::zeromq::Message& msg) -> void {
            EXPECT_EQ(1, msg.size());
            const std::string& inputString = *msg.Body().begin();

            EXPECT_EQ(message, inputString);

            callbackFinished = true;
        });

    ASSERT_NE(nullptr, &listenCallback.get());

    ASSERT_NE(nullptr, pairSocket_);

    auto pairSocket =
        network::zeromq::PairSocket::Factory(listenCallback, *pairSocket_);

    ASSERT_NE(nullptr, &pairSocket.get());
    ASSERT_EQ(SocketType::Pair, pairSocket->Type());

    auto end = std::time(nullptr) + 15;
    while (!callbackFinished && std::time(nullptr) < end) {
        Log::Sleep(std::chrono::milliseconds(100));
    }

    ASSERT_TRUE(callbackFinished);
}

}  // namespace

TEST_F(Test_PairSocket, PairSocket_Factory1)
{
    ASSERT_NE(nullptr, &Test_PairSocket::context_.get());

    auto pairSocket = network::zeromq::PairSocket::Factory(
        Test_PairSocket::context_, network::zeromq::ListenCallback::Factory());

    ASSERT_NE(nullptr, &pairSocket.get());
    ASSERT_EQ(SocketType::Pair, pairSocket->Type());
}

TEST_F(Test_PairSocket, PairSocket_Factory2)
{
    ASSERT_NE(nullptr, &Test_PairSocket::context_.get());

    auto peer = network::zeromq::PairSocket::Factory(
        Test_PairSocket::context_, network::zeromq::ListenCallback::Factory());

    ASSERT_NE(nullptr, &peer.get());
    ASSERT_EQ(SocketType::Pair, peer->Type());

    auto pairSocket = network::zeromq::PairSocket::Factory(
        network::zeromq::ListenCallback::Factory(), peer);

    ASSERT_NE(nullptr, &pairSocket.get());
    ASSERT_EQ(SocketType::Pair, pairSocket->Type());
    ASSERT_EQ(pairSocket->Endpoint(), peer->Endpoint());
}

TEST_F(Test_PairSocket, PairSocket_Factory3)
{
    ASSERT_NE(nullptr, &Test_PairSocket::context_.get());

    auto pairSocket = network::zeromq::PairSocket::Factory(
        Test_PairSocket::context_,
        network::zeromq::ListenCallback::Factory(),
        TEST_ENDPOINT);

    ASSERT_NE(nullptr, &pairSocket.get());
    ASSERT_EQ(SocketType::Pair, pairSocket->Type());
    ASSERT_EQ(pairSocket->Endpoint(), TEST_ENDPOINT);
}

TEST_F(Test_PairSocket, PairSocket_Send1)
{
    ASSERT_NE(nullptr, &Test_PairSocket::context_.get());

    bool callbackFinished = false;

    auto listenCallback = network::zeromq::ListenCallback::Factory(
        [this, &callbackFinished](network::zeromq::Message& msg) -> void {
            EXPECT_EQ(1, msg.size());
            const std::string& inputString = *msg.Body().begin();

            EXPECT_EQ(testMessage_, inputString);

            callbackFinished = true;
        });

    ASSERT_NE(nullptr, &listenCallback.get());

    auto peer = network::zeromq::PairSocket::Factory(
        Test_PairSocket::context_, listenCallback);

    ASSERT_NE(nullptr, &peer.get());
    ASSERT_EQ(SocketType::Pair, peer->Type());

    auto pairSocket = network::zeromq::PairSocket::Factory(
        network::zeromq::ListenCallback::Factory(), peer);

    ASSERT_NE(nullptr, &pairSocket.get());
    ASSERT_EQ(SocketType::Pair, pairSocket->Type());

    auto sent = pairSocket->Send(testMessage_);

    ASSERT_TRUE(sent);

    auto end = std::time(nullptr) + 15;
    while (!callbackFinished && std::time(nullptr) < end) {
        Log::Sleep(std::chrono::milliseconds(100));
    }

    ASSERT_TRUE(callbackFinished);
}

TEST_F(Test_PairSocket, PairSocket_Send2)
{
    ASSERT_NE(nullptr, &Test_PairSocket::context_.get());

    bool callbackFinished = false;

    auto listenCallback = network::zeromq::ListenCallback::Factory(
        [this, &callbackFinished](network::zeromq::Message& msg) -> void {
            EXPECT_EQ(1, msg.size());
            const std::string& inputString = *msg.Body().begin();

            EXPECT_EQ(testMessage_, inputString);

            callbackFinished = true;
        });

    ASSERT_NE(nullptr, &listenCallback.get());

    auto peer = network::zeromq::PairSocket::Factory(
        Test_PairSocket::context_, listenCallback);

    ASSERT_NE(nullptr, &peer.get());
    ASSERT_EQ(SocketType::Pair, peer->Type());

    auto pairSocket = network::zeromq::PairSocket::Factory(
        network::zeromq::ListenCallback::Factory(), peer);

    ASSERT_NE(nullptr, &pairSocket.get());
    ASSERT_EQ(SocketType::Pair, pairSocket->Type());

    auto data = Data::Factory(testMessage_.data(), testMessage_.size());

    ASSERT_NE(nullptr, &data.get());

    auto sent = pairSocket->Send(data);

    ASSERT_TRUE(sent);

    auto end = std::time(nullptr) + 15;
    while (!callbackFinished && std::time(nullptr) < end) {
        Log::Sleep(std::chrono::milliseconds(100));
    }

    ASSERT_TRUE(callbackFinished);
}

TEST_F(Test_PairSocket, PairSocket_Send3)
{
    ASSERT_NE(nullptr, &Test_PairSocket::context_.get());

    bool callbackFinished = false;

    auto listenCallback = network::zeromq::ListenCallback::Factory(
        [this, &callbackFinished](network::zeromq::Message& msg) -> void {
            EXPECT_EQ(1, msg.size());
            const std::string& inputString = *msg.Body().begin();

            EXPECT_EQ(testMessage_, inputString);

            callbackFinished = true;
        });

    ASSERT_NE(nullptr, &listenCallback.get());

    auto peer = network::zeromq::PairSocket::Factory(
        Test_PairSocket::context_, listenCallback);

    ASSERT_NE(nullptr, &peer.get());
    ASSERT_EQ(SocketType::Pair, peer->Type());

    auto pairSocket = network::zeromq::PairSocket::Factory(
        network::zeromq::ListenCallback::Factory(), peer);

    ASSERT_NE(nullptr, &pairSocket.get());
    ASSERT_EQ(SocketType::Pair, pairSocket->Type());

    auto msg = network::zeromq::Message::Factory(testMessage_);

    ASSERT_NE(nullptr, &msg.get());

    auto sent = pairSocket->Send(msg);

    ASSERT_TRUE(sent);

    auto end = std::time(nullptr) + 15;
    while (!callbackFinished && std::time(nullptr) < end) {
        Log::Sleep(std::chrono::milliseconds(100));
    }

    ASSERT_TRUE(callbackFinished);
}

TEST_F(Test_PairSocket, PairSocket_Send_Two_Way)
{
    ASSERT_NE(nullptr, &Test_PairSocket::context_.get());

    bool peerCallbackFinished = false;

    auto peerCallback = network::zeromq::ListenCallback::Factory(
        [this, &peerCallbackFinished](network::zeromq::Message& msg) -> void {
            EXPECT_EQ(1, msg.size());
            const std::string& inputString = *msg.Body().begin();

            EXPECT_EQ(testMessage_, inputString);

            peerCallbackFinished = true;
        });

    ASSERT_NE(nullptr, &peerCallback.get());

    auto peer = network::zeromq::PairSocket::Factory(
        Test_PairSocket::context_, peerCallback);

    ASSERT_NE(nullptr, &peer.get());
    ASSERT_EQ(SocketType::Pair, peer->Type());

    bool callbackFinished = false;

    auto listenCallback = network::zeromq::ListenCallback::Factory(
        [this, &callbackFinished](network::zeromq::Message& msg) -> void {
            EXPECT_EQ(1, msg.size());
            const std::string& inputString = *msg.Body().begin();

            EXPECT_EQ(testMessage2_, inputString);

            callbackFinished = true;
        });

    ASSERT_NE(nullptr, &listenCallback.get());

    auto pairSocket =
        network::zeromq::PairSocket::Factory(listenCallback, peer);

    ASSERT_NE(nullptr, &pairSocket.get());
    ASSERT_EQ(SocketType::Pair, pairSocket->Type());

    auto sent = pairSocket->Send(testMessage_);

    ASSERT_TRUE(sent);

    sent = peer->Send(testMessage2_);

    ASSERT_TRUE(sent);

    auto end = std::time(nullptr) + 15;
    while (!peerCallbackFinished && !callbackFinished &&
           std::time(nullptr) < end) {
        Log::Sleep(std::chrono::milliseconds(100));
    }

    ASSERT_TRUE(peerCallbackFinished);
    ASSERT_TRUE(callbackFinished);
}

TEST_F(Test_PairSocket, PairSocket_Send_Separate_Thread)
{
    ASSERT_NE(nullptr, &Test_PairSocket::context_.get());

    auto pairSocket = network::zeromq::PairSocket::Factory(
        Test_PairSocket::context_, network::zeromq::ListenCallback::Factory());

    pairSocket_ = &pairSocket;

    ASSERT_NE(nullptr, &pairSocket.get());
    ASSERT_EQ(SocketType::Pair, pairSocket->Type());

    std::thread pairSocketThread1(
        &Test_PairSocket::pairSocketThread, this, testMessage_);

    auto sent = pairSocket->Send(testMessage_);

    ASSERT_TRUE(sent);

    pairSocketThread1.join();
}
