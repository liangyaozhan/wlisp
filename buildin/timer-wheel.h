// -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
//
// 支持动态申请对象的模式
// Copyright 2016 Juho Snellman, released under a MIT license (see
// LICENSE).
//
// A timer queue which allows events to be scheduled for execution
// at some later point. Reasons you might want to use this implementation
// instead of some other are:
//
// - A single-file C++11 implementation with no external dependencies.
// - Optimized for high occupancy rates, on the assumption that the
//   utilization of the timer queue is proportional to the utilization
//   of the system as a whole. When a tradeoff needs to be made
//   between efficiency of one operation at a low occupancy rate and
//   another operation at a high rate, we choose the latter.
// - Tries to minimize the cost of event rescheduling or cancelation,
//   on the assumption that a large percentage of events will never
//   be triggered. The implementation avoids unnecessary work when an
//   event is rescheduled, and provides a way for the user specify a
//   range of acceptable execution times instead of just an exact one.
// - Facility for limiting the number of events to execute on a
//   single invocation, to allow fine grained interleaving of timer
//   processing and application logic.
// - An interface that at least the author finds convenient.
//
// The exact implementation strategy is a hierarchical timer
// wheel. A timer wheel is effectively a ring buffer of linked lists
// of events, and a pointer to the ring buffer. As the time advances,
// the pointer moves forward, and any events in the ring buffer slots
// that the pointer passed will get executed.
//
// A hierarchical timer wheel layers multiple timer wheels running at
// different resolutions on top of each other. When an event is
// scheduled so far in the future than it does not fit the innermost
// (core) wheel, it instead gets scheduled on one of the outer
// wheels. On each rotation of the inner wheel, one slot's worth of
// events are promoted from the second wheel to the core. On each
// rotation of the second wheel, one slot's worth of events is
// promoted from the third wheel to the second, and so on.
//
// The basic usage is to create a single TimerWheel object and
// multiple TimerEvent or MemberTimerEvent objects. The events are
// scheduled for execution using TimerWheel::schedule() or
// TimerWheel::schedule_in_range(), or unscheduled using the event's
// cancel() method.
//
// Example usage:
//
//      typedef std::function<void()> Callback;
//      TimerWheel timers;
//      int count = 0;
//      TimerEvent<Callback> timer([&count] () { ++count; });
//
//      timers.schedule(&timer, 5);
//      timers.advance(4);
//      assert(count == 0);
//      timers.advance(1);
//      assert(count == 1);
//
//      timers.schedule(&timer, 5);
//      timer.cancel();
//      timers.advance(4);
//      assert(count == 1);
//
// To tie events to specific member functions of an object instead of
// a callback function, use MemberTimerEvent instead of TimerEvent.
// For example:
//
//      class Test {
//        public:
//            Test() : inc_timer_(this) {
//            }
//            void start(TimerWheel* timers) {
//                timers->schedule(&inc_timer_, 10);
//            }
//            void on_inc() {
//                count_++;
//            }
//            int count() { return count_; }
//        private:
//            MemberTimerEvent<Test, &Test::on_inc> inc_timer_;
//            int count_ = 0;
//      };


