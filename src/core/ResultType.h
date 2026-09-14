#pragma once
#include <utility>
#include <cassert>
#include <variant>

namespace tcp {

template <typename T>
class Result {
    public:
        static Result ok(T value) {
            return Result(std::in_place_index<0>, std::move(value));
        }
        static Result err(int code) {
            return Result(std::in_place_index<1>, code);
        }
        bool has_value() const noexcept { return storage_.index() == 0; }
        explicit operator bool() const noexcept { return has_value(); }

        T& value() {
            assert(has_value());
            return std::get<0>(storage_);
        }
        const T& value() const {
            assert(has_value());
            return std::get<0>(storage_);
        }
        int error() const noexcept {
            return std::get<1>(storage_);
        }

    private:
        template <typename Tag, typename U>
        Result(Tag tag, U&& v) : storage_(tag, std::forward<U>(v)) {}

        std::variant<T, int> storage_;
};

template <>
class Result<void> {
    public:
        static Result ok() {
            return Result(std::in_place_index<0>, std::monostate{});
        }
        static Result err(int code) {
            return Result(std::in_place_index<1>, code);
        }
        bool has_value() const noexcept { return storage_.index() == 0; }
        explicit operator bool() const noexcept { return has_value(); }

        int error() const noexcept {
            return std::get<1>(storage_);
        }

    private:
        template <typename Tag, typename U>
        Result(Tag tag, U&& v) : storage_(tag, std::forward<U>(v)) {}

        std::variant<std::monostate, int> storage_;
};

}
