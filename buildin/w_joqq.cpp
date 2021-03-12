
#include <chrono>

#include "jobq.hpp"
#include "wlisp.hpp"
#include "timer-wheel.h"

class w_jobq:public lisp::user_data{
public:
    jobq joq;
    tw::TimerWheel wheel;
	int wheel_tick = 1000;
	uint64_t last_tick = 0;
	std::chrono::system_clock::time_point last = std::chrono::system_clock::now();
	lisp::Environment *p_env = nullptr;

	static lisp::Value make_value(std::vector<lisp::Value> args, lisp::Environment &env) {
		auto ptr = std::make_shared<w_jobq>();
        if (!ptr){
            throw std::runtime_error("no memory");
        }
		if (args.size()){
			ptr->wheel_tick = args[0].as_int();
		}
		lisp::Value ret("", ptr);
		return ret;
	}
    lisp::Value value(){
        auto p = this->shared_from_this();
        return lisp::Value("<jobq>", p);
    }
    static std::shared_ptr<w_jobq> ptr_from_value(lisp::Value &v, lisp::Environment &env){
        auto p = std::dynamic_pointer_cast<w_jobq>(v.as_user_data());
        if (!p){
            throw lisp::Error(lisp::Value::string("ptr casting result is nullptr"), env, "ptr casting result is nullptr");
        }
        return p;
    }
	
	std::string display() const {
		return std::string("<jobq>") + std::to_string((int64_t)this);
	}
};



void w_jobq_init()
{
	lisp::global_set( "jobq", lisp::Value("jobq", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size()!=0 && args.size() != 1) {
			throw lisp::Error(lisp::Value::string("jobq"), env, "need 0/1 args, [timer-wheel-tick]");
		}
		lisp::eval_args(args, env);
		return w_jobq::make_value(args, env);
	}),"<> -> jobq");
	lisp::global_set( "jobq-add", lisp::Value("jobq-add", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("jobq-add"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_jobq::ptr_from_value(args[0], env);
		auto func = args[1];
		ptr->joq.add([func, ptr](){
			auto lambda = func;
			std::vector<lisp::Value> a;
        	lambda.apply(a, *ptr->p_env);
		});
		return args[0];
	}), "<jobq> <lambda()> -> jobq");
	lisp::global_set( "jobq-timer-add", lisp::Value("jobq-timer-add", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 3 && args.size() != 4) {
			throw lisp::Error(lisp::Value::string("jobq-timer-add"), env, "need 3 args: <obj> <func> <tick> [cycle-count]");
		}
		lisp::eval_args(args, env);
		auto ptr = w_jobq::ptr_from_value(args[0], env);
		auto func = args[1];
		int tick = args[2].as_int();
		unsigned int cycle = 1;
		if (args.size() >3 ){
			cycle = args[3].as_int();
		}
		ptr->joq.add([ptr, tick, func, cycle](){
			ptr->wheel.add([func, cycle, ptr](){
				if (ptr->wheel.running_timer->exe_count >= cycle){
					ptr->wheel.running_timer->cancel();
				}
				auto lambda = func;
				std::vector<lisp::Value> a;
				lambda.apply(a, *ptr->p_env);
			}, tick, (cycle>1));
		});
		return args[0];
	}), "<jobq> <lambda()> <tick> -> args0");
	lisp::global_set( "jobq-timer-set", lisp::Value("jobq-timer-set", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("jobq-timer-set"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_jobq::ptr_from_value(args[0], env);
		int tick = args[1].as_int();
		ptr->joq.add([ptr, tick](){
			ptr->wheel_tick = tick;
		});
		return args[0];
	}), "<jobq> <ms> -> args0");
	lisp::global_set( "jobq-timer-cancel-this", lisp::Value("jobq-timer-cancel-this", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("jobq-timer-cancel-this"), env, "need 1 args:<jobq>");
		}
		lisp::eval_args(args, env);
		auto ptr = w_jobq::ptr_from_value(args[0], env);
		int tick = args[1].as_int();
		ptr->joq.add([ptr, tick](){
			if (ptr->wheel.running_timer){
				ptr->wheel.running_timer->cancel();
			}
		});
		return args[0];
	}), "<jobq>");
	
	lisp::global_set( "jobq-poll", lisp::Value("jobq-poll", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("jobq-poll"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_jobq::ptr_from_value(args[0], env);
		ptr->p_env = &env;
		auto now = std::chrono::system_clock::now();
		std::chrono::milliseconds diff( std::chrono::duration_cast<std::chrono::milliseconds>(now - ptr->last));
		uint64_t tick_diff = (diff.count() - ptr->last_tick)/ptr->wheel_tick;
		if (tick_diff > 0){
			ptr->last_tick = diff.count();
			ptr->wheel.advance( tick_diff );
		}
		ptr->joq.poll();
		ptr->p_env = nullptr;
		return args[0];
	}), "<jobq> -> <jobq>");
	
	lisp::global_set( "time", lisp::Value("time", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 0) {
			throw lisp::Error(lisp::Value::string("time"), env, "need 0 args");
		}
		lisp::eval_args(args, env);
		auto now = std::chrono::system_clock::now();
		int seconds = int(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
		return lisp::Value(int(seconds));
	}), "<> -> now");

	
	
}

