#include <sstream>
#include <filesystem>
#include <iostream>
#include <fstream>

#include <graphql_vss_server_libs/support/debug.hpp>

#include "jwtauthorizer.hpp"
#include "exceptions.hpp"

JwtAuthorizer::JwtAuthorizer(JwtAuthorizer::Verifier&& jwtVerifier,
	std::unordered_map<std::string_view, ClientPermissions::Key>&& knownPermissions)
	: Authorizer()
	, m_jwtVerifier(std::move(jwtVerifier))
	, m_knownPermissions(std::move(knownPermissions))
{
}

JwtAuthorizer::Verifier JwtAuthorizer::createDefaultVerifier(const std::string& pubKey)
{
	return jwt::verify().allow_algorithm(jwt::algorithm::rs256(pubKey));
}

JwtAuthorizer::Verifier JwtAuthorizer::createDefaultVerifier()
{
	/*
	 * This is for development purposes only.
	 */
	auto selfPath = std::filesystem::read_symlink("/proc/self/exe");
	auto sname = std::filesystem::path(selfPath).remove_filename() / PUB_KEY_PATH;
	return createDefaultVerifier(sname);
}

JwtAuthorizer::Verifier JwtAuthorizer::createDefaultVerifier(const std::filesystem::path& path)
{
	dbg(COLOR_BG_BLUE << "JwtAuthorizer reading public key from=" << path);
	std::ifstream ifs(path);
	std::string pubKey((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	return createDefaultVerifier(pubKey);
}

const std::shared_ptr<const ClientPermissions> JwtAuthorizer::authorize(std::string&& token)
{
	return authorize(token);
}

const std::shared_ptr<const ClientPermissions> JwtAuthorizer::authorize(const std::string& token)
{
	if (token.empty())
	{
		// We don't want to throw an error if no token was provided, since this can
		// be the case for introspection queries
		dbg(COLOR_BLUE << "JwtAuthorizer: no token to be decoded");
		return m_emptyClientPermissions;
	}

	try
	{
		auto decoded = jwt::decode(token);

		m_jwtVerifier.verify(decoded);

		picojson::value claims;
		picojson::parse(claims, decoded.get_payload());
		if (!claims.is<picojson::object>())
		{
			throw InvalidToken("Token claims are not in a valid format");
		}

		auto permissions = std::make_shared<ClientPermissions>();
		for (auto& e : claims.get<picojson::object>())
		{
			if (e.first == PERMISSIONS_CLAIM)
			{
				dbg(COLOR_BLUE << "JwtAuthorizer: " << e.first << " = " << e.second);
				// TODO: cache claims, the same set will result in the same client permissions
				if (!e.second.is<picojson::array>())
				{
					throw InvalidToken("Token claims permissions is not an array");
				}
				const auto& array = e.second.get<picojson::array>();
				for (const auto& item : array)
				{
					if (item.is<std::string>())
					{
						// TODO: support legacy tokens with string permissions,
						// this will be removed in the future and `knownPermission` may
						// go away with it.
						const auto& permission = item.get<std::string>();
						const auto& itr = m_knownPermissions.find(permission);
						if (itr == m_knownPermissions.cend())
						{
							dbg("Ignored unknown client permission: " << permission);
							continue;
						}
						permissions->insert(itr->second);
					}
					else
					{
						ClientPermissions::Key permission = item.get<double>();
						permissions->insert(permission);
					}
				}
				return permissions;
			}
		}
		throw InvalidToken("Token claims do not contain permissions");
	}
	catch (jwt::error::signature_verification_exception& ex)
	{
		throw InvalidToken(ex.what());
	}
	catch (std::invalid_argument& ex)
	{
		throw InvalidToken(ex.what());
	}
}
