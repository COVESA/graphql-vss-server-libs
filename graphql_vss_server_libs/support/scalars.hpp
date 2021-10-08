// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gustavo Sverzut Barbieri (barbieri@profusion.mobi),
//   Author: Gabriel Fernandes (g7fernandes@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#pragma once

#include <graphqlservice/GraphQLResponse.h>
#include <boost/lexical_cast.hpp>

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <cstdio>

#include "graphql_vss_server_libs-support_export.h"

using namespace graphql;
template <typename T, std::enable_if_t<std::is_arithmetic<T>::value>* = nullptr>
struct RangeInterval
{
	T min = std::numeric_limits<T>::lowest();
	T max = std::numeric_limits<T>::max();
};

template <typename T, std::enable_if_t<std::is_arithmetic<T>::value>* = nullptr>
T stringToNumber(const std::string& str);

template <typename TResult, typename TInput,
	std::enable_if_t<std::is_arithmetic<TResult>::value && std::is_arithmetic<TInput>::value>* =
		nullptr>
TResult validateRange(TInput value, const struct RangeInterval<TResult>& range);

template <typename TResult, std::enable_if_t<std::is_arithmetic<TResult>::value>* = nullptr>
TResult validateRange(const std::string& value, const struct RangeInterval<TResult>& range);

template <typename TResult, std::enable_if_t<std::is_arithmetic<TResult>::value>* = nullptr>
TResult validateRange(const response::Value& value, const struct RangeInterval<TResult>& range);

template <typename TResult, typename TValue,
	std::enable_if_t<std::is_arithmetic<TResult>::value>* = nullptr>
std::optional<TResult>
validateRange(const std::optional<TValue>& value, const struct RangeInterval<TResult>& range);

#ifndef SCALARS_HPP_LOCAL_TEMPLATES
extern template int64_t stringToNumber<int64_t>(const std::string& str);
extern template uint64_t stringToNumber<uint64_t>(const std::string& str);
extern template int32_t stringToNumber<int32_t>(const std::string& str);
extern template uint32_t stringToNumber<uint32_t>(const std::string& str);
extern template int16_t stringToNumber<int16_t>(const std::string& str);
extern template uint16_t stringToNumber<uint16_t>(const std::string& str);
extern template int8_t stringToNumber<int8_t>(const std::string& str);
extern template uint8_t stringToNumber<uint8_t>(const std::string& str);
extern template double stringToNumber<double>(const std::string& str);
extern template float stringToNumber<float>(const std::string& str);

extern template int64_t
validateRange<int64_t>(int64_t value, const struct RangeInterval<int64_t>& range);
extern template uint64_t
validateRange<uint64_t>(uint64_t value, const struct RangeInterval<uint64_t>& range);
extern template int32_t
validateRange<int32_t>(int32_t value, const struct RangeInterval<int32_t>& range);
extern template uint32_t
validateRange<uint32_t>(uint32_t value, const struct RangeInterval<uint32_t>& range);
extern template int16_t
validateRange<int16_t>(int16_t value, const struct RangeInterval<int16_t>& range);
extern template uint16_t
validateRange<uint16_t>(uint16_t value, const struct RangeInterval<uint16_t>& range);
extern template int8_t
validateRange<int8_t>(int8_t value, const struct RangeInterval<int8_t>& range);
extern template uint8_t
validateRange<uint8_t>(uint8_t value, const struct RangeInterval<uint8_t>& range);
extern template double
validateRange<double>(double value, const struct RangeInterval<double>& range);
extern template float validateRange<float>(float value, const struct RangeInterval<float>& range);

// more for testing, shouldn't exist in real code
extern template uint64_t
validateRange<uint64_t, int>(int value, const struct RangeInterval<uint64_t>& range);
extern template uint32_t
validateRange<uint32_t, int>(int value, const struct RangeInterval<uint32_t>& range);
extern template uint16_t
validateRange<uint16_t, int>(int value, const struct RangeInterval<uint16_t>& range);
extern template uint8_t
validateRange<uint8_t, int>(int value, const struct RangeInterval<uint8_t>& range);
extern template int64_t
validateRange<int64_t, int>(int value, const struct RangeInterval<int64_t>& range);
extern template int16_t
validateRange<int16_t, int>(int value, const struct RangeInterval<int16_t>& range);
extern template int8_t
validateRange<int8_t, int>(int value, const struct RangeInterval<int8_t>& range);
extern template uint64_t
validateRange<uint64_t, double>(double value, const struct RangeInterval<uint64_t>& range);
extern template uint32_t
validateRange<uint32_t, double>(double value, const struct RangeInterval<uint32_t>& range);
extern template uint16_t
validateRange<uint16_t, double>(double value, const struct RangeInterval<uint16_t>& range);
extern template uint8_t
validateRange<uint8_t, double>(double value, const struct RangeInterval<uint8_t>& range);
extern template int64_t
validateRange<int64_t, double>(double value, const struct RangeInterval<int64_t>& range);
extern template int32_t
validateRange<int32_t, double>(double value, const struct RangeInterval<int32_t>& range);
extern template int16_t
validateRange<int16_t, double>(double value, const struct RangeInterval<int16_t>& range);
extern template int8_t
validateRange<int8_t, double>(double value, const struct RangeInterval<int8_t>& range);

