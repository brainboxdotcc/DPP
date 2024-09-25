#include "logger.h"

#include <atomic>
#include <cstring>
#include <iostream>

namespace discord {
namespace dave {

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

} // namespace dave
} // namespace discord
