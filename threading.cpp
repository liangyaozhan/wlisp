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
        }catch (lisp::ctrl_c_event &e){
			std::cerr << "thread " << std::this_thread::get_id() << " ctrl+c break" << e.value.display() << std::endl;
		}catch (lisp::func_return &e){
			std::cout << "thread " << std::this_thread::get_id() << " return should be used in function:" << e.value.display() << std::endl;
		} catch (lisp::loop_continue &e){
			std::cout << "thread " << std::this_thread::get_id() << " continue should be used in a loop:" << e.value.display() << std::endl;
		} catch (lisp::loop_break &e){
			std::cout << "thread " << std::this_thread::get_id() << " break should be used in a loop:" << e.value.display() << std::endl;
		} catch (lisp::exception &e){
			std::cout << "thread " << std::this_thread::get_id() << " unhandled lisp-exception:" << e.value.display() << std::endl;
		}catch (...){
			std::cerr << "thread " << std::this_thread::get_id() << " unkown expetion" << std::endl;
		}
    }){

    }

    
    ~w_thread(){
        if (thread.joinable()){
            thread.detach();
            std::cerr << "thread detached. tid:" << std::this_thread::get_id() << std::endl;
            //thread.join();
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
    }), "(thread <lambda>) -> <thread>");
    
    lisp::global_set( "thread-id", lisp::Value("thread-id", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()==0) {
            throw lisp::Error(lisp::Value::string("thread"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_thread::ptr_from_value(args[0], env);
        std::stringstream ss;
        ss << ptr->thread.get_id();
        return lisp::Value( atol(ss.str().c_str() ));
    }), "<thread> -> threadid:int");
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
    }), "<thread>");
    
    lisp::global_set( "tid", lisp::Value("tid", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return lisp::Value( atol( ss.str().c_str() ));
    }), "current thread id");
    lisp::global_set( "sleep", lisp::Value("sleep", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value::string("thread"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        int ms = args[0].as_int();
        std::this_thread::sleep_for( std::chrono::milliseconds(ms) );
        return args[0];
    }),"(sleep <ms>) -> <ms>");
    
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
    }), "(queue <size> [flags]) flags: 'blocking','discard','discard-old'");
    
    lisp::global_set( "queue-push", lisp::Value("queue-push", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 2 && n != 3 ){
            throw lisp::Error(lisp::Value::string("queue-push"), env, "need 2|3 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_queue::ptr_from_value(args[0], env);
        if (n == 2)
        {
            return lisp::Value(int(ptr->qdata.push(args[1])));
        } else{
            try {
                return lisp::Value(int(ptr->qdata.push(args[1], std::chrono::milliseconds(args[2].as_int()))));
            } catch (ns_chan::chan_timeout ct) {
                (void)ct;
                lisp::exception e(env);
                e.type = "timeout";
                e.value = lisp::Value::string("queue-push timeout");
                throw e;
            }
        }
    }), "(queue-push <q> <value> [ms]) -> int|timeout exception");

    lisp::global_set( "queue-pop", lisp::Value("queue-pop", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1 && args.size() != 2){
            throw lisp::Error(lisp::Value::string("queue-pop"), env, "need 1|2 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_queue::ptr_from_value(args[0], env);
        lisp::Value v;
        if (args.size() == 2){
            try {
                auto pu = ptr->qdata.pop(std::chrono::milliseconds(args[1].as_int()));
                v = *pu;
            } catch (ns_chan::chan_timeout ct) {
                (void)ct;
                lisp::exception e(env);
                e.type = "timeout";
                e.value = lisp::Value::string("queue-pop timeout");
                throw e;
            }
        } else {
            auto pu = ptr->qdata.pop();
            v = *pu;
        }
        return v;
    }), "(<q> [ms]) -> value|timeout exception");
    lisp::global_set( "queue-is-closed", lisp::Value("queue-is-closed", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1){
            throw lisp::Error(lisp::Value::string("queue-is-closed"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_queue::ptr_from_value(args[0], env);
        return lisp::Value(int(ptr->qdata.is_closed()));
    }), "(queue-is-closed) -> int");

    lisp::global_set( "queue-close", lisp::Value("queue-close", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1){
            throw lisp::Error(lisp::Value::string("queue-close"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_queue::ptr_from_value(args[0], env);
        ptr->qdata.close();
        return args[0];
    }),"(queue-close <q>) -> <q>");
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
    }), "<> -> int");
    lisp::global_set( "random", lisp::Value("random", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        lisp::eval_args(args, env);
        return w_random::make_value(args, env);
    }), "<> ->rd");
    lisp::global_set( "random-seed", lisp::Value("random-seed", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        lisp::eval_args(args, env);
        auto ptr = w_random::ptr_from_value(args[0], env);
        ptr->rd.seed(args[1].as_int());
        return args[0];
    }), "<rd> <seed:int> -> args[0]");
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
    }), "<rd> [list-size] -> int|(list)");
    struct rand_uidg
    {
        std::mt19937 rd;
        int min = 0;
        int max = 0;
        std::uniform_int_distribution<> distrib;
        int gen(){ return distrib(rd);}
    };
    lisp::global_set( "uniform-int-distribution", lisp::Value("uniform-int-distribution", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2 && args.size() != 3){
            throw lisp::exception(env, "syntax", "uniform-int-distribution need 2|3 args");
        }
        lisp::eval_args(args, env);
        auto ptr = lisp::extend<rand_uidg>::make_value();
        int begin = args[0].as_int();
        int end = args[1].as_int();
        auto &a = ptr->cxx_value;
        if (args.size() == 3){
            a.rd.seed( args[2].as_int() );
        } else {
            a.rd.seed( std::random_device()() );
        }
        
        a.min = begin;
        a.max = end;
        a.distrib = std::uniform_int_distribution<>(begin, end);
        return ptr->value();
    }), "<begin> <end> [seed] -> obj");
    lisp::global_set( "uniform-int-distribution-get", lisp::Value("uniform-int-distribution-get", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1){
            throw lisp::exception(env, "syntax", "uniform-int-distribution-get need 1 args");
        }
        lisp::eval_args(args, env);

        auto ptr = lisp::extend<rand_uidg>::ptr_from_value(args[0], env);
        auto &a = ptr->cxx_value;
        return lisp::Value(int(a.gen()));
    }), "<obj> -> int");
    struct rand_discret
    {
        std::mt19937 rd;
        std::discrete_distribution<> distrib;
        int gen(){ return distrib(rd);}
    };
    lisp::global_set( "discrete-distribution", lisp::Value("discrete-distribution", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1 && args.size() != 2){
            throw lisp::exception(env, "syntax", "discrete-distribution need 1|2 args");
        }
        lisp::eval_args(args, env);
        auto ptr = lisp::extend<rand_discret>::make_value();
        auto array = args[0].as_list();
        auto &a = ptr->cxx_value;
        if (args.size() == 2){
            a.rd.seed( args[1].as_int() );
        } else {
            a.rd.seed( std::random_device()() );
        }
        std::vector<int> arr;
        arr.reserve(array.size());
        for (auto &x:array){
            arr.push_back(x.as_int());
        }
        a.distrib = std::discrete_distribution<int>(arr.begin(), arr.end());
        return ptr->value();
    }), "<int-list> [seed] -> obj");
    lisp::global_set( "discrete-distribution-get", lisp::Value("discrete-distribution-get", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1){
            throw lisp::exception(env, "syntax", "discrete-distribution-get need 1 args");
        }
        lisp::eval_args(args, env);

        auto ptr = lisp::extend<rand_discret>::ptr_from_value(args[0], env);
        auto &a = ptr->cxx_value;
        return lisp::Value(int(a.gen()));
    }), "<obj> -> int");
    
    
}

