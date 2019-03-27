# Copyright (c) 2018 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.8)
project(opentxs)
include(GNUInstallDirs)

#-----------------------------------------------------------------------------
if(NOT ANDROID)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM LAST)
find_package(Git)
if(GIT_FOUND)
    execute_process(COMMAND git "describe" OUTPUT_VARIABLE GIT_VERSION WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" VERSION_MAJOR "${GIT_VERSION}")
    string(REGEX REPLACE "^${VERSION_MAJOR}\\.([0-9]+).*" "\\1" VERSION_MINOR "${GIT_VERSION}")
    string(REGEX REPLACE "^${VERSION_MAJOR}\\.${VERSION_MINOR}\\.([0-9]+).*" "\\1" VERSION_BUGFIX "${GIT_VERSION}")
    string(REGEX REPLACE "^${VERSION_MAJOR}\\.${VERSION_MINOR}\\.${VERSION_BUGFIX}-([0-9]+)-.*" "\\1" VERSION_NEW_COMMITS "${GIT_VERSION}")
    string(REGEX REPLACE "^${VERSION_MAJOR}\\.${VERSION_MINOR}\\.${VERSION_BUGFIX}-${VERSION_NEW_COMMITS}-(.*)" "\\1" VERSION_SHA1 "${GIT_VERSION}")
    if("${VERSION_NEW_COMMITS}" STREQUAL "${GIT_VERSION}")
        set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUGFIX}")
        set(VERSION_NEW_COMMITS "")
        set(VERSION_SHA1 "")
    else()
        set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUGFIX}-${VERSION_NEW_COMMITS}-${VERSION_SHA1}")
    endif()
    if ("${VERSION_STRING}" STREQUAL "..")
        message(FATAL_ERROR "Version string missing. Did you git clone using the --depth parameter?")
    endif()
else()
  message(FATAL_ERROR "Git not found.")
endif()
endif() # NOT ANDROID
#-----------------------------------------------------------------------------
# Options for building

option(BUILD_VERBOSE       "Verbose build output." ON)

if(ANDROID)
  option(BUILD_DOCUMENTATION "Build the Doxygen documentation." OFF)
  option(BUILD_TESTS         "Build the unit tests." OFF)
else()
  option(BUILD_DOCUMENTATION "Build the Doxygen documentation." ON)
  option(BUILD_TESTS         "Build the unit tests." ON)
endif()

option(OT_STRICT           "Use pedantic compiler options." ON)
option(OT_VALGRIND         "Use Valgrind annotations." OFF)
option(USE_CCACHE          "Use ccache." OFF)
option(OT_SANITIZE         "Enable sanitize options" OFF)

option(OT_SHARED           "Build shared libopentxs." ON)
option(OT_STATIC           "Build static libopentxs." OFF)

if(OT_SHARED)
  option(BUILD_SHARED_LIBS   "Preferred shared linking." ON)
else()
  option(BUILD_SHARED_LIBS   "Preferred shared linking." OFF)
endif()

option(RPM                 "Build a RPM" OFF)
option(DEB                 "Build a DEB" OFF)
set(PACKAGE_CONTACT        ""                             CACHE STRING  "Package Maintainer")
set(PACKAGE_VENDOR         "Open Transactions Developers" CACHE STRING  "Package Vendor")
option(AUTOCOMPLETION      "CL autocompletion to opentxs CL client" OFF)

option(KEYRING_WINDOWS     "Build with Windows Keyring" OFF)
option(KEYRING_MAC         "Build with Mac OSX Keyring" OFF)
option(KEYRING_IOS         "Build with iOS Keyring" OFF)
option(KEYRING_GNOME       "Build with Gnome Keyring" OFF)
option(KEYRING_KWALLET     "Build with KWallet Keyring" OFF)
option(KEYRING_FLATFILE    "Build with Flatfile Keyring" OFF)
option(NO_PASSWORD         "Use default password for all prompts" OFF)

option(OT_STORAGE_FS       "Enable filesystem backend for storage" OFF)
option(OT_STORAGE_SQLITE   "Enable sqlite backend for storage" OFF)
option(OT_STORAGE_LMDB     "Enable LMDB backend for storage" ON)

option(OT_DHT    "Enable OpenDHT support" OFF)

option(OT_CRYPTO_SUPPORTED_ALGO_AES     "Enable AES encryption algorithm" ON)

option(OT_CRYPTO_SUPPORTED_SOURCE_BIP47     "Enable support for BIP-47 nyms" ON)