/*
 *
 *
新增接口类使用方法:

static void __100ms(poll_delay_work_queue *p, std::function<void()> f)
{
	f();

	p->add([p,f](){
		__100ms(p,f);
	}, 100 );
}

void threading(poll_delay_work_queue *p_dwork_q)
{
	static tw::TimerWheel wheel;
	static tw::timer tt;

	__100ms(p_dwork_q, [&wheel, &tt](){
		wheel.advance(1);
		std::cout << "wheel: tt.scheduled_at=" << tt.scheduled_at() << " now=" << wheel.now()  << " Left=" << tt.left() << " elapse=" << tt.elapse() << std::endl;
	});

	tt.call([](){
		std::cout << "tw::timer tt timeout" << std::endl;
	});
	tt.runat(&wheel, 10*3, false);
	tt.startup();
}

运行结果如下：
wheel: tt.scheduled_at=0 now=1 Left=0 elapse=0
wheel: tt.scheduled_at=31 now=2 Left=29 elapse=1
wheel: tt.scheduled_at=31 now=3 Left=28 elapse=2
wheel: tt.scheduled_at=31 now=4 Left=27 elapse=3
wheel: tt.scheduled_at=31 now=5 Left=26 elapse=4
wheel: tt.scheduled_at=31 now=6 Left=25 elapse=5
wheel: tt.scheduled_at=31 now=7 Left=24 elapse=6
wheel: tt.scheduled_at=31 now=8 Left=23 elapse=7
wheel: tt.scheduled_at=31 now=9 Left=22 elapse=8
wheel: tt.scheduled_at=31 now=10 Left=21 elapse=9
wheel: tt.scheduled_at=31 now=11 Left=20 elapse=10
wheel: tt.scheduled_at=31 now=12 Left=19 elapse=11
wheel: tt.scheduled_at=31 now=13 Left=18 elapse=12
wheel: tt.scheduled_at=31 now=14 Left=17 elapse=13
wheel: tt.scheduled_at=31 now=15 Left=16 elapse=14
wheel: tt.scheduled_at=31 now=16 Left=15 elapse=15
wheel: tt.scheduled_at=31 now=17 Left=14 elapse=16
wheel: tt.scheduled_at=31 now=18 Left=13 elapse=17
wheel: tt.scheduled_at=31 now=19 Left=12 elapse=18
wheel: tt.scheduled_at=31 now=20 Left=11 elapse=19
wheel: tt.scheduled_at=31 now=21 Left=10 elapse=20
wheel: tt.scheduled_at=31 now=22 Left=9 elapse=21
wheel: tt.scheduled_at=31 now=23 Left=8 elapse=22
wheel: tt.scheduled_at=31 now=24 Left=7 elapse=23
wheel: tt.scheduled_at=31 now=25 Left=6 elapse=24
wheel: tt.scheduled_at=31 now=26 Left=5 elapse=25
wheel: tt.scheduled_at=31 now=27 Left=4 elapse=26
wheel: tt.scheduled_at=31 now=28 Left=3 elapse=27
wheel: tt.scheduled_at=31 now=29 Left=2 elapse=28
wheel: tt.scheduled_at=31 now=30 Left=1 elapse=29
tw::timer tt timeout
wheel: tt.scheduled_at=31 now=31 Left=0 elapse=30
wheel: tt.scheduled_at=31 now=32 Left=0 elapse=31
wheel: tt.scheduled_at=31 now=33 Left=0 elapse=32
wheel: tt.scheduled_at=31 now=34 Left=0 elapse=33
wheel: tt.scheduled_at=31 now=35 Left=0 elapse=34
wheel: tt.scheduled_at=31 now=36 Left=0 elapse=35
wheel: tt.scheduled_at=31 now=37 Left=0 elapse=36

 */

#ifndef RATAS_TIMER_WHEEL_H
#define RATAS_TIMER_WHEEL_H

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <functional>

#include <atomic>



namespace tw{

extern std::atomic<int> g_tw_allocate_count;

typedef uint64_t Tick;

class TimerWheelSlot;
class TimerWheel;
class timer;

// An abstract class representing an event that can be scheduled to
// happen at some later time.
class TimerEventInterface {
public:
    TimerEventInterface() {
    }

    // TimerEvents are automatically canceled on destruction.
    virtual ~TimerEventInterface() {
        cancel();
    }

    // Unschedule this event. It's safe to cancel an event that is inactive.
	virtual void cancel() {
		// It's ok to cancel a event that's not scheduled.
		if (!slot_) {
			return;
		}

		relink(NULL);
	}

    // Return true iff the event is currently scheduled for execution.
    bool active() const {
        return slot_ != NULL;
    }

