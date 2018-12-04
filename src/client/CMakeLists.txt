#[[
// clang-format off
]]#
# Copyright (c) 2018 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(MODULE_NAME opentxs-client)

set(cxx-sources
  Helpers.cpp
  NymData.cpp
  OT_API.cpp
  OTAPI_Exec.cpp
  OTAPI_Func.cpp
  OTClient.cpp
  OTMessageOutbuffer.cpp
  OTWallet.cpp
  SwigWrap.cpp
  Utility.cpp
)

set(cxx-install-headers
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/opentxs/client/Helpers.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/opentxs/client/NymData.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/opentxs/client/OTAPI_Exec.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/opentxs/client/OT_API.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/opentxs/client/OTClient.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/opentxs/client/OTMessageOutbuffer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/opentxs/client/OTWallet.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/opentxs/client/ServerAction.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/opentxs/client/SwigWrap.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/opentxs/client/Utility.hpp
)

set(cxx-headers
  ${cxx-install-headers}
  OTAPI_Func.hpp
)

if(WIN32)
  # suppress "decorated name length exceeded, name was truncated" warnings
  add_definitions("/wd4503")

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/windll.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/module.rc
    @ONLY
  )

  # we have an extra Windows-specific file to compile
  add_library(${MODULE_NAME} OBJECT
  ${cxx-sources}
  ${cxx-headers}
    ${CMAKE_CURRENT_BINARY_DIR}/module.rc
  )
else()
  add_library(${MODULE_NAME} OBJECT
    ${cxx-sources}
    ${cxx-headers}
  )
endif()

install(FILES ${cxx-install-headers}
        DESTINATION include/opentxs/client
        COMPONENT dev)

set_property(TARGET ${MODULE_NAME} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_lib_property(${MODULE_NAME})
target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../deps/")
#[[
// clang-format on
]]#
