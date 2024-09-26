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

