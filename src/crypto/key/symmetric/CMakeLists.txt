# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/key/symmetric/Algorithm.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/key/symmetric/Source.hpp"
)

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/crypto/key/symmetric
    COMPONENT dev
  )
endif()
