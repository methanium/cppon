/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-references.h : Reference resolution and circular reference handling
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef CPPON_REFERENCES_H
#define CPPON_REFERENCES_H

#include "c++on-types.h"
#include <unordered_map>

namespace ch5 {
namespace details {

using reference_vector_t = std::vector<std::tuple<string_view_t, pointer_t>>;
using pointer_map_t = std::unordered_map<string_view_t, pointer_t>;

/**
 * @brief Checks if an object is nested within a parent object.
 */
inline auto is_object_inside(cppon& parent, pointer_t object) -> bool {
    return std::visit(
        [&](auto&& vector) -> bool {
            using type = std::decay_t<decltype(vector)>;
            if constexpr (std::is_same_v<type, array_t>) {
                for (auto& value : vector)
                    if (std::get_if<pointer_t>(&value) && object == *std::get_if<pointer_t>(&value))
                        return true;
                for (auto& value : vector)
                    if (is_object_inside(value, object))
                        return true;
            } else if constexpr (std::is_same_v<type, object_t>) {
                for (auto& [name, value] : vector)
                    if (std::get_if<pointer_t>(&value) && object == *std::get_if<pointer_t>(&value))
                        return true;
                for (auto& [dummy, value] : vector)
                    if (is_object_inside(value, object))
                        return true;
            } else if constexpr (std::is_same_v<type, pointer_t>) {
                if (object == vector)
                    return true;
            }
            return false;
        },
        static_cast<value_t&>(parent));
}

/**
 * @brief Checks for cyclic references.
 */
inline auto is_pointer_cyclic(pointer_t pointer) -> bool {
    return is_object_inside(*pointer, pointer);
}

/**
 * @brief Recursively finds the path of an object.
 */
inline auto find_object_path(cppon& from, pointer_t object) -> std::string {
    std::string result;
    result.reserve(64);
    std::visit(
        [&](auto&& vector) -> string_view_t {
            using type = std::decay_t<decltype(vector)>;
            if constexpr (std::is_same_v<type, array_t>) {
                for (size_t i = 0; i < vector.size(); ++i) {
                    if (object == &vector[i]) {
                        result += "/";
                        result += std::to_string(i);
                        return result;
                    }
					const auto& path = find_object_path(vector[i], object);
					if (!path.empty()) {
						result += "/";
						result += std::to_string(i);
						result += path;
						return result;
					}
				}
            } else if constexpr (std::is_same_v<type, object_t>) {
                for (auto& [name, value] : vector) {
                    if (object == &value) {
                        result += "/";
                        result += name;
                        return result;
                    }
                }
                for (auto& [name, value] : vector) {
                    const auto& path = find_object_path(value, object);
                    if (!path.empty()) {
                        result += "/";
                        result += name;
                        result += path;
                        return result;
                    }
                }
            }
            return result;
        },
        static_cast<value_t&>(from));
    return result;
}

/**
 * @brief Gets the path associated with a pointer.
 */
inline auto get_object_path(reference_vector_t& refs, pointer_t ptr) -> path_t {
    for (auto& [path, obj_ptr] : refs) {
        if (*std::get_if<pointer_t>(obj_ptr) == ptr)
            return path;
    }
    CPPON_ASSERT(!"The given pointer has no associated path in reference_vector_t.");
    throw std::runtime_error("The given pointer has no associated path in reference_vector_t.");
}

/**
 * @brief Finds and stores object references based on paths.
 */
inline void find_objects(cppon& object, pointer_map_t& objects, reference_vector_t& refs) {
    for (auto& [reference_path, path_source] : refs) {
        string_view_t path{ reference_path };
        pointer_t target_object = &object;
        size_t offset = 1;
        do {
            auto next = path.find('/', offset);
            auto segment = path.substr(offset, next - offset);
            auto& immutable = std::as_const(*target_object);
            target_object = const_cast<cppon*>(&immutable[segment]);
            if (target_object->is_null()) {
                path_source = nullptr;
                break;
            }
            if (next == string_view_t::npos) {
                objects.emplace(path, target_object);
                break;
            }
            offset = next + 1;
        } while (true);
    }
}

/**
 * @brief Identifies and records references in a cppon structure.
 */
inline void find_references(cppon& object, reference_vector_t& refs) {
    std::visit(
        [&](auto&& node) {
            using type = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<type, path_t>) {
                refs.emplace_back(node, &object); // std::tuple(node, &object)
            } else if constexpr (std::is_same_v<type, array_t>) {
                for (auto& sub_object : node)
                    find_references(sub_object, refs);
            } else if constexpr (std::is_same_v<type, object_t>) {
                for (auto& [name, sub_object] : node) {
                    find_references(sub_object, refs);
                }
            }
        },
        static_cast<value_t&>(object));
}

/**
 * @brief Establishes connections between referenced and target objects.
 */
inline auto resolve_paths(cppon& object) -> reference_vector_t {
    reference_vector_t references;
    pointer_map_t objects;
    references.reserve(16);
    objects.reserve(16);
    find_references(object, references);
    find_objects(object, objects, references);
    for (auto& [var, ptr] : references) {
        if (ptr)
            ptr->emplace<pointer_t>(objects[var]);
    }
    return references;
}

/**
 * @brief Reverts the linking process by replacing object pointers with their original reference strings.
 */
inline void restore_paths(const reference_vector_t& references) {
    for (auto& [var, ptr] : references) {
        ptr->emplace<path_t>(var);
    }
}

} // namespace details

using details::reference_vector_t;
using details::is_object_inside;
using details::is_pointer_cyclic;
using details::find_object_path;
using details::get_object_path;
using details::resolve_paths;
using details::restore_paths;

} // namespace ch5

#endif // CPPON_REFERENCES_H
