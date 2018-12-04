// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_LOG_HPP
#define OPENTXS_CORE_LOG_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/util/Assert.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"

#include <chrono>
#include <cstdint>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>

#if defined(unix) || defined(__unix__) || defined(__unix) ||                   \
    defined(__APPLE__) || defined(linux) || defined(__linux) ||                \
    defined(__linux__)
#define PREDEF_PLATFORM_UNIX 1
#endif

#if defined(debug) || defined(_DEBUG) || defined(DEBUG)
#define PREDEF_MODE_DEBUG 1
#endif

namespace opentxs
{

typedef std::deque<OTString> dequeOfStrings;

class OTLogStream;

namespace api
{

class Settings;

}  // namespace api

#ifdef _WIN32
#ifdef OTLOG_IMPORT
#undef OTLOG_IMPORT
#define OTLOG_IMPORT __declspec(dllexport)
#else
#define OTLOG_IMPORT __declspec(dllimport)
#endif
#else
#ifndef OTLOG_IMPORT
#define OTLOG_IMPORT
#endif
#endif

extern LogSource LogOutput;  // otErr
extern LogSource LogNormal;  // otOut
extern LogSource LogDetail;
extern LogSource LogVerbose;
extern LogSource LogDebug;
extern LogSource LogTrace;
extern LogSource LogInsane;

class OTLogStream : public std::ostream, std::streambuf
{
private:
    int logLevel{0};
    int next{0};
    char* pBuffer{nullptr};
    std::recursive_mutex lock_;

public:
    explicit OTLogStream(int _logLevel);
    ~OTLogStream();

    virtual int overflow(int c) override;
};

class Log
{
private:
    static Log* pLogger;
    static const OTString m_strVersion;
    static const OTString m_strPathSeparator;

    const api::Settings& config_;
    std::int32_t m_nLogLevel{0};
    bool m_bInitialized{false};
    bool write_log_file_{false};
    OTString m_strLogFileName;
    OTString m_strLogFilePath;
    std::recursive_mutex lock_;

    /** For things that represent internal inconsistency in the code. Normally
     * should NEVER happen even with bad input from user. (Don't call this
     * directly. Use the above #defined macro instead.) */
    static Assert::fpt_Assert_sz_n_sz(logAssert);
    static bool CheckLogger(Log* pLogger);

    Log(const api::Settings& config);
    Log() = delete;
    Log(const Log&) = delete;
    Log(Log&&) = delete;
    Log& operator=(const Log&) = delete;
    Log& operator=(Log&&) = delete;

public:
    /** now the logger checks the global config file itself for the
     * log-filename. */
    EXPORT static bool Init(
        const api::Settings& config,
        const OTString strThreadContext = String::Factory(),
        const std::int32_t& nLogLevel = 0);

    EXPORT static bool IsInitialized();

    EXPORT static bool Cleanup();

    // OTLog Constants.
    //

    // Compiled into OTLog:

    EXPORT static const char* Version();
    EXPORT static const String& GetVersion();

    EXPORT static const char* PathSeparator();
    EXPORT static const String& GetPathSeparator();

    // Set in constructor:

    EXPORT static const char* LogFilePath();
    EXPORT static const String& GetLogFilePath();

    EXPORT static std::int32_t LogLevel();
    EXPORT static bool SetLogLevel(const std::int32_t& nLogLevel);

    // OTLog Functions:
    //

    EXPORT static bool LogToFile(const String& strOutput);

    EXPORT static bool Sleep(const std::chrono::microseconds us);

    EXPORT static void vOutput(
        std::int32_t nVerbosity,
        const char* szOutput,
        ...) ATTR_PRINTF(2, 3);

    EXPORT static void vError(const char* szError, ...)  // stderr
        ATTR_PRINTF(1, 2);

    // String Helpers
    EXPORT static bool StringFill(
        String& out_strString,
        const char* szString,
        std::int32_t iLength,
        const char* szAppend = nullptr);

private:
    friend OTLogStream;

    static void Error(const char* szError);
    static void Output(
        std::int32_t nVerbosity,
        const char* szOutput);  // stdout
};
}  // namespace opentxs
#endif
