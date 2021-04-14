# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_subdirectory(key)
add_subdirectory(library)

add_library(
  opentxs-crypto OBJECT
  "Bip32.cpp"
  "Bip32.hpp"
  "Bip39.cpp"
  "Bip39.hpp"
  "Envelope.cpp"
  "Envelope.hpp"
  "HDNode.cpp"
  "HDNode.hpp"
  "Seed.cpp"
  "Seed.hpp"
  "bip39_word_list.cpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/Bip32.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/Bip32Child.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/Bip39.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/Bip43Purpose.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/Bip44Type.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/Envelope.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/HashType.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/Language.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/SecretStyle.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/SeedStrength.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/SeedStyle.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/SignatureRole.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/Types.hpp"
)
target_link_libraries(opentxs-crypto PRIVATE Boost::headers opentxs::messages)
target_sources(opentxs-crypto PRIVATE ${cxx-install-headers})
target_sources(opentxs PRIVATE $<TARGET_OBJECTS:opentxs-crypto>)
add_dependencies(opentxs-crypto generated_code)

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/crypto
    COMPONENT dev
  )
endif()
