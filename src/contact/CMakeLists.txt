# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(
  opentxs-contact OBJECT
  "Contact.cpp"
  "ContactData.cpp"
  "ContactGroup.cpp"
  "ContactItem.cpp"
  "ContactSection.cpp"
  "Internal.cpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/contact/Contact.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/contact/ContactData.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/contact/ContactGroup.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/contact/ContactItem.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/contact/ContactItemAttribute.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/contact/ContactItemType.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/contact/ContactSection.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/contact/ContactSectionName.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/contact/Types.hpp"
)
target_link_libraries(opentxs-contact PRIVATE opentxs::messages)
target_include_directories(
  opentxs-contact SYSTEM PRIVATE "${opentxs_SOURCE_DIR}/deps/"
)
target_sources(opentxs-contact PRIVATE ${cxx-install-headers})
target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-contact>)
add_dependencies(opentxs-contact generated_code)

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/contact
    COMPONENT dev
  )
endif()
