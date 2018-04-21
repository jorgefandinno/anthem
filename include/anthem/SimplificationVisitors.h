#ifndef __ANTHEM__SIMPLIFICATION_VISITORS_H
#define __ANTHEM__SIMPLIFICATION_VISITORS_H

#include <anthem/AST.h>
#include <anthem/Simplification.h>
#include <anthem/Utils.h>

namespace anthem
{
namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Simplification Visitor
//
////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
struct FormulaSimplificationVisitor
{
	template <class... Arguments>
	OperationResult visit(And &and_, Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : and_.arguments)
			if (argument.accept(*this, argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
				return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Biconditional &biconditional, Formula &formula, Arguments &&... arguments)
	{
		if (biconditional.left.accept(*this, biconditional.left, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		if (biconditional.right.accept(*this, biconditional.right, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Boolean &, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Comparison &, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Exists &exists, Formula &formula, Arguments &&... arguments)
	{
		if (exists.argument.accept(*this, exists.argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(ForAll &forAll, Formula &formula, Arguments &&... arguments)
	{
		if (forAll.argument.accept(*this, forAll.argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Implies &implies, Formula &formula, Arguments &&... arguments)
	{
		if (implies.antecedent.accept(*this, implies.antecedent, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		if (implies.consequent.accept(*this, implies.consequent, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(In &, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Not &not_, Formula &formula, Arguments &&... arguments)
	{
		if (not_.argument.accept(*this, not_.argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Or &or_, Formula &formula, Arguments &&... arguments)
	{
		for (auto &argument : or_.arguments)
			if (argument.accept(*this, argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
				return OperationResult::Changed;

		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Predicate &, Formula &formula, Arguments &&... arguments)
	{
		return T::accept(formula, std::forward<Arguments>(arguments)...);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, class OperationResult = void>
struct TermSimplificationVisitor
{
	template <class... Arguments>
	OperationResult visit(BinaryOperation &binaryOperation, Term &term, Arguments &&... arguments)
	{
		if (binaryOperation.left.accept(*this, binaryOperation.left, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		if (binaryOperation.right.accept(*this, binaryOperation.right, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Boolean &, Term &term, Arguments &&... arguments)
	{
		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Function &function, Term &term, Arguments &&... arguments)
	{
		for (auto &argument : function.arguments)
			if (argument.accept(*this, argument, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
				return OperationResult::Changed;

		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Integer &, Term &term, Arguments &&... arguments)
	{
		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Interval &interval, Term &term, Arguments &&... arguments)
	{
		if (interval.from.accept(*this, interval.from, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		if (interval.to.accept(*this, interval.to, std::forward<Arguments>(arguments)...) == OperationResult::Changed)
			return OperationResult::Changed;

		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(SpecialInteger &, Term &term, Arguments &&... arguments)
	{
		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(String &, Term &term, Arguments &&... arguments)
	{
		return T::accept(term, std::forward<Arguments>(arguments)...);
	}

	template <class... Arguments>
	OperationResult visit(Variable &, Term &term, Arguments &&... arguments)
	{
		return T::accept(term, std::forward<Arguments>(arguments)...);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
