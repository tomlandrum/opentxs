# Copyright (c) 2018 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(MODULE_NAME opentxs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PEDANTIC_CXX_FLAGS}")

set(dependency_include_dir
  ${CMAKE_CURRENT_SOURCE_DIR}/../deps/
)

if(KEYRING_GNOME)
  set(keyring libgnome-keyring.so)
endif()

add_subdirectory(iterator)
add_subdirectory(storage)
add_subdirectory(crypto)
add_subdirectory(core)
add_subdirectory(consensus)
add_subdirectory(ext)
add_subdirectory(identity)
add_subdirectory(network)
add_subdirectory(client)
add_subdirectory(api)
add_subdirectory(rpc)
add_subdirectory(server)
add_subdirectory(contact)
add_subdirectory(ui)
add_subdirectory(util)
add_subdirectory(otx)

if(TREZORCRYPTO_EXPORT)
  set(trezor $<TARGET_OBJECTS:TrezorCrypto>)
else()
  set(trezor "")
endif()

if(CASH_LUCRE_EXPORT)
  set(lucre $<TARGET_OBJECTS:lucre>)
else()
  set(lucre "")
endif()

if(OT_CASH_EXPORT)
  set(blind $<TARGET_OBJECTS:opentxs-blind>)
  add_subdirectory(blind)
else()
  set(blind "")
endif()

if(BLOCKCHAIN_EXPORT)
  add_subdirectory(blockchain)
endif()

if(OT_BUNDLED_OPENTXS_PROTO)
  set(bundled-opentxs-proto
    $<TARGET_OBJECTS:opentxs-proto-objects>
    $<TARGET_OBJECTS:verify>
  )
else()
  set(bundled-opentxs-proto "")
endif()

if(OT_BUNDLED_PROTOBUF)
  set_source_files_properties(
    ${bundled-protobuf-path}
    PROPERTIES
    EXTERNAL_OBJECT true
    GENERATED true
  )

  if(OT_CASH_EXPORT)
    add_dependencies(opentxs-blind protobuf)
  endif()

  if(CASH_LUCRE_EXPORT)
    add_dependencies(lucre protobuf)
  endif()

  if(TREZORCRYPTO_EXPORT)
    add_dependencies(TrezorCrypto protobuf)
  endif()

  if(OT_BUNDLED_OPENTXS_PROTO)
    add_dependencies(opentxs-proto-objects protobuf)
    add_dependencies(verify protobuf)
  endif()

  add_dependencies(otprotob protobuf)
  add_dependencies(base64 protobuf)
  add_dependencies(irrxml protobuf)
  add_dependencies(otprotob protobuf)
  add_dependencies(opentxs-api-client protobuf)
  add_dependencies(opentxs-api-crypto protobuf)
  add_dependencies(opentxs-api-network protobuf)
  add_dependencies(opentxs-api-server protobuf)
  add_dependencies(opentxs-api-storage protobuf)
  add_dependencies(opentxs-api protobuf)
  add_dependencies(opentxs-client protobuf)
  add_dependencies(opentxs-consensus protobuf)
  add_dependencies(opentxs-core-contract-basket protobuf)
  add_dependencies(opentxs-core-contract-peer protobuf)
  add_dependencies(opentxs-core-contract protobuf)
  add_dependencies(opentxs-core-crypto protobuf)
  add_dependencies(opentxs-core-transaction protobuf)
  add_dependencies(opentxs-core-util protobuf)
  add_dependencies(opentxs-core protobuf)
  add_dependencies(opentxs-crypto-key protobuf)
  add_dependencies(opentxs-crypto-library protobuf)
  add_dependencies(opentxs-crypto protobuf)
  add_dependencies(opentxs-cron protobuf)
  add_dependencies(opentxs-ext protobuf)
  add_dependencies(opentxs-identity-credential protobuf)
  add_dependencies(opentxs-identity protobuf)
  add_dependencies(opentxs-iterator protobuf)
  add_dependencies(opentxs-network-zeromq-zap protobuf)
  add_dependencies(opentxs-network-zeromq protobuf)
  add_dependencies(opentxs-network protobuf)
  add_dependencies(opentxs-otx protobuf)
  add_dependencies(opentxs-recurring protobuf)
  add_dependencies(opentxs-rpc protobuf)
  add_dependencies(opentxs-script protobuf)
  add_dependencies(opentxs-server protobuf)
  add_dependencies(opentxs-storage protobuf)
  add_dependencies(opentxs-storage-drivers protobuf)
  add_dependencies(opentxs-storage-tree protobuf)
  add_dependencies(opentxs-trade protobuf)
  add_dependencies(opentxs-contact protobuf)
  add_dependencies(opentxs-ui protobuf)
  add_dependencies(opentxs-util protobuf)
