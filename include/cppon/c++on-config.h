/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-config.h : C++ON Configuration and Settings
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_CONFIG_H
#define CPPON_CONFIG_H

#include "c++on-visitors.h"

namespace ch5 {

class config;
inline SimdLevel effective_simd_level() noexcept;
inline SimdLevel set_effective_simd_level(SimdLevel Level);
inline const config& get_config(string_view_t key) noexcept;
inline const std::array<string_view_t, 12>& get_vars() noexcept;
inline void update_config(size_t index);

class config : public cppon {
    inline static thread_local size_t dirty = ~0ULL;
public:
    config() = default;
    config(const cppon& lhr) : cppon{ lhr } {}
    config(cppon&& lvr) : cppon{ std::move(lvr) } {}
    config(const config&) = default;
    config(config&&) noexcept = default;
    ~config() noexcept = default;
    config& operator=(const config&) = default;
    config& operator=(config&&) noexcept = default;

    inline void update() {
        if (dirty == ~0ULL) return;
        update_config(dirty);
        dirty = ~0ULL;
    }

    inline config& operator[](string_view_t path) {
        auto& vars = get_vars();
        auto& var = cppon::operator[](path);
        auto name = path.substr(path.rfind("/") + 1);
        auto found = std::find(vars.begin(), vars.end(), name);
        dirty = (found == vars.end()) ? ~0ULL : distance(vars.begin(), found);
        return static_cast<config&>(var);
    }
    inline config& operator[](size_t index) {
        dirty = ~0ULL;
        return static_cast<config&>(cppon::operator[](index));
    }
    inline const config& operator[](string_view_t path) const {
        return static_cast<const config&>(cppon::operator[](path));
    }
    inline const config& operator[](size_t index) const {
        return static_cast<const config&>(cppon::operator[](index));
    }

    config& operator=(const char* val) {
        cppon::operator=( val );
        update();
        return *this;
    }

    config& operator=(pointer_t pointer) {
        cppon::operator=(pointer);
        update();
        return *this;
    }

    template<
        typename T,
        typename = std::enable_if_t<is_in_variant_lv<T, value_t>::value>>
    config& operator=(const T& val) {
        cppon::operator=(val);
        update();
        return *this;
    }

