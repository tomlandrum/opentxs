# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_subdirectory(client)
add_subdirectory(consensus)

add_library(
  opentxs-otx OBJECT
  "OTX.cpp"
  "Reply.cpp"
  "Reply.hpp"
  "Request.cpp"
  "Request.hpp"
)
set(cxx-install-headers "${opentxs_SOURCE_DIR}/include/opentxs/otx/Reply.hpp"
                        "${opentxs_SOURCE_DIR}/include/opentxs/otx/Request.hpp"
)
target_link_libraries(opentxs-otx PRIVATE opentxs::messages)
target_include_directories(
  opentxs-otx SYSTEM PRIVATE "${opentxs_SOURCE_DIR}/deps/"
)
target_sources(opentxs-otx PRIVATE ${cxx-install-headers})
target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-otx>)
add_dependencies(opentxs-otx generated_code)

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/otx
    COMPONENT dev
  )
endif()
