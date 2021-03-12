
#include <fstream>

#include "wlisp.hpp"
#include "chd_msg.h"
#include "Poco/String.h"
#include "w_global_auto_init.hpp"
#include "Poco/DigestEngine.h"
#include "Poco/MD5Engine.h"


void __data_init()
{
    lisp::global_set( "clob", lisp::Value("clob", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 0) {
			throw lisp::Error(lisp::Value::string("clob"), env, "need 0 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Data::CLOB>::make_value();
        return ptr->value();
	}), " -> obj:clob");

	lisp::global_set( "md5sum", lisp::Value("md5sum", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		lisp::eval_args(args, env);
        Poco::MD5Engine md;
        for (auto &arg:args)
        {
            if (arg.is_string()){
                md.update(arg.as_string());
            } else {
                auto ptr = lisp::extend<Poco::Data::CLOB>::ptr_from_value(arg, env);
                md.update(ptr->cxx_value.rawContent(), ptr->cxx_value.size());
            }
        }
        auto sum = Poco::MD5Engine::digestToHex(md.digest());
        return lisp::Value::string(sum);
	}), "<clob|string> ... -> string");

	lisp::global_set( "clob-to-string", lisp::Value("clob-to-string", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("clob-to-string"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[0], env);
        return lisp::Value::string(std::string(ptr->cxx_value.rawContent(), ptr->cxx_value.size()));
	}));
	lisp::global_set( "clob-to-list", lisp::Value("clob-to-list", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("clob-to-list"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[0], env);
        std::vector<lisp::Value> list;
        int len = ptr->cxx_value.size();
        list.reserve(len);
        auto data = ptr->cxx_value.rawContent();
        for (int i=0; i<len; i++){
            list.push_back(lisp::Value(int( (unsigned char) data[i] )));
        }
        return lisp::Value(list);
	}));
    lisp::global_set( "clob-assign-value", lisp::Value("clob-assign-value", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 3) {
			throw lisp::Error(lisp::Value::string("clob-assign-value"), env, "need 3 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[0], env);
        int count = args[1].as_int();
        int value = args[2].as_int();
        ptr->cxx_value.assignVal(count, value);
        return args[0];
	}), "<clob> <count> <value-repeated> -> clob");
    lisp::global_set( "clob-clear", lisp::Value("clob-clear", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() < 1) {
			throw lisp::Error(lisp::Value::string("clob-clear"), env, "need >=1 args");
		}
		lisp::eval_args(args, env);
        for (int i=0; i<args.size(); i++){
            auto ptr = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[i], env);
            ptr->cxx_value.clear();
        }
        return args[args.size()-1];
	}), "<clob> ... -> <clob[n-1]>");
    lisp::global_set( "clob-size", lisp::Value("clob-size", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("clob-size"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[0], env);
        return lisp::Value(int(ptr->cxx_value.size()));
	}), "<clob> ... -> size:int");
    
    lisp::global_set( "clob-swap", lisp::Value("clob-swap", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("clob-swap"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
        auto ptr1 = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[0], env);
        auto ptr2 = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[1], env);
        ptr1->cxx_value.swap( ptr2->cxx_value );
        return args[0];
	}), "<clob1> <clob2> -> <clob1>");
    lisp::global_set( "clob-from-clob", lisp::Value("clob-from-clob", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2 && args.size() != 3) {
			throw lisp::Error(lisp::Value::string("clob-from-clob"), env, "need 2|3 args");
		}
		lisp::eval_args(args, env);
        auto ptr1 = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[0], env);
        int offset = args[1].as_int();
        int subsize = ptr1->cxx_value.size();
        if (args.size() == 3){
            int s  = args[2].as_int();
            if (s>0){
                subsize = s;
            }
        }
        int end = subsize + offset;
        if (end > ptr1->cxx_value.size()){ /* too large size */
            subsize = ptr1->cxx_value.size() - offset; /* max privided. */
        }
        auto ptr_new = lisp::extend<Poco::Data::CLOB>::make_value();
        ptr_new->cxx_value.assignRaw(ptr1->cxx_value.rawContent()+ offset, subsize);
        return ptr_new->value();
	}), "<clob> <offset> [subsize] -> <clob-new>");

	lisp::global_set( "clob-append", lisp::Value("clob-append", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() < 2) {
			throw lisp::Error(lisp::Value::string("clob-append"), env, "need >2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[0], env);
        for (int i=1; i<args.size(); i++){
            if (args[i].is_string()){
                auto str = args[i].as_string();
                ptr->cxx_value.appendRaw(str.data(), str.size());
            } else if( args[i].is_number() ){
                char c = args[i].as_int();
                ptr->cxx_value.appendRaw(&c, 1);
            } else if (args[i].is_list() ){
                auto list = args[i].as_list();
                for (auto &li:list){
                    if (!li.is_int()){
                        throw lisp::Error(lisp::Value::string("clob-append"), env, "append list should be integer");
                    }
                    char c = li.as_int();
                    ptr->cxx_value.appendRaw(&c, 1);
                }
            } else{
                auto ptr_rh = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[i], env);
                ptr->cxx_value.appendRaw(ptr_rh->cxx_value.rawContent(), ptr_rh->cxx_value.size());
            }
        }
        
        return args[0];
	}), "<string|clob|int as char|int-list as char list> ... -> clob");

    lisp::global_set( "message-pack", lisp::Value("message-pack", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		lisp::eval_args(args, env);
        int msgid = 0;
        int size = 4096;
        bool header = true;
        if (args.size()){
            size = args[0].as_int();
        }
        auto ptr = lisp::extend<std::shared_ptr<MessageOut>>::make_value();
        ptr->cxx_value = std::make_shared<MessageOut>(MSG_ID_TYPE(msgid), size, header);
        if (!ptr->cxx_value){
              throw lisp::Error(lisp::Value::string("message-pack"), env, "no memory");
        }
        return ptr->value();
	}), "[size <size>] -> <message-pack>");

    lisp::global_set( "message-pack-write", lisp::Value("message-pack-write", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<MessageOut>>::ptr_from_value(args[0], env);
        int n = args.size();
        auto &out = *ptr->cxx_value;
        std::function<void(const lisp::Value &)> write_list;
        write_list = [&write_list,&env,&out](const lisp::Value &v){
            if (v.is_string()){
                out << v.as_string();
            } else if (v.is_list())
            {
                auto list = v.as_list();
                for (auto &x:list){
                    write_list(x);
                }
            } else {
                throw lisp::Error(lisp::Value::string("message-pack-write"), env, "only string supported.");
            }
        };
        for (int i=1; i<n; i++){
            auto &v = args[i];
            write_list(v);
        }
        return args[0];
	}), "<message-pack> [value1] [value2] ... -> <message-pack>");

    lisp::global_set( "message-pack-to-clob", lisp::Value("message-pack-to-clob", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<MessageOut>>::ptr_from_value(args[0], env);
        auto ptr_clob = lisp::extend<Poco::Data::CLOB>::make_value();
        ptr_clob->cxx_value = ptr->cxx_value->clob();
        return ptr_clob->value();
	}), "<message-pack> -> <clob>");

    lisp::global_set( "message-raw-size", lisp::Value("message-raw-size", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        lisp::eval_args(args, env);
        auto ptr_clob = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[0], env);
        MessageIn in(ptr_clob->cxx_value.rawContent(), ptr_clob->cxx_value.size());
        int size = 0;
        auto ok = in.ReadHead();
        if (!ok){
            throw lisp::Error(lisp::Value::string("message-raw-size"), env, "more data needed.");
        }
        size = in.msgLen + CHD_MSG_HEADER_SIZE;
        return lisp::Value(int(size));
	}), "<clob> -> size:int");

    lisp::global_set( "message-unpack", lisp::Value("message-unpack", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        lisp::eval_args(args, env);
        auto ptr_clob = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[0], env);
        MessageIn in(ptr_clob->cxx_value.rawContent(), ptr_clob->cxx_value.size());

        if (!in.ReadHead()){
              throw lisp::Error(lisp::Value::string("message-unpack"), env, "more data needed");
        }
        if (!in.verify()){
            throw lisp::Error(lisp::Value::string("message-unpack"), env, "invalid message");
        }
        std::vector<lisp::Value> vec;
        vec.reserve(64);
        while (in.size() > 0){
            std::string str;
            in >> str;
            vec.push_back(lisp::Value::string(str));
        }
        return lisp::Value(vec);
	}), "<clob> -> <message-unpack>");
}

