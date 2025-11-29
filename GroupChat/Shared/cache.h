#pragma once
#include <vector>
#include <cstddef>

template <typename T>
class CircularCache {
public:
    explicit CircularCache(std::size_t capacity = 50)
        : buf_(capacity), capacity_(capacity), size_(0), head_(0) {}

    void push(const T& value) {
        if (capacity_ == 0) return;
        buf_[head_] = value;
        head_ = (head_ + 1) % capacity_;
        if (size_ < capacity_) ++size_;
    }

    template <typename F>
    void forEach(F&& func) const {
        for (std::size_t i = 0; i < size_; ++i) {
            std::size_t idx = (head_ + capacity_ - size_ + i) % capacity_;
            func(buf_[idx]);
        }
    }

private:
    std::vector<T> buf_;
    std::size_t capacity_;
    std::size_t size_;
    std::size_t head_;
};
