#pragma once

#include <chrono>

namespace discord {
namespace dave {

class IClock {
public:
    using BaseClock = std::chrono::steady_clock;
    using TimePoint = BaseClock::time_point;
    using Duration = BaseClock::duration;

    virtual ~IClock() = default;
    virtual TimePoint Now() const = 0;
};

class Clock : public IClock {
public:
    TimePoint Now() const override { return BaseClock::now(); }
};

} // namespace dave
} // namespace discord