void __string_init()
{
    lisp::global_set( "string-to-clob", lisp::Value("string-to-clob", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("string-to-clob"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        if (args[0].is_string()){
            auto ptr = lisp::extend<Poco::Data::CLOB>::make_value();
            auto str = args[0].as_string();
            ptr->cxx_value = Poco::Data::CLOB(str);
            return ptr->value();
        } else {
            auto ptr_str = lisp::extend<std::string>::ptr_from_value(args[0], env);
            auto ptr = lisp::extend<Poco::Data::CLOB>::make_value();
            ptr->cxx_value = Poco::Data::CLOB(ptr_str->cxx_value);
            return ptr->value();
        }
	}), "<string> -> <clob>");
 
    lisp::global_set( "string-substring", lisp::Value("string-substring", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 3) {
            throw lisp::Error(lisp::Value::string("string-substring"), env, "need 3 args");
        }
		lisp::eval_args(args, env);
        int offset = args[1].as_int();
        int count = args[2].as_int();
        return lisp::Value::string(args[0].as_string().substr(offset, count));
	}), "<string> <offset> <count> -> <string>");
    lisp::global_set( "string-size", lisp::Value("string-size", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("string-size"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        return lisp::Value(int(args[0].as_string().length()));
	}), "<string> -> size:int");
    
    lisp::global_set( "string-to-list", lisp::Value("string-to-list", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("string-to-list"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        std::string str = args[0].as_string();
        std::vector<lisp::Value> list;
        int len = str.length();
        list.reserve(len);
        for (int i=0; i<len; i++){
            list.push_back(lisp::Value(int( (unsigned char)str[i] )));
        }
        return lisp::Value(list);
	}), "<string> -> list:int-list");
    lisp::global_set( "string-list-to-string", lisp::Value("string-list-to-string", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("string-list-to-string"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        std::vector<char> list_char;
        auto list = args[0].as_list();
        list_char.reserve(list.size());
        for (auto &li:list){
            if (!li.is_number()) {
                throw lisp::Error(lisp::Value::string("string-list-to-string"), env, "only int can be cast to string");
            }
            int v = li.as_int();
            if (v < 0 || v > 255){
                throw lisp::Error(lisp::Value::string("string-list-to-string"), env, "out of range when casting to string");
            }
            list_char.push_back(v);
        }
        std::string str( &list_char[0], list_char.size() );
        return lisp::Value::string(str);
	}), "<int-list> -> string");
