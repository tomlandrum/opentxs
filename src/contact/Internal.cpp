// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                  // IWYU pragma: associated
#include "1_Internal.hpp"                // IWYU pragma: associated
#include "internal/contact/Contact.hpp"  // IWYU pragma: associated

#include "opentxs/contact/ContactItemAttribute.hpp"
#include "opentxs/contact/ContactItemType.hpp"
#include "opentxs/contact/ContactSectionName.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "util/Container.hpp"

namespace opentxs::contact::internal
{
auto contactitemattribute_map() noexcept -> const ContactItemAttributeMap&
{
    static const auto map = ContactItemAttributeMap{
        {ContactItemAttribute::Error, proto::CITEMATTR_ERROR},
        {ContactItemAttribute::Active, proto::CITEMATTR_ACTIVE},
        {ContactItemAttribute::Primary, proto::CITEMATTR_PRIMARY},
        {ContactItemAttribute::Local, proto::CITEMATTR_LOCAL},
    };

    return map;
}

auto contactitemtype_map() noexcept -> const ContactItemTypeMap&
{
    static const auto map = ContactItemTypeMap{
        {ContactItemType::Error, proto::CITEMTYPE_ERROR},
        {ContactItemType::Individual, proto::CITEMTYPE_INDIVIDUAL},
        {ContactItemType::Organization, proto::CITEMTYPE_ORGANIZATION},
        {ContactItemType::Business, proto::CITEMTYPE_BUSINESS},
        {ContactItemType::Government, proto::CITEMTYPE_GOVERNMENT},
        {ContactItemType::Server, proto::CITEMTYPE_SERVER},
        {ContactItemType::Prefix, proto::CITEMTYPE_PREFIX},
        {ContactItemType::Forename, proto::CITEMTYPE_FORENAME},
        {ContactItemType::Middlename, proto::CITEMTYPE_MIDDLENAME},
        {ContactItemType::Surname, proto::CITEMTYPE_SURNAME},
        {ContactItemType::Pedigree, proto::CITEMTYPE_PEDIGREE},
        {ContactItemType::Suffix, proto::CITEMTYPE_SUFFIX},
        {ContactItemType::Nickname, proto::CITEMTYPE_NICKNAME},
        {ContactItemType::Commonname, proto::CITEMTYPE_COMMONNAME},
        {ContactItemType::Passport, proto::CITEMTYPE_PASSPORT},
        {ContactItemType::National, proto::CITEMTYPE_NATIONAL},
        {ContactItemType::Provincial, proto::CITEMTYPE_PROVINCIAL},
        {ContactItemType::Military, proto::CITEMTYPE_MILITARY},
        {ContactItemType::PGP, proto::CITEMTYPE_PGP},
        {ContactItemType::OTR, proto::CITEMTYPE_OTR},
        {ContactItemType::SSL, proto::CITEMTYPE_SSL},
        {ContactItemType::Physical, proto::CITEMTYPE_PHYSICAL},
        {ContactItemType::Official, proto::CITEMTYPE_OFFICIAL},
        {ContactItemType::Birthplace, proto::CITEMTYPE_BIRTHPLACE},
        {ContactItemType::Home, proto::CITEMTYPE_HOME},
        {ContactItemType::Website, proto::CITEMTYPE_WEBSITE},
        {ContactItemType::Opentxs, proto::CITEMTYPE_OPENTXS},
        {ContactItemType::Phone, proto::CITEMTYPE_PHONE},
        {ContactItemType::Email, proto::CITEMTYPE_EMAIL},
        {ContactItemType::Skype, proto::CITEMTYPE_SKYPE},
        {ContactItemType::Wire, proto::CITEMTYPE_WIRE},
        {ContactItemType::QQ, proto::CITEMTYPE_QQ},
        {ContactItemType::Bitmessage, proto::CITEMTYPE_BITMESSAGE},
        {ContactItemType::Whatsapp, proto::CITEMTYPE_WHATSAPP},
        {ContactItemType::Telegram, proto::CITEMTYPE_TELEGRAM},
        {ContactItemType::KIK, proto::CITEMTYPE_KIK},
        {ContactItemType::BBM, proto::CITEMTYPE_BBM},
        {ContactItemType::Wechat, proto::CITEMTYPE_WECHAT},
        {ContactItemType::Kakaotalk, proto::CITEMTYPE_KAKAOTALK},
        {ContactItemType::Facebook, proto::CITEMTYPE_FACEBOOK},
        {ContactItemType::Google, proto::CITEMTYPE_GOOGLE},
        {ContactItemType::Linkedin, proto::CITEMTYPE_LINKEDIN},
        {ContactItemType::VK, proto::CITEMTYPE_VK},
        {ContactItemType::Aboutme, proto::CITEMTYPE_ABOUTME},
        {ContactItemType::Onename, proto::CITEMTYPE_ONENAME},
        {ContactItemType::Twitter, proto::CITEMTYPE_TWITTER},
        {ContactItemType::Medium, proto::CITEMTYPE_MEDIUM},
        {ContactItemType::Tumblr, proto::CITEMTYPE_TUMBLR},
        {ContactItemType::Yahoo, proto::CITEMTYPE_YAHOO},
        {ContactItemType::Myspace, proto::CITEMTYPE_MYSPACE},
        {ContactItemType::Meetup, proto::CITEMTYPE_MEETUP},
        {ContactItemType::Reddit, proto::CITEMTYPE_REDDIT},
        {ContactItemType::Hackernews, proto::CITEMTYPE_HACKERNEWS},
        {ContactItemType::Wikipedia, proto::CITEMTYPE_WIKIPEDIA},
        {ContactItemType::Angellist, proto::CITEMTYPE_ANGELLIST},
        {ContactItemType::Github, proto::CITEMTYPE_GITHUB},
        {ContactItemType::Bitbucket, proto::CITEMTYPE_BITBUCKET},
        {ContactItemType::Youtube, proto::CITEMTYPE_YOUTUBE},
        {ContactItemType::Vimeo, proto::CITEMTYPE_VIMEO},
        {ContactItemType::Twitch, proto::CITEMTYPE_TWITCH},
        {ContactItemType::Snapchat, proto::CITEMTYPE_SNAPCHAT},
        {ContactItemType::Vine, proto::CITEMTYPE_VINE},
        {ContactItemType::Instagram, proto::CITEMTYPE_INSTAGRAM},
        {ContactItemType::Pinterest, proto::CITEMTYPE_PINTEREST},
        {ContactItemType::Imgur, proto::CITEMTYPE_IMGUR},
        {ContactItemType::Flickr, proto::CITEMTYPE_FLICKR},
        {ContactItemType::Dribble, proto::CITEMTYPE_DRIBBLE},
        {ContactItemType::Behance, proto::CITEMTYPE_BEHANCE},
        {ContactItemType::Deviantart, proto::CITEMTYPE_DEVIANTART},
        {ContactItemType::Spotify, proto::CITEMTYPE_SPOTIFY},
        {ContactItemType::Itunes, proto::CITEMTYPE_ITUNES},
        {ContactItemType::Soundcloud, proto::CITEMTYPE_SOUNDCLOUD},
        {ContactItemType::Askfm, proto::CITEMTYPE_ASKFM},
        {ContactItemType::Ebay, proto::CITEMTYPE_EBAY},
        {ContactItemType::Etsy, proto::CITEMTYPE_ETSY},
        {ContactItemType::Openbazaar, proto::CITEMTYPE_OPENBAZAAR},
        {ContactItemType::Xboxlive, proto::CITEMTYPE_XBOXLIVE},
        {ContactItemType::Playstation, proto::CITEMTYPE_PLAYSTATION},
        {ContactItemType::Secondlife, proto::CITEMTYPE_SECONDLIFE},
        {ContactItemType::Warcraft, proto::CITEMTYPE_WARCRAFT},
        {ContactItemType::Alias, proto::CITEMTYPE_ALIAS},
        {ContactItemType::Acquaintance, proto::CITEMTYPE_ACQUAINTANCE},
        {ContactItemType::Friend, proto::CITEMTYPE_FRIEND},
        {ContactItemType::Spouse, proto::CITEMTYPE_SPOUSE},
        {ContactItemType::Sibling, proto::CITEMTYPE_SIBLING},
        {ContactItemType::Member, proto::CITEMTYPE_MEMBER},
        {ContactItemType::Colleague, proto::CITEMTYPE_COLLEAGUE},
        {ContactItemType::Parent, proto::CITEMTYPE_PARENT},
        {ContactItemType::Child, proto::CITEMTYPE_CHILD},
        {ContactItemType::Employer, proto::CITEMTYPE_EMPLOYER},
        {ContactItemType::Employee, proto::CITEMTYPE_EMPLOYEE},
        {ContactItemType::Citizen, proto::CITEMTYPE_CITIZEN},
        {ContactItemType::Photo, proto::CITEMTYPE_PHOTO},
        {ContactItemType::Gender, proto::CITEMTYPE_GENDER},
        {ContactItemType::Height, proto::CITEMTYPE_HEIGHT},
        {ContactItemType::Weight, proto::CITEMTYPE_WEIGHT},
        {ContactItemType::Hair, proto::CITEMTYPE_HAIR},
        {ContactItemType::Eye, proto::CITEMTYPE_EYE},
        {ContactItemType::Skin, proto::CITEMTYPE_SKIN},
        {ContactItemType::Ethnicity, proto::CITEMTYPE_ETHNICITY},
        {ContactItemType::Language, proto::CITEMTYPE_LANGUAGE},
        {ContactItemType::Degree, proto::CITEMTYPE_DEGREE},
        {ContactItemType::Certification, proto::CITEMTYPE_CERTIFICATION},
        {ContactItemType::Title, proto::CITEMTYPE_TITLE},
        {ContactItemType::Skill, proto::CITEMTYPE_SKILL},
        {ContactItemType::Award, proto::CITEMTYPE_AWARD},
        {ContactItemType::Likes, proto::CITEMTYPE_LIKES},
        {ContactItemType::Sexual, proto::CITEMTYPE_SEXUAL},
        {ContactItemType::Political, proto::CITEMTYPE_POLITICAL},
        {ContactItemType::Religious, proto::CITEMTYPE_RELIGIOUS},
        {ContactItemType::Birth, proto::CITEMTYPE_BIRTH},
        {ContactItemType::Secondarygraduation,
         proto::CITEMTYPE_SECONDARYGRADUATION},
        {ContactItemType::Universitygraduation,
         proto::CITEMTYPE_UNIVERSITYGRADUATION},
        {ContactItemType::Wedding, proto::CITEMTYPE_WEDDING},
        {ContactItemType::Accomplishment, proto::CITEMTYPE_ACCOMPLISHMENT},
        {ContactItemType::BTC, proto::CITEMTYPE_BTC},
        {ContactItemType::ETH, proto::CITEMTYPE_ETH},
        {ContactItemType::XRP, proto::CITEMTYPE_XRP},
        {ContactItemType::LTC, proto::CITEMTYPE_LTC},
        {ContactItemType::DAO, proto::CITEMTYPE_DAO},
        {ContactItemType::XEM, proto::CITEMTYPE_XEM},
        {ContactItemType::DASH, proto::CITEMTYPE_DASH},
        {ContactItemType::MAID, proto::CITEMTYPE_MAID},
        {ContactItemType::LSK, proto::CITEMTYPE_LSK},
        {ContactItemType::DOGE, proto::CITEMTYPE_DOGE},
        {ContactItemType::DGD, proto::CITEMTYPE_DGD},
        {ContactItemType::XMR, proto::CITEMTYPE_XMR},
        {ContactItemType::WAVES, proto::CITEMTYPE_WAVES},
        {ContactItemType::NXT, proto::CITEMTYPE_NXT},
        {ContactItemType::SC, proto::CITEMTYPE_SC},
        {ContactItemType::STEEM, proto::CITEMTYPE_STEEM},
        {ContactItemType::AMP, proto::CITEMTYPE_AMP},
        {ContactItemType::XLM, proto::CITEMTYPE_XLM},
        {ContactItemType::FCT, proto::CITEMTYPE_FCT},
        {ContactItemType::BTS, proto::CITEMTYPE_BTS},
        {ContactItemType::USD, proto::CITEMTYPE_USD},
        {ContactItemType::EUR, proto::CITEMTYPE_EUR},
        {ContactItemType::GBP, proto::CITEMTYPE_GBP},
        {ContactItemType::INR, proto::CITEMTYPE_INR},
        {ContactItemType::AUD, proto::CITEMTYPE_AUD},
        {ContactItemType::CAD, proto::CITEMTYPE_CAD},
        {ContactItemType::SGD, proto::CITEMTYPE_SGD},
        {ContactItemType::CHF, proto::CITEMTYPE_CHF},
        {ContactItemType::MYR, proto::CITEMTYPE_MYR},
        {ContactItemType::JPY, proto::CITEMTYPE_JPY},
        {ContactItemType::CNY, proto::CITEMTYPE_CNY},
        {ContactItemType::NZD, proto::CITEMTYPE_NZD},
        {ContactItemType::THB, proto::CITEMTYPE_THB},
        {ContactItemType::HUF, proto::CITEMTYPE_HUF},
        {ContactItemType::AED, proto::CITEMTYPE_AED},
        {ContactItemType::HKD, proto::CITEMTYPE_HKD},
        {ContactItemType::MXN, proto::CITEMTYPE_MXN},
        {ContactItemType::ZAR, proto::CITEMTYPE_ZAR},
        {ContactItemType::PHP, proto::CITEMTYPE_PHP},
        {ContactItemType::SEC, proto::CITEMTYPE_SEK},
        {ContactItemType::TNBTC, proto::CITEMTYPE_TNBTC},
        {ContactItemType::TNXRP, proto::CITEMTYPE_TNXRP},
        {ContactItemType::TNLTX, proto::CITEMTYPE_TNLTC},
        {ContactItemType::TNXEM, proto::CITEMTYPE_TNXEM},
        {ContactItemType::TNDASH, proto::CITEMTYPE_TNDASH},
        {ContactItemType::TNMAID, proto::CITEMTYPE_TNMAID},
        {ContactItemType::TNLSK, proto::CITEMTYPE_TNLSK},
        {ContactItemType::TNDOGE, proto::CITEMTYPE_TNDOGE},
        {ContactItemType::TNXMR, proto::CITEMTYPE_TNXMR},
        {ContactItemType::TNWAVES, proto::CITEMTYPE_TNWAVES},
        {ContactItemType::TNNXT, proto::CITEMTYPE_TNNXT},
        {ContactItemType::TNSC, proto::CITEMTYPE_TNSC},
        {ContactItemType::TNSTEEM, proto::CITEMTYPE_TNSTEEM},
        {ContactItemType::Philosophy, proto::CITEMTYPE_PHILOSOPHY},
        {ContactItemType::Met, proto::CITEMTYPE_MET},
        {ContactItemType::Fan, proto::CITEMTYPE_FAN},
        {ContactItemType::Supervisor, proto::CITEMTYPE_SUPERVISOR},
        {ContactItemType::Subordinate, proto::CITEMTYPE_SUBORDINATE},
        {ContactItemType::Contact, proto::CITEMTYPE_CONTACT},
        {ContactItemType::Refreshed, proto::CITEMTYPE_REFRESHED},
        {ContactItemType::BOT, proto::CITEMTYPE_BOT},
        {ContactItemType::BCH, proto::CITEMTYPE_BCH},
        {ContactItemType::TNBCH, proto::CITEMTYPE_TNBCH},
        {ContactItemType::Owner, proto::CITEMTYPE_OWNER},
        {ContactItemType::Property, proto::CITEMTYPE_PROPERTY},
        {ContactItemType::Unknown, proto::CITEMTYPE_UNKNOWN},
        {ContactItemType::Ethereum_Olympic, proto::CITEMTYPE_ETHEREUM_OLYMPIC},
        {ContactItemType::Ethereum_Classic, proto::CITEMTYPE_ETHEREUM_CLASSIC},
        {ContactItemType::Ethereum_Expanse, proto::CITEMTYPE_ETHEREUM_EXPANSE},
        {ContactItemType::Ethereum_Morden, proto::CITEMTYPE_ETHEREUM_MORDEN},
        {ContactItemType::Ethereum_Ropsten, proto::CITEMTYPE_ETHEREUM_ROPSTEN},
        {ContactItemType::Ethereum_Rinkeby, proto::CITEMTYPE_ETHEREUM_RINKEBY},
        {ContactItemType::Ethereum_Kovan, proto::CITEMTYPE_ETHEREUM_KOVAN},
        {ContactItemType::Ethereum_Sokol, proto::CITEMTYPE_ETHEREUM_SOKOL},
        {ContactItemType::Ethereum_POA, proto::CITEMTYPE_ETHEREUM_POA},
        {ContactItemType::PKT, proto::CITEMTYPE_PKT},
        {ContactItemType::TNPKT, proto::CITEMTYPE_TNPKT},
        {ContactItemType::Regtest, proto::CITEMTYPE_REGTEST},
    };

    return map;
}

auto contactsectionname_map() noexcept -> const ContactSectionNameMap&
{
    static const auto map = ContactSectionNameMap{
        {ContactSectionName::Error, proto::CONTACTSECTION_ERROR},
        {ContactSectionName::Scope, proto::CONTACTSECTION_SCOPE},
        {ContactSectionName::Identifier, proto::CONTACTSECTION_IDENTIFIER},
        {ContactSectionName::Address, proto::CONTACTSECTION_ADDRESS},
        {ContactSectionName::Communication,
         proto::CONTACTSECTION_COMMUNICATION},
        {ContactSectionName::Profile, proto::CONTACTSECTION_PROFILE},
        {ContactSectionName::Relationship, proto::CONTACTSECTION_RELATIONSHIP},
        {ContactSectionName::Descriptor, proto::CONTACTSECTION_DESCRIPTOR},
        {ContactSectionName::Event, proto::CONTACTSECTION_EVENT},
        {ContactSectionName::Contract, proto::CONTACTSECTION_CONTRACT},
        {ContactSectionName::Procedure, proto::CONTACTSECTION_PROCEDURE},
    };

    return map;
}

auto translate(const ContactItemAttribute in) noexcept
    -> proto::ContactItemAttribute
{
    try {
        return contactitemattribute_map().at(in);
    } catch (...) {
        return proto::CITEMATTR_ERROR;
    }
}

auto translate(const ContactItemType in) noexcept -> proto::ContactItemType
{
    try {
        return contactitemtype_map().at(in);
    } catch (...) {
        return proto::CITEMTYPE_ERROR;
    }
}

auto translate(const ContactSectionName in) noexcept
    -> proto::ContactSectionName
{
    try {
        return contactsectionname_map().at(in);
    } catch (...) {
        return proto::CONTACTSECTION_ERROR;
    }
}

auto translate(const proto::ContactItemAttribute in) noexcept
    -> ContactItemAttribute
{
    static const auto map = reverse_arbitrary_map<
        ContactItemAttribute,
        contact::ContactItemAttribute,
        ContactItemAttributeReverseMap>(contactitemattribute_map());

    try {
        return map.at(in);
    } catch (...) {
        return ContactItemAttribute::Error;
    }
}

auto translate(const proto::ContactItemType in) noexcept -> ContactItemType
{
    static const auto map = reverse_arbitrary_map<
        ContactItemType,
        contact::ContactItemType,
        ContactItemTypeReverseMap>(contactitemtype_map());

    try {
        return map.at(in);
    } catch (...) {
        return ContactItemType::Error;
    }
}

auto translate(const proto::ContactSectionName in) noexcept
    -> ContactSectionName
{
    static const auto map = reverse_arbitrary_map<
        ContactSectionName,
        contact::ContactSectionName,
        ContactSectionNameReverseMap>(contactsectionname_map());

    try {
        return map.at(in);
    } catch (...) {
        return ContactSectionName::Error;
    }
}
}  // namespace opentxs::contact::internal
