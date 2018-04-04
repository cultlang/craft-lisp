#include "lisp/common.h"
#include "lisp/lisp.h"

#include <stack>
#include <queue>

#include "replxx/replxx.hxx"
#include "lisp/library/system/prelude.h"


using namespace craft;
using namespace craft::lisp;


//
//// the default read-eval-print-loop
//void repl(const std::string & prompt, instance<Environment> env)
//{
//	std::string long_line = "";
//
//	auto live_module = env->ns_user->requireModule("repl:console");
//
//	for (;;)
//	{
//		if (long_line.empty())
//			std::cout << prompt;
//		else
//			std::cout << std::string(prompt.size(), '.');
//
//		std::string line;
//		std::getline(std::cin, line);
//		long_line += line;
//
//		instance<Sexpr> top_level;
//		try
//		{
//			top_level = env->parse(env->ns_user, long_line);
//		}
//		catch (std::exception const& e)
//		{
//			if (line.empty())
//			{
//				long_line = "";
//				std::cout << "parser: " << e.what() << '\n';
//			}
//
//			continue;
//		}
//
//		long_line = "";
//
//		try
//		{
//			if (top_level)
//			{
//				std::cout << live_module->liveContinueWith(top_level).toString() << '\n';
//			}
//		}
//		catch (std::exception const& e)
//		{
//			std::cout << e.what() << '\n';
//		}
//	}
//}

namespace _impl {
	using cl = replxx::Replxx::Color;
	std::vector<std::pair<std::string, cl>> regex_color{
		// single chars
		{ "\\`", cl::BRIGHTCYAN },
		{ "\\'", cl::BRIGHTBLUE },
		{ "\\\"", cl::BRIGHTBLUE },
		{ "\\-", cl::BRIGHTBLUE },
		{ "\\+", cl::BRIGHTBLUE },
		{ "\\=", cl::BRIGHTBLUE },
		{ "\\/", cl::BRIGHTBLUE },
		{ "\\*", cl::BRIGHTBLUE },
		{ "\\^", cl::BRIGHTBLUE },
		{ "\\.", cl::BRIGHTMAGENTA },
		{ "\\(", cl::BRIGHTMAGENTA },
		{ "\\)", cl::BRIGHTMAGENTA },
		{ "\\[", cl::BRIGHTMAGENTA },
		{ "\\]", cl::BRIGHTMAGENTA },
		{ "\\{", cl::BRIGHTMAGENTA },
		{ "\\}", cl::BRIGHTMAGENTA },

		// color keywords
		{ "color_black", cl::BLACK },
		{ "color_red", cl::RED },
		{ "color_green", cl::GREEN },
		{ "color_brown", cl::BROWN },
		{ "color_blue", cl::BLUE },
		{ "color_magenta", cl::MAGENTA },
		{ "color_cyan", cl::CYAN },
		{ "color_lightgray", cl::LIGHTGRAY },
		{ "color_gray", cl::GRAY },
		{ "color_brightred", cl::BRIGHTRED },
		{ "color_brightgreen", cl::BRIGHTGREEN },
		{ "color_yellow", cl::YELLOW },
		{ "color_brightblue", cl::BRIGHTBLUE },
		{ "color_brightmagenta", cl::BRIGHTMAGENTA },
		{ "color_brightcyan", cl::BRIGHTCYAN },
		{ "color_white", cl::WHITE },
		{ "color_normal", cl::NORMAL },

		// commands
		{ "\\.help", cl::BRIGHTMAGENTA },
		{ "\\.history", cl::BRIGHTMAGENTA },
		{ "\\.quit", cl::BRIGHTMAGENTA },
		{ "\\.exit", cl::BRIGHTMAGENTA },
		{ "\\.clear", cl::BRIGHTMAGENTA },
		{ "\\.prompt", cl::BRIGHTMAGENTA },

		// numbers
		{ "[\\-|+]{0,1}[0-9]+", cl::YELLOW }, // integers
		{ "[\\-|+]{0,1}[0-9]*\\.[0-9]+", cl::YELLOW }, // decimals
		{ "[\\-|+]{0,1}[0-9]+e[\\-|+]{0,1}[0-9]+", cl::YELLOW }, // scientific notation

																 // strings
		{ "\".*?\"", cl::BRIGHTGREEN }, // double quotes
		{ "\'.*?\'", cl::BRIGHTGREEN }, // single quotes
	};
}


