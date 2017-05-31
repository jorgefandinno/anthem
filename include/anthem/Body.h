#ifndef __ANTHEM__BODY_H
#define __ANTHEM__BODY_H

#include <algorithm>

#include <anthem/AST.h>
#include <anthem/Term.h>
#include <anthem/Utils.h>
#include <anthem/output/Formatting.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Body
//
////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Comparison::Operator translate(Clingo::AST::ComparisonOperator comparisonOperator)
{
	switch (comparisonOperator)
	{
		case Clingo::AST::ComparisonOperator::GreaterThan:
			return ast::Comparison::Operator::GreaterThan;
		case Clingo::AST::ComparisonOperator::LessThan:
			return ast::Comparison::Operator::LessThan;
		case Clingo::AST::ComparisonOperator::LessEqual:
			return ast::Comparison::Operator::LessEqual;
		case Clingo::AST::ComparisonOperator::GreaterEqual:
			return ast::Comparison::Operator::GreaterEqual;
		case Clingo::AST::ComparisonOperator::NotEqual:
			return ast::Comparison::Operator::NotEqual;
		case Clingo::AST::ComparisonOperator::Equal:
			return ast::Comparison::Operator::Equal;
	}

	return ast::Comparison::Operator::NotEqual;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyTermTranslateVisitor
{
	// TODO: refactor
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Function &function, const Clingo::AST::Literal &literal, const Clingo::AST::Term &, RuleContext &ruleContext, ast::VariableStack &variableStack)
	{
		if (literal.sign == Clingo::AST::Sign::DoubleNegation)
			throw TranslationException(literal.location, "double-negated literals currently unsupported");

		if (function.arguments.empty())
		{
			auto predicate = ast::Formula::make<ast::Predicate>(std::string(function.name));

			if (literal.sign == Clingo::AST::Sign::None)
				return std::move(predicate);
			else if (literal.sign == Clingo::AST::Sign::Negation)
				return ast::Formula::make<ast::Not>(std::move(predicate));
		}

		// Create new body variable declarations
		std::vector<std::unique_ptr<ast::VariableDeclaration>> parameters;
		parameters.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));

		// TODO: implement pushing with scoped guards to avoid bugs
		variableStack.push(&parameters);

		ast::And conjunction;

		for (size_t i = 0; i < function.arguments.size(); i++)
		{
			auto &argument = function.arguments[i];
			conjunction.arguments.emplace_back(ast::Formula::make<ast::In>(ast::Variable(parameters[i].get()), translate(argument, ruleContext, variableStack)));
		}

		variableStack.pop();

		ast::Predicate predicate(std::string(function.name));
		predicate.arguments.reserve(function.arguments.size());

		for (size_t i = 0; i < function.arguments.size(); i++)
			predicate.arguments.emplace_back(ast::Variable(parameters[i].get()));

		if (literal.sign == Clingo::AST::Sign::None)
			conjunction.arguments.emplace_back(std::move(predicate));
		else if (literal.sign == Clingo::AST::Sign::Negation)
			conjunction.arguments.emplace_back(ast::Formula::make<ast::Not>(std::move(predicate)));

		return ast::Formula::make<ast::Exists>(std::move(parameters), std::move(conjunction));
	}

	template<class T>
	std::experimental::optional<ast::Formula> visit(const T &, const Clingo::AST::Literal &, const Clingo::AST::Term &term, RuleContext &, ast::VariableStack &)
	{
		assert(!term.data.is<Clingo::AST::Function>());

		throw TranslationException(term.location, "term currently unsupported in this context, expected function");
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyLiteralTranslateVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Boolean &boolean, const Clingo::AST::Literal &, RuleContext &, ast::VariableStack &)
	{
		return ast::Formula::make<ast::Boolean>(boolean.value);
	}

	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Term &term, const Clingo::AST::Literal &literal, RuleContext &ruleContext, ast::VariableStack &variableStack)
	{
		return term.data.accept(BodyTermTranslateVisitor(), literal, term, ruleContext, variableStack);
	}

	// TODO: refactor
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Comparison &comparison, const Clingo::AST::Literal &literal, RuleContext &ruleContext, ast::VariableStack &variableStack)
	{
		// Comparisons should never have a sign, because these are converted to positive comparisons by clingo
		if (literal.sign != Clingo::AST::Sign::None)
			throw TranslationException(literal.location, "negated comparisons currently unsupported");

		const auto operator_ = translate(comparison.comparison);

		std::vector<std::unique_ptr<ast::VariableDeclaration>> parameters;
		parameters.reserve(2);
		parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));
		parameters.emplace_back(std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::Body));

		ast::And conjunction;
		conjunction.arguments.reserve(3);
		conjunction.arguments.emplace_back(ast::Formula::make<ast::In>(ast::Variable(parameters[0].get()), translate(comparison.left, ruleContext, variableStack)));
		conjunction.arguments.emplace_back(ast::Formula::make<ast::In>(ast::Variable(parameters[1].get()), translate(comparison.right, ruleContext, variableStack)));
		conjunction.arguments.emplace_back(ast::Formula::make<ast::Comparison>(operator_, ast::Variable(parameters[0].get()), ast::Variable(parameters[1].get())));

		return ast::Formula::make<ast::Exists>(std::move(parameters), std::move(conjunction));
	}

	template<class T>
	std::experimental::optional<ast::Formula> visit(const T &, const Clingo::AST::Literal &literal, RuleContext &, ast::VariableStack &)
	{
		throw TranslationException(literal.location, "literal currently unsupported in this context, expected function or term");
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct BodyBodyLiteralTranslateVisitor
{
	std::experimental::optional<ast::Formula> visit(const Clingo::AST::Literal &literal, const Clingo::AST::BodyLiteral &bodyLiteral, RuleContext &ruleContext, ast::VariableStack &variableStack)
	{
		if (bodyLiteral.sign != Clingo::AST::Sign::None)
			throw TranslationException(bodyLiteral.location, "only positive body literals supported currently");

		return literal.data.accept(BodyLiteralTranslateVisitor(), literal, ruleContext, variableStack);
	}

	template<class T>
	std::experimental::optional<ast::Formula> visit(const T &, const Clingo::AST::BodyLiteral &bodyLiteral, RuleContext &, ast::VariableStack &)
	{
		throw TranslationException(bodyLiteral.location, "body literal currently unsupported in this context, expected literal");
		return std::experimental::nullopt;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
