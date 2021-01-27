/*
 * main.cpp
 *
 *  Created on: Jan 7, 2021
 *      Author: Administrator
 */
#include <iostream>


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
)";


class A:public lisp::user_data{
public:
	int r = 0;
	int i = 0;
	~A(){
	}

	static lisp::Value make_value(int a, int b) {
		auto ptr = std::make_shared<A>(a, b);
		std::string n(std::to_string(ptr->r) + "+" + std::to_string(ptr->i) + "j");
		lisp::Value ret(n, ptr);
		return ret;
	}
	static lisp::Value make_value(const A& o) {
		return make_value(o.r, o.i);
	}

	A(int a, int b):r(a),i(b){
	}
	A operator + (const A &o){
		A t(r + o.r, i + o.i);
		return t;
	}
	A operator - (const A &o){
		A t(r - o.r, i - o.i);
		return t;
	}
	A operator * (const A &o){
		int a = r;
		int b = i;
		int c = o.r;
		int d = o.i;
		A t(a*c - b*d, a*d + b*c);
		return t;
	}
	std::string display() const {
		std::string a(std::to_string(r) + "+" + std::to_string(i) + "j");
		return a;
	}
};

static lisp::Value _A_construct(std::vector<lisp::Value> args, lisp::Environment &env)
{
	lisp::eval_args(args, env);
	return A::make_value(args[0].as_int(), args[1].as_int());
}

void do_test()
{
	lisp::global_set("A", _A_construct);
	lisp::global_set( "A+", lisp::Value("A+", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value(), env, args.size() > 2 ? "too many args" : "too few args");
		}
		lisp::eval_args(args, env);
		auto p1 = std::dynamic_pointer_cast<A>(args[0].as_user_data());
		auto p2 = std::dynamic_pointer_cast<A>(args[1].as_user_data());

		if (p1 == nullptr || p2 == nullptr) {
			throw lisp::Error(lisp::Value(), env, "A+ nullptr found");
		}

		auto r = *p1 + *p2;

		return A::make_value(r);
		}));
	lisp::global_set("A-", lisp::Value("A-", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value(), env, args.size() > 2 ? "too many args" : "too few args");
		}
		lisp::eval_args(args, env);
		auto p1 = std::dynamic_pointer_cast<A>(args[0].as_user_data());
		auto p2 = std::dynamic_pointer_cast<A>(args[1].as_user_data());

		if (p1 == nullptr || p2 == nullptr) {
			throw lisp::Error(lisp::Value(), env, "A- nullptr found");
		}

		auto r = *p1 - *p2;

		return A::make_value(r);
		}));
	lisp::global_set("A*", lisp::Value("A*", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value(), env, args.size() > 2 ? "too many args" : "too few args");
		}
		lisp::eval_args(args, env);
		auto p1 = std::dynamic_pointer_cast<A>(args[0].as_user_data());
		auto p2 = std::dynamic_pointer_cast<A>(args[1].as_user_data());

		if (p1 == nullptr || p2 == nullptr) {
			throw lisp::Error(lisp::Value(), env, "A* nullptr found");
		}

		auto r = *p1 * *p2;

		return A::make_value(r);
		}));
	lisp::global_set("Ar", lisp::Value("Ar", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value(), env, args.size() > 1 ? "too many args" : "too few args");
		}
		lisp::eval_args(args, env);
		auto p1 = std::dynamic_pointer_cast<A>(args[0].as_user_data());

		return lisp::Value(p1->r);
		}));
	lisp::global_set("Ai", lisp::Value("Ai", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value(), env, args.size() > 1 ? "too many args" : "too few args");
		}
		lisp::eval_args(args, env);
		auto p1 = std::dynamic_pointer_cast<A>(args[0].as_user_data());

		return lisp::Value(p1->i);
		}));
}

int main(int argc, const char **argv) {
    lisp::Environment env;
    std::vector<lisp::Value> args;
    for (int i=0; i<argc; i++)
        args.push_back(lisp::Value::string(argv[i]));
    env.set("cmd-args", lisp::Value(args));

    lisp::run(buildin_funs, env);
    do_test();

    srand(time(NULL));
    try {
    	repl(env);
    	lisp::run(lisp::read_file_contents(argv[2]), env);
    } catch (lisp::Error &e) {
        std::cerr << e.description() << std::endl;
    } catch (std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

