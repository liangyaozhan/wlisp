#ifndef _MPMC_BUFFER_H
#define _MPMC_BUFFER_H

#include <vector>
#include <memory>
#include <deque>
#include <functional>

#include "Poco/Thread.h"
#include "Poco/Runnable.h"
#include "Poco/Mutex.h"
#include "Poco/Logger.h"
#include "Poco/Data/SessionPool.h"
#include "Poco/Data/Session.h"


extern int register_mysql();

class db_buffer:Poco::Runnable
{
public:
	void set_transaction(bool tf);
	db_buffer(const char *str, bool reg=true, int sess_min=16, int sess_max=128);
	virtual ~db_buffer(){
		stop();
		my_thread->join();
		delete my_thread;
	}

public:// settings
	bool debug_msg_on;
	int min_buffer_time;
	int max_buffer_time;

	/*
	 * 无序
	 */
	void add(std::function<bool(Poco::Data::Session &sec)> exec, std::function<void(bool)> done); // can be called by multi thread.
	void stop(){ this->_stop=1;}
	void join(){}

private:
	friend class mpmc_runnable;
	bool have_transfer;
	struct exec_para
	{
		exec_para(const exec_para &) = default;
		exec_para(exec_para &&) = default;
		exec_para & operator=(const exec_para &) = default;
		exec_para & operator=(exec_para &&) = default;

		std::function<bool(Poco::Data::Session &sec)> exec;
		std::function<void(bool)> done;
		exec_para(std::function<bool(Poco::Data::Session &sec)> &exe, std::function<void(bool)> &d):exec(exe),done(d){}
	};
	Poco::FastMutex lock;
	std::vector<exec_para> execs;
	volatile int _stop;
	Poco::Logger &log;
	void run();
	std::vector<struct exec_para> get(); // can be called by multi thread
	Poco::Thread* my_thread = nullptr;
public:
	Poco::Data::SessionPool pool;
};


#endif
