#pragma once
#include <tuple>

template <typename S>
struct function_signature;

template <typename R, typename... Args>
struct function_signature<R(Args...)>
{
	using return_type = R;
	using arguments_types = std::tuple<Args...>;
};

template <typename R, typename... Args>
struct function_signature<R(Args...) const>
{
	using return_type = R;
	using arguments_types = std::tuple<Args...>;
};

template <typename>
struct pointer_member_traits;

template <class C, class M>
struct pointer_member_traits<M C::*>
{
	using container_type = C;
	using member_type = M;
};

template <typename AttributeGetterPointer>
struct member_conversion_traits
{
	using getter_type = pointer_member_traits<AttributeGetterPointer>;
	using container_type = typename getter_type::container_type;
	using return_value =
		typename function_signature<typename getter_type::member_type>::return_type;
	using arguments_types =
		typename function_signature<typename getter_type::member_type>::arguments_types;
};

#define CONVERSION_TRAITS(_value, _getter)                                                         \
	member_conversion_traits<decltype(&_value::_getter)>::container_type,                          \
		member_conversion_traits<decltype(&_value::_getter)>::return_value, &_value::_getter
