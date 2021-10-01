#include <boost/lexical_cast.hpp>

#define SCALARS_HPP_LOCAL_TEMPLATES 1
#include "scalars.hpp"

template <typename T, std::enable_if_t<std::is_arithmetic<T>::value>*>
T stringToNumber(const std::string& str)
{
	bool isNegative = str.at(0) == '-';
	bool isFloating = str.find('.') != std::string::npos;

	if (isFloating)
	{
		return boost::numeric_cast<T>(boost::lexical_cast<double>(str));
	}

	if (isNegative)
	{
		return boost::numeric_cast<T>(boost::lexical_cast<int64_t>(str));
	}

	return boost::numeric_cast<T>(boost::lexical_cast<uint64_t>(str));
}

template <typename TResult, typename TInput,
	std::enable_if_t<std::is_arithmetic<TResult>::value && std::is_arithmetic<TInput>::value>*>
TResult validateRange(TInput value, const struct RangeInterval<TResult>& range)
{
	TResult result = boost::numeric_cast<TResult>(value);

	if (result < range.min)
	{
		throw std::out_of_range("Value lower than minimun range");
	}
	if (result > range.max)
	{
		throw std::out_of_range("Value greater than maximun range");
	}

	return result;
}

template <typename TResult, std::enable_if_t<std::is_arithmetic<TResult>::value>*>
TResult validateRange(const std::string& value, const struct RangeInterval<TResult>& range)
{
	return validateRange(stringToNumber<TResult>(value), range);
}

template <typename TResult, std::enable_if_t<std::is_arithmetic<TResult>::value>*>
TResult validateRange(const response::Value& value, const struct RangeInterval<TResult>& range)
{
	switch (value.type())
	{
		case response::Type::Int:
			return validateRange(boost::numeric_cast<TResult>(value.get<response::IntType>()),
				range);

		case response::Type::Float:
			return validateRange(boost::numeric_cast<TResult>(value.get<response::FloatType>()),
				range);

		case response::Type::String:
			return validateRange(value.get<response::StringType>(), range);

		default:
			throw std::domain_error("Unsupported type (not Int, Float or String)");
	}
}

template <typename TResult, typename TValue, std::enable_if_t<std::is_arithmetic<TResult>::value>*>
std::optional<TResult>
validateRange(const std::optional<TValue>& value, const struct RangeInterval<TResult>& range)
{
	if (!value)
	{
		return std::nullopt;
	}

	return validateRange<TResult>(value.value(), range);
}

template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int64_t stringToNumber<int64_t>(const std::string& str);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint64_t stringToNumber<uint64_t>(const std::string& str);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int32_t stringToNumber<int32_t>(const std::string& str);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint32_t stringToNumber<uint32_t>(const std::string& str);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int16_t stringToNumber<int16_t>(const std::string& str);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint16_t stringToNumber<uint16_t>(const std::string& str);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int8_t stringToNumber<int8_t>(const std::string& str);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint8_t stringToNumber<uint8_t>(const std::string& str);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT double stringToNumber<double>(const std::string& str);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT float stringToNumber<float>(const std::string& str);

