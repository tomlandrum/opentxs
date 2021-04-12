// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                 // IWYU pragma: associated
#include "1_Internal.hpp"               // IWYU pragma: associated
#include "opentxs/contact/Contact.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/blockchain/AddressStyle.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/contact/ContactData.hpp"
#include "opentxs/contact/ContactGroup.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/contact/ContactSection.hpp"  // IWYU pragma: keep
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/crypto/PaymentCode.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/protobuf/Check.hpp"
#include "opentxs/protobuf/Contact.pb.h"
#include "opentxs/protobuf/ContactData.pb.h"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "opentxs/protobuf/ContactItem.pb.h"
#include "opentxs/protobuf/verify/ContactItem.hpp"
#include "opentxs/protobuf/verify/VerifyContacts.hpp"
#include "util/Container.hpp"

namespace opentxs
{
namespace proto
{
class Nym;
}  // namespace proto
}  // namespace opentxs

#define ID_BYTES 32

#define OT_METHOD "opentxs::Contact::"

using AddressStyle = opentxs::Contact::AddressStyle;

const std::map<AddressStyle, std::string> address_style_map_{
    {AddressStyle::P2PKH,
     std::to_string(static_cast<int>(AddressStyle::P2PKH))},
    {AddressStyle::P2SH, std::to_string(static_cast<int>(AddressStyle::P2SH))},
    {AddressStyle::P2WPKH,
     std::to_string(static_cast<int>(AddressStyle::P2WPKH))},
};
const std::map<std::string, AddressStyle> address_style_reverse_map_{
    opentxs::reverse_map(address_style_map_)};

auto translate_style(const std::string& in) noexcept -> AddressStyle;
auto translate_style(const std::string& in) noexcept -> AddressStyle
{
    try {

        return address_style_reverse_map_.at(in);
    } catch (...) {

        return AddressStyle::Unknown;
    }
}

auto translate_style(const AddressStyle& in) noexcept -> std::string;
auto translate_style(const AddressStyle& in) noexcept -> std::string
{
    try {

        return address_style_map_.at(in);
    } catch (...) {

        return std::to_string(static_cast<int>(AddressStyle::Unknown));
    }
}

namespace opentxs
{
struct Contact::Imp {
    const api::client::internal::Manager& api_;
    VersionNumber version_{0};
    std::string label_{""};
    mutable std::mutex lock_{};
    const OTIdentifier id_;
    OTIdentifier parent_;
    OTNymID primary_nym_;
    std::map<OTNymID, Nym_p> nyms_;
    std::set<OTIdentifier> merged_children_;
    std::unique_ptr<ContactData> contact_data_{};
    mutable std::shared_ptr<ContactData> cached_contact_data_{};
    std::atomic<std::uint64_t> revision_{0};

    static auto check_version(
        const VersionNumber in,
        const VersionNumber targetVersion) -> VersionNumber
    {
        // Upgrade version
        if (targetVersion > in) { return targetVersion; }

        return in;
    }

    static auto generate_id(const api::internal::Core& api) -> OTIdentifier
    {
        auto& encode = api.Crypto().Encode();
        auto random = Data::Factory();
        encode.Nonce(ID_BYTES, random);

        return api.Factory().Identifier(random->Bytes());
    }

    static auto translate(
        const api::client::internal::Manager& api,
        const proto::ContactItemType chain,
        const std::string& value,
        const std::string& subtype) noexcept(false)
        -> Contact::BlockchainAddress
    {
        auto output = BlockchainAddress{
            api.Factory().Data(value, StringStyle::Hex),
            translate_style(subtype),
            Translate(chain)};
        auto& [outBytes, outStyle, outChain] = output;
        const auto bad = outBytes->empty() ||
                         (AddressStyle::Unknown == outStyle) ||
                         (BlockchainType::Unknown == outChain);

        if (bad) { throw std::runtime_error("Invalid address"); }

        return output;
    }

