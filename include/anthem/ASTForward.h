#ifndef __ANTHEM__AST_FORWARD_H
#define __ANTHEM__AST_FORWARD_H

#include <memory>

#include <clingo.hh>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AST Forward Declarations
//
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ast
{

////////////////////////////////////////////////////////////////////////////////////////////////////

struct And;
struct BinaryOperation;
struct Biconditional;
struct Boolean;
struct Comparison;
struct Constant;
struct Exists;
struct ForAll;
struct Function;
struct Implies;
struct In;
struct Integer;
struct Interval;
struct Not;
struct Or;
struct Predicate;
struct SpecialInteger;
struct String;
struct Variable;
struct VariableDeclaration;
using VariableDeclarationPointer = std::unique_ptr<VariableDeclaration>;
using VariableDeclarationPointers = std::vector<VariableDeclarationPointer>;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Variants
////////////////////////////////////////////////////////////////////////////////////////////////////

using Formula = Clingo::Variant<
	And,
	Biconditional,
	Boolean,
	Comparison,
	Exists,
	ForAll,
	Implies,
	In,
	Not,
	Or,
	Predicate>;

using Term = Clingo::Variant<
	BinaryOperation,
	Boolean,
	Constant,
	Function,
	Integer,
	Interval,
	SpecialInteger,
	String,
	Variable>;

////////////////////////////////////////////////////////////////////////////////////////////////////
// High-Level
////////////////////////////////////////////////////////////////////////////////////////////////////

struct ScopedFormula;

////////////////////////////////////////////////////////////////////////////////////////////////////

}
}

#endif
