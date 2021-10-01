#include "graphqlrequesthandlers.hpp"

void GraphQLNotifyTriggers::merge(GraphQLNotifyTriggers& other)
{
	subscriptionKeys.merge(other.subscriptionKeys);
}

std::string GraphQLNotifyTriggers::toString() const
{
	std::ostringstream buf;
	buf << "{";

	if (subscriptionKeys.size())
	{
		buf << "subscriptionKeys=[";
		bool isFirst = true;
		for (const auto& k : subscriptionKeys)
		{
			if (isFirst)
				isFirst = false;
			else
				buf << " ";
			buf << k;
		}
		buf << "]";
	}

	buf << "}";
	return buf.str();
}
