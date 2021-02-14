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

#include "goap/Action.h"
#include "goap/Planner.h"
#include "goap/WorldState.h"

#include "wlisp.hpp"



const char *buildin_funs = R"(
(defun dec (n) (- n 1))
(defun inc (n) (+ n 1))
(defun not (x) (if x 0 1))

(defun neg (n) (- 0 n))

(defun is-pos (n) (> n 0))
(defun is-neg (n) (< n 0))

(defun const (n) (lambda (_) n))

(defun pow (n exp)
    (if (= exp 0)
        1
        (if (< exp 0)
            (pow (/ 1 n) (neg exp))
            (reduce
                (lambda (acc x) (* acc x))
                n
                (map (const n) (range 1 exp))
            ))
        )
    )
(defun format (fmt ...) (do
	(for li (regex-search (regex "`(.*?)`") fmt  )
		(setq fmt (replace fmt (index li 0) (concat (eval (index li 1) ))))
	) fmt))
)";

void winit_do_init();
void random_init();

void math_init();
void process_init();
void w_jobq_init();
void threading_init();
void threading_ipc_init();


#ifdef _MSC_VER

BOOL WINAPI consoleHandler(DWORD signal) {

    if (signal == CTRL_C_EVENT)
    {
		lisp::g_running = false;
 		std::cout << "ctrl+c event" << std::endl;
	}   

    return TRUE;
}
#endif


int main(int argc, const char **argv) {
	
#ifdef _MSC_VER
	if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
        std::cerr << "ERROR: Could not set control handler" << std::endl; 
        return 1;
    }
#endif

    lisp::Environment env;
    std::vector<lisp::Value> args;
    for (int i=0; i<argc; i++)
        args.push_back(lisp::Value::string(argv[i]));
    env.set("cmd-args", lisp::Value(args));

    lisp::run(buildin_funs, env);
	math_init();
	winit_do_init();
	w_jobq_init();
	threading_init();
	threading_ipc_init();
	process_init();
	random_init();

	std::string image(argv[0]);
	std::string app_path;

	int pos = image.rfind('/');
	if (pos == image.npos ){
		pos = image.rfind('\\');
		if (pos != image.npos){
			app_path = image.substr(0, pos+1);
		}
	}
	std::cout << "app-path:" << app_path << std::endl;
	lisp::global_set("app-path", lisp::Value::string(app_path));

    srand(time(NULL));
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
		}catch (...){
			std::cerr << "unkown expetion" << std::endl;
		}
		if (argc > 1){
			try {
				lisp::run(lisp::read_file_contents( argv[1] ), env);
			}catch (lisp::Error &e){
				std::cerr << "cannot run " << argv[1] << std::endl;
			} catch (std::runtime_error &e) {
				std::cerr << "cannot run " << argv[1] << std::endl;
				std::cerr << e.what() << std::endl;
			}
		}
		
    	repl(env);
    } catch (lisp::Error &e) {
        std::cerr << e.description() << std::endl;
    } catch (std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
    } catch (...){
		std::cout << "unkown exception" << std::endl;
	}

    return 0;
}

