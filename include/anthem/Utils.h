#ifndef __ANTHEM__UTILS_H
#define __ANTHEM__UTILS_H

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Utils
//
////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr const auto HeadVariablePrefix = "V";
constexpr const auto BodyVariablePrefix = "X";
constexpr const auto UserVariablePrefix = "U";
constexpr const auto IntegerVariablePrefix = "N";

////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr const auto AuxiliaryPredicateNameIsInteger = "p__is_integer__";
constexpr const auto AuxiliaryPredicateNameLessEqual = "p__less_equal__";
constexpr const auto AuxiliaryPredicateNameLess = "p__less__";
constexpr const auto AuxiliaryPredicateNameGreaterEqual = "p__greater_equal__";
constexpr const auto AuxiliaryPredicateNameGreater = "p__greater__";
constexpr const auto AuxiliaryFunctionNameInteger = "f__integer__";
constexpr const auto AuxiliaryFunctionNameSymbolic = "f__symbolic__";
constexpr const auto AuxiliaryFunctionNameSum = "f__sum__";
constexpr const auto AuxiliaryFunctionNameDifference = "f__difference__";
constexpr const auto AuxiliaryFunctionNameUnaryMinus = "f__unary_minus__";
constexpr const auto AuxiliaryFunctionNameProduct = "f__product__";

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class Tristate
{
	True,
	False,
	Unknown,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class OperationResult
{
	Unchanged,
	Changed,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class EvaluationResult
{
	True,
	False,
	Unknown,
	Error,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class Domain
{
	Symbolic,
	Integer,
	Unknown,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

enum class SetSize
{
	Empty,
	Unit,
	Multi,
	Unknown,
};

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Type
{
	Domain domain{Domain::Unknown};
	SetSize setSize{SetSize::Unknown};
};

////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif
