# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_subdirectory(basket)
add_subdirectory(peer)

add_library(
  opentxs-core-contract OBJECT
  "${opentxs_SOURCE_DIR}/src/internal/core/contract/Contract.hpp"
  "Contract.cpp"
  "CurrencyContract.cpp"
  "CurrencyContract.hpp"
  "SecurityContract.cpp"
  "SecurityContract.hpp"
  "ServerContract.cpp"
  "ServerContract.hpp"
  "Signable.cpp"
  "Signable.hpp"
  "UnitDefinition.cpp"
  "UnitDefinition.hpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/core/contract/CurrencyContract.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/contract/ProtocolVersion.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/contract/SecurityContract.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/contract/ServerContract.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/contract/Signable.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/contract/Types.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/contract/UnitDefinition.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/contract/UnitType.hpp"
)
target_link_libraries(opentxs-core-contract PRIVATE opentxs::messages)
target_include_directories(
  opentxs-core-contract SYSTEM PRIVATE "${opentxs_SOURCE_DIR}/deps/"
)
target_sources(opentxs-core-contract PRIVATE ${cxx-install-headers})
target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-core-contract>)
add_dependencies(opentxs-core-contract generated_code)

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/core/contract
    COMPONENT dev
  )
endif()
