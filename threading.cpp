/*
 * main.cpp
 *
 *  Created on: Jan 7, 2021
 *      Author: Administrator
 */
#include <iostream>

#include <chrono>
#include <thread>
#include <mutex>
#include <future>
#include <list>

#include "wlisp.hpp"
#include "chan.hpp"

//D:\code\cpp\wisp-main\goap\build\Debug\helloworld.exe

class w_thread:public lisp::user_data{
public:
    std::thread thread;
    lisp::Value exit_value;

    w_thread(std::vector<lisp::Value> &args, lisp::Environment &o):thread([args,this, o](){
        lisp::Environment env;
        env.combine(o);
        auto f = args[0];
        std::vector<lisp::Value> a;
        for (int i=1; i<args.size(); i++){
            a.push_back(args[i]);
        }
        try {
            this->exit_value = f.apply(a, env);
        } catch (lisp::Error &e) {
            std::cerr << e.description() << std::endl;
        } catch (std::runtime_error &e) {
            std::cerr << e.what() << std::endl;
        }
    }){

    }

    
    ~w_thread(){
        if (thread.joinable()){
            thread.join();
        }
    }

	static lisp::Value make_value(std::vector<lisp::Value> &args, lisp::Environment &env) {
		auto ptr = std::make_shared<w_thread>(args, env);
        if (!ptr){
            throw std::runtime_error("no memory");
        }
		lisp::Value ret("", ptr);
		return ret;
	}
    lisp::Value value(){
        auto p = this->shared_from_this();
        return lisp::Value("<thread>", p);
    }
    static std::shared_ptr<w_thread> ptr_from_value(lisp::Value &v, lisp::Environment &env){
        auto p = std::dynamic_pointer_cast<w_thread>(v.as_user_data());
        if (!p){
            throw lisp::Error(lisp::Value::string("ptr casting result is nullptr"), env, "ptr casting result is nullptr");
        }
        return p;
    }
	
	std::string display() const {
		return std::string("<thread>") + std::to_string((int64_t)this);
	}
};



void threading_init()
{
    lisp::global_set( "thread", lisp::Value("thread", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()==0) {
            throw lisp::Error(lisp::Value::string("thread"), env, "need more args");
        }
        lisp::eval_args(args, env);
        return w_thread::make_value(args, env);
    }));
    
    lisp::global_set( "thread-id", lisp::Value("thread-id", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()==0) {
            throw lisp::Error(lisp::Value::string("thread"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_thread::ptr_from_value(args[0], env);
        std::stringstream ss;
        ss << ptr->thread.get_id();
        return lisp::Value( atol(ss.str().c_str() ));
    }));
    lisp::global_set( "thread-join", lisp::Value("thread-join", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()==0) {
            throw lisp::Error(lisp::Value::string("thread-join"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_thread::ptr_from_value(args[0], env);
        if (ptr->thread.joinable()){
            ptr->thread.join();
        }
        return ptr->exit_value;
    }));
    
    lisp::global_set( "tid", lisp::Value("tid", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return lisp::Value( atol( ss.str().c_str() ));
    }));
    lisp::global_set( "sleep", lisp::Value("sleep", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value::string("thread"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        int ms = args[0].as_int();
        std::this_thread::sleep_for( std::chrono::milliseconds(ms) );
        return args[0];
    }));
    
}

class w_queue:public lisp::user_data{
public:
    chan<lisp::Value> qdata;

    w_queue(int n, push_policy po, lisp::Environment &o):qdata(n, po){
    }

    ~w_queue(){
    }

	static lisp::Value make_value(std::vector<lisp::Value> &args, lisp::Environment &env) {
        int n = 0;
        push_policy po = push_policy::blocking;
        if (args.size() > 0){
            n = args[0].as_int();
        }
        if (args.size() > 1){
            std::string opt = args[1].as_string();
            if (opt == "blocking"){
            } else if (opt == "discard-old"){
                po = push_policy::discard_old;
            } else if (opt == "discard"){
                po = push_policy::discard;
            } else {
                throw lisp::Error(lisp::Value::string("queue"), env, "queue option not supported. only:blocking,discard-old,discard");
            }
        }
		auto ptr = std::make_shared<w_queue>(n, po, env);
        if (!ptr){
            throw std::runtime_error("no memory");
        }
		lisp::Value ret("", ptr);
		return ret;
	}
    lisp::Value value(){
        auto p = this->shared_from_this();
        return lisp::Value("<queue>", p);
    }
    static std::shared_ptr<w_queue> ptr_from_value(lisp::Value &v, lisp::Environment &env){
        auto p = std::dynamic_pointer_cast<w_queue>(v.as_user_data());
        if (!p){
            throw lisp::Error(lisp::Value::string("ptr casting result is nullptr"), env, "ptr casting result is nullptr");
        }
        return p;
    }
	
