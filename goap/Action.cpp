
#include "goap/WorldState.h"
#include "goap/Action.h"

#include <cassert>

goap::Action::Action() : cost_(0) {
}

goap::Action::Action(std::string name, int cost) : Action() {
    // Because delegating constructors cannot initialize & delegate at the same time...
    name_ = name;
    cost_ = cost;
}



bool goap::Action::operableOn(const WorldState& ws) const {
    for (const auto& precond : preconditions_) {
        try {
            if (ws.vars_.at(precond.first) != precond.second) {
                return false;
            }
        } catch (const std::out_of_range&) {
            return false;
        }
    }
    return true;
}

goap::WorldState goap::Action::actOn(const WorldState& ws) const {
    goap::WorldState tmp(ws);
    for (const auto& effect : effects_) {
        tmp.setVariable(effect.first, effect.second);
    }
    return tmp;
}

void goap::Action::action_do(WorldState &ws, bool no_except)
{
    if (operableOn(ws)){
        ws = actOn(ws);
        return ;
    }
    if (!no_except)
        throw std::runtime_error(std::string("invalid-action:")+this->name_);
}

