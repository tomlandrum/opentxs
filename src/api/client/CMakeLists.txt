# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_subdirectory(blockchain)

add_library(
  opentxs-api-client OBJECT
  "Activity.cpp"
  "${opentxs_SOURCE_DIR}/src/internal/api/client/Client.hpp"
  "${opentxs_SOURCE_DIR}/src/internal/api/client/Factory.hpp"
  "Activity.hpp"
  "Blockchain.cpp"
  "Blockchain.hpp"
  "Client.cpp"
  "Contacts.cpp"
  "Contacts.hpp"
  "Factory.cpp"
  "Factory.hpp"
  "Issuer.cpp"
  "Issuer.hpp"
  "Manager.cpp"
  "Manager.hpp"
  "OTX.cpp"
  "OTX.hpp"
  "Pair.cpp"
  "Pair.hpp"
  "ServerAction.cpp"
  "ServerAction.hpp"
  "UI.cpp"
  "UI.hpp"
  "Wallet.cpp"
  "Wallet.hpp"
  "Workflow.cpp"
  "Workflow.hpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/Activity.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/Blockchain.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/Contacts.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/Issuer.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/Manager.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/OTX.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/Pair.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/PaymentWorkflowState.hpp"    
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/PaymentWorkflowType.hpp"
	"${opentxs_SOURCE_DIR}/include/opentxs/api/client/ServerAction.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/Types.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/UI.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/api/client/Workflow.hpp"
)
target_link_libraries(
  opentxs-api-client
  PRIVATE
    Boost::headers
    opentxs::messages
    bech32
)
target_include_directories(
  opentxs-api-client SYSTEM PRIVATE "${opentxs_SOURCE_DIR}/deps/"
                                    "${opentxs_SOURCE_DIR}/deps/bech32/ref/c++"
)

if(OT_QT_EXPORT)
  target_link_libraries(opentxs-api-client PRIVATE Qt5::Core Qt5::Gui)
  target_link_libraries(opentxs PRIVATE Qt5::Core Qt5::Gui)

  if(OT_QML_EXPORT)
    target_link_libraries(opentxs-api-client PUBLIC Qt5::Qml)
    target_link_libraries(opentxs PUBLIC Qt5::Qml)
    target_compile_definitions(opentxs-api-client PRIVATE -DOT_WITH_QML=1)
  else()
    target_compile_definitions(opentxs-api-client PRIVATE -DOT_WITH_QML=0)
  endif()
endif()

if(OT_BLOCKCHAIN_EXPORT)
  target_link_libraries(opentxs-api-client PRIVATE lmdb)
  target_link_libraries(opentxs PRIVATE lmdb)
endif()

target_sources(opentxs-api-client PRIVATE ${cxx-install-headers})
target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-api-client>)
add_dependencies(opentxs-api-client generated_code)

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/api/client
    COMPONENT dev
  )
endif()
