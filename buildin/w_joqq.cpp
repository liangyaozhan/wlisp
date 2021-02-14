
#include "jobq.hpp"
#include "wlisp.hpp"


class w_jobq:public lisp::user_data{
public:
    jobq joq;

	static lisp::Value make_value(lisp::Environment &env) {
		auto ptr = std::make_shared<w_jobq>();
        if (!ptr){
            throw std::runtime_error("no memory");
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
		if (args.size()!=0) {
			throw lisp::Error(lisp::Value::string("jobq"), env, "need 0 args");
		}
		lisp::eval_args(args, env);
		return w_jobq::make_value(env);
	}));
	lisp::global_set( "jobq-add", lisp::Value("jobq-add", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("jobq-add"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_jobq::ptr_from_value(args[0], env);
		auto func = args[1];
		ptr->joq.add([func, &env](){
			auto lambda = func;
			std::vector<lisp::Value> a;
        	lambda.apply(a, env);
		});
		return args[0];
	}));
	
	lisp::global_set( "jobq-poll", lisp::Value("jobq-poll", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("jobq-poll"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_jobq::ptr_from_value(args[0], env);
		ptr->joq.poll();
		return args[0];
	}));
	
}

