/**
 * Copyright (c) 2026 Gauvain CHÉRY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_GENERATOR
#define MINT_GENERATOR

#include <coroutine>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace mint {

template<typename T>
class generator {
	static_assert(std::is_object_v<std::remove_cvref_t<T>>, "generator<T> requires an object type or a reference type");
public:
	using value_type = std::remove_cvref_t<T>;
	using reference = T;
	using pointer = std::add_pointer_t<std::remove_reference_t<T>>;

	struct promise_type;

	using handle_type = std::coroutine_handle<promise_type>;

	struct promise_type {
		pointer current = nullptr;
		std::exception_ptr exception;

		generator get_return_object() noexcept {
			return generator {handle_type::from_promise(*this)};
		}

		[[nodiscard]] std::suspend_always initial_suspend() const noexcept {
			return {};
		}

		[[nodiscard]] std::suspend_always final_suspend() const noexcept {
			return {};
		}

		std::suspend_always yield_value(reference value) noexcept {
			current = std::addressof(value);
			return {};
		}

		void return_void() const noexcept {}

		void unhandled_exception() noexcept {
			exception = std::current_exception();
		}
	};

	class iterator {
	public:
		using iterator_concept = std::input_iterator_tag;
		using iterator_category = std::input_iterator_tag;
		using value_type = generator::value_type;
		using difference_type = std::ptrdiff_t;
		using reference = generator::reference;
		using pointer = generator::pointer;

		iterator() noexcept = default;

		explicit iterator(handle_type handle) noexcept :
		    _handle(handle) {}

		reference operator*() const noexcept {
			return *_handle.promise().current;
		}

		pointer operator->() const noexcept {
			return _handle.promise().current;
		}

		iterator& operator++() {
			_handle.resume();
			if (_handle.done()) {
				if (auto exception = _handle.promise().exception) {
					std::rethrow_exception(exception);
				}
			}
			return *this;
		}

		void operator++(int) {
			++(*this);
		}

		friend bool operator==(const iterator& lhs, std::default_sentinel_t /*sentinel*/) noexcept {
			return !lhs._handle || lhs._handle.done();
		}

	private:
		handle_type _handle = nullptr;
	};

	generator() noexcept = default;

	explicit generator(handle_type handle) noexcept :
	    _handle(handle) {}

	generator(const generator&) = delete;
	generator& operator=(const generator&) = delete;

	generator(generator&& other) noexcept :
	    _handle(std::exchange(other._handle, nullptr)) {}

	generator& operator=(generator&& other) noexcept {
		if (this != &other) {
			if (_handle) {
				_handle.destroy();
			}
			_handle = std::exchange(other._handle, nullptr);
		}

		return *this;
	}

	~generator() {
		if (_handle) {
			_handle.destroy();
		}
	}

	[[nodiscard]] iterator begin() {

		if (!_handle) {
			return {};
		}

		_handle.resume();

		if (_handle.done()) {
			auto exception = _handle.promise().exception;

			if (exception) {
				std::rethrow_exception(exception);
			}

			return {};
		}

		return iterator {_handle};
	}

	[[nodiscard]] std::default_sentinel_t end() const noexcept {
		return {};
	}

private:
	handle_type _handle = nullptr;
};

}

#endif // MINT_GENERATOR
