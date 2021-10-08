// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gabriel Fernandes (g7fernandes@profusion.mobi)
//   Author: Leandro Ferlin (leandroferlin@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>
#include <dlt_types.h>
#include <dlt_user.h>

#ifndef DLT_SIZED_UTF8
[[maybe_unused]] static inline DltReturnValue dlt_user_log_write_sized_utf8_string_fallback(DltContextData *log, const char *text, uint16_t length)
{
	std::string buf(text, length);
	return dlt_user_log_write_utf8_string(log, buf.c_str());
}
#define DLT_SIZED_UTF8(TEXT, LEN) \
    (void)dlt_user_log_write_sized_utf8_string_fallback(&log_local, TEXT, LEN)
#endif

#ifndef DLT_SIZED_CSTRING
[[maybe_unused]] static inline DltReturnValue dlt_user_log_write_sized_constant_string_fallback(DltContextData *log, const char *text, uint16_t length)
{
	std::string buf(text, length);
	return dlt_user_log_write_constant_string(log, buf.c_str());
}
#define DLT_SIZED_CSTRING(TEXT, LEN) \
    (void)dlt_user_log_write_sized_constant_string_fallback(&log_local, TEXT, LEN)
#endif

#ifndef DLT_SIZED_STRING
[[maybe_unused]] static inline DltReturnValue dlt_user_log_write_sized_string_fallback(DltContextData *log, const char *text, uint16_t length)
{
	std::string buf(text, length);
	return dlt_user_log_write_string(log, buf.c_str());
}
#define DLT_SIZED_STRING(TEXT, LEN) \
    (void)dlt_user_log_write_sized_string_fallback(&log_local, TEXT, LEN)
#endif
