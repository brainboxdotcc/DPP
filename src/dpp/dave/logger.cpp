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
#include "logger.h"

#include <atomic>
#include <cstring>
#include <iostream>

namespace dpp::dave {

std::atomic<LogSink> gLogSink = nullptr;

void SetLogSink(LogSink sink)
{
    gLogSink = sink;
}

LogStreamer::LogStreamer(LoggingSeverity severity, const char* file, int line)
  : severity_(severity)
  , file_(file)
  , line_(line)
{
}

LogStreamer::~LogStreamer()
{
    std::string logLine = stream_.str();
    if (logLine.empty()) {
        return;
    }

    auto sink = gLogSink.load();
    if (sink) {
        sink(severity_, file_, line_, logLine);
        return;
    }

    switch (severity_) {
    case LS_VERBOSE:
    case LS_INFO:
    case LS_WARNING:
    case LS_ERROR: {
        const char* file = file_;
        if (auto separator = strrchr(file, '/')) {
            file = separator + 1;
        }
        std::cout << "(" << file << ":" << line_ << ") " << logLine << std::endl;
        break;
    }
    case LS_NONE:
        break;
    }
}

} // namespace dpp::dave

