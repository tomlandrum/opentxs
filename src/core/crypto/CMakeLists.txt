#[[
// clang-format off
]]#
# Copyright (c) 2018 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(MODULE_NAME opentxs-core-crypto)

set(cxx-sources
  ChildKeyCredential.cpp
  ContactCredential.cpp
  Credential.cpp
  CredentialSet.cpp
  CryptoSymmetricDecryptOutput.cpp
  KeyCredential.cpp
  Letter.cpp
  LowLevelKeyGenerator.cpp
  MasterCredential.cpp
  NullCallback.cpp
  NymParameters.cpp
  OTCachedKey.cpp
  OTCallback.cpp
  OTCaller.cpp
  OTEnvelope.cpp
  OTKeyring.cpp
  OTPassword.cpp
  OTPasswordData.cpp
  OTSignatureMetadata.cpp
  OTSignedFile.cpp
  PaymentCode.cpp
  Signature.cpp
  VerificationCredential.cpp
  mkcert.cpp
)

set(cxx-install-headers
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/ChildKeyCredential.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/ContactCredential.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/Credential.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/CredentialSet.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/KeyCredential.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/Letter.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/LowLevelKeyGenerator.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/MasterCredential.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/mkcert.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/NymParameters.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/OTCachedKey.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/OTCallback.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/OTCaller.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/OTEnvelope.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/OTKeyring.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/OTPasswordData.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/OTPassword.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/OTSignatureMetadata.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/OTSignedFile.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/PaymentCode.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/Signature.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/opentxs/core/crypto/VerificationCredential.hpp"
)

set(cxx-headers
  ${cxx-install-headers}
  NullCallback.hpp
  PaymentCode.hpp
  Signature.hpp
)

if(WIN32)
  # suppress warnings about exported internal symbols (global log stream objects)
  add_definitions(-DOTLOG_IMPORT)

  # suppress linker warnings about locallly defined imported symbols
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4049 /ignore:4217")

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
        DESTINATION include/opentxs/core/crypto
        COMPONENT dev)

set_property(TARGET ${MODULE_NAME} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_lib_property(${MODULE_NAME})
target_include_directories(${MODULE_NAME} PUBLIC ${OT_SSL_HEADERS})
target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/")
#[[
// clang-format on
]]#