endif()

if(OT_BUNDLED_SSL)
  set_source_files_properties(
    ${OT_SSL_LIBRARIES}
    PROPERTIES
    EXTERNAL_OBJECT true
    GENERATED true
  )

  if(OT_CASH_EXPORT)
    add_dependencies(opentxs-blind libressl)
  endif()

  if(CASH_LUCRE_EXPORT)
    add_dependencies(lucre libressl)
  endif()

  add_dependencies(opentxs-core-crypto libressl)
  add_dependencies(opentxs-crypto-key libressl)
  add_dependencies(opentxs-crypto-library libressl)
endif()

if(OT_BUNDLED_SODIUM)
  set_source_files_properties(
    ${bundled-sodium-path}
    PROPERTIES
    EXTERNAL_OBJECT true
    GENERATED true
  )
  add_dependencies(opentxs-core sodium)
  add_dependencies(opentxs-crypto-key sodium)
  add_dependencies(opentxs-crypto-library sodium)
endif()

if(OT_BUNDLED_SECP256K1)
  set_source_files_properties(
    ${bundled-secp256k1-path}
    PROPERTIES
    EXTERNAL_OBJECT true
    GENERATED true
  )
  add_dependencies(opentxs-crypto-library secp256k1)
endif()

if(OT_STORAGE_SQLITE AND OT_BUNDLED_SQLITE)
  set(bundled-sqlite $<TARGET_OBJECTS:opentxs-bundled-sqlite>)
else()
  set(bundled-sqlite "")
endif()

if(OT_BUNDLED_LMDB)
  set_source_files_properties(
    ${bundled-lmdb-path}
    PROPERTIES
    EXTERNAL_OBJECT true
    GENERATED true
  )
  add_dependencies(opentxs-storage-drivers lmdb)
endif()

if(OT_BUNDLED_LIBZMQ)
  set_source_files_properties(
    ${bundled-zmq-path}
    PROPERTIES
    EXTERNAL_OBJECT true
    GENERATED true
  )

  if(OT_BUNDLED_SODIUM)
    add_dependencies(libzmq sodium)
  endif()

  add_dependencies(opentxs-network-zeromq-zap libzmq)
  add_dependencies(opentxs-network-zeromq libzmq)
endif()

set(object-deps
  $<TARGET_OBJECTS:base64>
  $<TARGET_OBJECTS:irrxml>
  ${lucre}
  ${trezor}
  $<TARGET_OBJECTS:otprotob>
  $<TARGET_OBJECTS:opentxs-api-client>
  $<TARGET_OBJECTS:opentxs-api-crypto>
  $<TARGET_OBJECTS:opentxs-api-network>
  $<TARGET_OBJECTS:opentxs-api-server>
  $<TARGET_OBJECTS:opentxs-api-storage>
  $<TARGET_OBJECTS:opentxs-api>
  ${blind}
  $<TARGET_OBJECTS:opentxs-client>
  $<TARGET_OBJECTS:opentxs-consensus>
  $<TARGET_OBJECTS:opentxs-core-contract-basket>
  $<TARGET_OBJECTS:opentxs-core-contract-peer>
  $<TARGET_OBJECTS:opentxs-core-contract>
  $<TARGET_OBJECTS:opentxs-core-crypto>
  $<TARGET_OBJECTS:opentxs-core-transaction>
  $<TARGET_OBJECTS:opentxs-core-util>
  $<TARGET_OBJECTS:opentxs-core>
  $<TARGET_OBJECTS:opentxs-crypto-key>
  $<TARGET_OBJECTS:opentxs-crypto-library>
  $<TARGET_OBJECTS:opentxs-crypto>
  $<TARGET_OBJECTS:opentxs-cron>
  $<TARGET_OBJECTS:opentxs-ext>
  $<TARGET_OBJECTS:opentxs-identity-credential>
  $<TARGET_OBJECTS:opentxs-identity>
  $<TARGET_OBJECTS:opentxs-iterator>
  $<TARGET_OBJECTS:opentxs-network-zeromq-zap>
  $<TARGET_OBJECTS:opentxs-network-zeromq>
  $<TARGET_OBJECTS:opentxs-network>
  $<TARGET_OBJECTS:opentxs-otx>
  $<TARGET_OBJECTS:opentxs-recurring>
  $<TARGET_OBJECTS:opentxs-rpc>
  $<TARGET_OBJECTS:opentxs-script>
  $<TARGET_OBJECTS:opentxs-server>
  $<TARGET_OBJECTS:opentxs-storage>
  $<TARGET_OBJECTS:opentxs-storage-drivers>
  $<TARGET_OBJECTS:opentxs-storage-tree>
  $<TARGET_OBJECTS:opentxs-trade>
  $<TARGET_OBJECTS:opentxs-contact>
  $<TARGET_OBJECTS:opentxs-ui>
  $<TARGET_OBJECTS:opentxs-util>
  ${bundled-opentxs-proto}
  ${bundled-zmq-path}
  ${bundled-protobuf-path}
  ${bundled-sodium-path}
  ${bundled-secp256k1-path}
  ${bundled-sqlite}
  ${bundled-lmdb-path}
  ${OT_SSL_LIBRARIES}
)

