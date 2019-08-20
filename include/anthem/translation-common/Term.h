#ifndef __ANTHEM__TRANSLATION_COMMON__TERM_H
#define __ANTHEM__TRANSLATION_COMMON__TERM_H

#include <algorithm>

#include <anthem/AST.h>
#include <anthem/ASTUtils.h>
#include <anthem/Exception.h>
#include <anthem/RuleContext.h>
#include <anthem/Utils.h>

namespace anthem
{
namespace translationCommon
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Term
//
////////////////////////////////////////////////////////////////////////////////////////////////////

inline ast::BinaryOperation::Operator translate(Clingo::AST::BinaryOperator binaryOperator)
{
	switch (binaryOperator)
	{
		case Clingo::AST::BinaryOperator::XOr:
			throw TranslationException("binary operation “xor” not yet supported");
		case Clingo::AST::BinaryOperator::Or:
			throw TranslationException("binary operation “or” not yet supported");
		case Clingo::AST::BinaryOperator::And:
			throw TranslationException("binary operation “and” not yet supported");
		case Clingo::AST::BinaryOperator::Plus:
			return ast::BinaryOperation::Operator::Plus;
		case Clingo::AST::BinaryOperator::Minus:
			return ast::BinaryOperation::Operator::Minus;
		case Clingo::AST::BinaryOperator::Multiplication:
			return ast::BinaryOperation::Operator::Multiplication;
		case Clingo::AST::BinaryOperator::Division:
			return ast::BinaryOperation::Operator::Division;
		case Clingo::AST::BinaryOperator::Modulo:
			return ast::BinaryOperation::Operator::Modulo;
		case Clingo::AST::BinaryOperator::Power:
			return ast::BinaryOperation::Operator::Power;
		default:
			throw LogicException("unexpected binary operation, please report to bug tracker");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline ast::BinaryOperation::Operator translate(Clingo::AST::BinaryOperator binaryOperator, const Clingo::AST::Term &term)
{
	try
	{
		return translate(binaryOperator);
	}
	catch (TranslationException &exception)
	{
		exception.setLocation(term.location);
		throw;
	}
	catch (LogicException &exception)
	{
		exception.setLocation(term.location);
		throw;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline ast::UnaryOperation::Operator translate(Clingo::AST::UnaryOperator unaryOperator, const Clingo::AST::Term &term)
{
	switch (unaryOperator)
	{
		case Clingo::AST::UnaryOperator::Absolute:
			return ast::UnaryOperation::Operator::Absolute;
		case Clingo::AST::UnaryOperator::Minus:
			return ast::UnaryOperation::Operator::Minus;
		case Clingo::AST::UnaryOperator::Negation:
			throw TranslationException(term.location, "unary operation “negation” not yet supported");
		default:
			throw LogicException(term.location, "unexpected unary operator, please report to bug tracker");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ast::Term translate(const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack);

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TermTranslateVisitor
{
	ast::Term visit(const Clingo::Symbol &symbol, const Clingo::AST::Term &term, RuleContext &, Context &context, const ast::VariableStack &)
	{
		switch (symbol.type())
		{
			case Clingo::SymbolType::Number:
				return ast::Integer(symbol.number());
			case Clingo::SymbolType::Infimum:
				return ast::SpecialInteger(ast::SpecialInteger::Type::Infimum);
			case Clingo::SymbolType::Supremum:
				return ast::SpecialInteger(ast::SpecialInteger::Type::Supremum);
			case Clingo::SymbolType::String:
				return ast::String(std::string(symbol.string()));
			case Clingo::SymbolType::Function:
			{
				// Functions with arguments are represented as Clingo::AST::Function by the parser. At this
				// point, we only have to handle (0-ary) constants
				if (!symbol.arguments().empty())
					throw LogicException(term.location, "unexpected arguments, expected (0-ary) constant symbol, please report to bug tracker");

				auto constantDeclaration = context.findOrCreateFunctionDeclaration(symbol.name(), 0);

				// TODO: remove workaround
				// Currently, the integer detection doesn’t cover the return types of functions. Setting the
				// return type to the symbolic domain lets us detect more integer variables. This workaround
				// sets the symbolic domain by default until a proper return type detection is implemented
				constantDeclaration->domain = Domain::Symbolic;

				return ast::Function(constantDeclaration);
			}
			default:
				throw LogicException("unexpected symbol type, please report to bug tracker");
		}
	}

	ast::Term visit(const Clingo::AST::Variable &variable, const Clingo::AST::Term &, RuleContext &ruleContext, Context &, const ast::VariableStack &variableStack)
	{
		const auto matchingVariableDeclaration = variableStack.findUserVariableDeclaration(variable.name);
		const auto isAnonymousVariable = (strcmp(variable.name, "_") == 0);
		const auto isUndeclaredUserVariable = !matchingVariableDeclaration;
		const auto isUndeclared = isAnonymousVariable || isUndeclaredUserVariable;

		if (!isUndeclared)
			return ast::Variable(*matchingVariableDeclaration);

		auto variableDeclaration = std::make_unique<ast::VariableDeclaration>(ast::VariableDeclaration::Type::UserDefined, std::string(variable.name));
		ruleContext.freeVariables.emplace_back(std::move(variableDeclaration));

		return ast::Variable(ruleContext.freeVariables.back().get());
	}

	ast::Term visit(const Clingo::AST::BinaryOperation &binaryOperation, const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack)
	{
		const auto operator_ = translate(binaryOperation.binary_operator, term);
		auto left = translate(binaryOperation.left, ruleContext, context, variableStack);
		auto right = translate(binaryOperation.right, ruleContext, context, variableStack);

		return ast::BinaryOperation(operator_, std::move(left), std::move(right));
	}

	ast::Term visit(const Clingo::AST::UnaryOperation &unaryOperation, const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack)
	{
		const auto operator_ = translate(unaryOperation.unary_operator, term);
		auto argument = translate(unaryOperation.argument, ruleContext, context, variableStack);

		return ast::UnaryOperation(operator_, std::move(argument));
	}

	ast::Term visit(const Clingo::AST::Interval &interval, const Clingo::AST::Term &, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack)
	{
		auto left = translate(interval.left, ruleContext, context, variableStack);
		auto right = translate(interval.right, ruleContext, context, variableStack);

		return ast::Interval(std::move(left), std::move(right));
	}

	ast::Term visit(const Clingo::AST::Function &function, const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack)
	{
		if (function.external)
			throw TranslationException(term.location, "external functions not yet supported");

		// Functions with arguments are represented as Clingo::AST::Function by the parser. At this point,
		// we only have to handle functions with arguments
		if (function.arguments.empty())
			throw LogicException(term.location, "unexpected 0-ary function, expected at least one argument, please report to bug tracker");

		std::vector<ast::Term> arguments;
		arguments.reserve(function.arguments.size());

		for (const auto &argument : function.arguments)
			arguments.emplace_back(translate(argument, ruleContext, context, variableStack));

		auto functionDeclaration = context.findOrCreateFunctionDeclaration(function.name, function.arguments.size());

		// TODO: remove workaround
		// Currently, the integer detection doesn’t cover the return types of functions. Setting the
		// return type to the symbolic domain lets us detect more integer variables. This workaround
		// sets the symbolic domain by default until a proper return type detection is implemented
		functionDeclaration->domain = Domain::Symbolic;

		return ast::Function(functionDeclaration, std::move(arguments));
	}

	ast::Term visit(const Clingo::AST::Pool &, const Clingo::AST::Term &term, RuleContext &, Context &, const ast::VariableStack &)
	{
		throw TranslationException(term.location, "pool terms not yet supported");
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

inline ast::Term translate(const Clingo::AST::Term &term, RuleContext &ruleContext, Context &context, const ast::VariableStack &variableStack)
{
	return term.data.accept(TermTranslateVisitor(), term, ruleContext, context, variableStack);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
