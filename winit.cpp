
#include <iostream>
#include <regex>
#include "winit.hpp"
#include "wlisp.hpp"


static lisp::Value _winit_construct(std::vector<lisp::Value> args, lisp::Environment &env)
{
    if (args.size()== 2){
        lisp::eval_args(args, env);
        return w_action::make_value(args[0].as_string(), args[1].as_int(), env);
    } else {
        return w_action::make_value("", 0, env);
    }
}

static lisp::Value _w_state_construct(std::vector<lisp::Value> args, lisp::Environment &env)
{
    if (args.size()>0){
    	lisp::eval_args(args, env);
    	return w_state::make_value(args[0].as_string(), env);
    } else {
        return w_state::make_value("", env);
    }
}

static void __state_init();
static void __dict_init();
static void __regex_init();

void winit_do_init()
{
    __dict_init();
    __state_init();
    __regex_init();

    lisp::global_set("action", _winit_construct);

    lisp::global_set( "action-name", lisp::Value("action-name", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "too many args");
        }
        lisp::eval_args(args, env);
        auto p1 = std::dynamic_pointer_cast<w_action>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "action.name nullptr found");
        }

        return lisp::Value::string(p1->action.name());
    }));

    lisp::global_set( "action-cost", lisp::Value("action-cost", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "too many args");
        }
        lisp::eval_args(args, env);
        auto p1 = std::dynamic_pointer_cast<w_action>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "action.name nullptr found");
        }

        return lisp::Value(p1->action.cost());
    }));
    
    lisp::global_set( "action-pre", lisp::Value("action-pre", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() < 3) {
            throw lisp::Error(lisp::Value(), env, "need >=3 args");
        }
        lisp::eval_args(args, env);

        auto p1 = std::dynamic_pointer_cast<w_action>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "action.pre nullptr found");
        }
        auto def = p1->def;
        int n = (args.size()-1)/2;
        for (int i=0; i<n; i++){
            std::string k = args[1+i*2].as_string();
            bool v = args[2+i*2].as_bool();
            int ki = def->add(k);
            p1->action.setPrecondition(ki,v);
        }
        return args[0];
    }));
    lisp::global_set( "action-prekeys", lisp::Value("action-prekeys", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "1 args expected");
        }
        lisp::eval_args(args, env);
        auto p1 = std::dynamic_pointer_cast<w_action>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "action.prekeys nullptr found");
        }
        std::vector<lisp::Value> allkeys;
        const auto &conditions = p1->action.conditions();
        allkeys.reserve(conditions.size());
        auto def = p1->def;
        for (auto &kv:conditions){
            allkeys.push_back(lisp::Value::string(def->get(kv.first,env)));
        }
        return lisp::Value(allkeys);
    }));
    lisp::global_set( "action-prevalues", lisp::Value("action-prevalues", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "1 args expected");
        }
        lisp::eval_args(args, env);
        auto p1 = std::dynamic_pointer_cast<w_action>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "action.prevalues nullptr found");
        }
        std::vector<lisp::Value> values;
        const auto &conditions = p1->action.conditions();
        values.reserve(conditions.size());
        auto def = p1->def;
        for (auto &kv:conditions){
            values.push_back(lisp::Value(int(kv.second)));
        }
        return lisp::Value(values);
    }));

    lisp::global_set( "action-eff", lisp::Value("action-eff", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() < 3) {
            throw lisp::Error(lisp::Value(), env, "need 2*n+1 args");
        }
        lisp::eval_args(args, env);

        auto p1 = std::dynamic_pointer_cast<w_action>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "action.eff nullptr found");
        }
        auto def = p1->def;
        int n = (args.size()-1)/2;
        for (int i=0; i<n; i++){
            std::string k = args[1+i*2].as_string();
            bool v = args[2+i*2].as_bool();
            int ki = def->add(k);
            p1->action.setEffect(ki,v);
        }
        return args[0];
    }));
    lisp::global_set( "action-effkeys", lisp::Value("action-effkeys", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "1 args expected");
        }
        lisp::eval_args(args, env);
        auto p1 = std::dynamic_pointer_cast<w_action>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "action.effkeys nullptr found");
        }
        std::vector<lisp::Value> allkeys;
        const auto &effects = p1->action.effects();
        allkeys.reserve(effects.size());
        auto def = p1->def;
        for (auto &kv:effects) {
            allkeys.push_back(lisp::Value::string(def->get(kv.first,env)));
        }
        return lisp::Value(allkeys);
    }));
    lisp::global_set( "action-effvalues", lisp::Value("action-effvalues", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "1 args expected");
        }
        lisp::eval_args(args, env);
        auto p1 = std::dynamic_pointer_cast<w_action>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "action.effvalues nullptr found");
        }
        std::vector<lisp::Value> values;
        const auto &effects = p1->action.effects();
        values.reserve(effects.size());
        for (auto &kv:effects) {
            values.push_back(lisp::Value(int(kv.second)));
        }
        return lisp::Value(values);
    }));
    lisp::global_set( "action-print", lisp::Value("action-print", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "need 3 args");
        }
        lisp::eval_args(args, env);

        auto p1 = std::dynamic_pointer_cast<w_action>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "action.eff nullptr found");
        }
        auto def = p1->def;
        const auto &effects = p1->action.effects();
        const auto &conditions = p1->action.conditions();
        std::cout << "preconditions:\n";
        for (auto &x:conditions){
            std::cout << "\t" << def->get(x.first,env) << ":" << x.second << "\n";
        }
        std::cout << "effects:\n";
        for (auto &x:effects){
            std::cout << "\t" << def->get(x.first,env) << ":" << x.second << "\n";
        }
        std::cout << std::endl;
        return args[0];
    }));

    lisp::global_set( "action-clone", lisp::Value("action-clone", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "need 1 args");
        }
        lisp::eval_args(args, env);

        auto p1 = std::dynamic_pointer_cast<w_action>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "action.clone nullptr found");
        }
        auto v2 = w_action::make_value(p1->display(), p1->action.cost(), env);
        auto def = p1->def;
        auto p2 = std::dynamic_pointer_cast<w_action>(v2.as_user_data());
        p2->action = p1->action;
        return v2;
    }));
    lisp::global_set( "action-condition-ok", lisp::Value("action-condition-ok", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2) {
            throw lisp::Error(lisp::Value(), env, "need 2 args");
        }
        lisp::eval_args(args, env);

        auto p_action = std::dynamic_pointer_cast<w_action>(args[0].as_user_data());
        auto p_state = std::dynamic_pointer_cast<w_state>(args[1].as_user_data());

        if (p_state == nullptr || p_action == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "action.condition-ok nullptr found");
        }
        bool ok = p_action->action.operableOn( p_state->state );
        return lisp::Value(int(ok));
    }));

}