    // Return the absolute tick this event is scheduled to be executed on.
    Tick scheduled_at() const { return scheduled_at_; }

private:
    TimerEventInterface(const TimerEventInterface& other) = delete;
    TimerEventInterface& operator=(const TimerEventInterface& other) = delete;
    friend TimerWheelSlot;
    friend TimerWheel;

    // Implement in subclasses. Executes the event callback.
    virtual void execute() = 0;

    void set_scheduled_at(Tick ts) { scheduled_at_ = ts; }
    // Move the event to another slot. (It's safe for either the current
    // or new slot to be NULL).
    inline void relink(TimerWheelSlot* slot);

    Tick scheduled_at_ = 0;
    // The slot this event is currently in (NULL if not currently scheduled).
    TimerWheelSlot* slot_ = NULL;
    // The events are linked together in the slot using an internal
    // doubly-linked list; this iterator does double duty as the
    // linked list node for this event.
    TimerEventInterface* next_ = NULL;
    TimerEventInterface* prev_ = NULL;
};

// An event that takes the callback (of type CBType) to execute as
// a constructor parameter.
template<typename CBType = std::function<void()> >
class TimerEvent : public TimerEventInterface {
public:
    explicit TimerEvent<CBType>(const CBType& callback)
      : callback_(callback) {
    }

    void execute() {
    	callback_();
    }

private:
    TimerEvent<CBType>(const TimerEvent<CBType>& other) = delete;
    TimerEvent<CBType>& operator=(const TimerEvent<CBType>& other) = delete;
    CBType callback_;
};


// An event that's specialized with a (static) member function of class T,
// and a dynamic instance of T. Event execution causes an invocation of the
// member function on the instance.
template<typename T, void(T::*MFun)() >
class MemberTimerEvent : public TimerEventInterface {
public:
    MemberTimerEvent(T* obj) : obj_(obj) {
    }

    virtual void execute () {
        (obj_->*MFun)();
    }

private:
    T* obj_;
};

// Purely an implementation detail.
class TimerWheelSlot {
public:
    TimerWheelSlot() {
    }

private:
    // Return the first event queued in this slot.
    const TimerEventInterface* events() const { return events_; }
    // Deque the first event from the slot, and return it.
    TimerEventInterface* pop_event() {
        auto event = events_;
        events_ = event->next_;
        if (events_) {
            events_->prev_ = NULL;
        }
        event->next_ = NULL;
        event->slot_ = NULL;
        return event;
    }

    TimerWheelSlot(const TimerWheelSlot& other) = delete;
    TimerWheelSlot& operator=(const TimerWheelSlot& other) = delete;
    friend TimerEventInterface;
    friend TimerWheel;

    // Doubly linked (inferior) list of events.
    TimerEventInterface* events_ = NULL;
};

// A TimerWheel is the entity that TimerEvents can be scheduled on
// for execution (with schedule() or schedule_in_range()), and will
// eventually be executed once the time advances far enough with the
// advance() method.
class TimerWheel {
public:
    TimerWheel(Tick now = 0) {
        for (int i = 0; i < NUM_LEVELS; ++i) {
            now_[i] = now >> (WIDTH_BITS * i);
        }
        ticks_pending_ = 0;
    }

    // Advance the TimerWheel by the specified number of ticks, and execute
    // any events scheduled for execution at or before that time. The
    // number of events executed can be restricted using the max_execute
    // parameter. If that limit is reached, the function will return false,
    // and the excess events will be processed on a subsequent call.
    //
    // - It is safe to cancel or schedule events from within event callbacks.
    // - During the execution of the callback the observable event tick will
    //   be the tick it was scheduled to run on; not the tick the clock will
    //   be advanced to.
    // - Events will happen in order; all events scheduled for tick X will
    //   be executed before any event scheduled for tick X+1.
    //
    // Delta should be non-0. The only exception is if the previous
    // call to advance() returned false.
    //
    // advance() should not be called from an event callback.
    inline bool advance(Tick delta,
                        size_t max_execute=std::numeric_limits<size_t>::max(),
                        int level = 0);

