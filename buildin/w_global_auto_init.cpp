
#include <iostream>

#include <map>

#include "w_global_auto_init.hpp"


static std::map<std::string, std::function<void()>> *g_module_init_funcs = 0;


global_auto_init::global_auto_init(std::string name, std::function<void()> f)
{
    if (!g_module_init_funcs){
        g_module_init_funcs = new std::map<std::string, std::function<void()>>();
    }
    (*g_module_init_funcs)[name] = f;
}


void module_init_all()
{
    if (!g_module_init_funcs){
        std::cerr << "module_init error. unkown order" << std::endl;
    }
    for (auto &m:*g_module_init_funcs){
        if (m.second){
            m.second();
            //std::cout << "module " << m.first << " loaded" << std::endl;
        }
    }
}

