#ifndef __ANTHEM__VERIFY_PROPERTIES__HEAD_H
#define __ANTHEM__VERIFY_PROPERTIES__HEAD_H

#include <algorithm>
#include <optional>

#include <anthem/AST.h>
#include <anthem/Exception.h>
#include <anthem/RuleContext.h>
#include <anthem/Utils.h>
#include <anthem/translation-common/ChooseValueInTerm.h>

namespace anthem
{
namespace verifyProperties
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Head
//
////////////////////////////////////////////////////////////////////////////////////////////////////

enum class HeadType
{
	SingleAtom,
	ChoiceSingleAtom,
	IntegrityConstraint,
	Fact,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadAtom
{
	ast::PredicateDeclaration *predicateDeclaration;
	const std::vector<Clingo::AST::Term> &arguments;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadTranslationResult
{
	HeadType headType;
	std::optional<HeadAtom> headAtom;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

inline HeadAtom makeHeadAtom(const Clingo::AST::Function &function, Context &context)
{
	auto predicateDeclaration = context.findOrCreatePredicateDeclaration(function.name, function.arguments.size());
	predicateDeclaration->isUsed = true;

	return HeadAtom{predicateDeclaration, function.arguments};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct HeadLiteralVisitor
{
	HeadTranslationResult visit(const Clingo::AST::Aggregate &aggregate, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		if (aggregate.left_guard || aggregate.right_guard)
			throw TranslationException(headLiteral.location, "aggregates with left or right guards not yet supported in rule head");

		if (aggregate.elements.size() != 1)
			throw TranslationException("aggregates with more than one element not yet supported in rule head");

		if (!aggregate.elements[0].condition.empty())
			throw TranslationException(headLiteral.location, "conditional literals not yet supported in rule head");

		if (aggregate.elements[0].literal.sign != Clingo::AST::Sign::None)
			throw TranslationException(headLiteral.location, "negated literals in aggregates not yet supported in rule head");

		const auto &literal = aggregate.elements[0].literal;

		if (!literal.data.is<Clingo::AST::Term>())
			throw TranslationException(headLiteral.location, "only terms currently supported in aggregates in rule head");

		const auto &term = literal.data.get<Clingo::AST::Term>();

		if (!term.data.is<Clingo::AST::Function>())
			throw TranslationException(headLiteral.location, "only atoms currently supported in aggregates in rule head");

		const auto &function = term.data.get<Clingo::AST::Function>();

		return HeadTranslationResult{HeadType::ChoiceSingleAtom, makeHeadAtom(function, context)};
	}

	HeadTranslationResult visit(const Clingo::AST::Literal &literal, const Clingo::AST::HeadLiteral &headLiteral, Context &context)
	{
		if (literal.sign != Clingo::AST::Sign::None)
			throw TranslationException(literal.location, "negated head literals not yet supported in rule head");

		if (literal.data.is<Clingo::AST::Boolean>())
		{
			if (literal.data.get<Clingo::AST::Boolean>().value == true)
				return HeadTranslationResult{HeadType::Fact, std::nullopt};

			return HeadTranslationResult{HeadType::IntegrityConstraint, std::nullopt};
		}

		if (!literal.data.is<Clingo::AST::Term>())
			throw TranslationException(headLiteral.location, "only terms currently supported in literals in rule head");

		const auto &term = literal.data.get<Clingo::AST::Term>();

		if (!term.data.is<Clingo::AST::Function>())
			throw TranslationException(headLiteral.location, "only atoms currently supported in literals in rule head");

		const auto &function = term.data.get<Clingo::AST::Function>();

		return HeadTranslationResult{HeadType::SingleAtom, makeHeadAtom(function, context)};
	}

	template<class T>
	HeadTranslationResult visit(const T &, const Clingo::AST::HeadLiteral &headLiteral, Context &)
	{
		throw TranslationException(headLiteral.location, "head literal not yet supported in rule head, expected literal or aggregate");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif