# Copyright (c) 2010-2021 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Prevent building tests that still include protobuf generated header files.
#add_opentx_test(unittests-opentxs-rpc-async Test_Rpc_Async.cpp)
add_opentx_test(
  unittests-opentxs-rpc-get-account-activity GetAccountActivity.cpp
)
add_opentx_test(unittests-opentxs-rpc-get-account-balance GetAccountBalance.cpp)
add_opentx_test(unittests-opentxs-rpc-list-accounts ListAccounts.cpp)
add_opentx_test(unittests-opentxs-rpc-list-nyms ListNyms.cpp)
add_opentx_test(unittests-opentxs-rpc-send-payment SendPayment.cpp)

if(OT_BLOCKCHAIN_EXPORT)
  add_opentx_test(
    unittests-opentxs-rpc-send-payment-blockchain SendPayment_blockchain.cpp
  )
endif()

# Prevent building tests that still include protobuf generated header files.
#add_opentx_test(unittests-opentxs-rpc-sync Test_Rpc.cpp)

#set_tests_properties(unittests-opentxs-rpc-async PROPERTIES DISABLED TRUE)
#set_tests_properties(unittests-opentxs-rpc-sync PROPERTIES DISABLED TRUE)
