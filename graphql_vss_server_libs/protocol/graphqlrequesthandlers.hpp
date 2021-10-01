#pragma once

#include <graphqlservice/GraphQLService.h>
#include <boost/asio/steady_timer.hpp>

#include <set>

struct GraphQLNotifyTriggers
{
	GraphQLNotifyTriggers(
		GraphQLNotifyTriggers const&) = delete; // usage bug, copying sets are expensive
	GraphQLNotifyTriggers(GraphQLNotifyTriggers&& other) = delete;

	GraphQLNotifyTriggers(graphql::service::SubscriptionName&& _subscriptionName,
		std::set<graphql::service::SubscriptionKey>&& _subscriptionKeys)
		: subscriptionName(std::move(_subscriptionName))
		, subscriptionKeys(std::move(_subscriptionKeys))
	{
	}

	graphql::service::SubscriptionName subscriptionName; // root subscription field
	std::set<graphql::service::SubscriptionKey> subscriptionKeys;

	void merge(GraphQLNotifyTriggers& other);

	inline bool hasSubscriptionKey(const graphql::service::SubscriptionKey& key) const
	{
		return subscriptionKeys.find(key) != subscriptionKeys.cend();
	}

	std::string toString() const;
};

struct GraphQLRequestHandlers
{
	std::function<void(graphql::response::Value&&)> onReply;
	std::function<void(std::function<void(void)>&&)> defer;
	std::function<void(std::function<void(void)>&&)> offloadWork;
	std::function<std::unique_ptr<boost::asio::steady_timer>(void)> createTimer;
	std::function<void(std::shared_ptr<GraphQLNotifyTriggers>&&)> notify;
	std::function<const GraphQLNotifyTriggers&(void)> currentNotificationTriggers;
	std::function<void(void)> terminate;
};
