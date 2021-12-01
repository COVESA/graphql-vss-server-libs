// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gustavo Sverzut Barbieri (barbieri@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

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
struct member_implementation_traits
{
    using getter_type = pointer_member_traits<AttributeGetterPointer>;
    using container_type = typename getter_type::container_type;
    using return_value =
        typename function_signature<typename getter_type::member_type>::return_type;
    using arguments_types =
        typename function_signature<typename getter_type::member_type>::arguments_types;
};

#define IMPLEMENTATION_TRAITS(_value, _getter)                                                         \
    member_implementation_traits<decltype(&_value::_getter)>::container_type,                          \
        member_implementation_traits<decltype(&_value::_getter)>::return_value, &_value::_getter
