// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "opentxs/opentxs.hpp"

#include <gtest/gtest.h>

using namespace opentxs;

TEST(Message, Factory)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    ASSERT_NE(nullptr, &multipartMessage.get());
}

TEST(Message, AddFrame)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    network::zeromq::Frame& message = multipartMessage->AddFrame();
    ASSERT_EQ(1, multipartMessage->size());
    ASSERT_NE(nullptr, message.data());
    ASSERT_EQ(0, message.size());
}

TEST(Message, AddFrame_Data)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    network::zeromq::Frame& message =
        multipartMessage->AddFrame(Data::Factory("testString", 10));
    ASSERT_EQ(1, multipartMessage->size());
    ASSERT_NE(nullptr, message.data());
    ASSERT_EQ(10, message.size());

    std::string messageString = message;
    ASSERT_STREQ("testString", messageString.c_str());
}

TEST(Message, AddFrame_string)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    network::zeromq::Frame& message = multipartMessage->AddFrame("testString");
    ASSERT_EQ(1, multipartMessage->size());
    ASSERT_NE(nullptr, message.data());
    ASSERT_EQ(10, message.size());

    std::string messageString = message;
    ASSERT_STREQ("testString", messageString.c_str());
}

TEST(Message, at)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame("msg3");

    network::zeromq::Frame& message = multipartMessage->at(0);
    std::string messageString = message;
    ASSERT_STREQ("msg1", messageString.c_str());

    network::zeromq::Frame& message2 = multipartMessage->at(1);
    messageString = message2;
    ASSERT_STREQ("msg2", messageString.c_str());

    network::zeromq::Frame& message3 = multipartMessage->at(2);
    messageString = message3;
    ASSERT_STREQ("msg3", messageString.c_str());
}

TEST(Message, at_const)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame("msg3");

    const network::zeromq::Frame& message = multipartMessage->at(0);
    std::string messageString = message;
    ASSERT_STREQ("msg1", messageString.c_str());

    const network::zeromq::Frame& message2 = multipartMessage->at(1);
    messageString = message2;
    ASSERT_STREQ("msg2", messageString.c_str());

    const network::zeromq::Frame& message3 = multipartMessage->at(2);
    messageString = message3;
    ASSERT_STREQ("msg3", messageString.c_str());
}

TEST(Message, begin)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    network::zeromq::FrameIterator it = multipartMessage->begin();
    ASSERT_EQ(multipartMessage->end(), it);
    ASSERT_EQ(0, std::distance(it, multipartMessage->end()));

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame("msg3");

    ASSERT_NE(multipartMessage->end(), it);
    ASSERT_EQ(3, std::distance(it, multipartMessage->end()));

    std::advance(it, 3);
    ASSERT_EQ(multipartMessage->end(), it);
    ASSERT_EQ(0, std::distance(it, multipartMessage->end()));
}

TEST(Message, Body)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame();
    multipartMessage->AddFrame("msg3");
    multipartMessage->AddFrame("msg4");

    const network::zeromq::FrameSection bodySection = multipartMessage->Body();
    ASSERT_EQ(2, bodySection.size());

    const auto& message = bodySection.at(1);
    std::string msgString = message;
    ASSERT_STREQ("msg4", msgString.c_str());
}

TEST(Message, Body_at)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame();
    multipartMessage->AddFrame("msg3");
    multipartMessage->AddFrame("msg4");

    const network::zeromq::Frame& message = multipartMessage->Body_at(1);
    std::string msgString = message;
    ASSERT_STREQ("msg4", msgString.c_str());
}

TEST(Message, Body_begin)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame();
    multipartMessage->AddFrame("msg3");
    multipartMessage->AddFrame("msg4");

    network::zeromq::FrameIterator bodyBegin = multipartMessage->Body_begin();
    auto body = multipartMessage->Body();
    ASSERT_EQ(body.begin(), bodyBegin);
}

TEST(Message, Body_end)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame();
    multipartMessage->AddFrame("msg3");
    multipartMessage->AddFrame("msg4");

    network::zeromq::FrameIterator bodyEnd = multipartMessage->Body_end();
    auto body = multipartMessage->Body();
    ASSERT_EQ(body.end(), bodyEnd);
}

TEST(Message, end)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    network::zeromq::FrameIterator it = multipartMessage->end();
    ASSERT_EQ(multipartMessage->begin(), it);
    ASSERT_EQ(0, std::distance(multipartMessage->begin(), it));

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame("msg3");

    network::zeromq::FrameIterator it2 = multipartMessage->end();
    ASSERT_NE(multipartMessage->begin(), it2);
    ASSERT_EQ(3, std::distance(multipartMessage->begin(), it2));
}

