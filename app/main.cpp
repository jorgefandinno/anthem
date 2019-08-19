#include <iostream>

#include <cxxopts.hpp>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/examine-semantics/Translation.h>
#include <anthem/verify-properties/Translation.h>
#include <anthem/verify-strong-equivalence/Translation.h>

int main(int argc, char **argv)
{
	anthem::Context context;

	cxxopts::Options options("anthem", "Translate ASP programs to the language of first-order theorem provers.");

	options.add_options()
		("h,help", "Display this help message")
		("v,version", "Display version information")
		("i,input", "Input files (one file for plain translation, two files for verifying strong equivalence)", cxxopts::value<std::vector<std::string>>())
		("target", "Translation target (verify-strong-equivalence, examine-semantics)", cxxopts::value<std::string>()->default_value("verify-strong-equivalence"))
		("output-format", "Output format (human-readable, tptp)", cxxopts::value<std::string>()->default_value("human-readable"))
		("unify-domains", "Unify program and integer variables into one type (always, auto)", cxxopts::value<std::string>()->default_value("auto"))
		("no-simplify", "Do not simplify the output (only for examine-semantics translation target)")
		("no-complete", "Do not perform completion (only for examine-semantics translation target)")
		("no-detect-integers", "Do not detect integer variables (only for examine-semantics translation target)")
		("color", "Colorize output (always, never, auto)", cxxopts::value<std::string>()->default_value("auto"))
		("parentheses", "Parenthesis style (normal, full) (only with human-readable output format)", cxxopts::value<std::string>()->default_value("normal"))
		("p,log-priority", "Log messages starting from this priority (debug, info, warning, error)", cxxopts::value<std::string>()->default_value("info"));

	options.parse_positional("input");
	options.positional_help("[<input file...>]");

	const auto printHelp =
		[&]()
		{
			std::cout << options.help();
		};

	bool help;
	bool version;
	std::vector<std::string> inputFiles;
	std::string translationTargetString;
	std::string outputFormatString;
	std::string unifyDomainsPolicyString;
	std::string variableDomainString;
	std::string colorPolicyString;
	std::string parenthesisStyleString;
	std::string logPriorityString;

	try
	{
		const auto parseResult = options.parse(argc, argv);

		help = (parseResult.count("help") > 0);
		version = (parseResult.count("version") > 0);

		if (parseResult.count("input") > 0)
			inputFiles = parseResult["input"].as<std::vector<std::string>>();

		translationTargetString = parseResult["target"].as<std::string>();
		outputFormatString = parseResult["output-format"].as<std::string>();
		unifyDomainsPolicyString = parseResult["unify-domains"].as<std::string>();
		context.performSimplification = (parseResult.count("no-simplify") == 0);
		context.performCompletion = (parseResult.count("no-complete") == 0);
		context.performIntegerDetection = (parseResult.count("no-detect-integers") == 0);
		colorPolicyString = parseResult["color"].as<std::string>();
		parenthesisStyleString = parseResult["parentheses"].as<std::string>();
		logPriorityString = parseResult["log-priority"].as<std::string>();
	}
	catch (const std::exception &exception)
	{
		context.logger.log(anthem::output::Priority::Error) << exception.what();
		context.logger.errorStream() << std::endl;
		printHelp();
		return EXIT_FAILURE;
	}

	if (help)
	{
		printHelp();
		return EXIT_SUCCESS;
	}

	if (version)
	{
		std::cout << "anthem version 0.1.9+git" << std::endl;
		return EXIT_SUCCESS;
	}

	if (outputFormatString == "human-readable")
		context.outputFormat = anthem::OutputFormat::HumanReadable;
	else if (outputFormatString == "tptp")
		context.outputFormat = anthem::OutputFormat::TPTP;
	else
	{
		context.logger.log(anthem::output::Priority::Error) << "unknown output format “" << outputFormatString << "”";
		context.logger.errorStream() << std::endl;
		printHelp();
		return EXIT_FAILURE;
	}

	if (unifyDomainsPolicyString == "auto")
		context.unifyDomainsPolicy = anthem::UnifyDomainsPolicy::Auto;
	else if (unifyDomainsPolicyString == "always")
		context.unifyDomainsPolicy = anthem::UnifyDomainsPolicy::Always;
	else
	{
		context.logger.log(anthem::output::Priority::Error) << "unknown unify-domains policy “" << unifyDomainsPolicyString << "”";
		context.logger.errorStream() << std::endl;
		printHelp();
		return EXIT_FAILURE;
	}

	if (colorPolicyString == "auto")
		context.logger.setColorPolicy(anthem::output::ColorStream::ColorPolicy::Auto);
	else if (colorPolicyString == "never")
		context.logger.setColorPolicy(anthem::output::ColorStream::ColorPolicy::Never);
	else if (colorPolicyString == "always")
		context.logger.setColorPolicy(anthem::output::ColorStream::ColorPolicy::Always);
	else
	{
		context.logger.log(anthem::output::Priority::Error) << "unknown color policy “" << colorPolicyString << "”";
		context.logger.errorStream() << std::endl;
		printHelp();
		return EXIT_FAILURE;
	}

	if (parenthesisStyleString == "normal")
		context.parenthesisStyle = anthem::output::ParenthesisStyle::Normal;
	else if (parenthesisStyleString == "full")
		context.parenthesisStyle = anthem::output::ParenthesisStyle::Full;
	else
	{
		context.logger.log(anthem::output::Priority::Error) << "unknown parenthesis style “" << parenthesisStyleString << "”";
		context.logger.errorStream() << std::endl;
		printHelp();
		return EXIT_FAILURE;
	}

	try
	{
		const auto logPriority = anthem::output::priorityFromName(logPriorityString.c_str());
		context.logger.setLogPriority(logPriority);
	}
	catch (const std::exception &e)
	{
		context.logger.log(anthem::output::Priority::Error) << "unknown log priorty “" << logPriorityString << "”";
		context.logger.errorStream() << std::endl;
		printHelp();
		return EXIT_FAILURE;
	}

	try
	{
		// TODO: refactor
		if (translationTargetString == "examine-semantics")
		{
			if (!inputFiles.empty())
				anthem::examineSemantics::translate(inputFiles, context);
			else
				anthem::examineSemantics::translate("std::cin", std::cin, context);
		}
		else if (translationTargetString == "verify-properties")
		{
			if (!inputFiles.empty())
				anthem::verifyProperties::translate(inputFiles, context);
			else
				anthem::verifyProperties::translate("std::cin", std::cin, context);
		}
		else if (translationTargetString == "verify-strong-equivalence")
		{
			if (!inputFiles.empty())
				anthem::verifyStrongEquivalence::translate(inputFiles, context);
			else
				anthem::verifyStrongEquivalence::translate("std::cin", std::cin, context);
		}
		else
		{
			context.logger.log(anthem::output::Priority::Error) << "unknown translation target “" << translationTargetString << "”";
			context.logger.errorStream() << std::endl;
			printHelp();
			return EXIT_FAILURE;
		}
	}
	catch (const std::exception &e)
	{
		context.logger.log(anthem::output::Priority::Error) << e.what();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