    Imp(const api::client::internal::Manager& api,
        const proto::Contact& serialized)
        : api_(api)
        , version_(check_version(serialized.version(), OT_CONTACT_VERSION))
        , label_(serialized.label())
        , lock_()
        , id_(api_.Factory().Identifier(serialized.id()))
        , parent_(api_.Factory().Identifier(serialized.mergedto()))
        , primary_nym_(api_.Factory().NymID())
        , nyms_()
        , merged_children_()
        , contact_data_(new ContactData(
              api_,
              serialized.id(),
              CONTACT_CONTACT_DATA_VERSION,
              CONTACT_CONTACT_DATA_VERSION,
              ContactData::SectionMap{}))
        , cached_contact_data_()
        , revision_(serialized.revision())
    {
        if (serialized.has_contactdata()) {
            contact_data_.reset(new ContactData(
                api_,
                serialized.id(),
                CONTACT_CONTACT_DATA_VERSION,
                serialized.contactdata()));
        }

        OT_ASSERT(contact_data_);

        for (const auto& child : serialized.merged()) {
            merged_children_.emplace(api_.Factory().Identifier(child));
        }

        init_nyms();
    }

    Imp(const api::client::internal::Manager& api, const std::string& label)
        : api_(api)
        , version_(OT_CONTACT_VERSION)
        , label_(label)
        , lock_()
        , id_(generate_id(api_))
        , parent_(api_.Factory().Identifier())
        , primary_nym_(api_.Factory().NymID())
        , nyms_()
        , merged_children_()
        , contact_data_(nullptr)
        , cached_contact_data_()
        , revision_(1)
    {
        contact_data_.reset(new ContactData(
            api_,
            String::Factory(id_)->Get(),
            CONTACT_CONTACT_DATA_VERSION,
            CONTACT_CONTACT_DATA_VERSION,
            ContactData::SectionMap{}));

        OT_ASSERT(contact_data_);
    }

    auto add_claim(const std::shared_ptr<ContactItem>& item) -> bool
    {
        Lock lock(lock_);

        return add_claim(lock, item);
    }

    auto add_claim(const Lock& lock, const std::shared_ptr<ContactItem>& item)
        -> bool
    {
        OT_ASSERT(verify_write_lock(lock));

        if (false == bool(item)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Null claim.").Flush();

            return false;
        }

        const auto version = std::make_pair(item->Version(), item->Section());
        const proto::ContactItem serialized(*item);

        if (false ==
            proto::Validate<proto::ContactItem>(
                serialized, VERBOSE, proto::ClaimType::Indexed, version)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid claim.").Flush();

            return false;
        }

        add_verified_claim(lock, item);

        return true;
    }

    auto add_nym(const Lock& lock, const Nym_p& nym, const bool primary) -> bool
    {
        OT_ASSERT(verify_write_lock(lock));

        if (false == bool(nym)) { return false; }

        const auto contactType = type(lock);
        const auto nymType = ExtractType(*nym);
        const bool haveType = (proto::CITEMTYPE_ERROR != contactType) &&
                              (proto::CITEMTYPE_UNKNOWN != contactType);
        const bool typeMismatch = (contactType != nymType);

        if (haveType && typeMismatch) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong nym type.").Flush();

            return false;
        }

        const auto& id = nym->ID();
        const bool needPrimary = (0 == nyms_.size());
        const bool isPrimary = needPrimary || primary;
        nyms_[id] = nym;

        if (isPrimary) { primary_nym_ = id; }

        add_nym_claim(lock, id, isPrimary);

        return true;
    }

    void add_nym_claim(
        const Lock& lock,
        const identifier::Nym& nymID,
        const bool primary)
    {
        OT_ASSERT(verify_write_lock(lock));

        std::set<proto::ContactItemAttribute> attr{
            proto::CITEMATTR_LOCAL, proto::CITEMATTR_ACTIVE};

        if (primary) { attr.emplace(proto::CITEMATTR_PRIMARY); }

        std::shared_ptr<ContactItem> claim{nullptr};
        claim.reset(new ContactItem(
            api_,
            String::Factory(id_)->Get(),
            CONTACT_CONTACT_DATA_VERSION,
            CONTACT_CONTACT_DATA_VERSION,
            proto::CONTACTSECTION_RELATIONSHIP,
            proto::CITEMTYPE_CONTACT,
            String::Factory(nymID)->Get(),
            attr,
            NULL_START,
            NULL_END,
            ""));

        add_claim(lock, claim);
    }

