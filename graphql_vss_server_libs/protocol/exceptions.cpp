#include "exceptions.hpp"

InvalidPayload::InvalidPayload(const std::string& message)
	: m_message(std::string { messagePrefix } + message)
{
}

const char* InvalidPayload::what() const noexcept
{
	return m_message.c_str();
}

InvalidToken::InvalidToken(const std::string& message)
	: m_message(std::string { messagePrefix } + message)
{
}

const char* InvalidToken::what() const noexcept
{
	return m_message.c_str();
}

const char* ContextException::what() const noexcept
{
	return "Client not authenticated";
}
