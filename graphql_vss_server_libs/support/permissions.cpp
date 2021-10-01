#include "debug.hpp"
#include "permissions.hpp"

const char* PermissionException::what() const noexcept
{
	return "Client doesn't have all needed permissions";
}

void ClientPermissions::insert(const ClientPermissions::Key& permission)
{
	m_lookup.insert(permission);
}