int main(int argc, char** argv)
{
	::craft::types::system().init();
	instance<Environment> global_env = instance<Environment>::make(spdlog::stdout_color_mt("environment"));
	
	if (argc != 1)
	{
		std::string f;
		try
		{
			f = fs::read<std::string>(path::normalize(argv[1]), fs::string_read).get();
		}
		catch (...)
		{
			global_env->log()->info("No Such File: {0}", argv[1]);
		}
		try
		{
			global_env->eval(f);
		}
		catch (std::exception e)
		{
			global_env->log()->error(e.what());
			return -1;
		}
	}
	else
	{
		auto live_module = global_env->ns_user->requireModule("repl:console");
		using Replxx = replxx::Replxx;

		Replxx rx;
		rx.install_window_change_handler();
		std::string history_file{ "./replxx_history.txt" };
		rx.history_load(history_file);
		// set the max history size
		rx.set_max_history_size(12);
		// set the max input line size
		rx.set_max_line_size(128);
		// set the max number of hint rows to show
		rx.set_max_hint_rows(8);
		
		/*Replxx::completions_t hook_completion(std::string const& context, int index, void* user_data);
		Replxx::hints_t hook_hint(std::string const& context, int index, Replxx::Color& color, void* user_data);
		void hook_color(std::string const& str, Replxx::colors_t& colors, void* user_data);*/
		std::function<Replxx::hints_t(std::string const& context, int index, void* user_data)> complete = [](std::string const& context, int index, void* user_data)->Replxx::hints_t { 
			auto module = *static_cast<instance<Module>*>(user_data);
			Replxx::completions_t completions;

			instance<> query = (instance<>)instance<std::string>::make(context);
			instance<> ind = (instance<>)instance<int64_t>::make(index);

			auto env = module->environment();
			auto repl = env->eval(env->ns_user->lookup("replcomplete"));
			
			if (repl.typeId() == type<craft::lisp::MultiMethod>::typeId())
			{
				auto frame = instance<Frame>::make(env->ns_user);
				Execution::execute(frame);

				auto results = repl.asType<craft::lisp::MultiMethod>()->call(frame, { query, ind });
				auto data = results.asType<craft::lisp::library::List>()->data();

				for (auto& d : data)
				{
					
					completions.emplace_back(d.asFeature<std::string>()->c_str());
				}
			}
			return completions;
		};
		rx.set_completion_callback(complete, &live_module);

		std::function<void(std::string const&, Replxx::colors_t&, void*)> colors = [](std::string const& context, Replxx::colors_t& colors, void* user_data) 
		{
			for (auto const& e : _impl::regex_color) {
				size_t pos{ 0 };
				std::string str = context;
				std::smatch match;

				while (std::regex_search(str, match, std::regex(e.first))) {
					std::string c{ match[0] };
					pos += std::string(match.prefix()).size();

					for (size_t i = 0; i < c.size(); ++i) {
						colors.at(pos + i) = e.second;
					}

					pos += c.size();
					str = match.suffix();
				}
			}
		};
		rx.set_highlighter_callback(colors, &live_module);



		for (;;) {
			// display the prompt and retrieve input from the user
			auto cinp = rx.input("CULT>");
			if (cinp == nullptr) break;
			auto input = std::string(cinp);

			instance<Sexpr> top_level;
			try
			{
				top_level = global_env->parse(global_env->ns_user, input);
				
			}
			catch (std::exception e)
			{
				std::cout << "parser: " << e.what() << '\n';
			}
			try
			{
				if (top_level)
				{
					std::cout << live_module->liveContinueWith(top_level).toString() << '\n';
				}
			}
			catch (std::exception const& e)
			{
				std::cout << e.what() << '\n';
			}

			rx.history_add(input);
			rx.history_save(history_file);
		}
		// the path to the history file
		
	}

	return 0;
	
	
	//add_globals(global_env);
	
}