    template<
        typename T,
        typename = std::enable_if_t<is_in_variant_rv<T, value_t>::value>>
    config& operator=(T&& val) {
        cppon::operator=(std::forward<T>(val));
        update();
        return *this;
    }
};

inline static thread_local config Config{
    { object_t{
        { "parser", { object_t{
            { "prefix", { object_t{
                { "path", { string_view_t{CPPON_PATH_PREFIX} } },
                { "blob", { string_view_t{CPPON_BLOB_PREFIX} } },
                { "number", { string_view_t{CPPON_NUMBER_PREFIX} } }
            } } },
            { "exact", {false} }
        } } },
        { "scanner", { object_t{
            { "simd", { object_t{
                { "global", { nullptr } },
                { "thread", { nullptr } },
                { "current", { nullptr } }
            } } }
        } } },
        { "memory", { object_t{
            { "reserve", { object_t{
                { "object_safe", { (int64_t)CPPON_OBJECT_SAFE_RESERVE } },
                { "array_safe", { (int64_t)CPPON_ARRAY_SAFE_RESERVE } },
                { "object", { (int64_t)CPPON_OBJECT_MIN_RESERVE } },
                { "array", { (int64_t)CPPON_ARRAY_MIN_RESERVE } },
                { "printer", { (int64_t)CPPON_PRINTER_RESERVE_PER_ELEMENT } }
            } } }
        } } }
    } }
};

inline const config& get_config(string_view_t key) noexcept {
    try {
        return Config[key];
    }
    catch (...) { return static_cast<const config&>(null()); }
}
inline bool has_config(string_view_t key) noexcept {
    return get_config(key).is_null() ? false : true;
}
template<
    typename T,
    typename = std::enable_if_t<is_in_variant_lv<T, value_t>::value>>
inline config& set_config(string_view_t key, const T& value) {
    return Config[key] = value;
}
template<
    typename T,
    typename = std::enable_if_t<is_in_variant_rv<T, value_t>::value>>
inline config& set_config(string_view_t key, T&& value) {
    return Config[key] = std::move(value);
}

inline void set_path_prefix(string_view_t prefix) {
    set_config("parser/prefixes/path", prefix);
}
inline void set_blob_prefix(string_view_t prefix) {
    set_config("parser/prefixes/blob", prefix);
}
inline void set_number_prefix(string_view_t prefix) {
    set_config("parser/prefixes/number", prefix);
}
inline void set_exact_number_mode(boolean_t mode) {
    set_config("parser/exact", mode);
}
inline boolean_t get_exact_number_mode() noexcept {
    return thread::exact_number_mode;
}
inline void set_global_simd_override(SimdLevel Level) {
    thread::set_global_simd_override(Level);
    set_config("scanner/simd/global", (int64_t)thread::effective_simd_level());
}
inline void clear_global_simd_override() {
    thread::clear_global_simd_override();
    set_config("scanner/simd/global", nullptr);
}
inline void set_thread_simd_override(SimdLevel Level) {
    thread::set_thread_simd_override(Level);
    set_config("scanner/simd/thread", (int64_t)thread::effective_simd_level());
}
inline void clear_thread_simd_override() {
    thread::clear_thread_simd_override();
    set_config("scanner/simd/thread", nullptr);
}
inline SimdLevel set_effective_simd_level(SimdLevel Level) {
    set_config("scanner/simd/current", (int64_t)Level);
    return thread::effective_simd_level();
}
inline SimdLevel effective_simd_level() noexcept {
    return thread::effective_simd_level();
}
inline void set_object_safe_reserve(std::byte reserve) {
    set_config("memory/reserve/object_safe", int64_t(reserve));
}
inline void set_array_safe_reserve(std::byte reserve) {
    set_config("memory/reserve/array_safe", int64_t(reserve));
}
inline void set_object_min_reserve(std::byte reserve) {
    set_config("memory/reserve/object", int64_t(reserve));
}
inline void set_array_min_reserve(std::byte reserve) {
    set_config("memory/reserve/array", int64_t(reserve));
}
inline void set_printer_reserve_per_element(std::byte reserve) {
    set_config("memory/reserve/printer", int64_t(reserve));
}

constexpr static std::array<string_view_t, 12> vars{
    "path", "blob", "number", "exact", "global",
    "thread", "current", "object_safe", "array_safe", "object", "array", "printer" };

constexpr static std::array<void(*)(), 12> updaters{
    [] {const config& path = get_config("parser/prefix/path");
        if (path.is_null()) thread::path_prefix.current = CPPON_PATH_PREFIX;
        else if (static_cast<string_view_t>(thread::path_prefix) != std::get<string_view_t>(path))
            thread::path_prefix = std::get<string_view_t>(path);
    },
    [] {const auto& blob = get_config("parser/prefix/blob");
        if (blob.is_null()) thread::blob_prefix.current = CPPON_BLOB_PREFIX;
        else if (static_cast<string_view_t>(thread::blob_prefix) != std::get<string_view_t>(blob))
            thread::blob_prefix = std::get<string_view_t>(blob);
    },
    [] {const auto& number = get_config("parser/prefix/number");
        if (number.is_null()) thread::number_prefix.current = CPPON_NUMBER_PREFIX;
        else if (static_cast<string_view_t>(thread::number_prefix) != std::get<string_view_t>(number))
            thread::number_prefix = std::get<string_view_t>(number);
    },
    [] {const auto& exact = get_config("parser/exact");
        if (exact.is_null()) thread::exact_number_mode.current = false;
        else if (get_cast<boolean_t>(exact) != static_cast<boolean_t>(thread::exact_number_mode))
            thread::exact_number_mode.current = get_cast<boolean_t>(exact);
    },
    [] {const auto& global = get_config("scanner/simd/global");
        const auto& thread = get_config("scanner/simd/thread");
        const auto& current = get_config("scanner/simd/current");
        if (global.is_null()) thread::clear_global_simd_override();
        else thread::set_global_simd_override((SimdLevel)get_cast<int64_t>(global));
        if (thread.is_null()) thread::clear_thread_simd_override();
        else thread::set_thread_simd_override((SimdLevel)get_cast<int64_t>(thread));
        if (global.is_null() && thread.is_null() && (SimdLevel)get_cast<int64_t>(current) != thread::effective_simd_level()) {
            thread::set_thread_simd_override((SimdLevel)get_cast<int64_t>(current));
        }
        thread::simd_default.current = (int64_t)thread::effective_simd_level();
    },
    [] {updaters[4](); },
    [] {updaters[4](); },
    [] {const auto& object = get_config("memory/reserve/object_safe");
        if (object.is_null()) thread::object_safe_reserve.current = CPPON_OBJECT_SAFE_RESERVE;
        else if (get_cast<int64_t>(object) != static_cast<int64_t>(thread::object_safe_reserve))
            thread::object_safe_reserve.current = get_cast<int64_t>(object);
    },
    [] {const auto& array = get_config("memory/reserve/array_safe");
        if (array.is_null()) thread::array_safe_reserve.current = CPPON_ARRAY_SAFE_RESERVE;
        else if (get_cast<int64_t>(array) != static_cast<int64_t>(thread::array_safe_reserve))
            thread::array_safe_reserve.current = get_cast<int64_t>(array);
    },
    [] {const auto& object = get_config("memory/reserve/object");
        if (object.is_null()) thread::object_min_reserve.current = CPPON_OBJECT_MIN_RESERVE;
        else if (get_cast<int64_t>(object) != static_cast<int64_t>(thread::object_min_reserve))
            thread::object_min_reserve.current = get_cast<int64_t>(object);
    },
    [] {const auto& array = get_config("memory/reserve/array");
        if (array.is_null()) thread::array_min_reserve.current = CPPON_ARRAY_MIN_RESERVE;
        else if (get_cast<int64_t>(array) != static_cast<int64_t>(thread::array_min_reserve))
            thread::array_min_reserve.current = get_cast<int64_t>(array);
    },
   [] {const auto& printer = get_config("memory/reserve/printer");
        if (printer.is_null()) thread::printer_reserve_per_element.current = CPPON_PRINTER_RESERVE_PER_ELEMENT;
        else if (get_cast<int64_t>(printer) != static_cast<int64_t>(thread::printer_reserve_per_element))
            thread::printer_reserve_per_element.current = get_cast<int64_t>(printer);
    } };

inline void update_config(size_t index) {
    updaters[index]();
}

inline const std::array<string_view_t, 12>& get_vars() noexcept {
    return vars;
}

} // namespace ch5

#endif // CPPON_CONFIG_H