    void add_verified_claim(
        const Lock& lock,
        const std::shared_ptr<ContactItem>& item)
    {
        OT_ASSERT(verify_write_lock(lock));
        OT_ASSERT(contact_data_);

        contact_data_.reset(new ContactData(contact_data_->AddItem(item)));

        OT_ASSERT(contact_data_);

        revision_++;
        cached_contact_data_.reset();
    }

    void init_nyms()
    {
        OT_ASSERT(contact_data_);

        const auto nyms = contact_data_->Group(
            proto::CONTACTSECTION_RELATIONSHIP, proto::CITEMTYPE_CONTACT);

        if (false == bool(nyms)) { return; }

        // TODO conversion
        primary_nym_ = api_.Factory().NymID(nyms->Primary().str());

        for (const auto& it : *nyms) {
            const auto& item = it.second;

            OT_ASSERT(item);

            const auto nymID = api_.Factory().NymID(item->Value());
            auto& nym = nyms_[nymID];
            nym = api_.Wallet().Nym(nymID);

            if (false == bool(nym)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to load nym ")(
                    nymID)(".")
                    .Flush();
            }
        }
    }

    auto merged_data(const Lock& lock) const -> std::shared_ptr<ContactData>
    {
        OT_ASSERT(contact_data_);
        OT_ASSERT(verify_write_lock(lock));

        if (cached_contact_data_) { return cached_contact_data_; }

        cached_contact_data_.reset(new ContactData(*contact_data_));
        auto& output = cached_contact_data_;

        OT_ASSERT(output);

        if (false == primary_nym_->empty()) {
            try {
                auto& primary = nyms_.at(primary_nym_);

                if (primary) {
                    output.reset(new ContactData(*output + primary->Claims()));
                }
            } catch (const std::out_of_range&) {
            }
        }

        for (const auto& it : nyms_) {
            const auto& nymID = it.first;
            const auto& nym = it.second;

            if (false == bool(nym)) { continue; }

            if (nymID == primary_nym_) { continue; }

            output.reset(new ContactData(*output + nym->Claims()));
        }

        return output;
    }

    auto payment_codes(const Lock& lock, const proto::ContactItemType currency)
        const -> std::shared_ptr<ContactGroup>
    {
        const auto data = merged_data(lock);

        if (false == bool(data)) { return {}; }

        return data->Group(proto::CONTACTSECTION_PROCEDURE, currency);
    }

    auto type(const Lock& lock) const -> proto::ContactItemType
    {
        OT_ASSERT(verify_write_lock(lock));

        const auto data = merged_data(lock);

        if (false == bool(data)) { return proto::CITEMTYPE_ERROR; }

        return data->Type();
    }

    void update_label(const Lock& lock, const identity::Nym& nym)
    {
        OT_ASSERT(verify_write_lock(lock));

        if (false == label_.empty()) { return; }

        label_ = ExtractLabel(nym);
    }

    auto verify_write_lock(const Lock& lock) const -> bool
    {
        if (lock.mutex() != &lock_) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Incorrect mutex.").Flush();

            return false;
        }

        if (false == lock.owns_lock()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Lock not owned.").Flush();

            return false;
        }

        return true;
    }
};

Contact::Contact(
    const api::client::internal::Manager& api,
    const proto::Contact& serialized)
    : imp_(std::make_unique<Imp>(api, serialized))
{
    OT_ASSERT(imp_);
}

Contact::Contact(
    const api::client::internal::Manager& api,
    const std::string& label)
    : imp_(std::make_unique<Imp>(api, label))
{
    OT_ASSERT(imp_);
}

