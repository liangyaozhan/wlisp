

#pragma once
#include "goap/Action.h"
#include "goap/Planner.h"
#include "goap/WorldState.h"

#include <regex>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <map>
#include "wlisp.hpp"


class wstate_defs:public lisp::user_data
{
    int index = 0;
public:
    std::unordered_map<std::string,int> keys;
    std::unordered_map<int,std::string> keys_r;
    
	static lisp::Value make_value() {
		auto ptr = std::make_shared<wstate_defs>();
        if (!ptr){
            throw std::runtime_error("no memory");
        }
		lisp::Value ret("", ptr);
        return ret;
    }

    int add(const std::string &k ){
        auto it = this->keys.find(k);
        if (it == this->keys.end()){
            int v = this->index++;
            keys[k] = v;
            keys_r[v] = k;
            return v;
        } else {
            return it->second;
        }
    }
    int get(const std::string &k,  lisp::Environment& env ){
        auto it = this->keys.find(k);
        if (it != this->keys.end()){
            return it->second;
        }
        throw lisp::Error(lisp::Value(), env, "action not defined");
    }
    std::string get(int v,  lisp::Environment& env ){
        auto it = this->keys_r.find(v);
        if (it != this->keys_r.end()){
            return it->second;
        }
        throw lisp::Error(lisp::Value(), env, "action key not defined");
    }
};


class w_action:public lisp::user_data{
public:
    std::shared_ptr<wstate_defs> def;
    goap::Action action;
    int id_value = 0;

	~w_action(){
	}

    w_action(const std::string &name, int cost):action(name, cost){
    }

	static lisp::Value make_value(const std::string &name, int cost, lisp::Environment &env) {
		auto ptr = std::make_shared<w_action>(name,cost);
        if (!ptr){
            throw std::runtime_error("no memory");
        }
        lisp::Value def;
        try {
            def = env.get("__goap_def__");
        }catch (...){
            def = wstate_defs::make_value();
            env.set("__goap_def__", def);
        }
        ptr->def = std::dynamic_pointer_cast<wstate_defs>(def.as_user_data());
		lisp::Value ret(name, ptr);
		return ret;
	}
	
	std::string display() const {
		return std::string("<action>") + this->action.name() + "(" + std::to_string(this->action.cost()) + ")";
	}
};


class w_state:public lisp::user_data{
public:
    std::shared_ptr<wstate_defs> def;
    goap::WorldState state;

	~w_state(){
	}

    w_state(const std::string &name):state(name){
    }

	static lisp::Value make_value(const std::string &name, lisp::Environment &env) {
		auto ptr = std::make_shared<w_state>(name);
        if (!ptr){
            throw std::runtime_error("no memory");
        }
        lisp::Value def;
        try {
            def = env.get("__goap_def__");
        }catch (...){
            def = wstate_defs::make_value();
            env.set("__goap_def__", def);
        }
        ptr->def = std::dynamic_pointer_cast<wstate_defs>(def.as_user_data());
		lisp::Value ret(name, ptr);
		return ret;
	}
	
	std::string display() const {
		return std::string("<state>") + this->state.name_;
	}
};


class w_dict:public lisp::user_data{
public:
    std::map<std::string,lisp::Value> dict;
    std::string name;

	w_dict(const std::string &n):name(n){
	}

    ~w_dict(){
    }

	static lisp::Value make_value(const std::string &name, lisp::Environment &env) {
		auto ptr = std::make_shared<w_dict>(name);
        if (!ptr){
            throw std::runtime_error("no memory");
        }
		lisp::Value ret(name, ptr);
		return ret;
	}
    lisp::Value value(){
        auto p = this->shared_from_this();
        return lisp::Value(this->name, p);
    }
    static std::shared_ptr<w_dict> ptr_from_value(lisp::Value &v, lisp::Environment &env){
        auto p = std::dynamic_pointer_cast<w_dict>(v.as_user_data());
        if (!p){
            throw lisp::Error(lisp::Value::string("ptr casting result is nullptr"), env, "ptr casting result is nullptr");
        }
        return p;
    }
	
	std::string display() const {
		return std::string("<dict>") + this->name;
	}
};


class w_regex:public lisp::user_data{
public:
    std::string regstr;
    std::regex regex;
    w_regex(const std::string str):regstr(str),regex(str){
    }
	static lisp::Value make_value(const std::string &reg, lisp::Environment &env) {
		auto ptr = std::make_shared<w_regex>(reg);
        if (!ptr){
            throw std::runtime_error("no memory");
        }
		lisp::Value ret(reg, ptr);
		return ret;
	}
    lisp::Value value(){
        auto p = this->shared_from_this();
        return lisp::Value(this->regstr, p);
    }
    static std::shared_ptr<w_regex> ptr_from_value(lisp::Value &v, lisp::Environment &env){
        auto p = std::dynamic_pointer_cast<w_regex>(v.as_user_data());
        if (!p){
            throw lisp::Error(lisp::Value::string("ptr casting result is nullptr"), env, "ptr casting result is nullptr");
        }
        return p;
    }
	
	std::string display() const {
		return std::string("<regex>") + this->regstr;
	}
};

