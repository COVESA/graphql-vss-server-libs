#include "commonapi-singletons.hpp"

#ifndef GRAPHQL_SOMEIP_NAME
#define GRAPHQL_SOMEIP_NAME "graphql"
#endif

std::shared_ptr<CommonAPI::Runtime> commonAPISingletonProxyRuntime = CommonAPI::Runtime::get();
std::string commonAPISingletonProxyConnectionId(GRAPHQL_SOMEIP_NAME);
std::string commonAPISingletonProxyDomain("local");