Contact::operator proto::Contact() const
{
    Lock lock(imp_->lock_);
    proto::Contact output{};
    output.set_version(imp_->version_);
    output.set_id(String::Factory(imp_->id_)->Get());
    output.set_revision(imp_->revision_);
    output.set_label(imp_->label_);

    if (imp_->contact_data_) {
        auto& data = *output.mutable_contactdata();
        data = imp_->contact_data_->Serialize();
    }

    output.set_mergedto(String::Factory(imp_->parent_)->Get());

    for (const auto& child : imp_->merged_children_) {
        output.add_merged(String::Factory(child)->Get());
    }

    return output;
}

auto Contact::operator+=(Contact& rhs) -> Contact&
{
    Lock rLock(rhs.imp_->lock_, std::defer_lock);
    Lock lock(imp_->lock_, std::defer_lock);
    std::lock(rLock, lock);

    if (imp_->label_.empty()) { imp_->label_ = rhs.imp_->label_; }

    rhs.imp_->parent_ = imp_->id_;

    if (imp_->primary_nym_->empty()) {
        imp_->primary_nym_ = rhs.imp_->primary_nym_;
    }

    for (const auto& it : rhs.imp_->nyms_) {
        const auto& id = it.first;
        const auto& nym = it.second;

        if (0 == imp_->nyms_.count(id)) { imp_->nyms_.emplace(id, nym); }
    }

    rhs.imp_->nyms_.clear();

    for (const auto& it : rhs.imp_->merged_children_) {
        imp_->merged_children_.insert(it);
    }

    imp_->merged_children_.insert(rhs.imp_->id_);
    rhs.imp_->merged_children_.clear();

    if (imp_->contact_data_) {
        if (rhs.imp_->contact_data_) {
            imp_->contact_data_.reset(new ContactData(
                *imp_->contact_data_ + *rhs.imp_->contact_data_));
        }
    } else {
        if (rhs.imp_->contact_data_) {
            imp_->contact_data_.reset(
                new ContactData(*rhs.imp_->contact_data_));
        }
    }

    rhs.imp_->contact_data_.reset();
    imp_->cached_contact_data_.reset();
    rhs.imp_->cached_contact_data_.reset();

    return *this;
}

auto Contact::AddBlockchainAddress(
    const std::string& address,
    const proto::ContactItemType currency) -> bool
{
    const auto& api = imp_->api_;
    auto [bytes, style, chains, supported] =
        api.Blockchain().DecodeAddress(address);
    const auto bad =
        bytes->empty() || (AddressStyle::Unknown == style) || chains.empty();

    if (bad) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to decode address")
            .Flush();

        return false;
    }

    const auto type = Translate(currency);

    if (0 == chains.count(type)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Address is not valid for specified chain")
            .Flush();

        return false;
    }

    return AddBlockchainAddress(style, type, bytes);
}

auto Contact::AddBlockchainAddress(
    const api::client::blockchain::AddressStyle& style,
    const blockchain::Type chain,
    const opentxs::Data& bytes) -> bool
{
    Lock lock(imp_->lock_);

    std::shared_ptr<ContactItem> claim{nullptr};
    claim.reset(new ContactItem(
        imp_->api_,
        String::Factory(imp_->id_)->Get(),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        proto::CONTACTSECTION_ADDRESS,
        Translate(chain),
        bytes.asHex(),
        {proto::CITEMATTR_LOCAL, proto::CITEMATTR_ACTIVE},
        NULL_START,
        NULL_END,
        translate_style(style)));

    return imp_->add_claim(lock, claim);
}

auto Contact::AddEmail(
    const std::string& value,
    const bool primary,
    const bool active) -> bool
{
    if (value.empty()) { return false; }

    Lock lock(imp_->lock_);

    imp_->contact_data_.reset(
        new ContactData(imp_->contact_data_->AddEmail(value, primary, active)));

    OT_ASSERT(imp_->contact_data_);

    imp_->revision_++;
    imp_->cached_contact_data_.reset();

    return true;
}

auto Contact::AddNym(const Nym_p& nym, const bool primary) -> bool
{
    Lock lock(imp_->lock_);

    return imp_->add_nym(lock, nym, primary);
}

