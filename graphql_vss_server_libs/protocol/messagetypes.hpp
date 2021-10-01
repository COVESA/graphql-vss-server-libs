#pragma once

#include <string_view>

extern inline const std::string_view GQL_CONNECTION_INIT = "connection_init";	// Client -> Server
extern inline const std::string_view GQL_CONNECTION_ACK = "connection_ack";		// Server -> Client
extern inline const std::string_view GQL_CONNECTION_ERROR = "connection_error"; // Server -> Client
extern inline const std::string_view GQL_CONNECTION_KEEP_ALIVE = "ka";			// Server -> Client
extern inline const std::string_view GQL_CONNECTION_TERMINATE =
	"connection_terminate";										// Client -> Server
extern inline const std::string_view GQL_START = "start";		// Client -> Server
extern inline const std::string_view GQL_DATA = "data";			// Server -> Client
extern inline const std::string_view GQL_ERROR = "error";		// Server -> Client
extern inline const std::string_view GQL_COMPLETE = "complete"; // Server -> Client
extern inline const std::string_view GQL_STOP = "stop";			// Client -> Server

extern inline const std::string_view GQL_TYPE = "type";
extern inline const std::string_view GQL_ID = "id";
extern inline const std::string_view GQL_PAYLOAD = "payload";
extern inline const std::string_view GQL_QUERY = "query";
extern inline const std::string_view GQL_VARIABLES = "variables";
extern inline const std::string_view GQL_OPERATION_NAME = "operationName";
extern inline const std::string_view GQL_MESSAGE = "message";

extern inline const std::string_view GQL_AUTHORIZATION = "authorization";
extern inline const std::string_view GQL_STATUS_CODE = "statusCode";
