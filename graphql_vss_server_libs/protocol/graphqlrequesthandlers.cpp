// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gustavo Sverzut Barbieri (barbieri@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

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
