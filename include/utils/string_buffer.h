/*
 * String buffer with memory pool allocator
 * https://github.com/methanium/cppon
 *
 * MIT License
 * Copyright (c) 2019-2025 Manuel Zaccaria (methanium) / CH5 Design
 *
 * See LICENSE file for complete license details
 */

#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <deque>
#include <string>
#include <ostream>
#include <algorithm>
#include <cstdint>

namespace utils {

/**
 * @brief A class that represents a string buffer.
 * 
 * This class provides functionality to efficiently append strings and convert the buffer to a string.
 * It uses a deque to store the strings and supports dynamic resizing.
 * 
 * @tparam char_t The character type of the string buffer.
 * @tparam char_traits_t The character traits type for the string buffer.
 * @tparam allocator_t The allocator type for the string buffer.
 */
template<class char_t, class char_traits_t = std::char_traits<char_t>, class allocator_t = std::allocator<char_t>>
class string_buffer {
    using string_t      = std::basic_string<char_t, char_traits_t, allocator_t>;
    using stream_t      = std::basic_ostream<char_t, char_traits_t>;
    using string_view_t = std::basic_string_view<char_t, char_traits_t>;
    using deque_t       = std::deque<string_t>;

public:
    deque_t Buf;              /**< The deque to store the strings. */
    size_t  growth_numerator; /**< The growth numerator for dynamic resizing. */ 
    size_t  Size = 0;         /**< The total size of the string buffer. */

    /**
     * @brief Constructs a new string buffer object.
     * 
     * @param InitialSize The initial size of the buffer.
     * @param rate The growth rate for dynamic resizing. Default is 2.0 (clamped to [1.0; 4.0]).
	 * @exception std::bad_alloc Thrown if memory allocation fails.
     */
    string_buffer(size_t InitialSize = 16u, double rate = 2.0) : Buf{ 1 }, Size{ 0 } {
        set_growth_rate(rate);
        Buf.back().reserve(std::max(size_t(16u), InitialSize));
    }

    /**
     * @brief Destroys the string buffer object.
     */
    ~string_buffer() = default;

    /**
     * @brief Sets the growth rate for dynamic resizing.
     * 
     * @param rate The growth rate to set (clamped to [1.0; 4.0]).
     */
	void set_growth_rate(double rate) {
		constexpr double min_rate = 1.;
		constexpr double max_rate = 4.;
		rate = std::min(std::max(rate, min_rate), max_rate);
		growth_numerator = static_cast<size_t>(rate * 256);
	}

    /**
     * @brief Appends a string to the buffer.
     * 
     * @param Text The string to append.
     * @return The reference to the string buffer.
	 * @exception std::bad_alloc Thrown if memory allocation fails.
     */
    auto& append(string_view_t Text) {
		// Save the initial state of the buffer
		auto initialBufSize = Buf.size();
		auto initialBackSize = Buf.back().size();
		auto initialSize = Size;

		try {
			auto Remain = Buf.back().capacity() - Buf.back().size();
			auto ToWrite = Text.size();

			if (Remain == 0) {
				auto LastSize = Buf.back().size();
				Buf.emplace_back();
				Buf.back().reserve(LastSize * growth_numerator / 256);
				Remain = Buf.back().capacity(); // adjusted capacity
			}

			do {
				if (ToWrite <= Remain) {
					Buf.back().append(Text);
					Size += ToWrite;
					Remain -= ToWrite;
					ToWrite = 0;
				}
				else {
					Buf.back().append(Text.substr(0, Remain));
					Text.remove_prefix(Remain);
					Size += Remain;
					ToWrite -= Remain;
					auto LastSize = Buf.back().size();
					Buf.emplace_back();
					Buf.back().reserve(LastSize * growth_numerator / 256);
					Remain = Buf.back().capacity();
				}
			} while (ToWrite > 0);

			return *this;
		}
		catch (...) {
			// Rollback the buffer to its initial state
			while (Buf.size() > initialBufSize) {
				Buf.pop_back();
			}
			Buf.back().resize(initialBackSize);
			Size = initialSize;
			throw;
		}
    }

    /**
     * @brief Writes the contents of the buffer to a stream.
     * 
     * @param Stream The stream to write to.
     * @return The reference to the stream.
	 * @exception std::ios_base::failure Thrown if an I/O error occurs.
     */
    auto& write(stream_t& Stream) const {
        for(auto& Text : Buf)
            Stream.write(std::data(Text), std::size(Text));
        return Stream;
    }

    /**
     * @brief Converts the buffer to a string.
     * 
     * @return The string representation of the buffer.
	 * @exception std::bad_alloc Thrown if memory allocation fails.
     */
    auto to_string() const {
        string_t String;
        String.reserve(Size);
        for(auto& Text : Buf)
            String.append(Text);
        return String;
    }

    // Chaîne alignée 64B (zmm) avec padding tête/queue.
    // Retour: Storage (paddée) et HeadPad (début logique aligné).
    auto to_string(size_t& alignedOffset) const {
        constexpr size_t Align = 64u; // alignement zmm
        constexpr size_t AlignMask = Align - 1u;

        string_t s;
        // Réserve assez pour éviter toute réallocation (pire cas: headPad = 2*Align-1)
        s.reserve(Size + 3u * Align);

        // 1) Pré-padding minimal pour matérialiser data() et disposer d’un jeu
        s.resize(Align, char_t(' '));

        // 2) Calcule l’offset aligné: aligner (data()+size()) sur 64B
        const auto base = reinterpret_cast<std::uintptr_t>(s.data());
        alignedOffset = static_cast<size_t>(((base + s.size() + AlignMask) & ~static_cast<std::uintptr_t>(AlignMask)) - base);

        // 3) Redimensionne à l’offset aligné (rempli d’espaces)
        s.resize(alignedOffset, char_t(' '));

        // 4) Copie le contenu logique (append des chunks)
        for (const auto& chunk : Buf) {
            if (!chunk.empty())
                s.append(chunk);
        }

        // 5) Padding de queue: garantir au moins Align octets sûrs après le contenu
        const size_t target = alignedOffset + Size + Align;
        if (s.size() < target) {
            s.resize(target, char_t(' '));
        }

        return std::move(s);
    }

};

}//namespace utils

#endif