static void __state_init()
{
    lisp::global_set("state", _w_state_construct);
    lisp::global_set( "state-set", lisp::Value("state-set", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() < 3) {
            throw lisp::Error(lisp::Value(), env, "args count must be >3");
        }
        lisp::eval_args(args, env);
        auto p1 = std::dynamic_pointer_cast<w_state>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "state.set nullptr found");
        }
        auto def = p1->def;
        int n = (args.size()-1)/2;
        for (int i=0; i<n; i++){
            std::string k = args[1+i*2].as_string();
            bool v = args[2+i*2].as_bool();
            int ki = def->add(k);
            p1->state.setVariable(ki, v);
        }
        return args[0];
    }));
    lisp::global_set( "state-get", lisp::Value("state-get", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2) {
            throw lisp::Error(lisp::Value(), env, "args count must be 2");
        }
        lisp::eval_args(args, env);
        auto p1 = std::dynamic_pointer_cast<w_state>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "state.get nullptr found");
        }
        auto def = p1->def;
        std::string k = args[1].as_string();
        int ki = def->get(k, env);
        int ret = p1->state.getVariable(ki);
        return lisp::Value(ret);
    }));
    lisp::global_set( "state-getkeys", lisp::Value("state-getkeys", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "1 args expected");
        }
        lisp::eval_args(args, env);
        auto p1 = std::dynamic_pointer_cast<w_state>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "state.getkeys nullptr found");
        }
        std::vector<lisp::Value> allkeys;
        allkeys.reserve(p1->state.vars_.size());
        auto def = p1->def;
        for (auto &kv:p1->state.vars_){
            allkeys.push_back(lisp::Value::string( def->get(kv.first, env)));
        }
        return lisp::Value(allkeys);
    }));
    lisp::global_set( "state-getvalues", lisp::Value("state-getvalues", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "1 args expected");
        }
        lisp::eval_args(args, env);
        auto p1 = std::dynamic_pointer_cast<w_state>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "state.getvalues nullptr found");
        }
        std::vector<lisp::Value> allkeys;
        allkeys.reserve(p1->state.vars_.size());
        auto def = p1->def;
        for (auto &kv:p1->state.vars_){
            allkeys.push_back(lisp::Value( int(kv.second) ));
        }
        return lisp::Value(allkeys);
    }));
    lisp::global_set( "state-print", lisp::Value("state-print", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "args count must be 1");
        }
        lisp::eval_args(args, env);
        auto p1 = std::dynamic_pointer_cast<w_state>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "state.print nullptr found");
        }
        auto def = p1->def;
        std::cout << "state{\n";
        for (const auto& kv : p1->state.vars_) {
            std::cout << "\t" << def->get(kv.first, env) << ": " << kv.second << "\n";
        }
        std::cout << "}" << std::endl;
        
        return args[0];
    }));
    lisp::global_set( "state-clone", lisp::Value("state-clone", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value(), env, "need 1 args");
        }
        lisp::eval_args(args, env);

        auto p1 = std::dynamic_pointer_cast<w_state>(args[0].as_user_data());

        if (p1 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "state.clone nullptr found");
        }
        auto v2 = w_state::make_value(p1->display(), env);
        auto def = p1->def;
        auto p2 = std::dynamic_pointer_cast<w_state>(v2.as_user_data());
        p2->state = p1->state;
        return v2;
    }));
    lisp::global_set( "state-condition-ok", lisp::Value("state-condition-ok", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2) {
            throw lisp::Error(lisp::Value(), env, "need 2 args");
        }
        lisp::eval_args(args, env);

        auto p_state = std::dynamic_pointer_cast<w_state>(args[0].as_user_data());
        auto p_action = std::dynamic_pointer_cast<w_action>(args[1].as_user_data());

        if (p_state == nullptr || p_action == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "state.condition-ok nullptr found");
        }
        bool ok = p_action->action.operableOn( p_state->state );
        return lisp::Value(int(ok));
    }));
    lisp::global_set( "state-action-nocondition", lisp::Value("state-action-nocondition", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2) {
            throw lisp::Error(lisp::Value(), env, "need 2 args");
        }
        lisp::eval_args(args, env);

        auto p_state = std::dynamic_pointer_cast<w_state>(args[0].as_user_data());
        auto p_action = std::dynamic_pointer_cast<w_action>(args[1].as_user_data());

        if (p_state == nullptr || p_action == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "state.action-nocondition nullptr found");
        }
        auto new_value = w_state::make_value(p_state->state.name_, env);
        auto p_new_value = std::dynamic_pointer_cast<w_state>(new_value.as_user_data());
        p_new_value->state = p_action->action.actOn( p_state->state );
        return new_value;
    }));
    lisp::global_set( "state-action", lisp::Value("state-action", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2) {
            throw lisp::Error(lisp::Value(), env, "need 2 args");
        }
        lisp::eval_args(args, env);

        auto p_state = std::dynamic_pointer_cast<w_state>(args[0].as_user_data());
        auto p_action = std::dynamic_pointer_cast<w_action>(args[1].as_user_data());

        if (p_state == nullptr || p_action == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "state.action nullptr found");
        }
        auto new_value = w_state::make_value(p_state->state.name_, env);
        auto p_new_value = std::dynamic_pointer_cast<w_state>(new_value.as_user_data());
        if (p_action->action.operableOn( p_state->state )){
            p_new_value->state = p_action->action.actOn( p_state->state );
        } else {
            p_new_value->state = p_state->state;
        }
        return new_value;
    }));
    lisp::global_set( "state-meets", lisp::Value("state-meets", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2) {
            throw lisp::Error(lisp::Value(), env, "need 2 args");
        }
        lisp::eval_args(args, env);

        auto p_state1 = std::dynamic_pointer_cast<w_state>(args[0].as_user_data());
        auto p_state2 = std::dynamic_pointer_cast<w_state>(args[1].as_user_data());

        if (p_state1 == nullptr || p_state2 == nullptr ) {
            throw lisp::Error(lisp::Value(), env, "state.meets nullptr found");
        }
        return lisp::Value(int(p_state1->state.meetsGoal(p_state2->state)));
    }));
    lisp::global_set( "plan", lisp::Value("plan", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 3) {
            throw lisp::Error(lisp::Value(), env, "need 3 args");
        }
        lisp::eval_args(args, env);

        auto p_state1 = std::dynamic_pointer_cast<w_state>(args[0].as_user_data());
        auto p_state2 = std::dynamic_pointer_cast<w_state>(args[1].as_user_data());
        if (p_state1 == nullptr || p_state2 == nullptr  ) {
            throw lisp::Error(lisp::Value(), env, "plan nullptr found");
        }
        std::vector<goap::Action> actions;
        auto list = args[2].as_list();
        if (list.size() == 0) {
            throw lisp::Error(lisp::Value(), env, "plan actions needed");
        }
        actions.reserve(list.size());
        for (auto &li:list){
            auto p_action = std::dynamic_pointer_cast<w_action>(li.as_user_data());
            if (!p_action ) {
                throw lisp::Error(lisp::Value(), env, "plan nullptr found");
            }
            actions.push_back(p_action->action);
        }
        std::vector<goap::Action> result;
        try {
            result = goap::Planner().plan(p_state1->state, p_state2->state, actions);
        }catch (...){
        }
        
        std::vector<lisp::Value> result_valued;
        result_valued.reserve(result.size());
        auto it = result.rbegin();
        for (; it != result.rend(); ++it){
            auto &x = *it;
            auto v = w_action::make_value(x.name(), x.cost(), env);
            auto p = std::dynamic_pointer_cast<w_action>(v.as_user_data());
            if (p){
                p->action = x;
            }
            result_valued.push_back(v);
        }
        return result_valued;
    }));
}