	std::string display() const {
		return std::string("<queue>") + std::to_string((int64_t)this);
	}
};


void threading_ipc_init()
{
    lisp::global_set( "queue", lisp::Value("queue", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        lisp::eval_args(args, env);
        return w_queue::make_value(args, env);
    }));
    lisp::global_set( "queue-length", lisp::Value("queue-length", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1){
            throw lisp::Error(lisp::Value::string("queue-length"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_queue::ptr_from_value(args[0], env);
        return lisp::Value( int(ptr->qdata.length()) );
    }));
    
    lisp::global_set( "queue-push", lisp::Value("queue-push", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2){
            throw lisp::Error(lisp::Value::string("queue-push"), env, "need 2 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_queue::ptr_from_value(args[0], env);
        return lisp::Value(int(ptr->qdata.push(args[1])));
    }));

    lisp::global_set( "queue-pop", lisp::Value("queue-pop", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1){
            throw lisp::Error(lisp::Value::string("queue-pop"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_queue::ptr_from_value(args[0], env);
        lisp::Value v;
        bool ok = (ptr->qdata >> v);
        return v;
    }));
    lisp::global_set( "queue-is-closed", lisp::Value("queue-is-closed", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1){
            throw lisp::Error(lisp::Value::string("queue-is-closed"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_queue::ptr_from_value(args[0], env);
        return lisp::Value(int(ptr->qdata.is_closed()));
    }));

    lisp::global_set( "queue-close", lisp::Value("queue-close", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1){
            throw lisp::Error(lisp::Value::string("queue-close"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_queue::ptr_from_value(args[0], env);
        ptr->qdata.close();
        return args[0];
    }));
}

#include <random>
#include <chrono>

class w_random:public lisp::user_data{
public:
    std::mt19937 rd;

    ~w_random(){
    }

	static lisp::Value make_value(std::vector<lisp::Value> &args, lisp::Environment &env) {
 		auto ptr = std::make_shared<w_random>();
        if (!ptr){
            throw std::runtime_error("no memory");
        }
        if (args.size() > 0){
            ptr->rd.seed(args[0].as_int());
        }
		lisp::Value ret("", ptr);
		return ret;
	}
    lisp::Value value(){
        auto p = this->shared_from_this();
        return lisp::Value("<random>", p);
    }
    static std::shared_ptr<w_random> ptr_from_value(lisp::Value &v, lisp::Environment &env){
        auto p = std::dynamic_pointer_cast<w_random>(v.as_user_data());
        if (!p){
            throw lisp::Error(lisp::Value::string("ptr casting result is nullptr"), env, "ptr casting result is nullptr");
        }
        return p;
    }
	
	std::string display() const {
		return std::string("<random>") + std::to_string((int64_t)this);
	}
};

void random_init()
{
    lisp::global_set( "random-device-rand", lisp::Value("random-device-rand", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        lisp::eval_args(args, env);
        return lisp::Value(int(std::random_device()()));
    }));
    lisp::global_set( "random", lisp::Value("random", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        lisp::eval_args(args, env);
        return w_random::make_value(args, env);
    }));
    lisp::global_set( "random-seed", lisp::Value("random-seed", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        lisp::eval_args(args, env);
        auto ptr = w_random::ptr_from_value(args[0], env);
        ptr->rd.seed(args[1].as_int());
        return args[0];
    }));
    lisp::global_set( "random-rand", lisp::Value("random-rand", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        lisp::eval_args(args, env);
        auto ptr = w_random::ptr_from_value(args[0], env);
        if (args.size() > 1){
            int n = args[1].as_int();
            std::vector<lisp::Value> ret;
            ret.reserve(n);
            for (int i=0; i<n; i++){
                ret.push_back(lisp::Value( int(ptr->rd()) ));
            }
            return ret;
        }
        return lisp::Value(int(ptr->rd()));
    }));
}