#define DEF_STRING_FUNC_1(name, calling) \
    lisp::global_set( name, lisp::Value(name, [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {\
        int n = args.size();\
        if (n != 1) {\
            throw lisp::Error(lisp::Value::string(name), env, "need 1 args");\
        }\
		lisp::eval_args(args, env);\
        return lisp::Value::string(calling(args[0].as_string()));\
	}), "<string> -> obj:string")

    DEF_STRING_FUNC_1("string-trim-left", Poco::trimLeft );
    DEF_STRING_FUNC_1("string-trim-right", Poco::trimRight );
    DEF_STRING_FUNC_1("string-trim", Poco::trim );
    DEF_STRING_FUNC_1("string-to-upper", Poco::toUpper );
    DEF_STRING_FUNC_1("string-to-lower", Poco::toLower );
}

#include "Poco/FIFOBuffer.h"
#include "Poco/BasicEvent.h"
#include "Poco/Delegate.h"



void fifo_buffer_init()
{
    lisp::global_set( "fifo-buffer", lisp::Value("fifo-buffer", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("fifo-buffer"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        int size = args[0].as_int();
        bool notify = false;
        auto ptr = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::make_value();
        ptr->cxx_value = std::make_shared<Poco::FIFOBuffer>(size, notify);
        if (!ptr->cxx_value){
            throw lisp::Error(lisp::Value::string("fifo-buffer"), env, "no memory");
        }
        return ptr->value();
	}), "<size> -> <obj>:fifo-buffer (thread safety)");
    lisp::global_set( "fifo-buffer-from-clob", lisp::Value("fifo-buffer-from-clob", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("fifo-buffer-from-clob"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        auto ptr_clob = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[0], env);
        auto ptr = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::make_value();
        ptr->cxx_value = std::make_shared<Poco::FIFOBuffer>(ptr_clob->cxx_value.size(), false);
        if (!ptr->cxx_value){
            throw lisp::Error(lisp::Value::string("fifo-buffer"), env, "no memory");
        }
        int w = ptr->cxx_value->write(ptr_clob->cxx_value.rawContent(), ptr_clob->cxx_value.size());
        if (w != ptr_clob->cxx_value.size()){
            throw lisp::Error(lisp::Value::string("fifo-buffer-from-clob"), env, "copy failed.");
        }
        return ptr->value();
	}), "<clob> -> <obj>:fifo-buffer (thread safety)");

    lisp::global_set( "fifo-buffer-resize", lisp::Value("fifo-buffer-resize", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 2) {
            throw lisp::Error(lisp::Value::string("fifo-buffer-resize"), env, "need 2 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::ptr_from_value(args[0], env);
        int size = args[1].as_int();
        ptr->cxx_value->resize(size);
        return args[0];
	}), "<fifo-buffer> <size> -> <obj>:fifo-buffer");
    
    lisp::global_set( "fifo-buffer-size", lisp::Value("fifo-buffer-size", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("fifo-buffer-size"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::ptr_from_value(args[0], env);
        return lisp::Value(int(ptr->cxx_value->size()));
	}), "<fifo-buffer> -> size:int");
    lisp::global_set( "fifo-buffer-used", lisp::Value("fifo-buffer-used", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("fifo-buffer-used"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::ptr_from_value(args[0], env);
        return lisp::Value(int(ptr->cxx_value->used()));
	}), "<fifo-buffer> -> used:int");
    lisp::global_set( "fifo-buffer-available", lisp::Value("fifo-buffer-available", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("fifo-buffer-available"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::ptr_from_value(args[0], env);
        return lisp::Value(int(ptr->cxx_value->available()));
	}), "<fifo-buffer> -> available:int");

    lisp::global_set( "fifo-buffer-peek", lisp::Value("fifo-buffer-peek", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1 && n != 2) {
            throw lisp::Error(lisp::Value::string("fifo-buffer-peek"), env, "need 1|2 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::ptr_from_value(args[0], env);
        int used = ptr->cxx_value->used();
        int size = 0;
        if (n == 2){
            size = args[1].as_int();
        }
        if (size == 0) {
            size = used;
        } else if ( size > used ){
            size = used;
        }
        auto ptr_clob = lisp::extend<Poco::Data::CLOB>::make_value();
        ptr_clob->cxx_value.assignRaw(ptr->cxx_value->begin(), size);
        return ptr_clob->value();
	}), "<fifo-buffer> [size] -> <new-obj>:clob");

    lisp::global_set( "fifo-buffer-read", lisp::Value("fifo-buffer-read", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1 && n != 2) {
            throw lisp::Error(lisp::Value::string("fifo-buffer-read"), env, "need 1|2 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::ptr_from_value(args[0], env);
        int size = 0;
        if (n == 2){
            size = args[1].as_int();
        }
        auto ptr_clob = lisp::extend<Poco::Data::CLOB>::make_value();
        int max_size = ptr->cxx_value->size();
        std::shared_ptr<char> buf = std::shared_ptr<char>(new char [max_size], [](char *b){ delete [] b;});
        if (size == 0){
            size = max_size;
        }
        int count = ptr->cxx_value->read(buf.get(), size);
        ptr_clob->cxx_value.assignRaw(buf.get(), count);
        return ptr_clob->value();
	}), "<fifo-buffer> [size] -> <new-obj>:clob");
    lisp::global_set( "fifo-buffer-drain", lisp::Value("fifo-buffer-drain", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1 && n != 2) {
            throw lisp::Error(lisp::Value::string("fifo-buffer-drain"), env, "need 1|2 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::ptr_from_value(args[0], env);
        int size = 0;
        if (n == 2){
            size = args[1].as_int();
        }
        ptr->cxx_value->drain(size);
        return args[0];
	}), "<fifo-buffer> [size] -> <args[0]>:fifo-buffer");

    lisp::global_set( "fifo-buffer-write", lisp::Value("fifo-buffer-write", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 2) {
            throw lisp::Error(lisp::Value::string("fifo-buffer-write"), env, "need 2 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::ptr_from_value(args[0], env);
        auto ptr_clob = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[1], env);
        int written = ptr->cxx_value->write(ptr_clob->cxx_value.rawContent(), ptr_clob->cxx_value.size());
        return lisp::Value(written);
	}), "<fifo-buffer> <clob> -> size-written:int");

    lisp::global_set( "ofstream", lisp::Value("ofstream", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 2 && n != 1) {
            throw lisp::exception(env, "syntax", "ofstream need 2 args");
        }
		lisp::eval_args(args, env);
        std::ios_base::openmode mode = std::ios_base::out;
        if (n == 2){
            auto vec = args[1].as_list();
            for (auto &x:vec){
                auto m = x.as_string();
                if (m == "binary"){
                    mode |= std::ios_base::binary;
                } else if (m == "append"){
                    mode |= std::ios_base::app;
                } else if (m == "truncate"){
                    mode |= std::ios_base::trunc;
                }
            }
        }
        auto ptr = lisp::extend<std::shared_ptr<std::ofstream>>::make_value();
        ptr->cxx_value =   std::make_shared<std::ofstream>(args[0].as_string(), mode);
        if (!ptr->cxx_value ){
            throw lisp::exception(env, "os", "ofstream no memory");
        }
        if (!ptr->cxx_value->is_open()){
            throw lisp::exception(env, "fs", "ofstream ");
        }
        return ptr->value();
	}), "<filename> [mode] -> [ofstream]; mode:binary&[append|truncate]");
    lisp::global_set( "ofstream-write", lisp::Value("ofstream-write", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n < 2) {
            throw lisp::exception(env, "syntax", "ofstream-write need >=2 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<std::ofstream>>::ptr_from_value(args[0], env);
        auto &stream = *ptr->cxx_value;
        if (!stream.good()){
            throw lisp::exception(env, "fs", "ofstream-write ofstream not good");
        }
        for (int i=1; i<n; i++){
            if (args[i].is_string()){
                stream << args[i].as_string();
            } else if (args[i].is_int()){
                stream << args[i].as_int();
            } else if (args[i].is_float()){
                stream << args[i].as_float();
            } else if (args[i].is_lambda()){
                stream << args[i].display();
            } else if (args[i].is_userdata()){
                auto ptr_clob = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[i], env);
                auto &clob = ptr_clob->cxx_value;
                stream.write(clob.rawContent(), clob.size());
            }
        }
        return args[n-1];
	}), "<ofstream> <content> ... -> <last-arg> ");
    
    lisp::global_set( "ofstream-close", lisp::Value("ofstream-close", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::exception(env, "syntax", "ofstream-close need 1 args");
        }
		lisp::eval_args(args, env);
        
        auto ptr = lisp::extend<std::shared_ptr<std::ofstream>>::ptr_from_value(args[0], env);
        auto &stream = *ptr->cxx_value;
        if (!stream.good()){
            throw lisp::exception(env, "fs", "ofstream-write ofstream not good");
        }
        stream.close();
        return args[0];
	}), "<ofstream> -> <arg0> ");

}
#include "Poco/JSON/Object.h"
#include "Poco/JSON/Array.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Query.h"
static
void __json_obj_set(Poco::JSON::Object::Ptr &p, const std::vector<lisp::Value> &args, lisp::Environment& env);
static
void __json_array_add(Poco::JSON::Array::Ptr& p, const std::vector<lisp::Value>& args, lisp::Environment& env);

