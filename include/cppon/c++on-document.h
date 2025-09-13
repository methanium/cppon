/*
 * C++ON - High performance C++17 JSON parser with extended features
 * https://github.com/methanium/cppon
 *
 * File: c++on-document.h : CPPON document wrapper
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */


#ifndef CPPON_DOCUMENT_H
#define CPPON_DOCUMENT_H

#include "c++on-parser.h"
#include "c++on-printer.h"
#include "c++on-visitors.h"
#include <string_view>
#include <string>
#include <fstream>
#include <utility>

namespace ch5 {

/*
 * C++ON - document wrapper
 * Owns the input buffer so that all string_view_t / blob_string_t
 * produced by parsing remain valid for the lifetime of the document.
 */
class document : public cppon {
    std::string _buffer;

    void eval_and_assign(const options parse_mode) {
        static_cast<cppon&>(*this) = ch5::eval(_buffer, parse_mode);
        visitors::push_root(*this);
    }

public:
    document() = default;

    // Construct + eval (copies text)
    explicit document(const char* text, const options opt = Quick) {
        eval(text, opt);
    }

    // Construct + eval (take ownership)
    explicit document(std::string_view text, const options opt = Quick) {
        eval(text, opt);
    }

    // Construct + eval (take ownership)
    explicit document(std::string&& text, const options opt = Quick) {
        eval(std::move(text), opt);
    }

    // Non-copyable
    document(const document&)            = delete;
    document& operator=(const document&) = delete;

    // Movable
    document(document&& other) noexcept = default;
    document& operator=(document&& other) noexcept = default;

    // Access to underlying source
    bool empty() const noexcept { return _buffer.empty(); }
    std::string_view source_view() const noexcept { return _buffer; }
    const std::string& source() const noexcept { return _buffer; }

    document& set_source(std::string_view text) {
        _buffer.assign(text.begin(), text.end());
        return *this;
    }
    document& set_source(std::string&& text) noexcept {
        _buffer = std::move(text);
        return *this;
    }

    document& eval(const char* text, const options parse_mode = Quick) {
        set_source(std::string_view{ text });
		eval_and_assign(parse_mode);
        return *this;
    }
    document& eval(std::string_view text, const options parse_mode = Quick) {
        set_source(text);
        eval_and_assign(parse_mode);
        return *this;
    }
    document& eval(std::string&& text, const options parse_mode = Quick) {
        set_source(std::move(text));
        eval_and_assign(parse_mode);
        return *this;
    }

    // Reset everything (buffer + DOM)
    document& clear() noexcept {
        _buffer.clear();
        static_cast<cppon&>(*this) = cppon{}; // empty object_t
        return *this;
    }

    std::string serialize(const cppon& print_options = cppon{}) const {
        return to_string(*this, print_options);
	}

    void to_file(const std::string& filename, const cppon& print_options = cppon{}) const {
        std::ofstream out(filename, std::ios::binary);
        if (!out) throw file_operation_error(filename, "open");

        auto json = serialize(print_options);
        out.write(json.data(), static_cast<std::streamsize>(json.size()));

        if (!out) throw file_operation_error(filename, "write to");
    }

    bool to_file(const std::string& filename, std::string& error, const cppon& print_options = cppon{}) const noexcept {
        try {
            to_file(filename, print_options);
			return true;
        }
        catch(const std::exception& e) {
            error = e.what();
			return false;
		}
    }

    document& reparse(const options parse_mode = Quick) {
        eval_and_assign(parse_mode);
        return *this;
    }

    document & rematerialize(const cppon & print_options = cppon{}, const options parse_mode = Quick) {
        set_source(std::move(to_string(*this, print_options)));
        eval_and_assign(parse_mode);
        return *this;
    }

    // Load file (throws std::runtime_error on failure)
    static document from_string(std::string& file, const options opt = Quick) {
        return document{ std::move(file), opt };
    }

    // Load file (throws std::runtime_error on failure)
    static document from_file(const std::string& filename, const options opt = Quick) {
        std::ifstream in(filename, std::ios::binary);
        if (!in) throw file_operation_error(filename, "open");
        in.seekg(0, std::ios::end);
        std::string buf(static_cast<size_t>(in.tellg()), 0x00);
        in.seekg(0, std::ios::beg);
        in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        if (!in && !in.eof()) throw file_operation_error(filename, "read from");
        return document{ std::move(buf), opt };
    }

    // Load file (noexcept, returns invalid document on failure)
    static document from_file(const std::string& filename, std::string& error, const options opt = Quick) noexcept {
        try {
            return from_file(filename, opt);
        }
        catch(const std::exception& e) {
            error = e.what();
            document doc; static_cast<cppon&>(doc) = nullptr; // invalid document
            return doc;
        }
    }
};

} // namespace ch5

#endif // CPPON_DOCUMENT_H
