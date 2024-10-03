/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2021 Craig Edwards and D++ contributors
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This folder is a modified fork of libdave, https://github.com/discord/libdave
 * Copyright (c) 2024 Discord, Licensed under MIT
 *
 ************************************************************************************/
#pragma once

#include <sstream>

#if !defined(DISCORD_LOG)
#define DISCORD_LOG_FILE_LINE(sev, file, line) ::dpp::dave::LogStreamer(sev, file, line)
#define DISCORD_LOG(sev) DISCORD_LOG_FILE_LINE(::dpp::dave::sev, __FILE__, __LINE__)
#endif

namespace dpp::dave {

enum LoggingSeverity {
    LS_VERBOSE,
    LS_INFO,
    LS_WARNING,
    LS_ERROR,
    LS_NONE,
};

using LogSink = void (*)(LoggingSeverity severity,
                         const char* file,
                         int line,
                         const std::string& message);
void SetLogSink(LogSink sink);

class LogStreamer {
public:
    LogStreamer(LoggingSeverity severity, const char* file, int line);
    ~LogStreamer();

    template <typename T>
    LogStreamer& operator<<(const T& value)
    {
        stream_ << value;
        return *this;
    }

private:
    LoggingSeverity severity_;
    const char* file_;
    int line_;
    std::ostringstream stream_;
};

} // namespace dpp::dave