Poco::JSON::Object::Ptr __dict_to_json_obj(const lisp::Value &dict, lisp::Environment &env)
{
    Poco::JSON::Object::Ptr p = new Poco::JSON::Object();
    if (!p){
        throw lisp::exception(env, "os", "no memory for json");
    }
    auto ptr = lisp::extend<std::map<std::string,lisp::Value>>::ptr_from_value(dict, env);
    auto &m = ptr->cxx_value;
    for (auto &x:m){
        std::vector<lisp::Value> args;
        args.reserve(2);
        args.push_back(lisp::Value::string(x.first));
        args.push_back(x.second);
        __json_obj_set( p, args, env );
    }
    return p;
}

static
void __json_obj_set(Poco::JSON::Object::Ptr &p, const std::vector<lisp::Value> &args, lisp::Environment& env)
{
    int n = args.size();
    
    auto &obj = *p;
    for (int i=0; i+1<n; i+= 2)
    {
        std::string key = args[i].as_string();
        auto &v = args[i+1];
        if (v.is_string()){
            obj.set(key, v.as_string());
        } else if (v.is_int()){
            obj.set(key, v.as_int());
        } else if (v.is_float()){
            obj.set(key, v.as_float());
        } else if (v.is_nil()){
            obj.set(key, Poco::Nullable<int>());
        } else if (v.is_userdata()){
            if (lisp::user_data_type_match<Poco::JSON::Object::Ptr>(v)){
                auto ptr = lisp::extend<Poco::JSON::Object::Ptr>::ptr_from_value(v,env);
                obj.set(key, ptr->cxx_value );
            } else if (lisp::user_data_type_match<Poco::JSON::Array::Ptr>(v)){
                auto ptr = lisp::extend<Poco::JSON::Array::Ptr>::ptr_from_value(v, env);
                obj.set(key, ptr->cxx_value );
            } else if (lisp::user_data_type_match<std::map<std::string,lisp::Value>>(v)){
                obj.set(key, __dict_to_json_obj(v, env) );
            }
            else {
                throw lisp::exception(env, "syntax", "json not supported user type");
            }
        } else if (v.is_list()){
            auto list = v.as_list();
            Poco::JSON::Array::Ptr p = new Poco::JSON::Array();
            if (!p){
                throw lisp::exception(env, "os", "no memory for json");
            }
            __json_array_add(p, list, env);
            obj.set(key, p);
        }
    }
};
static
void __json_array_add(Poco::JSON::Array::Ptr &p, const std::vector<lisp::Value> &args, lisp::Environment& env)
{
    int n = args.size();
    
    auto &obj = *p;
    for (int i=0; i<n; i++)
    {
        auto &v = args[i];
        if (args[i].is_string()){
            obj.add(v.as_string());
        } else if (v.is_int()){
            obj.add(v.as_int());
        } else if (v.is_float()){
            obj.add(v.as_float());
        } else if (v.is_nil()){
            obj.add(Poco::Nullable<int>());
        } else if (v.is_userdata()){
            if (lisp::user_data_type_match<Poco::JSON::Object::Ptr>(v)){
                auto ptr = lisp::extend<Poco::JSON::Object::Ptr>::ptr_from_value(v,env);
                obj.add( ptr->cxx_value );
            } else if (lisp::user_data_type_match<Poco::JSON::Array::Ptr>(v)){
                auto ptr = lisp::extend<Poco::JSON::Array::Ptr>::ptr_from_value(v, env);
                obj.add( ptr->cxx_value );
            } else {
                throw lisp::exception(env, "syntax", "json not supported user type");
            }
        } else if (v.is_list()){
            auto list = v.as_list();
            Poco::JSON::Array::Ptr p = new Poco::JSON::Array();
            if (!p){
                throw lisp::exception(env, "os", "no memory for json");
            }
            __json_array_add(p, list, env);
            obj.add( p );
        }
    }
};