set(cxx-install-headers
  "${CMAKE_CURRENT_SOURCE_DIR}/../include/opentxs/Exclusive.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../include/opentxs/Forward.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../include/opentxs/opentxs.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../include/opentxs/OT.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../include/opentxs/Pimpl.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../include/opentxs/Proto.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../include/opentxs/Shared.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../include/opentxs/SharedPimpl.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../include/opentxs/Types.hpp"
)

set(cxx-headers
  ${cxx-install-headers}
  "${CMAKE_CURRENT_SOURCE_DIR}/Internal.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Exclusive.tpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Shared.tpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/stdafx.hpp"
)

if(OT_SHARED)
  add_library(${MODULE_NAME} SHARED
    ${cxx-headers}
    ${object-deps}
    ${CMAKE_CURRENT_SOURCE_DIR}/Factory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Globals.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OT.cpp
  )

  target_link_libraries(${MODULE_NAME}
    PRIVATE
    Threads::Threads
    ${OT_SSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${OPENTXS_SYSTEM_LIBRARIES}
    ${keyring}
    ${CMAKE_DL_LIBS}
  )

  if(NOT OT_BUNDLED_OPENTXS_PROTO)
    target_link_libraries(${MODULE_NAME} PRIVATE ${OPENTXS_PROTO_LIBRARIES})
  endif()

  if(NOT OT_BUNDLED_PROTOBUF)
    target_link_libraries(${MODULE_NAME} PRIVATE ${PROTOBUF_LITE_LIBRARIES})
  endif()

  if(NOT OT_BUNDLED_SODIUM)
    target_link_libraries(${MODULE_NAME} PRIVATE ${SODIUM_LIBRARY})
  endif()

  if (LIBSECP256K1_EXPORT AND NOT OT_BUNDLED_SECP256K1)
    target_link_libraries(${MODULE_NAME} PRIVATE ${LIBSECP256K1_LIBRARY})
  endif()

  if (LIBBITCOIN_EXPORT)
    target_link_libraries(${MODULE_NAME} PRIVATE ${LIBBITCOIN_LIBRARIES})
  endif()

  if(NOT OT_BUNDLED_LIBZMQ)
    target_link_libraries(${MODULE_NAME} PRIVATE ${ZMQ_LIBRARIES})
  endif()

  if(OT_DHT)
    target_link_libraries(${MODULE_NAME} PRIVATE ${OPENDHT_LIBRARIES} ${GNUTLS_LIBRARIES})
  endif()

  if (OT_STORAGE_SQLITE AND NOT OT_BUNDLED_SQLITE)
      target_link_libraries(${MODULE_NAME} PRIVATE ${SQLITE3_LIBRARIES})
  endif()

  if (OT_STORAGE_LMDB AND NOT OT_BUNDLED_LMDB)
      target_link_libraries(${MODULE_NAME} PRIVATE ${LMDB_LIBRARIES})
  endif()

  if (OT_STORAGE_FS)
      target_link_libraries(${MODULE_NAME} PRIVATE ${Boost_SYSTEM_LIBRARIES} ${Boost_FILESYSTEM_LIBRARIES} ${Boost_IOSTREAMS_LIBRARIES})
  endif()

  if(OT_WITH_QT)
     target_link_libraries(${MODULE_NAME} PRIVATE ${Qt5Widgets_LIBRARIES} ${Qt5Core_LIBRARIES})
  endif()

  install(TARGETS ${MODULE_NAME}
          DESTINATION ${CMAKE_INSTALL_LIBDIR}
          EXPORT ${MODULE_NAME}-targets
          COMPONENT main)
endif()

if(OT_STATIC)
  add_library(${MODULE_NAME}_static STATIC
    ${cxx-headers}
    ${object-deps}
    ${CMAKE_CURRENT_SOURCE_DIR}/Factory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Globals.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OT.cpp
  )

  if(NOT WIN32)
    set_target_properties(${MODULE_NAME}_static PROPERTIES OUTPUT_NAME ${MODULE_NAME})
  endif()

  target_link_libraries(${MODULE_NAME}_static
    PRIVATE
    Threads::Threads
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${OPENTXS_SYSTEM_LIBRARIES}
    ${keyring}
    ${CMAKE_DL_LIBS}
  )

  if(NOT OT_BUNDLED_OPENTXS_PROTO)
    target_link_libraries(${MODULE_NAME}_static PRIVATE ${OPENTXS_PROTO_LIBRARIES})
  endif()

  if(NOT OT_BUNDLED_PROTOBUF)
    target_link_libraries(${MODULE_NAME}_static PRIVATE ${PROTOBUF_LITE_LIBRARIES})
  endif()

  if(NOT OT_BUNDLED_SODIUM)
    target_link_libraries(${MODULE_NAME}_static PRIVATE ${SODIUM_LIBRARY})
  endif()

  if (LIBSECP256K1_EXPORT AND NOT OT_BUNDLED_SECP256K1)
    target_link_libraries(${MODULE_NAME}_static PRIVATE ${LIBSECP256K1_LIBRARY})
  endif()

  if (LIBBITCOIN_EXPORT)
    target_link_libraries(${MODULE_NAME}_static PRIVATE ${LIBBITCOIN_LIBRARIES})
  endif()

  if(NOT OT_BUNDLED_LIBZMQ)
    target_link_libraries(${MODULE_NAME}_static PRIVATE ${ZMQ_LIBRARIES})
  endif()

  if(OT_DHT)
    target_link_libraries(${MODULE_NAME}_static PRIVATE ${OPENDHT_LIBRARIES} ${GNUTLS_LIBRARIES})
  endif()

  if (OT_STORAGE_SQLITE AND NOT OT_BUNDLED_SQLITE)
      target_link_libraries(${MODULE_NAME}_static PRIVATE ${SQLITE3_LIBRARIES})
  endif()

  if (OT_STORAGE_LMDB AND NOT OT_BUNDLED_LMDB)
    target_link_libraries(${MODULE_NAME}_static PRIVATE ${LMDB_LIBRARIES})
  endif()

  if (OT_STORAGE_FS)
      target_link_libraries(${MODULE_NAME}_static PRIVATE ${Boost_SYSTEM_LIBRARIES} ${Boost_FILESYSTEM_LIBRARIES} ${Boost_IOSTREAMS_LIBRARIES})
  endif()

  if(OT_WITH_QT)
     target_link_libraries(${MODULE_NAME}_static PRIVATE ${Qt5Widgets_LIBRARIES} ${Qt5Core_LIBRARIES})
  endif()

  install(TARGETS ${MODULE_NAME}_static
          DESTINATION ${CMAKE_INSTALL_LIBDIR}
          EXPORT ${MODULE_NAME}-targets
          COMPONENT main)
endif()

if(WIN32)
  add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/lib/$<CONFIGURATION>/${MODULE_NAME}.dll"
    "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/${MODULE_NAME}.dll")

  add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/lib/$<CONFIGURATION>/${MODULE_NAME}.pdb"
    "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/${MODULE_NAME}.pdb")
endif()

install(FILES ${cxx-install-headers}
        DESTINATION include/opentxs
        COMPONENT dev)
