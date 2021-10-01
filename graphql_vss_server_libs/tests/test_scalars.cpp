#include <iostream>
#include <gtest/gtest.h>
#include <boost/lexical_cast/bad_lexical_cast.hpp>
#include <optional>

#include <stdexcept>
#include <graphql_vss_server_libs/support/scalars.hpp>

#define TEST_COLOR(STR, H, S, V)                                                                   \
	{                                                                                              \
		try                                                                                        \
		{                                                                                          \
			color = HSVColor(STR);                                                                 \
		}                                                                                          \
		catch (const std::exception& ex)                                                           \
		{                                                                                          \
			FAIL() << STR << ": " << ex.what();                                                    \
		}                                                                                          \
		EXPECT_DOUBLE_EQ(color.hue, H);                                                            \
		EXPECT_DOUBLE_EQ(color.saturation, S);                                                     \
		EXPECT_DOUBLE_EQ(color.value, V);                                                          \
	}

const std::string UINT64_MIN_VALUE_STR = "-9223372036854775808";
const std::string UINT64_MAX_VALUE_STR = "18446744073709551615";
const std::string UINT64_INVALID_MIN_VALUE_STR = "-9223372036854775809";
const std::string UINT64_INVALID_MAX_VALUE_STR = "18446744073709551616";

TEST(StringToNumber, CastingTest)
{
	EXPECT_EQ(stringToNumber<int32_t>("0"), 0);
	EXPECT_EQ(stringToNumber<float>("0.5"), 0.5);
}

TEST(StringToNumber, LimitsTest)
{
	EXPECT_NO_THROW(stringToNumber<int64_t>(UINT64_MIN_VALUE_STR));
	EXPECT_NO_THROW(stringToNumber<uint64_t>(UINT64_MAX_VALUE_STR));
	EXPECT_THROW(stringToNumber<int64_t>(UINT64_INVALID_MIN_VALUE_STR), boost::bad_lexical_cast);
	EXPECT_THROW(stringToNumber<uint64_t>(UINT64_INVALID_MAX_VALUE_STR), boost::bad_lexical_cast);
}

TEST(ValidateRange, RangeTest)
{
	EXPECT_NO_THROW(validateRange<int32_t>(0, {}));
	EXPECT_NO_THROW(validateRange<int32_t>(0, { 0, 0 }));
	EXPECT_NO_THROW(validateRange<int32_t>(0, { -1, 1 }));

	EXPECT_THROW(validateRange<int32_t>(0, { 1, 1 }), std::out_of_range);
	EXPECT_THROW(validateRange<int32_t>(1, { 0, 0 }), std::out_of_range);
	EXPECT_THROW(validateRange<int32_t>(0, { 1, 0 }), std::out_of_range);
}

TEST(ValidateRange, CastingTest)
{
	EXPECT_THROW(validateRange<uint8_t>(-1, {}), boost::numeric::bad_numeric_cast);
	EXPECT_THROW(validateRange<uint16_t>(-1, {}), boost::numeric::bad_numeric_cast);
	EXPECT_THROW(validateRange<uint32_t>(-1, {}), boost::numeric::bad_numeric_cast);
	EXPECT_THROW(validateRange<uint64_t>(-1, {}), boost::numeric::bad_numeric_cast);
}

TEST(ValidateRange, Int8CastingTest)
{
	EXPECT_NO_THROW(validateRange<int8_t>(0, {}));
	EXPECT_NO_THROW(validateRange<int8_t>(0.1, {}));
	EXPECT_NO_THROW(validateRange<int8_t>("0", {}));

	response::Value integerValue = response::Value(0);
	response::Value floatingValue = response::Value(0.1);
	response::Value stringValue = response::Value("0");
	EXPECT_NO_THROW(validateRange<int8_t>(integerValue, {}));
	EXPECT_NO_THROW(validateRange<int8_t>(floatingValue, {}));
	EXPECT_NO_THROW(validateRange<int8_t>(stringValue, {}));
}

TEST(ValidateRange, OptionalValueTest)
{
	std::optional<int32_t> optionalInt;
	EXPECT_FALSE(validateRange<int32_t>(optionalInt, {}).has_value());
	optionalInt = 1;
	EXPECT_TRUE(validateRange<int32_t>(optionalInt, {}).has_value());
}

TEST(HSVColor, StringInputTest)
{
	ASSERT_THROW([[maybe_unused]] auto _ = HSVColor("hsv(, 2%, 3% )"), ColorInputStringException);
	ASSERT_THROW([[maybe_unused]] auto _ = HSVColor("hsv(3%)"), ColorInputStringException);
	ASSERT_THROW([[maybe_unused]] auto _ = HSVColor("hsv()"), ColorInputStringException);
	ASSERT_THROW([[maybe_unused]] auto _ = HSVColor("hsv(1, 2g%, 3%)"), ColorInputStringException);
	EXPECT_THROW([[maybe_unused]] auto _ = HSVColor("(1,2%,3%)"), ColorInputStringException);
	EXPECT_THROW([[maybe_unused]] auto _ = HSVColor("hsv( 100 , .2, 3)"), ColorInputStringException);
	EXPECT_THROW([[maybe_unused]] auto _ = HSVColor("hsv( 52 , 100.2%, 3%)"), ColorInputRangeException);

	HSVColor color;

	auto response = graphql::response::Value("hsv( 1.1, 2%, 3%)");
	try
	{
		color = HSVColor(response);
	}
	catch (const std::exception& ex)
	{
		FAIL() << "hsv( 1.1, 2%, 3%)"
			   << ": " << ex.what();
	}
	EXPECT_DOUBLE_EQ(color.hue, 1.1);
	EXPECT_DOUBLE_EQ(color.saturation, 2);
	EXPECT_DOUBLE_EQ(color.value, 3);

	TEST_COLOR("hsv(  1 ,  2%, 3 % )", 1, 2, 3);
	TEST_COLOR("hsv(  1 ,  2%, 3 % )", 1, 2, 3);
	TEST_COLOR("hsv(  1 ,  2%, 3 % )", 1, 2 , 3); // space tolerance
	TEST_COLOR("hsv(1.1, 2%, 3%)", 1.1, 2, 3); // floating/double tolerance
	TEST_COLOR("hsv(1,2%,3%)", 1, 2, 3); // lack of space tolerance
	TEST_COLOR("hsv( 100 , .2%, 3%)", 100, 0.2, 3); // dot ahead tolerance
	auto stringValue = response::Value(color);
	EXPECT_EQ(stringValue.get<response::StringType>(), "hsv(100.0, 0.2%, 3.0%)");
	color = std::string("hsv(12.0, 12%, 12.0%)");
	stringValue = response::Value(color);
	EXPECT_EQ(stringValue.get<response::StringType>(), "hsv(12.0, 12.0%, 12.0%)");
}

int main(int argc, char** argv)
{
	std::cerr << std::boolalpha;
	::testing::InitGoogleTest(&argc, argv);
	std::cout << "Test scalars.hpp file (main thread)" << std::endl;
	return RUN_ALL_TESTS();
}