lisp::Value __var_to_value(const Poco::Dynamic::Var &var)
{
    if (var.type() == typeid(Poco::JSON::Array::Ptr)){
        auto arr = var.extract<Poco::JSON::Array::Ptr>();
        auto ptr = lisp::extend<Poco::JSON::Array::Ptr>::make_value();
        ptr->cxx_value = arr;
        return ptr->value();
    } else if (var.type() == typeid(Poco::JSON::Object::Ptr))
    {
        auto obj = var.extract<Poco::JSON::Object::Ptr>();
        auto ptr = lisp::extend<Poco::JSON::Object::Ptr>::make_value();
        ptr->cxx_value = obj;
        return ptr->value();
    }
        else if (var.isString())
    {
        return lisp::Value::string(var.extract<std::string>());
    } else if (var.isInteger() || var.isBoolean()){
        return lisp::Value(int(var.extract<int>()));
    } else if (var.isNumeric()){
        return lisp::Value(var.extract<double>());
    } else if (var.isEmpty()){
        return lisp::Value::nil();
    } else if (var.isDateTime()){
        auto dt = var.extract<Poco::DateTime>();
        return lisp::Value::string(Poco::DateTimeFormatter::format(dt, Poco::DateTimeFormat::SORTABLE_FORMAT));
    } else if (var.isDate()){
        return lisp::Value::nil();
    }
    return lisp::Value::nil();
}