option(OT_CRYPTO_SUPPORTED_KEY_HD     "Enable HD key derivation support" ON)
option(OT_CRYPTO_SUPPORTED_KEY_ED25519     "Enable ed25519 key support" ON)
option(OT_CRYPTO_SUPPORTED_KEY_RSA     "Enable RSA key support" OFF)
option(OT_CRYPTO_SUPPORTED_KEY_SECP256K1 "Enable secp256k1 key support" ON)

option(OT_CRYPTO_USING_OPENSSL       "Use OpenSSL crypto implementation" ON)
option(OT_CRYPTO_USING_LIBSECP256K1  "Use libsecp256k1 crypto implementation" ON)
option(OT_CRYPTO_USING_TREZOR  "Use trezor crypto implementation" ON)
option(OT_WITH_BLOCKCHAIN  "Enable blockchain support" OFF)

option(OT_CRYPTO_SHA2_PREFER_OPENSSL       "Use OpenSSL SHA2 implementation when available" OFF)

option(OT_CASH_USING_LUCRE  "Enable Lucre support" OFF)
option(OT_LUCRE_DEBUG       "Output Lucre debug info" OFF)

option(OT_WITH_QT       "Enable Qt5 model support for ui classes" OFF)

if(ANDROID)
  option(OT_SCRIPT_USING_CHAI  "Use chaiscript" OFF)
else()
  option(OT_SCRIPT_USING_CHAI  "Use chaiscript" ON)
endif()

# SWIG Bindings
if(ANDROID)
  option(JAVA                "Build with Java binding" ON)
else()
  option(JAVA                "Build with Java binding" OFF)
endif()
option(PERL    "Build with Perl binding" OFF)
option(PHP     "Build with PHP binding" OFF)
option(PYTHON3 "Build with Python3 binding" OFF)
option(PYTHON2 "Build with Python2 binding" OFF)
option(PYTHON  "Build with default Python binding" OFF)

option(OT_ZLIB_OVERRIDE "Manually specify zlib paths" OFF)
option(OT_HOST_OVERRIDE "Manually specify --host value for external projects" "")

option(OT_BUNDLED_SSL "Use bundled version of libressl" ON)
option(OT_GENERATE_PROTO "Regenerate protobuf classes" OFF)
option(OT_BUNDLED_PROTOBUF "Use bundled version of protobuf" ON)
option(OT_BUNDLED_SODIUM "Use bundled version of libsodium" ON)
option(OT_BUNDLED_SECP256K1 "Use bundled version of libsecp256k1" ON)
option(OT_BUNDLED_SQLITE "Use bundled version of sqlite" ON)
option(OT_BUNDLED_LIBZMQ "Use bundled version of libzmq" ON)
option(OT_BUNDLED_OPENTXS_PROTO "Use bundled version of opentxs-proto" ON)
option(OT_BUNDLED_LMDB "Use bundled version of lmdb" ON)

if(BUILD_VERBOSE)
  set(CMAKE_VERBOSE_MAKEFILE true)
endif()

if(RPM OR DEB)
    if(NOT PACKAGE_CONTACT)
        message(FATAL_ERROR "Specify a PACKAGE_CONTACT to build packages.")
    endif()
endif()

#-----------------------------------------------------------------------------
# Print system information and build options

message(STATUS "General -------------------------------------")
message(STATUS "opentxs version:        ${VERSION_STRING}")
message(STATUS "CMake version:          ${CMAKE_VERSION}")
message(STATUS "System:                 ${CMAKE_SYSTEM}")
message(STATUS "Processor:              ${CMAKE_SYSTEM_PROCESSOR}")

message(STATUS "Verbose:                ${BUILD_VERBOSE}")
message(STATUS "Testing:                ${BUILD_TESTS}")
message(STATUS "Documentation:          ${BUILD_DOCUMENTATION}")
message(STATUS "Using ccache            ${USE_CCACHE}")
message(STATUS "Pedantic compilation:   ${OT_STRICT}")
message(STATUS "Valgrind integration:   ${OT_VALGRIND}")

message(STATUS "Packaging -----------------------------------")
message(STATUS "Build RPM:              ${RPM}")
message(STATUS "Build DEB:              ${DEB}")
message(STATUS "Package Contact:        ${PACKAGE_CONTACT}")
message(STATUS "Package Vendor:         ${PACKAGE_VENDOR}")

message(STATUS "CL autocompletion:      ${AUTOCOMPLETION}")