static void __dict_init()
{
    lisp::global_set( "dict", lisp::Value("dict", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()>0){
            lisp::eval_args(args, env);
            return w_dict::make_value(args[0].as_string(), env);
        } else {
            return w_dict::make_value("", env);
        }
    }));
    lisp::global_set( "dict-print", lisp::Value("dict-print", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1) {
            throw lisp::Error(lisp::Value::string("dict-print"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto p_dict = w_dict::ptr_from_value(args[0], env);
        std::cout << "dict:" << p_dict->dict.size() << "{\n";
        for (auto &x:p_dict->dict){
            std::cout << "\t" << x.first << ": " << x.second.display() << "\n";
        }
        std::cout << "}" << std::endl;
        return args[0];
    }));
    lisp::global_set( "dict-set", lisp::Value("dict-set", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() < 3) {
            throw lisp::Error(lisp::Value::string("dict-set"), env, "need >=3 args");
        }
        lisp::eval_args(args, env);
        auto p_dict = w_dict::ptr_from_value(args[0], env);

        int n = (args.size()-1)/2;
        for (int i=0; i<n; i++){
            std::string k = args[1+i*2].as_string();
            p_dict->dict[k] = args[2+i*2];
        }

        return args[0];
    }));
    lisp::global_set( "dict-get", lisp::Value("dict-get", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2 ) {
            throw lisp::Error(lisp::Value::string("dict-get"), env, "need 2 args");
        }
        lisp::eval_args(args, env);
        auto p_dict = w_dict::ptr_from_value(args[0], env);
        auto key = args[1].as_string();
        auto it = p_dict->dict.find(key);
        if (it != p_dict->dict.end()){
            return it->second;
        }
        throw lisp::Error(lisp::Value::string("dict-get"), env, "not found");
    }));
    lisp::global_set( "dict-keys", lisp::Value("dict-keys", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1 ) {
            throw lisp::Error(lisp::Value(), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto p_dict = w_dict::ptr_from_value(args[0], env);
        std::vector<lisp::Value> ret;
        ret.reserve(p_dict->dict.size());
        for (auto &x:p_dict->dict){
            ret.push_back( lisp::Value::string(x.first) );
        }
        return ret;
    }));
    lisp::global_set( "dict-size", lisp::Value("dict-size", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 1 ) {
            throw lisp::Error(lisp::Value(), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto p_dict = w_dict::ptr_from_value(args[0], env);
        return lisp::Value(int(p_dict->dict.size()));
    }));
    // lisp::global_set( "calltest", lisp::Value("calltest", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
    //     lisp::eval_args(args, env);
        
    //     auto v = env.get("func");
    //     std::cout << "v.eval v=" << v.display() << std::endl;
    //     std::vector<lisp::Value> a;
    //     auto ret = v.apply(a, env);
    //     std::cout << "v.eval ret=" << ret.display() << std::endl;
    //     return ret;
    // }));
    
}

