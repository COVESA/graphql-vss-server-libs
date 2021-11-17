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

#include <string_view>

extern inline const std::string_view GQL_CONNECTION_INIT = "connection_init";    // Client -> Server
extern inline const std::string_view GQL_CONNECTION_ACK = "connection_ack";        // Server -> Client
extern inline const std::string_view GQL_CONNECTION_ERROR = "connection_error"; // Server -> Client
extern inline const std::string_view GQL_CONNECTION_KEEP_ALIVE = "ka";            // Server -> Client
extern inline const std::string_view GQL_CONNECTION_TERMINATE =
    "connection_terminate";                                        // Client -> Server
extern inline const std::string_view GQL_START = "start";        // Client -> Server
extern inline const std::string_view GQL_DATA = "data";            // Server -> Client
extern inline const std::string_view GQL_ERROR = "error";        // Server -> Client
extern inline const std::string_view GQL_COMPLETE = "complete"; // Server -> Client
extern inline const std::string_view GQL_STOP = "stop";            // Client -> Server

extern inline const std::string_view GQL_TYPE = "type";
extern inline const std::string_view GQL_ID = "id";
extern inline const std::string_view GQL_PAYLOAD = "payload";
extern inline const std::string_view GQL_QUERY = "query";
extern inline const std::string_view GQL_VARIABLES = "variables";
extern inline const std::string_view GQL_OPERATION_NAME = "operationName";
extern inline const std::string_view GQL_MESSAGE = "message";

extern inline const std::string_view GQL_AUTHORIZATION = "authorization";
extern inline const std::string_view GQL_STATUS_CODE = "statusCode";