auto Contact::AddNym(const identifier::Nym& nymID, const bool primary) -> bool
{
    Lock lock(imp_->lock_);

    const bool needPrimary = (0 == imp_->nyms_.size());
    const bool isPrimary = needPrimary || primary;

    if (isPrimary) { imp_->primary_nym_ = nymID; }

    imp_->add_nym_claim(lock, nymID, isPrimary);

    imp_->revision_++;

    return true;
}

auto Contact::AddPaymentCode(
    const opentxs::PaymentCode& code,
    const bool primary,
    const proto::ContactItemType currency,
    const bool active) -> bool
{
    std::set<proto::ContactItemAttribute> attr{proto::CITEMATTR_LOCAL};

    if (active) { attr.emplace(proto::CITEMATTR_ACTIVE); }

    if (primary) { attr.emplace(proto::CITEMATTR_PRIMARY); }

    const std::string value = code.asBase58();
    std::shared_ptr<ContactItem> claim{nullptr};
    claim.reset(new ContactItem(
        imp_->api_,
        String::Factory(imp_->id_)->Get(),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        proto::CONTACTSECTION_PROCEDURE,
        currency,
        value,
        attr,
        NULL_START,
        NULL_END,
        ""));

    if (false == imp_->add_claim(claim)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to add claim.").Flush();

        return false;
    }

    return true;
}

auto Contact::AddPhoneNumber(
    const std::string& value,
    const bool primary,
    const bool active) -> bool
{
    if (value.empty()) { return false; }

    Lock lock(imp_->lock_);

    imp_->contact_data_.reset(new ContactData(
        imp_->contact_data_->AddPhoneNumber(value, primary, active)));

    OT_ASSERT(imp_->contact_data_);

    imp_->revision_++;
    imp_->cached_contact_data_.reset();

    return true;
}

auto Contact::AddSocialMediaProfile(
    const std::string& value,
    const proto::ContactItemType type,
    const bool primary,
    const bool active) -> bool
{
    if (value.empty()) { return false; }

    Lock lock(imp_->lock_);

    imp_->contact_data_.reset(
        new ContactData(imp_->contact_data_->AddSocialMediaProfile(
            value, type, primary, active)));

    OT_ASSERT(imp_->contact_data_);

    imp_->revision_++;
    imp_->cached_contact_data_.reset();

    return true;
}

auto Contact::Best(const ContactGroup& group) -> std::shared_ptr<ContactItem>
{
    if (0 == group.Size()) { return {}; }

    const auto primary = group.PrimaryClaim();

    if (primary) { return primary; }

    for (const auto& it : group) {
        const auto& claim = it.second;

        if (claim->isActive()) { return claim; }
    }

    return group.begin()->second;
}

auto Contact::BestEmail() const -> std::string
{
    Lock lock(imp_->lock_);
    const auto data = imp_->merged_data(lock);
    lock.unlock();

    if (false == bool(data)) { return {}; }

    return data->BestEmail();
}

auto Contact::BestPhoneNumber() const -> std::string
{
    Lock lock(imp_->lock_);
    const auto data = imp_->merged_data(lock);
    lock.unlock();

    if (false == bool(data)) { return {}; }

    return data->BestPhoneNumber();
}

auto Contact::BestSocialMediaProfile(const proto::ContactItemType type) const
    -> std::string
{
    Lock lock(imp_->lock_);
    const auto data = imp_->merged_data(lock);
    lock.unlock();

    if (false == bool(data)) { return {}; }

    return data->BestSocialMediaProfile(type);
}

auto Contact::BlockchainAddresses() const
    -> std::vector<Contact::BlockchainAddress>
{
    auto output = std::vector<BlockchainAddress>{};
    Lock lock(imp_->lock_);
    auto data = imp_->merged_data(lock);
    lock.unlock();

    if (false == bool(data)) { return {}; }

    const auto& version = data->Version();
    const auto section = data->Section(proto::CONTACTSECTION_ADDRESS);

    if (false == bool(section)) { return {}; }

    for (const auto& it : *section) {
        const auto& type = it.first;
        const auto& group = it.second;

        OT_ASSERT(group);

        const bool currency = proto::ValidContactItemType(
            {version, proto::CONTACTSECTION_CONTRACT}, type);

        if (false == currency) { continue; }

        for (const auto& inner : *group) {
            const auto& item = inner.second;

            OT_ASSERT(item);

            try {
                output.push_back(Imp::translate(
                    imp_->api_, type, item->Value(), item->Subtype()));
            } catch (...) {
                continue;
            }
        }
    }

    return output;
}