void w_data_json()
{
    lisp::global_set( "json-object", lisp::Value("json-object", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n % 2) {
            throw lisp::Error(lisp::Value::string("json-object"), env, "need 2n args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<Poco::JSON::Object::Ptr>::make_value();
        Poco::JSON::Object::Ptr p = new Poco::JSON::Object();
        if (!p){
            throw lisp::exception(env, "os", "no memory for json");
        }
        __json_obj_set(p, args, env);
        ptr->cxx_value = p;
        return ptr->value();
	}), "[<key:string> <value>] [...] -> <json-object>");
    
    lisp::global_set( "dict-to-json", lisp::Value("dict-to-json", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("dict-to-json"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        auto pv  = lisp::extend<Poco::JSON::Object::Ptr>::make_value();
        pv->cxx_value = __dict_to_json_obj(args[0], env);
        return pv->value();
	}), "<dict> -> <json-object>");

#if 0
    lisp::global_set( "json-to-dict", lisp::Value("json-to-dict", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("json-to-dict"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<Poco::JSON::Object::Ptr>::ptr_from_value(args[0], env);
        auto json_ptr = ptr->cxx_value;
        auto pv = lisp::extend<std::map<std::string, lisp::Value>>::make_value();
        auto &m = pv->cxx_value;
        std::function<void(Poco::Dynamic::Var &dv, std::map<std::string,lisp::Value> &m)> func;
        func = [&func](Poco::Dynamic::Var &dv, std::map<std::string,lisp::Value> &m){

        };
        for (auto &x:*json_ptr){
            auto &second = x.second;
            if (second.isArray()){
                
            } else if (second.isString())
            {
                m[x.first] = lisp::Value::string(second.extract<std::string>());
            } else if (second.isInteger() || second.isBoolean()){
                m[x.first] = lisp::Value(int(second.extract<int>()));
            } else if (second.isNumeric()){
                m[x.first] = lisp::Value(second.extract<double>());
            } else if (second.isEmpty()){
            } else if (second.isDateTime()){
                auto dt = second.extract<Poco::DateTime>();
                m[x.first] = lisp::Value::string(Poco::DateTimeFormatter::format(dt, Poco::DateTimeFormat::SORTABLE_FORMAT));
            } else if (second.isDate()){
                throw lisp::exception(env, "sys", "not support json type convertion");
            }
        }
        return pv->value();
	}), "<dict> -> <json-object>");
#endif
    
    lisp::global_set( "json-object-keys", lisp::Value("json-object-keys", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("json-object-keys"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<Poco::JSON::Object::Ptr>::ptr_from_value(args[0], env);
        auto p = ptr->cxx_value;
        std::vector<std::string> keys;
        p->getNames(keys);
        std::vector<lisp::Value> kvs;
        kvs.reserve(keys.size());
        for (auto &k:keys){
            kvs.push_back(lisp::Value::string(k));
        }
        return lisp::Value(kvs);
	}), "<json-object> -> keynames:string-list");

    lisp::global_set( "json-object-set", lisp::Value("json-object-set", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n <= 1 || ((n-1) % 2)) {
            throw lisp::Error(lisp::Value::string("json-object-set"), env, "need 2n+1 args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<Poco::JSON::Object::Ptr>::ptr_from_value(args[0], env);
        auto p = ptr->cxx_value;
        args.erase(args.begin());
        __json_obj_set(p, args, env);
        return ptr->value();
	}), "<json-object> <key:string> <value> [...] -> <json-object>");
    
    lisp::global_set( "json-array", lisp::Value("json-array", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<Poco::JSON::Array::Ptr>::make_value();
        Poco::JSON::Array::Ptr p = new Poco::JSON::Array();
        if (!p){
            throw lisp::exception(env, "os", "no memory for json");
        }
        __json_array_add(p, args, env);
        ptr->cxx_value = p;
        return ptr->value();
	}), "<value> [<value> ...] -> <json-array>");
    lisp::global_set( "json-merge", lisp::Value("json-merge", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
		lisp::eval_args(args, env);
        if (lisp::user_data_type_match<Poco::JSON::Array::Ptr>(args[0])){
            auto ptr = lisp::extend<Poco::JSON::Array::Ptr>::ptr_from_value(args[0], env);
            auto &p = ptr->cxx_value;
            for (int i=1; i<n; i++){
                auto &v = args[i];
                if (v.is_list()){
                    auto list = v.as_list();
                    __json_array_add(p, list, env);
                } else if (lisp::user_data_type_match<Poco::JSON::Array::Ptr>(v)){
                    auto pnew = lisp::extend<Poco::JSON::Array::Ptr>::ptr_from_value(v, env);
                    auto &array = *pnew->cxx_value;
                    for (auto &x:array){
                        p->add(x);
                    }
                } else {
                    std::vector<lisp::Value> ags;
                    ags.push_back(v);
                    __json_array_add(p, ags, env);
                }
            }
        } else {
            auto ptr = lisp::extend<Poco::JSON::Object::Ptr>::ptr_from_value(args[0], env);
            auto &p = ptr->cxx_value;
            for (int i=1; i<n; i++){
                auto &v = args[i];
                if (lisp::user_data_type_match<Poco::JSON::Object::Ptr>(v)){
                    auto pnew = lisp::extend<Poco::JSON::Object::Ptr>::ptr_from_value(v, env);
                    auto & obj = *pnew->cxx_value;
                    for (auto &x:obj){
                        p->set(x.first, x.second);
                    }
                } else {
                    throw lisp::exception(env, "syntax", "json-merge object to object supported only.");
                }
                
            }
        }
        return args[0];
	}), "<json-array> <value> [<value> ...] -> <json-array>");

    lisp::global_set( "json-array-add", lisp::Value("json-array-add", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n < 1) {
            throw lisp::Error(lisp::Value::string("json-array-add"), env, "need more args");
        }
		lisp::eval_args(args, env);
        auto ptr = lisp::extend<Poco::JSON::Array::Ptr>::ptr_from_value(args[0], env);
        Poco::JSON::Array::Ptr p = ptr->cxx_value;
        args.erase(args.begin());
        __json_array_add(p, args, env);
        return ptr->value();
	}), "<json-array> <value> [ <value> ...] -> <json-array>");

    lisp::global_set( "json-parse", lisp::Value("json-parse", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("json-parse"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        std::string json = args[0].as_string();
        auto var = Poco::JSON::Parser().parse(json);
        if (var.isEmpty()){
            return lisp::Value::nil();
        } else if (var.isArray()) {
            auto ptr_array = var.extract<Poco::JSON::Array::Ptr>();
            auto pl = lisp::extend<Poco::JSON::Array::Ptr>::make_value();
            pl->cxx_value = ptr_array;
            return pl->value();
        } else {
            auto ptr_obj = var.extract<Poco::JSON::Object::Ptr>();
            auto pl = lisp::extend<Poco::JSON::Object::Ptr>::make_value();
            pl->cxx_value = ptr_obj;
            return pl->value();
        }
	}), "<json-string> -> <json-object|json-array>");

    
    lisp::global_set( "json-stringify", lisp::Value("json-stringify", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1 && n != 2) {
            throw lisp::Error(lisp::Value::string("json-stringify"), env, "need 1|2 args");
        }
		lisp::eval_args(args, env);
        auto &v = args[0];
        int indent = 0;
        if (n == 2){
            indent = args[1].as_int();
        }
        std::stringstream ss;
        if (lisp::user_data_type_match<Poco::JSON::Object::Ptr>(v)){
            auto ptr = lisp::extend<Poco::JSON::Object::Ptr>::ptr_from_value(v, env);
            ptr->cxx_value->stringify(ss, indent);
        } else if (lisp::user_data_type_match<Poco::JSON::Array::Ptr>(v)){
            auto ptr = lisp::extend<Poco::JSON::Array::Ptr>::ptr_from_value(v, env);
            ptr->cxx_value->stringify(ss, indent);
        } else {
            throw lisp::exception(env, "syntax", "invalaid type");
        }
        return lisp::Value::string(ss.str());
	}), "<json-object|json-array> [ <indent> ] -> <string>");
    
    lisp::global_set( "json-find", lisp::Value("json-find", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 2) {
            throw lisp::Error(lisp::Value::string("json-query-find"), env, "need 2 args");
        }
		lisp::eval_args(args, env);
        auto &v = args[0];
        Poco::Dynamic::Var var;
        std::string path(args[1].as_string() );
        if (lisp::user_data_type_match<Poco::JSON::Object::Ptr>(v)){
            auto ptr = lisp::extend<Poco::JSON::Object::Ptr>::ptr_from_value(v, env);
            Poco::JSON::Query query(ptr->cxx_value);
            var = query.find(path);
        } else if (lisp::user_data_type_match<Poco::JSON::Array::Ptr>(v)){
            auto ptr = lisp::extend<Poco::JSON::Array::Ptr>::ptr_from_value(v, env);
            Poco::JSON::Query query(ptr->cxx_value);
            var = query.find(path);
        } else {
            throw lisp::exception(env, "syntax", "invalaid type");
        }
        return __var_to_value(var);
	}), "<json-object|json-array> <path:string> -> <nil|string|json-object|json-array|int|float>");

    lisp::global_set( "json-array-to-list", lisp::Value("json-array-to-list", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("json-query-to-list"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        auto &v = args[0];
        auto ptr = lisp::extend<Poco::JSON::Array::Ptr>::ptr_from_value(v, env);
        std::vector<lisp::Value> result;
        result.reserve(ptr->cxx_value->size());
        for (auto &var:*ptr->cxx_value)
        {
            result.push_back(__var_to_value(var));
        }
        return lisp::Value(result);
	}), "<json-array> -> <list>");

    
}

