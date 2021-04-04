# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_subdirectory(contract)
add_subdirectory(cron)
add_subdirectory(crypto)
add_subdirectory(identifier)
add_subdirectory(otprotob)
add_subdirectory(recurring)
add_subdirectory(script)
add_subdirectory(trade)
add_subdirectory(transaction)
add_subdirectory(util)

add_library(
  opentxs-core OBJECT
  "${opentxs_SOURCE_DIR}/include/opentxs/core/UniqueQueue.hpp"
  "${opentxs_SOURCE_DIR}/src/internal/core/Core.hpp"
  "${opentxs_SOURCE_DIR}/src/internal/core/identifier/Identifier.hpp"
  "Account.cpp"
  "AccountList.cpp"
  "AccountVisitor.cpp"
  "Armored.cpp"
  "Armored.hpp"
  "Cheque.cpp"
  "Contract.cpp"
  "Core.cpp"
  "Data.cpp"
  "Data.hpp"
  "Flag.cpp"
  "Flag.hpp"
  "Identifier.cpp"
  "Identifier.hpp"
  "Instrument.cpp"
  "Item.cpp"
  "Ledger.cpp"
  "Log.cpp"
  "LogSource.cpp"
  "Message.cpp"
  "NumList.cpp"
  "NymFile.cpp"
  "NymFile.hpp"
  "OTStorage.cpp"
  "OTTrackable.cpp"
  "OTTransaction.cpp"
  "OTTransactionType.cpp"
  "PasswordPrompt.cpp"
  "Secret.cpp"
  "Secret.hpp"
  "Shutdown.cpp"
  "Shutdown.hpp"
  "StateMachine.cpp"
  "StateMachine.hpp"
  "String.cpp"
  "String.hpp"
  "StringXML.cpp"
  "StringXML.hpp"
  "Worker.hpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Account.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/AccountList.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/AccountVisitor.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Armored.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Cheque.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Contract.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Data.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Flag.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Helpers.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Identifier.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Instrument.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Item.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Ledger.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Lockable.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Log.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/LogSource.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Message.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/NumList.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/NymFile.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/OTTrackable.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/OTTransaction.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/OTTransactionType.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/PasswordPrompt.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/Secret.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/String.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/core/StringXML.hpp"
)
target_link_libraries(
  opentxs-core
  PRIVATE
    Boost::headers
    ZLIB::ZLIB
    opentxs-core-otprotob
    opentxs::messages
    unofficial-sodium::sodium
)
target_link_libraries(opentxs PUBLIC ZLIB::ZLIB unofficial-sodium::sodium)
target_include_directories(
  opentxs-core SYSTEM PRIVATE "${opentxs_SOURCE_DIR}/deps/"
)
target_sources(opentxs-core PRIVATE ${cxx-install-headers})
target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-core>)
add_dependencies(opentxs-core generated_code)

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/core
    COMPONENT dev
  )
endif()
