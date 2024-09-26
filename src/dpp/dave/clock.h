#pragma once

#include <chrono>

namespace dpp::dave {

class clock_interface {
public:
    using base_clock = std::chrono::steady_clock;
    using time_point = base_clock::time_point;
    using clock_duration = base_clock::duration;

    virtual ~clock_interface() = default;
    virtual time_point now() const = 0;
};

class Clock : public clock_interface {
public:
    time_point now() const override { return base_clock::now(); }
};

} // namespace dpp::dave

