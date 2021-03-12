
#include "Poco/File.h"
#include "Poco/DateTime.h"


#include "wlisp.hpp"


class w_file:public lisp::user_data{
public:
	std::string path;
	Poco::File file;
	w_file(const std::string &p):path(p),file(p){}

	static lisp::Value make_value(std::vector<lisp::Value> args, lisp::Environment &env) {
		if (args.size() != 1){
			throw lisp::Error(lisp::Value::string("file"), env, "file <path> need 1 arg");
		}
		auto path = args[0].as_string();
		auto ptr = std::make_shared<w_file>(path);
        if (!ptr){
            throw std::runtime_error("no memory");
        }
		lisp::Value ret("", ptr);
		return ret;
	}
    lisp::Value value(){
        auto p = this->shared_from_this();
        return lisp::Value("<file>", p);
    }
    static std::shared_ptr<w_file> ptr_from_value(lisp::Value &v, lisp::Environment &env){
        auto p = std::dynamic_pointer_cast<w_file>(v.as_user_data());
        if (!p){
            throw lisp::Error(lisp::Value::string("ptr casting result is nullptr"), env, "ptr casting result is nullptr");
        }
        return p;
    }
	
	std::string display() const {
		return std::string("<file>") + std::to_string((int64_t)this);
	}
};

void w_file_init()
{
	lisp::global_set( "file", lisp::Value("file", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		return w_file::make_value(args, env);
	}),"string -> file");
	lisp::global_set( "file-path", lisp::Value("file-path", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-path"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		return lisp::Value::string(ptr->file.path());
	}),"file -> string");
	lisp::global_set( "file-exist", lisp::Value("file-exist", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-exist"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		return lisp::Value(int(ptr->file.exists()));
	}),"file -> bool");
	lisp::global_set( "file-can-read", lisp::Value("file-can-read", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-can-read"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		return lisp::Value(int(ptr->file.canRead()));
	}),"file -> bool");
	lisp::global_set( "file-can-write", lisp::Value("file-can-write", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-can-write"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		return lisp::Value(int(ptr->file.canWrite()));
	}),"file -> bool");
	lisp::global_set( "file-can-execute", lisp::Value("file-can-execute", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-can-execute"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		return lisp::Value(int(ptr->file.canExecute()));
	}),"file -> bool");
	lisp::global_set( "file-is-file", lisp::Value("file-is-file", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-is-file"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		return lisp::Value(int(ptr->file.isFile()));
	}),"file -> bool");
	lisp::global_set( "file-is-link", lisp::Value("file-is-link", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-is-link"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		return lisp::Value(int(ptr->file.isLink()));
	}),"file -> bool");
	lisp::global_set( "file-is-directory", lisp::Value("file-is-directory", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-is-directory"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		return lisp::Value(int(ptr->file.isDirectory()));
	}),"file -> bool");
	lisp::global_set( "file-is-hidden", lisp::Value("file-is-hidden", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-is-hidden"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		return lisp::Value(int(ptr->file.isHidden()));
	}),"file -> bool");
	lisp::global_set( "file-get-last-modified", lisp::Value("file-get-last-modified", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-get-last-modified"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		auto time = ptr->file.getLastModified().epochTime();
		return lisp::Value(int(time));
	}), "<file>:file -> time:int");
	lisp::global_set( "file-set-last-modified", lisp::Value("file-set-last-modified", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("file-set-last-modified"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		ptr->file.setLastModified(Poco::Timestamp::fromEpochTime(args[1].as_int()));
		return args[0];
	}), "<file> -> <file>");
	lisp::global_set( "file-get-size", lisp::Value("file-get-size", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-get-size"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		auto size = ptr->file.getSize();
		return lisp::Value(int(size));
	}), "<file>:file -> size:int");
	lisp::global_set( "file-copy-to", lisp::Value("file-copy-to", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("file-copy-to"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		ptr->file.copyTo( args[1].as_string());
		return args[0];
	}), "<file>:file <path>:string -> args[0]:file");
	lisp::global_set( "file-move-to", lisp::Value("file-move-to", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("file-move-to"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		ptr->file.moveTo( args[1].as_string());
		return args[0];
	}), "<file>:file <path>:string -> args[0]:file");
	lisp::global_set( "file-rename-to", lisp::Value("file-rename-to", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("file-rename-to"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		ptr->file.moveTo( args[1].as_string());
		return args[0];
	}), "<file>:file <path>:string -> args[0]:file");
	lisp::global_set( "file-remove", lisp::Value("file-remove", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1 && args.size() != 2) {
			throw lisp::Error(lisp::Value::string("file-remove"), env, "need 1/2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		bool recursive = false;
		if (args.size() == 2 && args[1].as_string() == "recursive"){
			recursive = true;
		}
		ptr->file.remove(recursive);
		return args[0];
	}), "<file>:file ['recursive'] -> args[0]:file");
	lisp::global_set( "file-create-directory", lisp::Value("file-create-directory", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-create-directory"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		return lisp::Value(ptr->file.createDirectory());
	}), "<file>:file -> bool");
	lisp::global_set( "file-create-directories", lisp::Value("file-create-directories", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-create-directories"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		ptr->file.createDirectories();
		return args[0];
	}), "<file>:file -> file");
	
	lisp::global_set( "file-list-directory", lisp::Value("file-list-directory", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("file-list-directory"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_file::ptr_from_value(args[0], env);
		std::vector<std::string> files;
		ptr->file.list(files);
		std::vector<lisp::Value> ret;
		ret.reserve(files.size() + 1);
		for (auto &x:files){
			ret.push_back(lisp::Value::string(x));
		}
		return lisp::Value(ret);
	}), "<file>:file -> filenames:list");
}

