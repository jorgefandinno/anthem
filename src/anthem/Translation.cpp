#include <anthem/Translation.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <clingo.hh>

#include <anthem/Completion.h>
#include <anthem/Context.h>
#include <anthem/IntegerVariableDetection.h>
#include <anthem/Simplification.h>
#include <anthem/StatementVisitor.h>
#include <anthem/output/FormatterHumanReadable.h>
#include <anthem/output/FormatterTPTP.h>

namespace anthem
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Translation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const std::vector<std::string> &fileNames, Context &context)
{
	for (const auto &fileName : fileNames)
	{
		std::ifstream file(fileName, std::ios::in);

		if (!file.is_open())
			throw LogicException("could not read file “" + fileName + "”");

		translate(fileName.c_str(), file, context);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto printFormula =
	[](const auto &value, Context &context, output::PrintContext &printContext)
	{
		auto &stream = context.logger.outputStream();

		switch (context.outputFormat)
		{
			case OutputFormat::HumanReadable:
				output::print<output::FormatterHumanReadable>(stream, value, printContext);
				break;
			case OutputFormat::TPTP:
			{
				const auto formulaName = std::string("axiom") + std::to_string(printContext.currentFormulaID + 1);

				stream
					<< output::Keyword("tff")
					<< "(" << output::Function(formulaName.c_str())
					<< ", " << output::Keyword("axiom") << ", ";
				output::print<output::FormatterTPTP>(stream, value, printContext);
				stream << ").";

				break;
			}
		}

		printContext.currentFormulaID++;
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

const auto printTypeAnnotation =
	[](const ast::PredicateDeclaration &predicateDeclaration, Context &context, output::PrintContext &printContext)
	{
		auto &stream = context.logger.outputStream();

		switch (context.outputFormat)
		{
			case OutputFormat::HumanReadable:
				for (size_t i = 0; i < predicateDeclaration.parameters.size(); i++)
				{
					const auto &parameter = predicateDeclaration.parameters[i];

					// TODO: remove code duplication
					const auto domain = (printContext.context.variableDomain == Domain::Unknown)
						? parameter.domain
						: printContext.context.variableDomain;
					if (domain != Domain::Integer)
						continue;

					stream
						<< output::Keyword("int")
						<< "(" << predicateDeclaration.name
						<< "/" << output::Number(predicateDeclaration.arity())
						<< "@" << output::Number(i + 1)
						<< ")" << std::endl;
				}
				break;
			case OutputFormat::TPTP:
			{
				const auto typeName = std::string("type") + std::to_string(printContext.currentTypeID + 1);

				stream
					<< output::Keyword("tff")
					<< "(" << output::Function(typeName.c_str())
					<< ", " << output::Keyword("type")
					<< ", (" << predicateDeclaration.name << ": (";

				for (size_t i = 0; i < predicateDeclaration.parameters.size(); i++)
				{
					const auto &parameter = predicateDeclaration.parameters[i];

					if (i > 0)
						stream << " * ";

					// TODO: remove code duplication
					const auto domain = (printContext.context.variableDomain == Domain::Unknown)
						? parameter.domain
						: printContext.context.variableDomain;

					switch (domain)
					{
						case Domain::Program:
							stream << output::Keyword("$i");
							break;
						case Domain::Integer:
							stream << output::Keyword("$int");
							break;
						default:
							throw TranslationException("unexpected unknown variable domain, please report to bug tracker");
					}
				}

				stream
					<< ") > " << output::Keyword("$o")
					<< "))." << std::endl;

				break;
			}
		}

		printContext.currentTypeID++;
	};

////////////////////////////////////////////////////////////////////////////////////////////////////

void translateCompletion(std::vector<ast::ScopedFormula> &scopedFormulas, Context &context)
{
	assert(context.semantics == Semantics::ClassicalLogic);

	output::PrintContext printContext(context);

	const auto performSimplification = (context.performSimplification && context.semantics == Semantics::ClassicalLogic);

	if (!context.performCompletion)
	{
		// Simplify output if specified
		if (performSimplification)
			for (auto &scopedFormula : scopedFormulas)
				simplify(scopedFormula.formula);

		if (context.showStatementsUsed)
			context.logger.log(output::Priority::Warning) << "#show statements are ignored because completion is not enabled";

		if (context.externalStatementsUsed)
			context.logger.log(output::Priority::Warning) << "#external statements are ignored because completion is not enabled";

		for (const auto &scopedFormula : scopedFormulas)
		{
			printFormula(scopedFormula.formula, context, printContext);
			context.logger.outputStream() << std::endl;
		}

		return;
	}

	// Perform completion
	auto completedFormulas = complete(std::move(scopedFormulas), context);

	for (const auto &predicateDeclaration : context.predicateDeclarations)
	{
		if (predicateDeclaration->isUsed)
			continue;

		// Check for #show statements with undeclared predicates
		if (predicateDeclaration->visibility != ast::PredicateDeclaration::Visibility::Default)
			context.logger.log(output::Priority::Warning)
				<< "#show declaration of “"
				<< predicateDeclaration->name
				<< "/"
				<< predicateDeclaration->arity()
				<< "” does not match any declared predicate";

		// Check for #external statements with undeclared predicates
		if (predicateDeclaration->isExternal && !predicateDeclaration->isUsed)
			context.logger.log(output::Priority::Warning)
				<< "#external declaration of “"
				<< predicateDeclaration->name
				<< "/"
				<< predicateDeclaration->arity()
				<< "” does not match any declared predicate";
	}

	// Detect integer variables
	if (context.performIntegerDetection)
		detectIntegerVariables(completedFormulas);

	// Simplify output if specified
	if (performSimplification)
		for (auto &completedFormula : completedFormulas)
			simplify(completedFormula);

	// Print specifiers for integer predicate parameters
	for (auto &predicateDeclaration : context.predicateDeclarations)
	{
		// Check that the predicate is used and not declared #external
		if (!predicateDeclaration->isUsed || predicateDeclaration->isExternal)
			continue;

		const auto isPredicateVisible =
			(predicateDeclaration->visibility == ast::PredicateDeclaration::Visibility::Visible)
			|| (predicateDeclaration->visibility == ast::PredicateDeclaration::Visibility::Default
				&& context.defaultPredicateVisibility == ast::PredicateDeclaration::Visibility::Visible);

		// If the predicate ought to be visible, don’t eliminate it
		if (!isPredicateVisible)
			continue;

		printTypeAnnotation(*predicateDeclaration, context, printContext);
	}

	// TODO: remove variables that are not referenced after simplification

	for (const auto &completedFormula : completedFormulas)
	{
		printFormula(completedFormula, context, printContext);
		context.logger.outputStream() << std::endl;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translateHereAndThere(std::vector<ast::ScopedFormula> &scopedFormulas, Context &context)
{
	output::PrintContext printContext(context);

	if (context.variableDomain == Domain::Unknown)
		context.logger.log(output::Priority::Warning)
			<< "automatic detection of variable domains is currently experimental, consider using “--default-domain=program” or “--default-domain=integer”";

	switch (context.semantics)
	{
		case Semantics::ClassicalLogic:
			context.logger.log(output::Priority::Info) << "output semantics: classical logic";
			break;
		case Semantics::LogicOfHereAndThere:
			context.logger.log(output::Priority::Warning) << "output semantics: logic of here-and-there";
			break;
	}

	std::vector<ast::Formula> universallyClosedFormulas;
	universallyClosedFormulas.reserve(scopedFormulas.size());

	// Build the universal closure
	for (auto &scopedFormula : scopedFormulas)
	{
		auto universallyClosedFormula = (scopedFormula.freeVariables.empty())
			? std::move(scopedFormula.formula)
			: ast::Formula::make<ast::ForAll>(std::move(scopedFormula.freeVariables), std::move(scopedFormula.formula));

		universallyClosedFormulas.emplace_back(std::move(universallyClosedFormula));
	}

	// Print type annotations
	if (context.variableDomain == Domain::Unknown)
		context.logger.log(output::Priority::Warning) << "type annotations not yet supported with automatic detection of variable domains";
	else
		for (const auto &predicateDeclaration : context.predicateDeclarations)
			printTypeAnnotation(*predicateDeclaration, context, printContext);

	// Print translated formulas
	for (auto &universallyClosedFormula : universallyClosedFormulas)
	{
		printFormula(universallyClosedFormula, context, printContext);
		context.logger.outputStream() << std::endl;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void translate(const char *fileName, std::istream &stream, Context &context)
{
	context.logger.log(output::Priority::Info) << "reading " << fileName;

	auto fileContent = std::string(std::istreambuf_iterator<char>(stream), {});

	std::vector<ast::ScopedFormula> scopedFormulas;

	const auto translateStatement =
		[&scopedFormulas, &context](const Clingo::AST::Statement &statement)
		{
			statement.data.accept(StatementVisitor(), statement, scopedFormulas, context);
		};

	const auto logger =
		[&context](const Clingo::WarningCode, const char *text)
		{
			context.logger.log(output::Priority::Error) << text;
		};

	Clingo::parse_program(fileContent.c_str(), translateStatement, logger);

	switch (context.translationMode)
	{
		case TranslationMode::Completion:
			translateCompletion(scopedFormulas, context);
			break;
		case TranslationMode::HereAndThere:
			translateHereAndThere(scopedFormulas, context);
			break;
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////////

}