auto Contact::Data() const -> std::shared_ptr<ContactData>
{
    Lock lock(imp_->lock_);

    return imp_->merged_data(lock);
}

auto Contact::EmailAddresses(bool active) const -> std::string
{
    Lock lock(imp_->lock_);
    const auto data = imp_->merged_data(lock);
    lock.unlock();

    if (false == bool(data)) { return {}; }

    return data->EmailAddresses(active);
}

auto Contact::ExtractLabel(const identity::Nym& nym) -> std::string
{
    return nym.Claims().Name();
}

auto Contact::ExtractType(const identity::Nym& nym) -> proto::ContactItemType
{
    return nym.Claims().Type();
}

auto Contact::ID() const -> const Identifier& { return imp_->id_; }

auto Contact::Label() const -> const std::string& { return imp_->label_; }

auto Contact::LastUpdated() const -> std::time_t
{
    OT_ASSERT(imp_->contact_data_);

    const auto group = imp_->contact_data_->Group(
        proto::CONTACTSECTION_EVENT, proto::CITEMTYPE_REFRESHED);

    if (false == bool(group)) { return {}; }

    const auto claim = group->PrimaryClaim();

    if (false == bool(claim)) { return {}; }

    try {
        if (sizeof(int) == sizeof(std::time_t)) {

            return std::stoi(claim->Value());
        } else if (sizeof(long) == sizeof(std::time_t)) {

            return std::stol(claim->Value());
        } else if (sizeof(long long) == sizeof(std::time_t)) {

            return std::stoll(claim->Value());
        } else {
            OT_FAIL;
        }

    } catch (const std::out_of_range&) {

        return {};
    } catch (const std::invalid_argument&) {

        return {};
    }
}

auto opentxs::Contact::Nyms(const bool includeInactive) const
    -> std::vector<opentxs::OTNymID>
{
    Lock lock(imp_->lock_);
    const auto data = imp_->merged_data(lock);
    lock.unlock();

    if (false == bool(data)) { return {}; }

    const auto group = data->Group(
        proto::CONTACTSECTION_RELATIONSHIP, proto::CITEMTYPE_CONTACT);

    if (false == bool(group)) { return {}; }

    std::vector<OTNymID> output{};
    const auto& primaryID = group->Primary();

    for (const auto& it : *group) {
        const auto& item = it.second;

        OT_ASSERT(item);

        const auto& itemID = item->ID();

        if (false == (includeInactive || item->isActive())) { continue; }

        if (primaryID == itemID) {
            output.emplace(
                output.begin(), imp_->api_.Factory().NymID(item->Value()));
        } else {
            output.emplace(
                output.end(), imp_->api_.Factory().NymID(item->Value()));
        }
    }

    return output;
}

auto Contact::PaymentCode(
    const ContactData& data,
    const proto::ContactItemType currency) -> std::string
{
    auto group = data.Group(proto::CONTACTSECTION_PROCEDURE, currency);

    if (false == bool(group)) { return {}; }

    const auto item = Best(*group);

    if (false == bool(item)) { return {}; }

    return item->Value();
}

auto Contact::PaymentCode(const proto::ContactItemType currency) const
    -> std::string
{
    Lock lock(imp_->lock_);
    const auto data = imp_->merged_data(lock);
    lock.unlock();

    if (false == bool(data)) { return {}; }

    return PaymentCode(*data, currency);
}

auto Contact::PaymentCodes(const proto::ContactItemType currency) const
    -> std::vector<std::string>
{
    Lock lock(imp_->lock_);
    const auto group = imp_->payment_codes(lock, currency);
    lock.unlock();

    if (false == bool(group)) { return {}; }

    std::vector<std::string> output{};

    for (const auto& it : *group) {
        OT_ASSERT(it.second);

        const auto& item = *it.second;
        output.emplace_back(item.Value());
    }

    return output;
}

