// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gustavo Sverzut Barbieri (barbieri@profusion.mobi),
//   Author: Gabriel Fernandes (g7fernandes@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#include <graphqlservice/GraphQLService.h>
#include <graphqlservice/internal/Grammar.h>

// this is a copy of the class SubscriptionDefinitionVisitor and its dependency
// FragmentDefinitionVisitor from
// https://github.com/microsoft/cppgraphqlgen/blob/main/src/GraphQLService.cpp
// (licensed under MIT) trimmed down to the essential: discover the root field name
//
// This is required because when we deliver(), we need that name and we want to do an initial
// deliver for the subscribed operation, then we need that string from the input query and this is
// not available
//
// NOTE: since the subscription was previously validated by cppgraphqlgen, the checks were removed

namespace graphql {
using namespace graphql::service;

class FragmentDefinitionVisitor
{
public:
	FragmentDefinitionVisitor(const response::Value& variables);

	FragmentMap getFragments();

	void visit(const peg::ast_node& fragmentDefinition);

private:
	const response::Value& _variables;

	FragmentMap _fragments;
};

FragmentDefinitionVisitor::FragmentDefinitionVisitor(const response::Value& variables)
	: _variables(variables)
{
}

FragmentMap FragmentDefinitionVisitor::getFragments()
{
	FragmentMap result(std::move(_fragments));
	return result;
}

void FragmentDefinitionVisitor::visit(const peg::ast_node& fragmentDefinition)
{
	_fragments.emplace(fragmentDefinition.children.front()->string_view(),
		Fragment(fragmentDefinition, _variables));
}

class SubscriptionDefinitionNameVisitor
{
public:
	SubscriptionDefinitionNameVisitor(FragmentMap&& fragments);

	std::string getName();

	void visit(const peg::ast_node& operationDefinition);

private:
	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	SubscriptionName _field;
	FragmentMap _fragments;
};

SubscriptionDefinitionNameVisitor::SubscriptionDefinitionNameVisitor(FragmentMap&& fragments)
	: _fragments(std::move(fragments))
{
}

void SubscriptionDefinitionNameVisitor::visit(const peg::ast_node& operationDefinition)
{
	const auto& selection = *operationDefinition.children.back();

	for (const auto& child : selection.children)
	{
		if (child->is_type<peg::field>())
		{
			visitField(*child);
		}
		else if (child->is_type<peg::fragment_spread>())
		{
			visitFragmentSpread(*child);
		}
		else if (child->is_type<peg::inline_fragment>())
		{
			visitInlineFragment(*child);
		}
	}
}

std::string SubscriptionDefinitionNameVisitor::getName()
{
	return std::move(_field);
}

void SubscriptionDefinitionNameVisitor::visitField(const peg::ast_node& field)
{
	peg::on_first_child<peg::field_name>(field, [this](const peg::ast_node& child) {
		_field = child.string_view();
	});
}

void SubscriptionDefinitionNameVisitor::visitFragmentSpread(const peg::ast_node& fragmentSpread)
{
	const auto name = fragmentSpread.children.front()->string_view();
	auto itr = _fragments.find(name);

	for (const auto& selection : itr->second.getSelection().children)
	{
		visit(*selection);
	}
}

void SubscriptionDefinitionNameVisitor::visitInlineFragment(const peg::ast_node& inlineFragment)
{
	peg::on_first_child<peg::selection_set>(inlineFragment, [this](const peg::ast_node& child) {
		for (const auto& selection : child.children)
		{
			visit(*selection);
		}
	});
}

} // namespace graphql