    // Schedule the event to be executed delta ticks from the current time.
    // The delta must be non-0.
    inline void schedule(TimerEventInterface* event, Tick delta);

    // Schedule the event to happen at some time between start and end
    // ticks from the current time. The actual time will be determined
    // by the TimerWheel to minimize rescheduling and promotion overhead.
    // Both start and end must be non-0, and the end must be greater than
    // the start.
    inline void schedule_in_range(TimerEventInterface* event,
                                  Tick start, Tick end);

    // Return the current tick value. Note that if the time increases
    // by multiple ticks during a single call to advance(), during the
    // execution of the event callback now() will return the tick that
    // the event was scheduled to run on.
    Tick now() const { return now_[0]; }

    // Return the number of ticks remaining until the next event will get
    // executed. If the max parameter is passed, that will be the maximum
    // tick value that gets returned. The max parameter's value will also
    // be returned if no events have been scheduled.
    //
    // Will return 0 if the wheel still has unprocessed events from the
    // previous call to advance().
    inline Tick ticks_to_next_event(Tick max = std::numeric_limits<Tick>::max(),
                                    int level = 0);

private:
    TimerWheel(const TimerWheel& other) = delete;
    TimerWheel& operator=(const TimerWheel& other) = delete;

    // This handles the actual work of executing event callbacks and
    // recursing to the outer wheels.
    inline bool process_current_slot(Tick now, size_t max_execute, int level);

    static const int WIDTH_BITS = 8;
    static const int NUM_LEVELS = (64 + WIDTH_BITS - 1) / WIDTH_BITS;
    static const int MAX_LEVEL = NUM_LEVELS - 1;
    static const int NUM_SLOTS = 1 << WIDTH_BITS;
    // A bitmask for looking at just the bits in the timestamp relevant to
    // this wheel.
    static const int MASK = (NUM_SLOTS - 1);

    // The current timestamp for this wheel. This will be right-shifted
    // such that each slot is separated by exactly one tick even on
    // the outermost wheels.
    Tick now_[NUM_LEVELS];
    // We've done a partial tick advance. This is how many ticks remain
    // unprocessed.
    Tick ticks_pending_;
    TimerWheelSlot slots_[NUM_LEVELS][NUM_SLOTS];

private:
    timer *free_list = 0;
    inline void timer_free(timer*p);
public:
    inline std::shared_ptr<timer> timer_allocate();
    timer *running_timer = nullptr;

    /*
     * 只要把函数增加进去即可，不需要自己管理timer对象的生存时间。如果把shared_ptr保存起来，则可以操作此定时器，而且不会被释放.
     */
    inline std::shared_ptr<timer> add(std::function<void()> cb, Tick tick, bool is_cycle=false);

