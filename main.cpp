/*
 * main.cpp
 *
 *  Created on: Jan 7, 2021
 *      Author: Administrator
 */
#include <iostream>

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <thread>

#include "wlisp.hpp"

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"

#ifdef _MSC_VER
BOOL WINAPI consoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT)
    {
		lisp::lisp_try_break();
 		std::cout << "ctrl+c event" << std::endl;
	}   
    return TRUE;
}
#endif


class __server_app : public Poco::Util::ServerApplication
{

public:
	__server_app() {
	}

protected:
	void initialize(Poco::Util::Application& self)
	{
		loadConfiguration(); // load default configuration files, if present
		ServerApplication::initialize(self);
	}

	void uninitialize()
	{
		ServerApplication::uninitialize();
	}

	void defineOptions(Poco::Util::OptionSet& options)
	{
	}

	void handleOption(const std::string& name, const std::string& value)
	{
		ServerApplication::handleOption(name, value);
	}

	void display_help()
	{
	}

	int main(const std::vector<std::string>& args)
	{
		auto tid = std::this_thread::get_id();
		lisp::Environment env;
		std::vector<lisp::Value> args_value;
		for (auto &a:args)
		{
			args_value.push_back(lisp::Value::string(a));
		}
		env.set("cmd-args", lisp::Value(args_value));
		lisp::global_set( "is-interactive", lisp::Value("is-interactive", [this,&tid](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
			if (args.size() != 0) {
				throw lisp::exception(env, "", "is-interactive need 0 args");
			}
			
			if (tid != std::this_thread::get_id()){
				throw lisp::exception(env, "", "is-interactive should be used in main thread only");
			}
			return lisp::Value(int(this->isInteractive()));
		}), "<> -> bool");

		lisp::global_set( "wait-for-termination-request", lisp::Value("wait-for-termination-request", [this, &tid](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
			if (args.size() != 0) {
				throw lisp::exception(env, "", "wait-for-termination-request need 0 args");
			}
			if (tid != std::this_thread::get_id()){
				throw lisp::exception(env, "", "wait-for-termination-request should be used in main thread only");
			}
			waitForTerminationRequest();
			return lisp::Value::nil();
		}), "<> -> nil");

		std::string image(this->commandPath());
		std::string app_path;

		int pos = image.rfind('/');
		if (pos == image.npos ){
			pos = image.rfind('\\');
			if (pos != image.npos){
				app_path = image.substr(0, pos+1);
			}
		}
		// std::cout << "app-path:" << app_path << std::endl;
		env.set("app-path", lisp::Value::string(app_path));
		env.set("command-name", lisp::Value::string(image));
		env.set("version", lisp::Value(int(1)));

		::srand(time(NULL));
		try {
			//lisp::run(lisp::read_file_contents(argv[2]), env);
			try {
				lisp::run(lisp::read_file_contents( app_path + "init.lisp"), env);
			}catch (lisp::Error &e){
				std::cerr << "cannot run init.lisp " << e.description() << std::endl;
			} catch (std::runtime_error &e) {
				std::cerr << "cannot run init.lisp " << e.what() << std::endl;
			}catch (lisp::ctrl_c_event &e){
				std::cerr << "ctrl+c break" << e.value.display() << std::endl;
			}catch (lisp::func_return &e){
				std::cout << "return should be used in function:" << e.value.display() << std::endl;
			} catch (lisp::loop_continue &e){
				std::cout << "continue should be used in a loop:" << e.value.display() << std::endl;
			} catch (lisp::loop_break &e){
				std::cout << "break should be used in a loop:" << e.value.display() << std::endl;
			} catch (lisp::exception &e){
				std::cout << "unhandled lisp-exception:" << e.value.display() << std::endl;
			}catch (...){
				std::cerr << "unkown expetion" << std::endl;
			}
			bool ok = false;
			try {
				auto value = env.get("run-repl");
				ok = value.as_bool();
			}catch (...){
			}
			
			if (ok){
				repl(env);
			}
		} catch (lisp::Error &e) {
			std::cerr << e.description() << std::endl;
		} catch (std::runtime_error &e) {
			std::cerr << e.what() << std::endl;
		} catch (...){
			std::cout << "unkown exception" << std::endl;
		}
		std::cout << "exited" << std::endl;
		return 0;
	}
};


int main(int argc, char **argv)
{
#ifdef _MSC_VER
	if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
        std::cerr << "ERROR: Could not set control handler" << std::endl; 
        return 1;
    }
#endif

	lisp::load_default_lib();

    __server_app app;
    return app.run(argc, argv);

}



