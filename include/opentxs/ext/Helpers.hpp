// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_EXT_HELPERS_HPP
#define OPENTXS_EXT_HELPERS_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/Log.hpp"

#include <iostream>
#include <string>

// Reads from cin until Newline.
inline std::string OT_CLI_ReadLine()
{
    std::string line;
    if (std::getline(std::cin, line)) { return line; }

    return "";
}

// Reads from cin until EOF. (Or until the ~ character as the first character on
// a line.)
inline std::string OT_CLI_ReadUntilEOF()
{
    std::string result("");

    for (;;) {
        std::string input_line("");
        if (std::getline(std::cin, input_line, '\n')) {
            input_line += "\n";

            if (input_line[0] == '~')  // This is our special "break" character
                                       // for multi-line input.
                break;

            result += input_line;
        } else {
            opentxs::LogOutput(": getline() was unable to "
                               "read a string from std::cin.")
                .Flush();
            break;
        }
        if (std::cin.eof() || std::cin.fail() || std::cin.bad()) {
            std::cin.clear();
            break;
        }
    }

    return result;
}

#endif
