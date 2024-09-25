#pragma once

#include <cassert>
#include <vector>

namespace discord {
namespace dave {

template <typename T>
class ArrayView {
public:
    ArrayView() = default;
    ArrayView(T* data, size_t size)
      : data_(data)
      , size_(size)
    {
    }

    size_t size() const { return size_; }
    T* data() const { return data_; }

    T* begin() const { return data_; }
    T* end() const { return data_ + size_; }

private:
    T* data_ = nullptr;
    size_t size_ = 0;
};

template <typename T>
inline ArrayView<T> MakeArrayView(T* data, size_t size)
{
    return ArrayView<T>(data, size);
}

template <typename T>
inline ArrayView<T> MakeArrayView(std::vector<T>& data)
{
    return ArrayView<T>(data.data(), data.size());
}

} // namespace dave
} // namespace discord
