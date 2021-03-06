# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(
  opentxs-api-client-blockchain OBJECT
  "AccountCache.cpp"
  "BalanceLists.cpp"
  "Imp.cpp"
  "Imp.hpp"
)
target_link_libraries(
  opentxs-api-client-blockchain PRIVATE opentxs::messages Boost::headers
)
target_include_directories(
  opentxs-api-client-blockchain SYSTEM
  PRIVATE "${opentxs_SOURCE_DIR}/deps/"
          "${opentxs_SOURCE_DIR}/deps/bech32/ref/c++"
)

if(OT_BLOCKCHAIN_EXPORT)
  target_sources(
    opentxs-api-client-blockchain
    PRIVATE
      "BalanceOracle.cpp"
      "Imp_blockchain.cpp"
      "Imp_blockchain.hpp"
      "NonNull.cpp"
  )
  target_link_libraries(
    opentxs-api-client-blockchain PRIVATE "${OT_ZMQ_TARGET}"
  )
  target_link_libraries(opentxs PUBLIC "${OT_ZMQ_TARGET}")
else()
  target_sources(opentxs-api-client-blockchain PRIVATE "Null.cpp")
endif()

target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-api-client-blockchain>)
add_dependencies(opentxs-api-client-blockchain generated_code)
