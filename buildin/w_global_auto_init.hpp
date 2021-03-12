
#include <string>
#include <functional>
#pragma once

#include "wlisp.hpp"

class global_auto_init
{
public:

     global_auto_init(std::string name, std::function<void()> f);


};

#define DEF_MODULE(name,initfunc) global_auto_init g_##initfunc(name,initfunc)

inline
std::shared_ptr<lisp::extend<std::string>> ensure_lisp_string( const lisp::Value &v, lisp::Environment &env )
{
    if (v.is_string()){
        auto ptr = lisp::extend<std::string>::make_value();
        ptr->cxx_value = v.as_string();
        return ptr;
    } else {
        auto ptr = lisp::extend<std::string>::ptr_from_value(v, env);
        return ptr;
    }
}


void module_init_all();