template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int64_t validateRange<int64_t>(
	int64_t value, const struct RangeInterval<int64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint64_t validateRange<uint64_t>(
	uint64_t value, const struct RangeInterval<uint64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int32_t validateRange<int32_t>(
	int32_t value, const struct RangeInterval<int32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint32_t validateRange<uint32_t>(
	uint32_t value, const struct RangeInterval<uint32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int16_t validateRange<int16_t>(
	int16_t value, const struct RangeInterval<int16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint16_t validateRange<uint16_t>(
	uint16_t value, const struct RangeInterval<uint16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int8_t validateRange<int8_t>(
	int8_t value, const struct RangeInterval<int8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint8_t validateRange<uint8_t>(
	uint8_t value, const struct RangeInterval<uint8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT double
validateRange<double>(double value, const struct RangeInterval<double>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT float
validateRange<float>(float value, const struct RangeInterval<float>& range);

// more for testing, shouldn't exist in real code
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint64_t validateRange<uint64_t, int>(
	int value, const struct RangeInterval<uint64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint32_t validateRange<uint32_t, int>(
	int value, const struct RangeInterval<uint32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint16_t validateRange<uint16_t, int>(
	int value, const struct RangeInterval<uint16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint8_t validateRange<uint8_t, int>(
	int value, const struct RangeInterval<uint8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int64_t validateRange<int64_t, int>(
	int value, const struct RangeInterval<int64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int16_t validateRange<int16_t, int>(
	int value, const struct RangeInterval<int16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int8_t validateRange<int8_t, int>(
	int value, const struct RangeInterval<int8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint64_t validateRange<uint64_t, double>(
	double value, const struct RangeInterval<uint64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint32_t validateRange<uint32_t, double>(
	double value, const struct RangeInterval<uint32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint16_t validateRange<uint16_t, double>(
	double value, const struct RangeInterval<uint16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint8_t validateRange<uint8_t, double>(
	double value, const struct RangeInterval<uint8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int64_t validateRange<int64_t, double>(
	double value, const struct RangeInterval<int64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int32_t validateRange<int32_t, double>(
	double value, const struct RangeInterval<int32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int16_t validateRange<int16_t, double>(
	double value, const struct RangeInterval<int16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int8_t validateRange<int8_t, double>(
	double value, const struct RangeInterval<int8_t>& range);

template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int64_t validateRange<int64_t>(
	const std::string& value, const struct RangeInterval<int64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint64_t validateRange<uint64_t>(
	const std::string& value, const struct RangeInterval<uint64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int32_t validateRange<int32_t>(
	const std::string& value, const struct RangeInterval<int32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint32_t validateRange<uint32_t>(
	const std::string& value, const struct RangeInterval<uint32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int16_t validateRange<int16_t>(
	const std::string& value, const struct RangeInterval<int16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint16_t validateRange<uint16_t>(
	const std::string& value, const struct RangeInterval<uint16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int8_t validateRange<int8_t>(
	const std::string& value, const struct RangeInterval<int8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint8_t validateRange<uint8_t>(
	const std::string& value, const struct RangeInterval<uint8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT double
validateRange<double>(const std::string& value, const struct RangeInterval<double>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT float
validateRange<float>(const std::string& value, const struct RangeInterval<float>& range);

template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int64_t validateRange<int64_t>(
	const response::Value& value, const struct RangeInterval<int64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint64_t validateRange<uint64_t>(
	const response::Value& value, const struct RangeInterval<uint64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int32_t validateRange<int32_t>(
	const response::Value& value, const struct RangeInterval<int32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint32_t validateRange<uint32_t>(
	const response::Value& value, const struct RangeInterval<uint32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int16_t validateRange<int16_t>(
	const response::Value& value, const struct RangeInterval<int16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint16_t validateRange<uint16_t>(
	const response::Value& value, const struct RangeInterval<uint16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT int8_t validateRange<int8_t>(
	const response::Value& value, const struct RangeInterval<int8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT uint8_t validateRange<uint8_t>(
	const response::Value& value, const struct RangeInterval<uint8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT double
validateRange<double>(const response::Value& value, const struct RangeInterval<double>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT float
validateRange<float>(const response::Value& value, const struct RangeInterval<float>& range);

template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<int64_t> validateRange<int64_t, int64_t>(
	const std::optional<int64_t>& value, const struct RangeInterval<int64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<uint64_t> validateRange<uint64_t, uint64_t>(
	const std::optional<uint64_t>& value, const struct RangeInterval<uint64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<int32_t> validateRange<int32_t, int32_t>(
	const std::optional<int32_t>& value, const struct RangeInterval<int32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<uint32_t> validateRange<uint32_t, uint32_t>(
	const std::optional<uint32_t>& value, const struct RangeInterval<uint32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<int16_t> validateRange<int16_t, int16_t>(
	const std::optional<int16_t>& value, const struct RangeInterval<int16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<uint16_t> validateRange<uint16_t, uint16_t>(
	const std::optional<uint16_t>& value, const struct RangeInterval<uint16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<int8_t> validateRange<int8_t, int8_t>(
	const std::optional<int8_t>& value, const struct RangeInterval<int8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<uint8_t> validateRange<uint8_t, uint8_t>(
	const std::optional<uint8_t>& value, const struct RangeInterval<uint8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<double> validateRange<double, double>(
	const std::optional<double>& value, const struct RangeInterval<double>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<float> validateRange<float, float>(
	const std::optional<float>& value, const struct RangeInterval<float>& range);

template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<int64_t>
validateRange<int64_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<int64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<uint64_t>
validateRange<uint64_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<uint64_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<int32_t>
validateRange<int32_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<int32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<uint32_t>
validateRange<uint32_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<uint32_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<int16_t>
validateRange<int16_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<int16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<uint16_t>
validateRange<uint16_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<uint16_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<int8_t>
validateRange<int8_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<int8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<uint8_t>
validateRange<uint8_t, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<uint8_t>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<double>
validateRange<double, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<double>& range);
template GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::optional<float> validateRange<float, response::Value>(
	const std::optional<response::Value>& value, const struct RangeInterval<float>& range);

ColorInputStringException::ColorInputStringException()
{
}

const char* ColorInputStringException::what() const noexcept
{
	return "Color HSV should be passed as string 'hsv(<hue>, <saturation>%, <value>%)'"
		   "The value ranges are: hue [0.0, 360.0]; saturation [0.0, 100.0]; value [0.0, "
		   "100.0]";
}

ColorInputRangeException::ColorInputRangeException()
{
}

const char* ColorInputRangeException::what() const noexcept
{
	return "A given value is out of range!"
		   "The value ranges are: hue [0.0, 360.0]; saturation [0.0, 100.0]; value [0.0, "
		   "100.0]";
}

HSVColor::HSVColor(const response::Value& value_)
{

	if (value_.type() == response::Type::String)
	{
		set(value_.get<response::StringType>());
	}
	else
	{
		throw ColorInputStringException();
	}
}

HSVColor::HSVColor(const std::string& stringInput)
{
	set(stringInput);
};

HSVColor::HSVColor(double h, double s, double v)
{
	set(h, s, v);
}

std::string HSVColor::str()
{
	char buffer[30];
	// Check if precision is ok
	snprintf(buffer, sizeof(buffer), "hsv(%.1f, %.1f%%, %.1f%%)", hue, saturation, value);
	return std::string(buffer);
}

void HSVColor::set(const std::string& stringInput)
{
	double h, s, v;
	int used;
	int n =
		sscanf(stringInput.c_str(), "hsv(%lf%*[ ,] %lf%*[ %%], %lf%*[ %%])%n", &h, &s, &v, &used);
	if (n == 3 && used == stringInput.size())
	{
		set(h, s, v);
	}
	else
	{
		throw ColorInputStringException();
	}
}

void HSVColor::set(double h, double s, double v)
{
	try
	{
		hue = validateRange<double>(h, { 0, 360 });
		saturation = validateRange<double>(s, { 0, 100 });
		value = validateRange<double>(v, { 0, 100 });
	}
	catch (const std::out_of_range& e)
	{
		throw ColorInputRangeException();
	}
}
