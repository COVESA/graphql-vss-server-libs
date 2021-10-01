#include <gtest/gtest.h>

#include <graphql_vss_server_libs/support/permissions.hpp>

static void setupPerms(ClientPermissions& perms)
{
	perms.insert(111);
	perms.insert(2222);
	perms.insert(1234);
}

TEST(test_permissions, single)
{
	ClientPermissions perms;
	setupPerms(perms);
	perms.validate(111);
	EXPECT_THROW(perms.validate(42), PermissionException);
}

TEST(test_permissions, multiple)
{
	ClientPermissions perms;
	setupPerms(perms);
	perms.validate(111, 2222, 1234);
	EXPECT_THROW(perms.validate(111, 42), PermissionException);
}

int main(int argc, char** argv)
{
	std::cerr << std::boolalpha;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