    inline std::shared_ptr<timer> add_no_ref(std::function<void()> cb, Tick tick, bool is_cycle=false);

};


// Implementation

void TimerEventInterface::relink(TimerWheelSlot* new_slot) {
    if (new_slot == slot_) {
        return;
    }

    // Unlink from old location.
    if (slot_) {
        auto prev = prev_;
        auto next = next_;
        if (next) {
            next->prev_ = prev;
        }
        if (prev) {
            prev->next_ = next;
        } else {
            // Must be at head of slot. Move the next item to the head.
            slot_->events_ = next;
        }
    }

    // Insert in new slot.
    {
        if (new_slot) {
            auto old = new_slot->events_;
            next_ = old;
            if (old) {
                old->prev_ = this;
            }
            new_slot->events_ = this;
        } else {
            next_ = NULL;
        }
        prev_ = NULL;
    }
    slot_ = new_slot;
}


bool TimerWheel::advance(Tick delta, size_t max_events, int level) {
    if (ticks_pending_) {
        if (level == 0) {
            // Continue collecting a backlog of ticks to process if
            // we're called with non-zero deltas.
            ticks_pending_ += delta;
        }
        // We only partially processed the last tick. Process the
        // current slot, rather incrementing like advance() normally
        // does.
        Tick now = now_[level];
        if (!process_current_slot(now, max_events, level)) {
            // Outer layers are still not done, propagate that information
            // back up.
            return false;
        }
        if (level == 0) {
            // The core wheel has been fully processed. We can now close
            // down the partial tick and pretend that we've just been
            // called with a delta containing both the new and original
            // amounts.
            delta = (ticks_pending_ - 1);
            ticks_pending_ = 0;
        } else {
            return true;
        }
    } else {
        // Zero deltas are only ok when in the middle of a partially
        // processed tick.
        assert(delta > 0);
    }

    while (delta--) {
        Tick now = ++now_[level];
        if (!process_current_slot(now, max_events, level)) {
            ticks_pending_ = (delta + 1);
            return false;
        }
    }
    return true;
}

bool TimerWheel::process_current_slot(Tick now, size_t max_events, int level) {
    size_t slot_index = now & MASK;
    auto slot = &slots_[level][slot_index];
    if (slot_index == 0 && level < MAX_LEVEL) {
        if (!advance(1, max_events, level + 1)) {
            return false;
        }
    }
    while (slot->events()) {
        auto event = slot->pop_event();
        if (level > 0) {
            assert((now_[0] & MASK) == 0);
            if (now_[0] >= event->scheduled_at()) {
                event->execute();
                if (!--max_events) {
                    return false;
                }
            } else {
                // There's a case to be made that promotion should
                // also count as work done. And that would simplify
                // this code since the max_events manipulation could
                // move to the top of the loop. But it's an order of
                // magnitude more expensive to execute a typical
                // callback, and promotions will naturally clump while
                // events triggering won't.
                schedule(event,
                         event->scheduled_at() - now_[0]);
            }
        } else {
            event->execute();
            if (!--max_events) {
                return false;
            }
        }
    }
    return true;
}

void TimerWheel::schedule(TimerEventInterface* event, Tick delta) {
	if (delta <= 0) {
		delta = 1;
	}
    event->set_scheduled_at(now_[0] + delta);

    int level = 0;
    while (delta >= NUM_SLOTS) {
        delta = (delta + (now_[level] & MASK)) >> WIDTH_BITS;
        ++level;
    }

    size_t slot_index = (now_[level] + delta) & MASK;
    auto slot = &slots_[level][slot_index];
    event->relink(slot);
}

void TimerWheel::schedule_in_range(TimerEventInterface* event,
                                   Tick start, Tick end) {
    assert(end > start);
    if (event->active()) {
        auto current = event->scheduled_at() - now_[0];
        // Event is already scheduled to happen in this range. Instead
        // of always using the old slot, we could check compute the
        // new slot and switch iff it's aligned better than the old one.
        // But it seems hard to believe that could be worthwhile.
        if (current >= start && current <= end) {
            return;
        }
    }

    // Zero as many bits (in WIDTH_BITS chunks) as possible
    // from "end" while still keeping the output in the
    // right range.
    Tick mask = ~0;
    while ((start & mask) != (end & mask)) {
        mask = (mask << WIDTH_BITS);
    }

    Tick delta = end & (mask >> WIDTH_BITS);

    schedule(event, delta);
}

Tick TimerWheel::ticks_to_next_event(Tick max, int level) {
    if (ticks_pending_) {
        return 0;
    }
    // The actual current time (not the bitshifted time)
    Tick now = now_[0];

    // Smallest tick (relative to now) we've found.
    Tick min = max;
    for (int i = 0; i < NUM_SLOTS; ++i) {
        // Note: Unlike the uses of "now", slot index calculations really
        // need to use now_.
        auto slot_index = (now_[level] + 1 + i) & MASK;
        // We've reached slot 0. In normal scheduling this would
        // mean advancing the next wheel and promoting or executing
        // those events.  So we need to look in that slot too
        // before proceeding with the rest of this wheel. But we
        // can't just accept those results outright, we need to
        // check the best result there against the next slot on
        // this wheel.
        if (slot_index == 0 && level < MAX_LEVEL) {
            // Exception: If we're in the core wheel, and slot 0 is
            // not empty, there's no point in looking in the outer wheel.
            // It's guaranteed that the events actually in slot 0 will be
            // executed no later than anything in the outer wheel.
            if (level > 0 || !slots_[level][slot_index].events()) {
                auto up_slot_index = (now_[level + 1] + 1) & MASK;
                const auto& slot = slots_[level + 1][up_slot_index];
                for (auto event = slot.events(); event != NULL;
                     event = event->next_) {
                    min = std::min(min, event->scheduled_at() - now);
                }
            }
        }
        bool found = false;
        const auto& slot = slots_[level][slot_index];
        for (auto event = slot.events(); event != NULL;
             event = event->next_) {
            min = std::min(min, event->scheduled_at() - now);
            // In the core wheel all the events in a slot are guaranteed to
            // run at the same time, so it's enough to just look at the first
            // one.
            if (level == 0) {
                return min;
            } else {
                found = true;
            }
        }
        if (found) {
            return min;
        }
    }

    // Nothing found on this wheel, try the next one (unless the wheel can't
    // possibly contain an event scheduled earlier than "max").
    if (level < MAX_LEVEL &&
        (max >> (WIDTH_BITS * level + 1)) > 0) {
        return ticks_to_next_event(max, level + 1);
    }

    return max;
}


class timer: public TimerEventInterface, std::enable_shared_from_this<timer> {
public:
    explicit timer(std::function<void()> cb = nullptr)
    : callback_(cb) {
    }

