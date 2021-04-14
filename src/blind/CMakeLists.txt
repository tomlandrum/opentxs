# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(
  opentxs-blind OBJECT
  "Blind.cpp"
  "Mint.cpp"
  "Mint.hpp"
  "Purse.cpp"
  "Purse.hpp"
  "Token.cpp"
  "Token.hpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/blind/CashType.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/blind/Mint.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/blind/Purse.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/blind/PurseType.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/blind/Token.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/blind/TokenState.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/blind/Types.hpp"
)

target_link_libraries(
  opentxs-blind PRIVATE opentxs::messages opentxs-api-crypto
)
target_include_directories(
  opentxs-blind SYSTEM PRIVATE "${opentxs_SOURCE_DIR}/deps/"
)

if(CASH_LUCRE_EXPORT)
  target_sources(
    opentxs-blind
    PRIVATE
      "Lucre.cpp"
      "Lucre.hpp"
      "mint/Lucre.cpp"
      "mint/Lucre.hpp"
      "token/Lucre.cpp"
      "token/Lucre.hpp"
  )
  target_include_directories(
    opentxs-blind SYSTEM PRIVATE "${opentxs_SOURCE_DIR}/deps/lucre/include"
                                 "${OPENSSL_INCLUDE_DIR}"
  )
endif()

if(OT_OPENSSL_FLAVOR_LIBRESSL)
  target_compile_definitions(opentxs-blind PRIVATE OT_BUNDLED_LIBRESSL)
endif()

if(OT_LUCRE_DEBUG)
  target_compile_definitions(opentxs-blind PRIVATE OT_LUCRE_DEBUG=1)
else()
  target_compile_definitions(opentxs-blind PRIVATE OT_LUCRE_DEBUG=0)
endif()

target_sources(opentxs-blind PRIVATE ${cxx-install-headers})
target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-blind>)
add_dependencies(opentxs-blind generated_code)

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/blind
    COMPONENT dev
  )
endif()
