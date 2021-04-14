# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_subdirectory(credential)
add_subdirectory(wot)

add_library(
  opentxs-identity OBJECT
  "Authority.cpp"
  "Authority.hpp"
  "Nym.cpp"
  "Nym.hpp"
  "Source.cpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/Authority.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/CredentialRole.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/CredentialType.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/Nym.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/Source.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/SourceProofType.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/SourceType.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/identity/Types.hpp"
)
target_link_libraries(
  opentxs-identity PRIVATE opentxs::messages unofficial-sodium::sodium
)
target_link_libraries(opentxs PUBLIC unofficial-sodium::sodium)
target_include_directories(
  opentxs-identity SYSTEM PRIVATE "${opentxs_SOURCE_DIR}/deps/"
)
target_sources(opentxs-identity PRIVATE ${cxx-install-headers})
target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-identity>)
add_dependencies(opentxs-identity generated_code)

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/identity
    COMPONENT dev
  )
endif()
