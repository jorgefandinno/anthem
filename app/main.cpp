#include <iostream>

#include <cxxopts.hpp>

#include <anthem/AST.h>
#include <anthem/Context.h>
#include <anthem/Translation.h>

int main(int argc, char **argv)
{
	anthem::Context context;

	cxxopts::Options options("anthem", "Translate ASP programs to the language of first-order theorem provers.");

	options.add_options()
		("h,help", "Display this help message")
		("v,version", "Display version information")
		("i,input", "Input files", cxxopts::value<std::vector<std::string>>())
		("head-translation", "Translate the head directly (direct) or to prepare for completion (for-completion)", cxxopts::value<std::string>()->default_value("direct"))
		("no-simplify", "Do not simplify the output")
		("no-complete", "Do not perform completion")
		("no-detect-integers", "Do not detect integer variables")
		("color", "Colorize output (always, never, auto)", cxxopts::value<std::string>()->default_value("auto"))
		("parentheses", "Parenthesis style (normal, full)", cxxopts::value<std::string>()->default_value("normal"))
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
	std::string headTranslationModeString;
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

		headTranslationModeString = parseResult["head-translation"].as<std::string>();
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

	if (headTranslationModeString == "direct")
		context.headTranslationMode = anthem::HeadTranslationMode::Direct;
	else if (headTranslationModeString == "for-completion")
		context.headTranslationMode = anthem::HeadTranslationMode::ForCompletion;
	else
	{
		context.logger.log(anthem::output::Priority::Error) << "unknown head mode “" << headTranslationModeString << "”";
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
		if (!inputFiles.empty())
			anthem::translate(inputFiles, context);
		else
			anthem::translate("std::cin", std::cin, context);
	}
	catch (const std::exception &e)
	{
		context.logger.log(anthem::output::Priority::Error) << e.what();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
