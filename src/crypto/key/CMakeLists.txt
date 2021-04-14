# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_subdirectory(asymmetric)
add_subdirectory(symmetric)

add_library(
  opentxs-crypto-key OBJECT
  "${opentxs_SOURCE_DIR}/src/internal/crypto/key/Factory.hpp"
  "Asymmetric.cpp"
  "Asymmetric.hpp"
  "EllipticCurve.cpp"
  "EllipticCurve.hpp"
  "HD.cpp"
  "HD.hpp"
  "Key.cpp"
  "Keypair.cpp"
  "Keypair.hpp"
  "Null.hpp"
  "Symmetric.cpp"
  "Symmetric.hpp"
  "SymmetricNull.hpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/key/Asymmetric.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/key/EllipticCurve.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/key/Ed25519.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/key/HD.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/key/Keypair.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/key/RSA.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/key/Secp256k1.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/key/Symmetric.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/key/Types.hpp"
)
target_link_libraries(
  opentxs-crypto-key PRIVATE opentxs::messages unofficial-sodium::sodium
)

if(RSA_EXPORT)
  target_sources(opentxs-crypto-key PRIVATE "RSA.cpp" "RSA.hpp")
endif()

if(ED25519_EXPORT)
  target_sources(opentxs-crypto-key PRIVATE "Ed25519.cpp" "Ed25519.hpp")
endif()

if(SECP256K1_EXPORT)
  target_sources(opentxs-crypto-key PRIVATE "Secp256k1.cpp" "Secp256k1.hpp")
endif()

if(OPENSSL_EXPORT)
  target_link_libraries(opentxs-crypto-key PRIVATE OpenSSL::Crypto)
  target_link_libraries(opentxs PRIVATE OpenSSL::Crypto)
endif()

target_sources(opentxs-crypto-key PRIVATE ${cxx-install-headers})
target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-crypto-key>)
add_dependencies(opentxs-crypto-key generated_code)

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/crypto/key
    COMPONENT dev
  )
endif()
