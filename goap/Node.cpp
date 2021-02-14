#include "goap/Node.h"
#include <iostream>

#include <atomic>

static std::atomic<int> last_id_(0); // a static that lets us assign incrementing, unique IDs to nodes


goap::Node::Node() : g_(0), h_(0) {
    id_ = ++last_id_;
}
goap::Node::Node(const WorldState state, int g, int h, int parent_id, const Action* action) :
    ws_(state), g_(g), h_(h), parent_id_(parent_id), action_(action) {
    id_ = ++last_id_;
}

bool goap::operator<(const goap::Node& lhs, const goap::Node& rhs) {
    return lhs.f() < rhs.f();
}

//bool goap::Node::operator<(const Node& other) {
//    return f() < other.f();
//}
