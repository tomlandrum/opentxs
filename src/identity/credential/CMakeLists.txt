# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(
  opentxs-identity-credential OBJECT
  "${opentxs_SOURCE_DIR}/src/internal/identity/credential/Credential.hpp"
  "Base.cpp"
  "Base.tpp"
  "Contact.cpp"
  "Contact.hpp"
  "Credential.cpp"
  "Key.cpp"
  "Key.hpp"
  "Primary.cpp"
  "Primary.hpp"
  "Secondary.cpp"
  "Secondary.hpp"
  "Verification.cpp"
  "Verification.hpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/credential/Base.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/credential/Contact.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/credential/Key.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/credential/Primary.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/credential/Secondary.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/credential/Verification.hpp"
)
target_link_libraries(opentxs-identity-credential PRIVATE opentxs::messages)
target_include_directories(
  opentxs-identity-credential SYSTEM PRIVATE "${opentxs_SOURCE_DIR}/deps/"
)
target_sources(opentxs-identity-credential PRIVATE ${cxx-install-headers})
target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-identity-credential>)
add_dependencies(opentxs-identity-credential generated_code)

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/identity/credential
    COMPONENT dev
  )
endif()
