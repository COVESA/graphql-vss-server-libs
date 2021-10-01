// This code is based on PEGTL (MIT License)
// https://github.com/taocpp/PEGTL/blob/master/include/tao/pegtl/demangle.hpp
// without the MS compiler support
//
// Requires modern C++ compiler, recommended: C++17
// - const_expr
// - static_assert

#pragma once

#ifndef _DEMANGLE_H_
#define _DEMANGLE_H_ 1

#include <string_view>
#include <iostream>

#if defined(__clang__)

#if defined(_LIBCPP_VERSION)

template <typename T>
[[nodiscard]] constexpr std::string_view demangle() noexcept
{
	constexpr std::string_view sv = __PRETTY_FUNCTION__;
	constexpr auto begin = sv.find('=');
	static_assert(begin != std::string_view::npos);
	return sv.substr(begin + 2, sv.size() - begin - 3);
}

#else

// When using libstdc++ with clang, std::string_view::find is not constexpr :(
template <char C>
constexpr const char* find(const char* p, std::size_t n) noexcept
{
	while (n)
	{
		if (*p == C)
		{
			return p;
		}
		++p;
		--n;
	}
	return nullptr;
}

template <typename T>
[[nodiscard]] constexpr std::string_view demangle() noexcept
{
	constexpr std::string_view sv = __PRETTY_FUNCTION__;
	constexpr auto begin = find<'='>(sv.data(), sv.size());
	static_assert(begin != nullptr);
	return { begin + 2, sv.data() + sv.size() - begin - 3 };
}

#endif // defined(_LIBCPP_VERSION)

#elif defined(__GNUC__)

#if (__GNUC__ == 7)

// GCC 7 wrongly sometimes disallows __PRETTY_FUNCTION__ in constexpr functions,
// therefore we drop the 'constexpr' and hope for the best.
template <typename T>
[[nodiscard]] std::string_view demangle() noexcept
{
	const std::string_view sv = __PRETTY_FUNCTION__;
	const auto begin = sv.find('=');
	const auto tmp = sv.substr(begin + 2);
	const auto end = tmp.rfind(';');
	return tmp.substr(0, end);
}

#elif (__GNUC__ == 9) && (__GNUC_MINOR__ < 3)

// GCC 9.1 and 9.2 have a bug that leads to truncated __PRETTY_FUNCTION__ names,
// see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91155
template <typename T>
[[nodiscard]] constexpr std::string_view demangle() noexcept
{
	// fallback: requires RTTI, no demangling
	return typeid(T).name();
}

#else

template <typename T>
[[nodiscard]] constexpr std::string_view demangle() noexcept
{
	constexpr std::string_view sv = __PRETTY_FUNCTION__;
	constexpr auto begin = sv.find('=');
	static_assert(begin != std::string_view::npos);
	constexpr auto tmp = sv.substr(begin + 2);
	constexpr auto end = tmp.rfind(';');
	static_assert(end != std::string_view::npos);
	return tmp.substr(0, end);
}

#endif // (__GNUC__ == 7)
#endif // defined(__clang__)

constexpr std::string_view membername(const std::string_view& sv)
{
	auto begin = sv.rfind("::");
	if (begin == std::string_view::npos)
	{
		return sv;
	}
	return sv.substr(begin + 2, sv.size() - begin - 2);
}

// number is 0 based (first argument is 0)
constexpr std::string_view templateargument(unsigned int number, const std::string_view& sv)
{
	size_t start = 0;
	unsigned int arg = 0;
	int nesting = 0;

	for (size_t itr = 0; itr < sv.size(); itr++)
	{
		const char ch = sv[itr];

		if (nesting == 0) // start
		{
			if (ch == '<')
			{
				nesting++;
				start = itr + 1;
			}
		}
		else if (nesting == 1) // core processing
		{
			if (ch == '<')
				nesting++;
			else if (ch == ' ')
				start++;
			else if (ch == '>')
			{
				if (arg == number)
					return sv.substr(start, itr - start);
				break;
			}
			else if (ch == ',')
			{
				if (arg == number)
					return sv.substr(start, itr - start);
				start = itr + 1;
				arg++;
			}
		}
		else // nested, ignore
		{
			if (ch == '<')
				nesting++;
			else if (ch == '>')
				nesting--;
		}
	}

	return sv;
}

#endif // _DEMANGLE_H_
