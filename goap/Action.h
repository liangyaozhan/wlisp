/**
 * @class Action
 * @brief Operates on the world state.
 *
 * @date  July 2014
 * @copyright (c) 2014 Prylis Inc.. All rights reserved.
 */

#pragma once

#include <exception>
#include <string>
#include <unordered_map>
#include <functional>



// To support Google Test for private members
#ifndef TEST_FRIENDS
#define TEST_FRIENDS
#endif

namespace goap {
    struct WorldState;

    class Action {
    private:
        std::string name_; // The human-readable action name
        int cost_;         // The numeric cost of this action

        // Preconditions are things that must be satisfied before this
        // action can be taken. Only preconditions that "matter" are here.
        std::unordered_map<int, bool> preconditions_;

        // Effects are things that happen when this action takes place.
        std::unordered_map<int, bool> effects_;

    public:
        std::function<void()> act_func;

        Action();
        Action(std::string name, int cost);
        Action(std::string name, int cost, std::function<void()>f):Action(name, cost){
        	act_func = f;
        }
        void set_para(const std::string &n, int c = 0, std::function<void()> f = nullptr){ this->name_ = n; this->cost_ = c; this->act_func = f;}

        /**
         Is this action eligible to operate on the given worldstate?
         @param ws the worldstate in question
         @return true if this worldstate meets the preconditions
         */
        bool operableOn(const goap::WorldState& ws) const;

        const std::unordered_map<int, bool> &conditions()const{
            return this->preconditions_;
        }
        const std::unordered_map<int, bool> &effects()const {
            return this->effects_;
        }

        /**
         Act on the given worldstate. Will not check for "eligiblity" and will happily
         act on whatever worldstate you provide it.
         @param the worldstate to act on
         @return a copy worldstate, with effects applied
         */
        WorldState actOn(const WorldState& ws) const;

        void action_do(WorldState &ws, bool no_except = false);

        /**
         Set the given precondition variable and value.
         @param key the name of the precondition
         @param value the value the precondition must hold
         */
        void setPrecondition(const int key, const bool value) {
            preconditions_[key] = value;
        }

        void removePrecondition(const int key){
        	preconditions_.erase(key);
        }

        /**
         Set the given effect of this action, in terms of variable and new value.
         @param key the name of the effect
         @param value the value that will result
         */
        void setEffect(const int key, const bool value) {
            effects_[key] = value;
        }

        int cost() const { return cost_; }

        std::string name() const { return name_; }

        /**
         * run action
         */
        void run(){
        	if (act_func){
        		act_func();
        	}
        }
        TEST_FRIENDS;
    };

}
