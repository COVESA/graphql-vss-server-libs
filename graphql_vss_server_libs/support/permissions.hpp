#pragma once

#include <memory>
#include <set>

#include "graphql_vss_server_libs-support_export.h"

struct GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT PermissionException : public std::exception
{
	const char* what() const noexcept override;
};

class ClientPermissions
{
public:
	using Key = uint16_t;

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT void insert(const Key& permission);

	ClientPermissions() = default;
	ClientPermissions(ClientPermissions const&) = delete;
	ClientPermissions(ClientPermissions&&) = delete;

	inline bool contains(const Key& permission) const noexcept
	{
		return m_lookup.find(permission) != m_lookup.cend();
	}

	template <typename... T>
	inline void validate(const Key& permission, const T&... rest) const
	{
		if (!contains(permission))
		{
			throw PermissionException();
		}
		if constexpr (sizeof...(rest) > 0)
		{
			validate(rest...);
		}
	}

private:
	// How the Request stores the client permissions,
	// must have an efficient lookup!
	std::set<Key> m_lookup;
};