    virtual ~timer()
    {
    }


    std::function<void()> notify;
    uint64_t flag = 0;

    void on_notify(uint64_t fg){
    	flag = fg;
    	if( notify ){
    		notify();
    	}
    }

	void cancel() {
		if (this->cancel_call_ifnot && (exe_count==0)){
			if (callback_)
			{
				tw_cycle->running_timer = this;
				exe_count++;
				callback_();
				tw_cycle->running_timer = nullptr;
			}
		}
		this->callback_ = nullptr;
		this->internal_holder = nullptr;
		this->notify = nullptr;
		TimerEventInterface::cancel();
	}

    static std::shared_ptr<timer> make_shared(){
    	auto p = std::make_shared<timer>();
    	return p;

    }

    /// how many tick to timeout.
    Tick left(){
    	if (tw_cycle){
    		if (this->active()){
    			return this->scheduled_at() - tw_cycle->now();
    		} else {
    			return 0;
    		}
    	} else {
    		return 0;
    	}
    }

    /// Ticks elapse since startup.
    Tick elapse(){
    	if (tw_cycle){
    		return tw_cycle->now() - this->begin_tick;
    	} else {
    		return 0;
    	}
    }

    void startup(){
    	this->flag = 0;
    	if (!this->callback_){
    		std::cout << "Error, timer startup without callback_" << std::endl;
			this->cancel();
    		throw std::runtime_error("timer startup without callback_");
    	}
    	if (tw_cycle){
    		this->begin_tick = tw_cycle->now();
    		TimerEventInterface::cancel();
    		tw_cycle->schedule(this, cycle_tick);
    	} else {
    		std::cout << "timer startup failed. tw_cycle is nullptr" << std::endl;
    		throw std::runtime_error("tw_cycle is nullptr");
    	}
    }

