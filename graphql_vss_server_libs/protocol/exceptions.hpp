#pragma once

#include <exception>
#include <string_view>
#include <string>

#include "graphql_vss_server_libs-protocol_export.h"

class GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT InvalidPayload : public std::exception
{
	const std::string m_message;

public:
	GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT InvalidPayload(const std::string& exceptionMessage);
	const char* what() const noexcept override;

	constexpr static std::string_view messagePrefix = "Invalid payload: ";
};

class GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT InvalidToken : public std::exception
{
	const std::string m_message;

public:
	GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT InvalidToken(const std::string& exceptionMessage);
	const char* what() const noexcept override;

	constexpr static std::string_view messagePrefix = "Token error: ";
};

class GRAPHQL_VSS_SERVER_LIBS_PROTOCOL_EXPORT ContextException : public std::exception
{
public:
	const char* what() const noexcept override;
};
