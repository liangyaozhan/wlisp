/*
 * main.cpp
 *
 *  Created on: Jan 7, 2021
 *      Author: Administrator
 */
#include <iostream>

#include "wlisp.hpp"
#include <cmath>


void math_init()
{
    lisp::global_set( "sqrt", lisp::Value("sqrt", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value::string("sqrt"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        return lisp::Value(sqrt(args[0].as_float()));
    }));
    lisp::global_set( "pow", lisp::Value("pow", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2) {
            throw lisp::Error(lisp::Value::string("pow"), env, "need 2 args");
        }
        lisp::eval_args(args, env);
        return lisp::Value(pow(args[0].as_float(), args[1].as_float()));
    }));
    lisp::global_set( "sin", lisp::Value("sin", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value::string("sin"), env, "need 2 args");
        }
        lisp::eval_args(args, env);
        return lisp::Value(sin(args[0].as_float()));
    }));
    lisp::global_set( "cos", lisp::Value("cos", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value::string("cos"), env, "need 2 args");
        }
        lisp::eval_args(args, env);
        return lisp::Value(sin(args[0].as_float()));
    }));
	
    lisp::global_set( "pi", lisp::Value(double(3.1415926535897932384626433832795028841)));
    lisp::global_set( "PI", lisp::Value(double(3.1415926535897932384626433832795028841)));


}