    void startup(Tick tick){
    	this->flag = 0;
    	if (!this->callback_){
    		std::cout << "Error, timer startup without callback_" << std::endl;
    		throw std::runtime_error("timer startup without callback_");
    	}
    	if (tw_cycle){
    		this->begin_tick = tw_cycle->now();
    		TimerEventInterface::cancel();
    		set_cycle(tick);
    		tw_cycle->schedule(this, cycle_tick);
    	} else {
    		std::cout << "timer startup failed. tw_cycle is nullptr" << std::endl;
    		throw std::runtime_error("tw_cycle is nullptr");
    	}
    }

    void runat(TimerWheel *tw, Tick tick, bool _is_cycle = false) {
    	this->tw_cycle = tw;
    	this->cycle_tick = tick;
    	is_cycle = _is_cycle;
    }

    void call(std::function<void()> cb = nullptr) {
    	callback_ = cb;
    	exe_count = 0;
    	this->flag = 0;
    }

    std::function<void()> get_call(){
    	return this->callback_;
    }

    void reset(){
    	this->cancel();
    	this->callback_ = nullptr;
    	this->internal_holder = nullptr;
    	this->notify = nullptr;
    	this->flag = 0;
    }

    void execute() {
    	/**
    	 * issue:
    	 *    2018-11-01, calling callback_() may reset this->callback_,
    	 *    this will also destroy function object memory, and will crash!
    	 *
    	 *    And tw::time should not be destroy while executing.
    	 */
    	auto callback_backup = this->callback_;
    	auto p_running_run = this->internal_holder;
    	if (callback_backup)
    	{
    		tw_cycle->running_timer = this;
    		exe_count++;
    		callback_backup();
    		tw_cycle->running_timer = nullptr;
    	}
    	if (is_cycle){
    		tw_cycle->schedule(this, cycle_tick);
    	} else {
    		reset();
    	}
    }

    void set_cycle(Tick tick){
    	cycle_tick = tick;
    }

    void make_cancel_call_if_not(bool ok = true){
    	cancel_call_ifnot = ok;
    }

    int exe_count = 0;

private:
    bool cancel_call_ifnot = false;

    Tick begin_tick = 0;
    TimerWheel *tw_cycle = 0;
    Tick cycle_tick = 0;
    bool is_cycle = false;
    timer(const timer& other) = delete;
    timer& operator=(const timer& other) = delete;
    std::function<void()> callback_;

private:
    friend class TimerWheel;
    timer *next = 0;
    std::shared_ptr<timer>  internal_holder;
};

void TimerWheel::timer_free(timer *p)
{
	if (!p){
		return ;
	}
	p->flag = 0;
	p->cancel();
	if (this->free_list == 0){
		this->free_list = p;
		p->next = 0;
		return ;
	}
	p->next = this->free_list;
	this->free_list = p;
}

std::shared_ptr<timer> TimerWheel::timer_allocate()
{
	std::shared_ptr<timer> ret = nullptr;
	if (this->free_list) {
		auto p = this->free_list;
		this->free_list = p->next;
		p->next = 0;
		ret = std::shared_ptr<timer>(p, std::bind(&TimerWheel::timer_free,this,std::placeholders::_1));
	} else {
		ret = std::shared_ptr<timer>(new timer(), std::bind(&TimerWheel::timer_free,this,std::placeholders::_1));
		g_tw_allocate_count++;
	}
	ret->reset();
	return ret;
}

std::shared_ptr<timer> TimerWheel::add(std::function<void()> cb, Tick tick, bool is_cycle)
{
	auto p = this->timer_allocate();
	p->runat(this, tick, is_cycle);
	p->call(std::move(cb));
	p->internal_holder = p;
	p->startup();
	return p;
}

std::shared_ptr<timer> TimerWheel::add_no_ref(std::function<void()> cb, Tick tick, bool is_cycle)
{
	auto p = this->timer_allocate();
	p->runat(this, tick, is_cycle);
	p->call(std::move(cb));
	p->startup();
	return p;
}



} // namespace tw


#endif //  RATAS_TIMER_WHEEL_H