static bool __reg_match_list(const std::vector<lisp::Value> &list, const std::regex &reg)
{
    for (auto &l:list){
        if (l.is_list()){
            auto sublist = l.as_list();
            if (!__reg_match_list(sublist, reg)){
                return false;
            }
            continue;
        }
        if (!std::regex_match(l.as_string(), reg)){
            return false;
        }
    }
    return true;
}

static std::string __concat(const lisp::Value &value)
{
    std::stringstream ss;
    if (value.is_list()){
        auto list = value.as_list();
        for (auto &l:list){
            ss << __concat(l);
        }
    } else if (value.is_float()){
        ss << value.as_float();
    } else if (value.is_int()){
        ss << value.as_int();
    } else if (value.is_string()){
        ss << value.as_string();
    } else {
        ss << value.display();
    }
    return ss.str();
}

static void __string_base_init()
{
    lisp::global_set( "concat", lisp::Value("concat", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()==0) {
            throw lisp::Error(lisp::Value::string("regex"), env, "need >1 args");
        }
        lisp::eval_args(args, env);
        std::string ret;
        for (auto &a:args){
            ret.append(__concat(a));
        }
        return lisp::Value::string(ret);
    }));
    
}
static void __regex_init()
{
    __string_base_init();
    lisp::global_set( "regex", lisp::Value("regex", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()==0) {
            throw lisp::Error(lisp::Value::string("regex"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        return w_regex::make_value(args[0].as_string(), env);
    }));
    lisp::global_set( "regex-is-match", lisp::Value("regex-is-match", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() < 2 ) {
            throw lisp::Error(lisp::Value(), env, "need >=2 args");
        }
        lisp::eval_args(args, env);
        auto p_regex = w_regex::ptr_from_value(args[0], env);
        bool ok = false;
        for (int i=1; i<args.size(); i++){
            if (args[i].is_list()){
                auto sublist = args[i].as_list();
                if (!__reg_match_list(sublist, p_regex->regex)){
                    return lisp::Value(int(0));
                }
            } else {
                if (!std::regex_match(args[i].as_string(), p_regex->regex)){
                    return lisp::Value(int(0));
                }
            }
        }
        return lisp::Value(int(1));
    }));
    lisp::global_set( "regex-match", lisp::Value("regex-match", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2 ) {
            throw lisp::Error(lisp::Value(), env, "need 2 args");
        }
        lisp::eval_args(args, env);
        auto p_regex = w_regex::ptr_from_value(args[0], env);
        std::vector<lisp::Value> results;
        bool ok = false;
        std::smatch sm;
        std::string content = args[1].as_string();
        std::regex_match(content, sm, p_regex->regex);
        results.reserve(sm.size());
        for (auto &str:sm){
            results.push_back(lisp::Value::string(str));
        }
        return results;
    }));
    lisp::global_set( "regex-search", lisp::Value("regex-search", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 2 ) {
            throw lisp::Error(lisp::Value(), env, "need 2 args");
        }
        lisp::eval_args(args, env);
        auto p_regex = w_regex::ptr_from_value(args[0], env);
        std::vector<lisp::Value> results;
        bool ok = false;
        std::smatch sm;
        std::string content = args[1].as_string();
        results.reserve(64);
        while (std::regex_search(content, sm, p_regex->regex)){
            std::vector<lisp::Value> groups;
            groups.reserve(sm.size());
            for (auto &str:sm){
                groups.push_back(lisp::Value::string(str));
            }
            results.push_back(lisp::Value(groups));
            content = sm.suffix().str();
        }

        return results;
    }));
    lisp::global_set( "regex-replace", lisp::Value("regex-replace", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() != 3 && args.size() != 4 ) {
            throw lisp::Error(lisp::Value(), env, "need 3 or 4 args");
        }
        lisp::eval_args(args, env);
        auto p_regex = w_regex::ptr_from_value(args[0], env);
        std::string content = args[1].as_string();
        std::string replace = args[2].as_string();
        std::regex_constants::match_flag_type flags = std::regex_constants::match_default | std::regex_constants::format_default;
        if (args.size() == 4){
            auto findvalue = [&env]( std::string flag_str)->int{
                int f = 0;
                if (flag_str == "sed"){
                    f |= std::regex_constants::format_sed;
                } else if (flag_str == "no_copy" || flag_str == "nocopy" || flag_str == "no-copy"){
                    f |= std::regex_constants::format_no_copy;
                } else if ( flag_str == "first_only" || flag_str == "first-only"){
                    f |= std::regex_constants::format_first_only;
                } else if (flag_str == "not-bol"){
                    f |= std::regex_constants::match_not_bol;
                } else if (flag_str == "not-eol"){
                    f |= std::regex_constants::match_not_eol;
                } else if (flag_str == "not-bow"){
                    f |= std::regex_constants::match_not_bow;
                } else if (flag_str == "not-eow"){
                    f |= std::regex_constants::match_not_eow;
                } else if (flag_str == "any"){
                    f |= std::regex_constants::match_any;
                } else if (flag_str == "not-null"){
                    f |= std::regex_constants::match_not_null;
                } else if (flag_str == "continuous"){
                    f |= std::regex_constants::match_continuous;
                } else if (flag_str == "prev-avail"){
                    f |= std::regex_constants::match_prev_avail;
                } else {
                    throw lisp::Error(lisp::Value::string(flag_str), env, "unkown flags");
                }
                return f;
            };
            int f = 0;
            if (args[3].is_list()){
                auto list = args[3].as_list();
                for (auto &l:list){
                    f |= findvalue(l.as_string());
                }
            } else {
                f = findvalue(args[3].as_string());
            }
            if (f){
                flags = std::regex_constants::match_flag_type(f);
            }
        }
        return lisp::Value::string(std::regex_replace(content, p_regex->regex, replace, flags));
    }));
}