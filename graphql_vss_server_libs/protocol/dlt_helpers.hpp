#pragma once

#include <dlt_types.h>
#include <dlt_user.h>

#ifndef DLT_SIZED_UTF8
static inline DltReturnValue dlt_user_log_write_sized_utf8_string_fallback(DltContextData *log, const char *text, uint16_t length)
{
	std::string buf(text, length);
	return dlt_user_log_write_utf8_string(log, buf.c_str());
}
#define DLT_SIZED_UTF8(TEXT, LEN) \
    (void)dlt_user_log_write_sized_utf8_string_fallback(&log_local, TEXT, LEN)
#endif

#ifndef DLT_SIZED_CSTRING
static inline DltReturnValue dlt_user_log_write_sized_constant_string_fallback(DltContextData *log, const char *text, uint16_t length)
{
	std::string buf(text, length);
	return dlt_user_log_write_constant_string(log, buf.c_str());
}
#define DLT_SIZED_CSTRING(TEXT, LEN) \
    (void)dlt_user_log_write_sized_constant_string_fallback(&log_local, TEXT, LEN)
#endif

#ifndef DLT_SIZED_STRING
static inline DltReturnValue dlt_user_log_write_sized_string_fallback(DltContextData *log, const char *text, uint16_t length)
{
	std::string buf(text, length);
	return dlt_user_log_write_string(log, buf.c_str());
}
#define DLT_SIZED_STRING(TEXT, LEN) \
    (void)dlt_user_log_write_sized_string_fallback(&log_local, TEXT, LEN)
#endif