auto Contact::PhoneNumbers(bool active) const -> std::string
{
    Lock lock(imp_->lock_);
    const auto data = imp_->merged_data(lock);
    lock.unlock();

    if (false == bool(data)) { return {}; }

    return data->PhoneNumbers(active);
}

auto Contact::Print() const -> std::string
{
    Lock lock(imp_->lock_);
    std::stringstream out{};
    out << "Contact: " << String::Factory(imp_->id_)->Get() << ", version "
        << imp_->version_ << "revision " << imp_->revision_ << "\n"
        << "Label: " << imp_->label_ << "\n";

    if (false == imp_->parent_->empty()) {
        out << "Merged to: " << String::Factory(imp_->parent_)->Get() << "\n";
    }

    if (false == imp_->merged_children_.empty()) {
        out << "Merged contacts:\n";

        for (const auto& id : imp_->merged_children_) {
            out << " * " << String::Factory(id)->Get() << "\n";
        }
    }

    if (0 < imp_->nyms_.size()) {
        out << "Contains nyms:\n";

        for (const auto& it : imp_->nyms_) {
            const auto& id = it.first;
            out << " * " << String::Factory(id)->Get();

            if (id == imp_->primary_nym_) { out << " (primary)"; }

            out << "\n";
        }
    }

    auto data = imp_->merged_data(lock);

    if (data) { out << std::string(*data); }

    out << std::endl;

    return out.str();
}

auto Contact::RemoveNym(const identifier::Nym& nymID) -> bool
{
    Lock lock(imp_->lock_);

    auto result = imp_->nyms_.erase(nymID);

    if (imp_->primary_nym_ == nymID) {
        imp_->primary_nym_ = imp_->api_.Factory().NymID();
    }

    return (0 < result);
}

void Contact::SetLabel(const std::string& label)
{
    Lock lock(imp_->lock_);
    imp_->label_ = label;
    imp_->revision_++;

    for (const auto& it : imp_->nyms_) {
        const auto& nymID = it.first;
        imp_->api_.Wallet().SetNymAlias(nymID, label);
    }
}

auto Contact::SocialMediaProfiles(
    const proto::ContactItemType type,
    bool active) const -> std::string
{
    Lock lock(imp_->lock_);
    const auto data = imp_->merged_data(lock);
    lock.unlock();

    if (false == bool(data)) { return {}; }

    return data->SocialMediaProfiles(type, active);
}

auto Contact::SocialMediaProfileTypes() const
    -> const std::set<proto::ContactItemType>
{
    Lock lock(imp_->lock_);
    const auto data = imp_->merged_data(lock);
    lock.unlock();

    return data->SocialMediaProfileTypes();
}

auto Contact::Type() const -> proto::ContactItemType
{
    Lock lock(imp_->lock_);

    return imp_->type(lock);
}

void Contact::Update(const proto::Nym& serialized)
{
    auto nym = imp_->api_.Wallet().Nym(serialized);

    if (false == bool(nym)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid serialized nym.").Flush();

        return;
    }

    Lock lock(imp_->lock_);
    const auto& nymID = nym->ID();
    auto it = imp_->nyms_.find(nymID);

    if (imp_->nyms_.end() == it) {
        imp_->add_nym(lock, nym, false);
    } else {
        it->second = nym;
    }

    imp_->update_label(lock, *nym);
    std::shared_ptr<ContactItem> claim(new ContactItem(
        imp_->api_,
        String::Factory(imp_->id_)->Get(),
        CONTACT_CONTACT_DATA_VERSION,
        CONTACT_CONTACT_DATA_VERSION,
        proto::CONTACTSECTION_EVENT,
        proto::CITEMTYPE_REFRESHED,
        std::to_string(std::time(nullptr)),
        {proto::CITEMATTR_PRIMARY,
         proto::CITEMATTR_ACTIVE,
         proto::CITEMATTR_LOCAL},
        NULL_START,
        NULL_END,
        ""));
    imp_->add_claim(lock, claim);
}

Contact::~Contact() = default;

}  // namespace opentxs