message(STATUS "Keyrings ------------------------------------")
message(STATUS "Keyring Windows:        ${KEYRING_WINDOWS}")
message(STATUS "Keyring MAC:            ${KEYRING_MAC}")
message(STATUS "Keyring iOS:            ${KEYRING_IOS}")
message(STATUS "Keyring Gnome:          ${KEYRING_GNOME}")
message(STATUS "Keyring KWallet:        ${KEYRING_KWALLET}")
message(STATUS "Keyring flatfile:       ${KEYRING_FLATFILE}")
message(STATUS "No password:            ${NO_PASSWORD}")

message(STATUS "Network plugins------------------------------")
message(STATUS "DHT:                    ${OT_DHT}")

message(STATUS "Storage backends-----------------------------")
message(STATUS "filesystem:             ${OT_STORAGE_FS}")
message(STATUS "sqlite                  ${OT_STORAGE_SQLITE}")
message(STATUS "LMDB                    ${OT_STORAGE_LMDB}")

message(STATUS "Nym ID sources------------------------------")
message(STATUS "BIP-47:                 ${OT_CRYPTO_SUPPORTED_SOURCE_BIP47}")

message(STATUS "Encryption algorithms------------------------")
message(STATUS "AES:                    ${OT_CRYPTO_SUPPORTED_ALGO_AES}")

message(STATUS "Key algorithms-------------------------------")
message(STATUS "HD derivation:          ${OT_CRYPTO_SUPPORTED_KEY_HD}")
message(STATUS "ed25519:                ${OT_CRYPTO_SUPPORTED_KEY_ED25519}")
message(STATUS "RSA:                    ${OT_CRYPTO_SUPPORTED_KEY_RSA}")
message(STATUS "secp256k1               ${OT_CRYPTO_SUPPORTED_KEY_SECP256K1}")

message(STATUS "Crypto library providers---------------------")
message(STATUS "OpenSSL:                ${OT_CRYPTO_USING_OPENSSL}")
message(STATUS "libsecp256k1:           ${OT_CRYPTO_USING_LIBSECP256K1}")
message(STATUS "trezor-crypto:          ${OT_CRYPTO_USING_TREZOR}")
message(STATUS "blockchain:             ${OT_WITH_BLOCKCHAIN}")

message(STATUS "Cash library providers---------------------")
message(STATUS "Lucre:                  ${OT_CASH_USING_LUCRE}")
message(STATUS "Lucre debug info:       ${OT_LUCRE_DEBUG}")

message(STATUS "Bindings ------------------------------------")
message(STATUS "Java binding:           ${JAVA}")
message(STATUS "Perl binding:           ${PERL}")
message(STATUS "PHP binding:            ${PHP}")
message(STATUS "Python3 binding:        ${PYTHON3}")
message(STATUS "Python2 binding:        ${PYTHON2}")
message(STATUS "Default Python binding: ${PYTHON}")

message(STATUS "Script engines-------------------------------")
message(STATUS "Chai:                   ${OT_SCRIPT_USING_CHAI}")

message(STATUS "UI-------------------------------------------")
message(STATUS "Qt5:                    ${OT_WITH_QT}")

message(STATUS "Dependencies---------------------------------")
message(STATUS "Bundled libressl:       ${OT_BUNDLED_SSL}")
message(STATUS "Bundled protobuf:       ${OT_BUNDLED_PROTOBUF}")
message(STATUS "Regenerate otprotob:    ${OT_GENERATE_PROTO}")
message(STATUS "Bundled sodium:         ${OT_BUNDLED_SODIUM}")
message(STATUS "Bundled libsecp256k1:   ${OT_BUNDLED_SECP256K1}")
message(STATUS "Bundled sqlite:         ${OT_BUNDLED_SQLITE}")
message(STATUS "Bundled lmdb:           ${OT_BUNDLED_LMDB}")
message(STATUS "Bundled libzmq:         ${OT_BUNDLED_LIBZMQ}")
message(STATUS "Bundled opentxs-proto:  ${OT_BUNDLED_OPENTXS_PROTO}")

message(STATUS "---------------------------------------------")

#-----------------------------------------------------------------------------
# Collect all binaries into bin subdirectory

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH  ${CMAKE_CURRENT_BINARY_DIR}/lib)