extern template int64_t
validateRange<int64_t>(const std::string& value, const struct RangeInterval<int64_t>& range);
extern template uint64_t
validateRange<uint64_t>(const std::string& value, const struct RangeInterval<uint64_t>& range);
extern template int32_t
validateRange<int32_t>(const std::string& value, const struct RangeInterval<int32_t>& range);
extern template uint32_t
validateRange<uint32_t>(const std::string& value, const struct RangeInterval<uint32_t>& range);
extern template int16_t
validateRange<int16_t>(const std::string& value, const struct RangeInterval<int16_t>& range);
extern template uint16_t
validateRange<uint16_t>(const std::string& value, const struct RangeInterval<uint16_t>& range);
extern template int8_t
validateRange<int8_t>(const std::string& value, const struct RangeInterval<int8_t>& range);
extern template uint8_t
validateRange<uint8_t>(const std::string& value, const struct RangeInterval<uint8_t>& range);
extern template double
validateRange<double>(const std::string& value, const struct RangeInterval<double>& range);
extern template float
validateRange<float>(const std::string& value, const struct RangeInterval<float>& range);

extern template int64_t
validateRange<int64_t>(const response::Value& value, const struct RangeInterval<int64_t>& range);
extern template uint64_t
validateRange<uint64_t>(const response::Value& value, const struct RangeInterval<uint64_t>& range);
extern template int32_t
validateRange<int32_t>(const response::Value& value, const struct RangeInterval<int32_t>& range);
extern template uint32_t
validateRange<uint32_t>(const response::Value& value, const struct RangeInterval<uint32_t>& range);
extern template int16_t
validateRange<int16_t>(const response::Value& value, const struct RangeInterval<int16_t>& range);
extern template uint16_t
validateRange<uint16_t>(const response::Value& value, const struct RangeInterval<uint16_t>& range);
extern template int8_t
validateRange<int8_t>(const response::Value& value, const struct RangeInterval<int8_t>& range);
extern template uint8_t
validateRange<uint8_t>(const response::Value& value, const struct RangeInterval<uint8_t>& range);
extern template double
validateRange<double>(const response::Value& value, const struct RangeInterval<double>& range);
extern template float
validateRange<float>(const response::Value& value, const struct RangeInterval<float>& range);

extern template std::optional<int64_t> validateRange<int64_t, int64_t>(
	const std::optional<int64_t>& value, const struct RangeInterval<int64_t>& range);
extern template std::optional<uint64_t> validateRange<uint64_t, uint64_t>(
	const std::optional<uint64_t>& value, const struct RangeInterval<uint64_t>& range);
extern template std::optional<int32_t> validateRange<int32_t, int32_t>(
	const std::optional<int32_t>& value, const struct RangeInterval<int32_t>& range);
extern template std::optional<uint32_t> validateRange<uint32_t, uint32_t>(
	const std::optional<uint32_t>& value, const struct RangeInterval<uint32_t>& range);
extern template std::optional<int16_t> validateRange<int16_t, int16_t>(
	const std::optional<int16_t>& value, const struct RangeInterval<int16_t>& range);
extern template std::optional<uint16_t> validateRange<uint16_t, uint16_t>(
	const std::optional<uint16_t>& value, const struct RangeInterval<uint16_t>& range);
extern template std::optional<int8_t> validateRange<int8_t, int8_t>(
	const std::optional<int8_t>& value, const struct RangeInterval<int8_t>& range);
extern template std::optional<uint8_t> validateRange<uint8_t, uint8_t>(
	const std::optional<uint8_t>& value, const struct RangeInterval<uint8_t>& range);
extern template std::optional<double> validateRange<double, double>(
	const std::optional<double>& value, const struct RangeInterval<double>& range);
extern template std::optional<float> validateRange<float, float>(
	const std::optional<float>& value, const struct RangeInterval<float>& range);

extern template std::optional<int64_t> validateRange<int64_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<int64_t>& range);
extern template std::optional<uint64_t> validateRange<uint64_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<uint64_t>& range);
extern template std::optional<int32_t> validateRange<int32_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<int32_t>& range);
extern template std::optional<uint32_t> validateRange<uint32_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<uint32_t>& range);
extern template std::optional<int16_t> validateRange<int16_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<int16_t>& range);
extern template std::optional<uint16_t> validateRange<uint16_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<uint16_t>& range);
extern template std::optional<int8_t> validateRange<int8_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<int8_t>& range);
extern template std::optional<uint8_t> validateRange<uint8_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<uint8_t>& range);
extern template std::optional<double> validateRange<double, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<double>& range);
extern template std::optional<float> validateRange<float, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<float>& range);
#endif

class GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT ColorInputStringException : public std::exception
{
public:
	ColorInputStringException();
	const char* what() const noexcept override;
};

class GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT ColorInputRangeException : public std::exception
{
public:
	ColorInputRangeException();
	const char* what() const noexcept override;
};

struct HSVColor
{
	double hue;
	double saturation;
	double value;

	HSVColor() = default;

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT HSVColor(const response::Value& value_);

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT HSVColor(const std::string& stringInput);

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT HSVColor(double h, double s, double v);

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::string str();

	inline operator std::string()
	{
		return str();
	}

	inline HSVColor& operator=(const std::string& inputString)
	{
		set(inputString);
		return *this;
	}

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT void set(const std::string& stringInput);
	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT void set(double h, double s, double v);
};