#include "db/cdbi.h"
#include "Poco/Data/RecordSet.h"

void w_data_mysql()
{
    lisp::global_set( "mysql-pool-init", lisp::Value("mysql-pool-init", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 1) {
            throw lisp::Error(lisp::Value::string("mysql-pool-init"), env, "need 1 args");
        }
		lisp::eval_args(args, env);
        cdbi::init(args[0].as_string());
        return lisp::Value::nil();
	}), "<connection-string> -> nil");
    
    lisp::global_set( "mysql-get-session", lisp::Value("mysql-get-session", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n != 0) {
            throw lisp::Error(lisp::Value::string("mysql-get-session"), env, "need 0 args");
        }
        auto ptr = lisp::extend<std::shared_ptr<Poco::Data::Session>>::make_value();
        ptr->cxx_value = std::make_shared<Poco::Data::Session>(cdbi::get().pool().get());
        return ptr->value();
	}), "<> -> <mysql-session>");
    
    lisp::global_set( "mysql-execute", lisp::Value("mysql-execute", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        int n = args.size();
        if (n < 2) {
            throw lisp::Error(lisp::Value::string("mysql-execute"), env, "need >=2 args");
        }
        lisp::eval_args(args, env);
        auto ptr = lisp::extend<std::shared_ptr<Poco::Data::Session>>::ptr_from_value(args[0], env);
        auto sql = args[1].as_string();
        bool has_lambda = false;
        lisp::Value lambda;
        auto sec = *ptr->cxx_value;
        auto stmt = (sec << sql);
        for (int i=2+(has_lambda); i<n; i++){
            if (args[i].is_string()){
                std::string var = args[i].as_string();
                stmt, Poco::Data::Keywords::useRef(var);
            } else if (args[i].is_int()){
                int var = args[i].as_int();
                stmt, Poco::Data::Keywords::useRef(var);
            } else if (args[i].is_float()){
                double var = args[i].as_float();
                stmt, Poco::Data::Keywords::useRef(var);
            } else if (args[i].is_userdata()){
                auto ptr = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[i], env);
                stmt, Poco::Data::Keywords::useRef(ptr->cxx_value);
            } else if (args[i].is_lambda()){
                if (has_lambda){
                    throw lisp::exception(env, "syntax", "only one lambda can be used");
                }
                has_lambda = true;
                lambda = args[i];
            } else {
                throw lisp::exception(env, "syntax", "invalid type");
            }
        }
        stmt.execute();
        Poco::Data::RecordSet rs(stmt);
        std::vector<lisp::Value> result;
        result.reserve(rs.rowCount() + 1);
        std::vector<lisp::Value> names,types;
        names.reserve(rs.columnCount());
        types.reserve(rs.columnCount());
        for (int i=0; i<rs.columnCount(); i++)
        {
            names.push_back( lisp::Value::string( rs.columnName(i) ) );
            switch (rs.columnType(i))
            {
                case Poco::Data::MetaColumn::FDT_BOOL:       types.push_back( lisp::Value::string("BOOL") );break;
                case Poco::Data::MetaColumn::FDT_INT8:       types.push_back( lisp::Value::string("INT8") );break;
                case Poco::Data::MetaColumn::FDT_UINT8:      types.push_back( lisp::Value::string("UINT8") );break;
                case Poco::Data::MetaColumn::FDT_INT16:      types.push_back( lisp::Value::string("INT16") );break;
                case Poco::Data::MetaColumn::FDT_UINT16:     types.push_back( lisp::Value::string("UINT16") );break;
                case Poco::Data::MetaColumn::FDT_INT32:      types.push_back( lisp::Value::string("INT32") );break;
                case Poco::Data::MetaColumn::FDT_UINT32:     types.push_back( lisp::Value::string("UINT32") );break;
                case Poco::Data::MetaColumn::FDT_INT64:      types.push_back( lisp::Value::string("INT64") );break;
                case Poco::Data::MetaColumn::FDT_UINT64:     types.push_back( lisp::Value::string("UINT64") );break;
                case Poco::Data::MetaColumn::FDT_FLOAT:      types.push_back( lisp::Value::string("FLOAT") );break;
                case Poco::Data::MetaColumn::FDT_DOUBLE:     types.push_back( lisp::Value::string("DOUBLE") );break;
                case Poco::Data::MetaColumn::FDT_STRING:     types.push_back( lisp::Value::string("STRING") );break;
                case Poco::Data::MetaColumn::FDT_WSTRING:    types.push_back( lisp::Value::string("WSTRING") );break;
                case Poco::Data::MetaColumn::FDT_BLOB:       types.push_back( lisp::Value::string("BLOB") );break;
                case Poco::Data::MetaColumn::FDT_CLOB:       types.push_back( lisp::Value::string("CLOB") );break;
                case Poco::Data::MetaColumn::FDT_DATE:       types.push_back( lisp::Value::string("DATE") );break;
                case Poco::Data::MetaColumn::FDT_TIME:       types.push_back( lisp::Value::string("TIME") );break;
                case Poco::Data::MetaColumn::FDT_TIMESTAMP:  types.push_back( lisp::Value::string("TIMESTAMP") );break;
                case Poco::Data::MetaColumn::FDT_UNKNOWN:    types.push_back( lisp::Value::string("UNKNOWN") );break;
            }
        }
        result.push_back(lisp::Value(names));
        result.push_back(lisp::Value(types));
        std::vector<lisp::Value> lambda_args;
        lambda_args.reserve(rs.rowCount());
        int row_index = 0;
        for (auto &row:rs){
            std::vector<lisp::Value> list;
            for (int i=0; i<rs.columnCount(); i++)
            {
                lisp::Value the_value = lisp::Value::nil();
                if (row.get(i).isEmpty()){
                    list.push_back(lisp::Value::nil());
                    continue;
                } else {
                    switch (rs.columnType(i))
                    {
                        case Poco::Data::MetaColumn::FDT_BOOL:       the_value = (lisp::Value(int(row.get(i).extract<bool>())));break;
                        case Poco::Data::MetaColumn::FDT_INT8:       the_value = (lisp::Value(int(row.get(i).extract<int8_t>())));break;
                        case Poco::Data::MetaColumn::FDT_UINT8:      the_value = (lisp::Value(int(row.get(i).extract<uint8_t>())));break;
                        case Poco::Data::MetaColumn::FDT_INT16:      the_value = (lisp::Value(int(row.get(i).extract<int16_t>())));break;
                        case Poco::Data::MetaColumn::FDT_UINT16:     the_value = (lisp::Value(int(row.get(i).extract<uint16_t>())));break;
                        case Poco::Data::MetaColumn::FDT_INT32:      the_value = (lisp::Value(int(row.get(i).extract<int32_t>()))); break;
                        case Poco::Data::MetaColumn::FDT_UINT32:     the_value = (lisp::Value(int(row.get(i).extract<uint32_t>()))); break;
                        case Poco::Data::MetaColumn::FDT_INT64:      the_value = (lisp::Value(int(row.get(i).extract<int64_t>()))); break;
                        case Poco::Data::MetaColumn::FDT_UINT64:     the_value = (lisp::Value(int(row.get(i).extract<uint64_t>()))); break;
                        case Poco::Data::MetaColumn::FDT_FLOAT:     
                        case Poco::Data::MetaColumn::FDT_DOUBLE:     the_value = (lisp::Value(row.get(i).extract<double>())); break;
                        case Poco::Data::MetaColumn::FDT_STRING:     the_value = (lisp::Value::string(row.get(i).extract<std::string>()));break;
                        case Poco::Data::MetaColumn::FDT_WSTRING:    throw lisp::exception(env, "", "extract type not supprted \"WSTRING\""); break;
                        case Poco::Data::MetaColumn::FDT_BLOB:       //throw lisp::exception(env, "", "extract type not supprted \"BLOB\""); break;
                        case Poco::Data::MetaColumn::FDT_CLOB:       {
                            auto ptr = lisp::extend<Poco::Data::CLOB>::make_value();
                            ptr->cxx_value = row.get(i).extract<Poco::Data::CLOB>();
                            the_value = ptr->value();
                            }break;
                        case Poco::Data::MetaColumn::FDT_DATE:       //throw lisp::exception(env, "", "extract type not supprted \"DATE\""); break;
                        case Poco::Data::MetaColumn::FDT_TIME:       //throw lisp::exception(env, "", "extract type not supprted \"TIME\""); break;
                        case Poco::Data::MetaColumn::FDT_TIMESTAMP:  {
                            auto ts = row.get(i).extract<Poco::DateTime>();
                            the_value = (lisp::Value::string(Poco::DateTimeFormatter::format(ts, Poco::DateTimeFormat::SORTABLE_FORMAT)));
                        }; break;
                        case Poco::Data::MetaColumn::FDT_UNKNOWN:    throw lisp::exception(env, "", "extract type not supprted \"UNKOWN\""); break;
                    }
                }
                list.push_back(the_value);
            }
            
            if (has_lambda){
                lambda_args.clear();
                lambda_args.push_back(lisp::Value(list));
                lambda_args.push_back(lisp::Value(int(row_index)));
                lambda_args.push_back(lisp::Value(int(rs.rowCount())));
                lambda_args.push_back(lisp::Value(names));
                lambda_args.push_back(lisp::Value(types));
                if (!lambda.apply(lambda_args,  env).as_bool()){
                    return lisp::Value(int(row_index));
                }
            } else {
                result.push_back(lisp::Value(list));
            }
        }
        return lisp::Value(result);
	}), "<mysql-session> <sql:string> [sql-args] <lambda(one-row:list row-index row-count colum-names colum-types) continue:bool> -> <lists>;");

}