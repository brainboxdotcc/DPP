#pragma once

#include <algorithm>
#include <functional>
#include <utility>

namespace discord {
namespace dave {

class [[nodiscard]] ScopeExit final {
public:
    template <typename Cleanup>
    explicit ScopeExit(Cleanup&& cleanup)
      : cleanup_{std::forward<Cleanup>(cleanup)}
    {
    }

    ScopeExit(ScopeExit&& rhs)
      : cleanup_{std::move(rhs.cleanup_)}
    {
        rhs.cleanup_ = nullptr;
    }

    ~ScopeExit()
    {
        if (cleanup_) {
            cleanup_();
        }
    }

    ScopeExit& operator=(ScopeExit&& rhs)
    {
        cleanup_ = std::move(rhs.cleanup_);
        rhs.cleanup_ = nullptr;
        return *this;
    }

    void Dismiss() { cleanup_ = nullptr; }

private:
    ScopeExit(ScopeExit const&) = delete;
    ScopeExit& operator=(ScopeExit const&) = delete;

    std::function<void()> cleanup_;
};

} // namespace dave
} // namespace discord