TEST(Message, EnsureDelimiter)
{
    // Empty message.
    auto message = network::zeromq::Message::Factory();

    ASSERT_EQ(0, message->size());

    message->EnsureDelimiter();  // Adds delimiter.

    ASSERT_EQ(1, message->size());

    message->AddFrame();

    ASSERT_EQ(2, message->size());

    message->EnsureDelimiter();  // Doesn't add delimiter

    ASSERT_EQ(2, message->size());

    // Message body only.
    auto message2 = network::zeromq::Message::Factory("msg");

    ASSERT_EQ(1, message2->size());

    message2->EnsureDelimiter();  // Inserts delimiter.

    ASSERT_EQ(2, message2->size());
    ASSERT_EQ(0, message2->Header().size());
    ASSERT_EQ(1, message2->Body().size());

    message2->EnsureDelimiter();  // Doesn't add delimiter.

    ASSERT_EQ(2, message2->size());
    ASSERT_EQ(0, message2->Header().size());
    ASSERT_EQ(1, message2->Body().size());

    // Header and message body.
    auto message3 = network::zeromq::Message::Factory("header");
    message3->AddFrame();
    message3->AddFrame("body");

    ASSERT_EQ(3, message3->size());
    ASSERT_EQ(1, message3->Header().size());
    ASSERT_EQ(1, message3->Body().size());

    message3->EnsureDelimiter();  // Doesn't add delimiter.

    ASSERT_EQ(3, message3->size());
    ASSERT_EQ(1, message3->Header().size());
    ASSERT_EQ(1, message3->Body().size());

    // Message body with 2 frames.
    auto message4 = network::zeromq::Message::Factory("frame1");
    message4->AddFrame("frame2");

    ASSERT_EQ(2, message4->size());
    ASSERT_EQ(0, message4->Header().size());
    ASSERT_EQ(2, message4->Body().size());

    message4->EnsureDelimiter();

    ASSERT_EQ(3, message4->size());
    ASSERT_EQ(1, message4->Header().size());
    ASSERT_EQ(1, message4->Body().size());
}

TEST(Message, Header)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame();
    multipartMessage->AddFrame("msg3");
    multipartMessage->AddFrame("msg4");

    network::zeromq::FrameSection headerSection = multipartMessage->Header();
    ASSERT_EQ(2, headerSection.size());

    const auto& message = headerSection.at(1);
    std::string msgString = message;
    ASSERT_STREQ("msg2", msgString.c_str());
}

TEST(Message, Header_at)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame();
    multipartMessage->AddFrame("msg3");
    multipartMessage->AddFrame("msg4");

    const network::zeromq::Frame& message = multipartMessage->Header_at(1);
    std::string msgString = message;
    ASSERT_STREQ("msg2", msgString.c_str());
}

TEST(Message, Header_begin)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame();
    multipartMessage->AddFrame("msg3");
    multipartMessage->AddFrame("msg4");

    network::zeromq::FrameIterator headerBegin =
        multipartMessage->Header_begin();
    auto header = multipartMessage->Header();
    ASSERT_EQ(header.begin(), headerBegin);
}

TEST(Message, Header_end)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame();
    multipartMessage->AddFrame("msg3");
    multipartMessage->AddFrame("msg4");

    network::zeromq::FrameIterator headerEnd = multipartMessage->Header_end();
    auto header = multipartMessage->Header();
    ASSERT_EQ(header.end(), headerEnd);
}

TEST(Message, PrependEmptyFrame)
{
    auto message = network::zeromq::Message::Factory("msg body");

    ASSERT_EQ(1, message->size());

    message->PrependEmptyFrame();

    ASSERT_EQ(2, message->size());
    ASSERT_EQ(0, message->Header().size());
    ASSERT_EQ(1, message->Body().size());

    auto message2 = network::zeromq::Message::Factory();

    message2->AddFrame("msg header");
    message2->AddFrame();
    message2->AddFrame("msg body");

    ASSERT_EQ(3, message2->size());
    ASSERT_EQ(1, message2->Header().size());
    ASSERT_EQ(1, message2->Body().size());

    message2->PrependEmptyFrame();

    ASSERT_EQ(4, message2->size());
    ASSERT_EQ(0, message2->Header().size());
    ASSERT_EQ(3, message2->Body().size());
}

TEST(Message, size)
{
    auto multipartMessage = network::zeromq::Message::Factory();

    std::size_t size = multipartMessage->size();
    ASSERT_EQ(0, size);

    multipartMessage->AddFrame("msg1");
    multipartMessage->AddFrame("msg2");
    multipartMessage->AddFrame("msg3");

    size = multipartMessage->size();
    ASSERT_EQ(3, size);
}