#-----------------------------------------------------------------------------
# Check which compiler to use

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
  set(GCC_WARN_FLAGS "-Wno-attributes -Wnoexcept -Wstrict-null-sentinel -Wlogical-op -Wno-parentheses")

  if(ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
  set(CMAKE_COMPILER_IS_CLANGXX 1)

  if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -stdlib=libc++")
  else()
    if(NOT ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    endif()
  endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug" AND OT_SANITIZE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
  set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -fsanitize=address")
  set(CMAKE_C_LINK_FLAGS "${CMAKE_CXLINK_FLAGS} -fsanitize=address")
  if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=gold")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fuse-ld=gold")
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -fuse-ld=gold")
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -fuse-ld=gold")
  endif() #GNU LINKER
endif() # DEBUG

if(BUILD_SHARED_LIBS)
  # Windows would complain about unknown flag -fPIC
  if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  endif()
endif()

if(WIN32)
  add_definitions(-D_UNICODE)
else()
  if(OT_STRICT)

    set(PEDANTIC_CXX_FLAGS "-Wall -Werror -Wextra -pedantic -Wno-missing-braces -Wcast-align -Wctor-dtor-privacy -Wdisabled-optimization -Winit-self -Wmissing-include-dirs -Wswitch-default -Wold-style-cast -Woverloaded-virtual -Wmissing-declarations -Wno-deprecated-declarations -Wno-unused-parameter -Wno-old-style-cast -Wunused-macros -Wundef -Wformat=2 -Wno-format-nonliteral ${GCC_WARN_FLAGS}")
  else()
    set(PEDANTIC_CXX_FLAGS "")
  endif()
endif()

if(ANDROID)
  set(PEDANTIC_CXX_FLAGS "${PEDANTIC_CXX_FLAGS} -Wno-missing-field-initializers")
  add_definitions(-D__STDC_LIMIT_MACROS)
endif()

if(CMAKE_COMPILER_IS_CLANGXX)
  set(PEDANTIC_CXX_FLAGS "${PEDANTIC_CXX_FLAGS} -Wno-unused-parameter -Wno-old-style-cast -Wno-null-dereference")
endif()

message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C Flags:         ${CMAKE_C_FLAGS}")
message(STATUS "C++ Flags:       ${CMAKE_CXX_FLAGS} ${PEDANTIC_CXX_FLAGS}")
message(STATUS "C++ link flags:  ${CMAKE_CXX_LINK_FLAGS}")

#-----------------------------------------------------------------------------
# Get submodules

execute_process(COMMAND git "submodule" "update" "--init" "--recursive" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

#-----------------------------------------------------------------------------
# Third party libraries

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(OT_VALGRIND)
  add_definitions(-DOT_VALGRIND=1)
else()
  add_definitions(-DOT_VALGRIND=0)
endif()

if(OT_BUNDLED_OPENTXS_PROTO)
  set(OT_OPENTXS_PROTO_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/opentxs-proto/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/opentxs-proto/generated"
  )
  include_directories("${OT_OPENTXS_PROTO_HEADERS}")
else()
  find_package(opentxs-proto REQUIRED)
  include_directories("${OPENTXS_PROTO_INCLUDE_DIRS}")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(OT_BUNDLED_SSL)
  if(OT_CASH_USING_LUCRE)
    message(FATAL_ERROR "Lucre is incompatible with bundled libressl")
  endif()

  set(OT_SSL_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/deps/libressl/install/include")
  add_definitions(-DOT_BUNDLED_LIBRESSL)
else()
  if(APPLE)
    message(STATUS "On Apple, you may have to use: brew install openssl ; brew link --force openssl")
  endif()

  find_package(OpenSSL 1.0.1 REQUIRED)
  set(OT_SSL_HEADERS "${OPENSSL_INCLUDE_DIR}")
endif()

if(OT_BUNDLED_PROTOBUF)
  set(OT_PROTOBUF_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/deps/protobuf/install/include")
  include_directories(SYSTEM ${OT_PROTOBUF_HEADERS})
else()
  find_package(Protobuf REQUIRED)
  set(OT_PROTOBUF_HEADERS "${PROTOBUF_INCLUDE_DIR}")
  include_directories(SYSTEM ${OT_PROTOBUF_HEADERS})
endif()

if(OT_ZLIB_OVERRIDE)
  set(ZLIB_INCLUDE_DIRS "${ZLIB_INCLUDE_DIRS_OVERRIDE}")
  set(ZLIB_LIBRARIES "${ZLIB_LIBRARIES_OVERRIDE}")
  set(ZLIB_FOUND TRUE)
else()
  find_package(ZLIB REQUIRED)
endif()

if(OT_BUNDLED_SODIUM)
  set(OT_SODIUM_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/deps/sodium/install/include")
else()
  find_package(Sodium REQUIRED)
  set(OT_SODIUM_HEADERS "${SODIUM_INCLUDE_DIR}")
endif()

if(OT_BUNDLED_LIBZMQ)
  set(OT_ZEROMQ_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/deps/libzmq/install/include")
else()
  find_package(ZeroMQ 4 REQUIRED)
  set(OT_ZEROMQ_HEADERS "${ZMQ_INCLUDE_DIRS}")
endif()

if(OT_DHT)
  find_package(OPENDHT 0.6 REQUIRED)
  find_package(GnuTLS REQUIRED)
endif()

if(OT_STORAGE_SQLITE)
  if(OT_BUNDLED_SQLITE)
    set(OT_SQLITE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/deps/sqlite-amalgamation-3240000")
  else()
    find_package(SQLite3 REQUIRED)
    set(OT_SQLITE_HEADERS "${SQLITE3_INCLUDE_DIRS}")
  endif()
endif()

if(OT_STORAGE_LMDB)
  if(OT_BUNDLED_LMDB)
    set(OT_LMDB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/deps/lmdb/libraries/liblmdb")
  else()
    find_package(LMDB REQUIRED)
    set(OT_LMDB_HEADERS "${LMDB_INCLUDE_DIR}")
  endif()
endif()

if(OT_STORAGE_FS)
  find_package(Boost REQUIRED filesystem)
  find_package(Boost REQUIRED iostreams)
  find_package(Boost REQUIRED system)
endif()

if(OT_CRYPTO_USING_LIBSECP256K1)
  if (OT_BUNDLED_SECP256K1)
    set(OT_SECP256K1_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/deps/secp256k1/install/include")
  else()
    find_package(Libsecp256k1 REQUIRED)
    set(OT_SECP256K1_HEADERS "${LIBSECP256K1_INCLUDE_DIR}")
  endif()
endif()

if (OT_WITH_BLOCKCHAIN)
  find_package(opentxs-blockchain REQUIRED)
  include_directories("${OPENTXS_BLOCKCHAIN_INCLUDE_DIRS}")
endif()

if(OT_WITH_QT)
  find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
  set_target_properties(Qt5::Core PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")
  list(GET Qt5Core_INCLUDE_DIRS 0 QT_INCLUDE_DIR)
  include_directories("${QT_INCLUDE_DIR}")
endif()

#-----------------------------------------------------------------------------
# System libraries used for linking.

if(UNIX AND NOT APPLE)
  list(APPEND OPENTXS_SYSTEM_LIBRARIES dl)
  if(ANDROID)
    list(APPEND OPENTXS_SYSTEM_LIBRARIES log)
  else()
    list(APPEND OPENTXS_SYSTEM_LIBRARIES rt)
  endif()
endif()


#-----------------------------------------------------------------------------
# Force out-of-source build

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
message(FATAL_ERROR "CMake generation is not allowed within the source directory!
Remove the CMakeCache.txt file and try again from another folder, e.g.:

   rm CMakeCache.txt
   mkdir build
   cd build
   cmake ..
")
endif()

#-----------------------------------------------------------------------------
# Use ccache

if (USE_CCACHE)
  find_program(CCACHE "ccache")
  if (CCACHE)
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "CCACHE_CPP2=yes ${CCACHE}")
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})
  endif(CCACHE)
endif (USE_CCACHE)

#-----------------------------------------------------------------------------
# Default to Release build

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif()


#-----------------------------------------------------------------------------
# Build Unit tests

if(BUILD_TESTS AND NOT ANDROID)
  find_package(GTest REQUIRED)
  enable_testing()
endif()


#-----------------------------------------------------------------------------
# Build Documentation

if(BUILD_DOCUMENTATION)
  set(DOC_GRAPHS "YES" CACHE STRING "Create dependency graphs (needs graphviz)")
  set(DOC_FULLGRAPHS "NO" CACHE STRING "Create call/callee graphs (large)")

  find_program(DOT_PATH dot)

  if (DOT_PATH STREQUAL "DOT_PATH-NOTFOUND")
    message("Doxygen: graphviz not found - graphs disabled")
    set(DOC_GRAPHS "NO")
  endif()

  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    configure_file("cmake/Doxyfile.in" "Doxyfile" @ONLY)
    configure_file("cmake/Doxygen.extra.css.in" "Doxygen.extra.css" @ONLY)
    add_custom_target(doc
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen.." VERBATIM)
  endif()
endif()


#-----------------------------------------------------------------------------
# Source Definitions

if(WIN32)
    if(NOT IDC_STATIC)
        add_definitions(-DIDC_STATIC=-1)
    endif()

    add_definitions(-DOT_COMPANYNAME="Open Transactions Community")
    add_definitions(-DOT_PRODUCTNAME="Open Transactions")
    add_definitions(-DOT_PRODUCTVERSION="${VERSION_MAJOR},${VERSION_MINOR},${VERSION_BUGFIX},0")
    add_definitions(-DOT_API_NAME="Open Transactions API")
    add_definitions(-DOT_LIBRARY_NAME="Open Transactions Library")
    add_definitions(-DOT_PROMPT_NAME="Open Transactions Prompt")
endif()

if(ANDROID)
  add_definitions(-DANDROID)
endif()

#Network

if(OT_DHT)
    set(DHT_EXPORT 1)
else()
    set(DHT_EXPORT 0)
endif()

#Storage backends

if(OT_STORAGE_FS)
  set(FS_EXPORT 1)
else()
  set(FS_EXPORT 0)
endif()

if(OT_STORAGE_SQLITE)
  set(SQLITE_EXPORT 1)
else()
  set(SQLITE_EXPORT 0)
endif()

if(OT_STORAGE_LMDB)
  set(LMDB_EXPORT 1)
else()
  set(LMDB_EXPORT 0)
endif()

if ((NOT OT_STORAGE_FS) AND (NOT OT_STORAGE_SQLITE) AND (NOT OT_STORAGE_LMDB))
  message(FATAL_ERROR "At least one storage backend must be defined.")
endif()

#Nym sources

if(OT_CRYPTO_SUPPORTED_SOURCE_BIP47)
  if (NOT OT_CRYPTO_SUPPORTED_KEY_HD)
    message(FATAL_ERROR "BIP-47 support requires HD key support")
  endif()
  if (NOT OT_CRYPTO_SUPPORTED_KEY_SECP256K1)
    message(FATAL_ERROR "BIP-47 support requires secp256k1 key support")
  endif()
  set(BIP47_EXPORT 1)
else()
  set(BIP47_EXPORT 0)
endif()

#Encryption algorithms

if (OT_CRYPTO_SUPPORTED_ALGO_AES)
  if (NOT OT_CRYPTO_USING_OPENSSL)
    message(FATAL_ERROR "AES support requires OpenSSL")
  endif()
  set(AES_EXPORT 1)
else()
  set(AES_EXPORT 0)
endif()

#Key types

if(OT_CRYPTO_SUPPORTED_KEY_HD)
  if (NOT OT_CRYPTO_SUPPORTED_KEY_SECP256K1)
    message(FATAL_ERROR "HD key support requires secp256k1 key support")
  endif()
  if (NOT (OT_CRYPTO_USING_TREZOR OR OT_WITH_BLOCKCHAIN))
    message(FATAL_ERROR "HD key support requires trezor-crypto or blockchain")
  endif()
  set(HD_EXPORT 1)
else()
  set(HD_EXPORT 0)
endif()

if (NOT OT_CRYPTO_SUPPORTED_KEY_ED25519)
  set(ED25519_EXPORT 0)
  if (NOT OT_CRYPTO_SUPPORTED_KEY_RSA)
    if (NOT OT_CRYPTO_SUPPORTED_KEY_SECP256K1)
        message(FATAL_ERROR "At least one type of key must be enabled")
    endif()
  endif()
else()
  set(ED25519_EXPORT 1)
endif()

if (OT_CRYPTO_SUPPORTED_KEY_RSA)
  if (NOT OT_CRYPTO_USING_OPENSSL)
    message(FATAL_ERROR "RSA support requires OpenSSL")
  endif()
  set(RSA_EXPORT 1)
else()
  set(RSA_EXPORT 0)
endif()

if (OT_CRYPTO_SUPPORTED_KEY_SECP256K1)
  if (NOT (OT_CRYPTO_USING_LIBSECP256K1 OR OT_CRYPTO_USING_TREZOR OR OT_WITH_BLOCKCHAIN))
    message(FATAL_ERROR "no provider for secp256k1 support configured")
  endif()
  set(SECP256K1_EXPORT 1)
else()
  set(SECP256K1_EXPORT 0)
endif()

#Crypto libraries

if(OT_CRYPTO_USING_OPENSSL)
  set(OPENSSL_EXPORT 1)
  if(OT_CRYPTO_SHA2_PREFER_OPENSSL)
    set(SHA2_VIA_OPENSSL_EXPORT 1)
  else()
    set(SHA2_VIA_OPENSSL_EXPORT 0)
  endif()
else()
  set(OPENSSL_EXPORT 0)
  set(SHA2_VIA_OPENSSL_EXPORT 0)
endif()

if(OT_CRYPTO_USING_LIBSECP256K1)
  if (OT_CRYPTO_SUPPORTED_KEY_SECP256K1)
    set(LIBSECP256K1_EXPORT 1)
  else()
    set(LIBSECP256K1_EXPORT 0)
    message(WARNING "Libsecp256k1 is pointless without secp256k1 keys.")
  endif()
else()
  set(LIBSECP256K1_EXPORT 0)
endif()

if (NOT (OT_CRYPTO_USING_TREZOR OR OT_WITH_BLOCKCHAIN))
  message(FATAL_ERROR "trezor-crypto or blockchain support is required for base58 encoding")
endif()

if (OT_WITH_BLOCKCHAIN)
  if (OT_CRYPTO_USING_LIBSECP256K1)
  	message(FATAL_ERROR "libsecp256k1 is not needed with blockchain support enabled.")
  endif()
  if (OT_CRYPTO_USING_TREZOR)
  	message(FATAL_ERROR "trezor-crypto is not needed with blockchain support enabled.")
  endif()
  if (NOT OT_CRYPTO_SUPPORTED_SOURCE_BIP47)
    message(FATAL_ERROR "Blockchain support requires BIP-47.")
  endif()
  if (NOT OT_CRYPTO_SUPPORTED_KEY_HD)
    message(FATAL_ERROR "Blockchain support requires HD key support.")
  endif()
  if (NOT OT_CRYPTO_SUPPORTED_KEY_SECP256K1)
    message(FATAL_ERROR "Blockchain support requires secp256k1 key support.")
  endif()
endif()

if(OT_CRYPTO_USING_TREZOR)
  set(TREZORCRYPTO_EXPORT 1)
else()
  set(TREZORCRYPTO_EXPORT 0)
endif()

if (OT_CRYPTO_SUPPORTED_KEY_HD)
  set(BIP32_EXPORT 1)
  set(BIP39_EXPORT 1)
else()
  set(BIP32_EXPORT 0)
  set(BIP39_EXPORT 0)
endif()

if(OT_WITH_BLOCKCHAIN)
  set(BLOCKCHAIN_EXPORT 1)
else()
  set(BLOCKCHAIN_EXPORT 0)
endif()

set(OT_CASH_EXPORT 0)

if(OT_CASH_USING_LUCRE)
    set(CASH_LUCRE_EXPORT 1)
    set(OT_CASH_EXPORT 1)
else()
    set(CASH_LUCRE_EXPORT 0)
endif()

if(OT_LUCRE_DEBUG)
    add_definitions(-DOT_LUCRE_DEBUG=1)
else()
    add_definitions(-DOT_LUCRE_DEBUG=0)
endif()

if(OT_WITH_QT)
    set(OT_QT_EXPORT 1)
else()
    set(OT_QT_EXPORT 0)
endif()

if (FALSE)
    set(CASH_MM_EXPORT 1)
    set(OT_CASH_EXPORT 1)
else()
    set(CASH_MM_EXPORT 0)
endif()

#Keyrings
if(KEYRING_WINDOWS AND WIN32)
  add_definitions(-DOT_KEYRING_WINDOWS)
endif()

if(KEYRING_MAC AND APPLE)
  add_definitions(-DOT_KEYRING_MAC)
endif()

if(KEYRING_IOS)
  add_definitions(-DOT_KEYRING_IOS)
endif()

if(KEYRING_GNOME)
  add_definitions(-DOT_KEYRING_GNOME)
endif()

if(KEYRING_KWALLET)
  add_definitions(-DOT_KEYRING_KWALLET)
endif()

if(KEYRING_FLATFILE)
  add_definitions(-DOT_KEYRING_FLATFILE)
endif()

if(NO_PASSWORD)
  add_definitions(-DOT_NO_PASSWORD)
endif()

if(WIN32)
  add_definitions("-DEXPORT=__declspec(dllexport)")
else()
  add_definitions(-DEXPORT=)
endif()

#Script engines
if(OT_SCRIPT_USING_CHAI AND NOT ANDROID AND NOT KEYRING_IOS)
  set(SCRIPT_CHAI_EXPORT 1)
else()
  set(SCRIPT_CHAI_EXPORT 0)
endif()

#-----------------------------------------------------------------------------
# Set in-source Version
configure_file("Version.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/include/opentxs/Version.hpp" @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/opentxs/Version.hpp
        DESTINATION include/opentxs/
        COMPONENT dev)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

#-----------------------------------------------------------------------------
# Build source

#Fix RPATH
if(APPLE)
  set(CMAKE_MACOSX_RPATH 0)
  set(CMAKE_INSTALL_NAME_DIR @rpath)
endif()
if(NOT RPM)
  SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
  if(NOT WIN32)
    include(GNUInstallDirs)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
  endif()
endif()

if(UNIX)
  add_definitions(-DOT_PREFIX_PATH=\"${CMAKE_INSTALL_PREFIX}\")
endif()

function(set_lib_property lib)
    set_target_properties(${lib} PROPERTIES VERSION ${VERSION_STRING} SOVERSION ${VERSION_MAJOR})
endfunction(set_lib_property)

add_subdirectory(deps)

if(NOT APPLE)
  add_definitions(-D_XOPEN_SOURCE=700)
endif()

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_subdirectory(src)
add_subdirectory(wrappers)

if (GTEST_FOUND AND NOT ANDROID)
  add_subdirectory(tests)
endif()

if (NOT ANDROID)
#-----------------------------------------------------------------------------
# Produce a cmake-package

include(CMakePackageConfigHelpers)
set(CMAKE_CONFIG_DEST "${CMAKE_INSTALL_LIBDIR}/opentxs")
set(OPENTXS_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")

if(BUILD_SHARED_LIBS)
  set(OPENTXS_LIBRARIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}opentxs${CMAKE_SHARED_LIBRARY_SUFFIX}")
else()
  set(OPENTXS_LIBRARIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}opentxs${CMAKE_STATIC_LIBRARY_PREFIX}")
endif()

configure_package_config_file(
       "cmake/opentxsConfig.cmake.in"
       "${CMAKE_BINARY_DIR}/opentxsConfig.cmake"
       INSTALL_DESTINATION ${CMAKE_CONFIG_DEST}
       PATH_VARS OPENTXS_INCLUDE_DIRS OPENTXS_LIBRARIES)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/opentxsConfigVersion.cmake"
  VERSION ${VERSION_STRING}
  COMPATIBILITY AnyNewerVersion)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/opentxsConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/opentxsConfigVersion.cmake"
        DESTINATION ${CMAKE_CONFIG_DEST}
        COMPONENT dev)

install(EXPORT opentxs-targets DESTINATION ${CMAKE_CONFIG_DEST})

#-----------------------------------------------------------------------------
# Produce a pkg-config file

string(REGEX REPLACE "^${CMAKE_INSTALL_PREFIX}(.*)" "\\1" LIB_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")
configure_file("cmake/opentxs.pc.in" "opentxs.pc" @ONLY)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/opentxs.pc"
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
          COMPONENT main)

#-----------------------------------------------------------------------------
# Install

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/containers/simple_ptr.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/containers/containers_fixes.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/containers/exceptions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/containers/copy_functors.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/containers/simple_ptr.tpp
    DESTINATION include/opentxs/core/containers
    COMPONENT dev)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/irrxml/
        DESTINATION include/irrxml
        COMPONENT dev
        FILES_MATCHING PATTERN "*.hpp")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts/ot/
        DESTINATION lib/opentxs
        COMPONENT dev
        FILES_MATCHING PATTERN "*.ot")

# opentxs CL client autocompletion
if(AUTOCOMPLETION)
  install(FILES
          ${CMAKE_CURRENT_SOURCE_DIR}/scripts/bash_completion.d/opentxs-bash-completion.sh
	  DESTINATION /etc/bash_completion.d/
          COMPONENT main)
endif()

#-----------------------------------------------------------------------------
# Uninstall
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake)

#-----------------------------------------------------------------------------
# Packaging
set(CPACK_GENERATOR "TGZ")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

set(CPACK_PACKAGE_VERSION ${VERSION_STRING})
set(CPACK_PACKAGE_NAME "opentxs")
set(CPACK_PACKAGE_CONTACT ${PACKAGE_CONTACT})
set(CPACK_PACKAGE_VENDOR ${PACKAGE_VENDOR})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open Transactions library and command line client")
set(CPACK_PACKAGE_DESCRIPTION "
Open-Transactions democratizes financial and monetary actions.
You can use it for issuing currencies/stock, paying dividends, creating asset accounts, sending/receiving digital cash, writing/depositing cheques, cashier's cheques, creating basket currencies, trading on markets, scripting custom agreements, recurring payments, escrow, etc.
")

if(RPM)
  set(CPACK_GENERATOR "RPM")
  set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
  set(CPACK_RPM_COMPONENT_INSTALL ON)
  set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ChangeLog")
endif()

if(DEB)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "openssl, libprotobuf9")
  set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
  set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${OPEN_TRANSACTIONS_WEBSITE})
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${PACKAGE_CONTACT})
endif()

include(CPack)
endif (NOT ANDROID)
