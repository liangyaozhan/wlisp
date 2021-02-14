/*
 * jobq.hpp
 *
 *  Created on: May 13, 2017
 */

#ifndef JOBQ_HPP_
#define JOBQ_HPP_

#include <iostream>
#include <functional>
#include <queue>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <mutex>

#include <atomic>

#ifndef NDEBUG
#include <iostream>
#endif

#include "list.h"

/*
 * for debug
 */
extern volatile bool g_show_running_job;
extern volatile bool g_sts_running_job;
extern std::atomic <int32_t> g_job_count;
class job_atomic_tag;
class jobq_no_lock;
class job;
class jobq;

class job_atomic_owner{
public:
	int seq = 0;

};

class job_atomic:public std::enable_shared_from_this<job_atomic>{
	friend class job_atomic_tag;
	int _recursive = 0;
	mutable std::shared_ptr<job_atomic_owner> p_owner;
	std::string owner_name;
	int seq = 0;

	std::map<int64_t,struct list_head> jobs_pending;

	bool can_run( job &j);

	void add_suspended_job(const job_atomic_tag &tag, job *p);
	int extract_first_suspended_jobs(jobq &joq);

public:
	bool is_idle()const{ return p_owner==nullptr;}
	std::string name;
};

class job_atomic_tag{
	friend class jobq;
	friend class jobq_no_lock;
	friend class job_atomic;
	std::shared_ptr<job_atomic> p_jm = nullptr;
	mutable std::shared_ptr<job_atomic_owner> p_owner;
	bool is_mime()const;
	mutable int give_call_count = 0;
	mutable int clear_call_count = 0;
public:
	const char *name = "";
	bool take() const;
	bool test_take() const;
	void give(jobq &joq)const ;
	void clear(jobq &joq){
		clear_call_count++;
		give(joq);
		p_owner = nullptr;
		p_jm = nullptr;
	}
	mutable int tag_type = 0; /* 0:nomal,  b1: begin   b2:end */
	void clear_type_begin(){ this->tag_type &= ~1;}
	void clear_type_end(){ this->tag_type &= ~2; }

	enum {
		NONE = 0,
		BEGIN = 1,
		END = 2,
		ALL = 3,
		BEGIN_END = 3
	};

	~job_atomic_tag(){
	}

	void inherit(const job_atomic_tag &o);
	void inherit_end(const job_atomic_tag &o);

	void add_suspended_job(job *p);

	job_atomic_tag(){}

	job_atomic_tag(std::shared_ptr<job_atomic> a, int type = job_atomic_tag::NONE):p_owner(std::make_shared<job_atomic_owner>()),p_jm(a),tag_type(type){}
	job_atomic_tag(const job_atomic_tag &o):p_owner(o.p_owner),p_jm(o.p_jm),tag_type(o.tag_type),name(o.name){}
	job_atomic_tag &operator = (const job_atomic_tag &o){ this->p_owner = o.p_owner; this->p_jm = o.p_jm; this->tag_type = o.tag_type; this->name=o.name; return *this;}
	job_atomic_tag(job_atomic_tag &&o):p_owner(std::move(o.p_owner)), p_jm(std::move(o.p_jm)), tag_type(o.tag_type),name(o.name){}
	job_atomic_tag &operator = (job_atomic_tag &&o){ this->p_owner = std::move(o.p_owner); this->p_jm = std::move(o.p_jm); this->tag_type = o.tag_type; name=o.name; return *this;}


};


class job{
public:
	job(std::function<void()> _func, const void *id = 0,const char *f="",int ln=0, const char *fname="") :
			key(id),file(f),function_name(fname),line(ln),func(std::move(_func))
	{
		g_job_count++;
	}

private:
	const void *key;

public:
	job():key(0),file(""),function_name("unkown"),line(0){
		g_job_count++;
	}

	bool operator ==(const job &o) const
	{
		if ((!this->key) || (!o.key)) {
			return false;
		}
		return this->key == o.key;
	}

	job(job&&j):
		key(j.key),
		file(j.file),
		function_name(j.function_name),
		line(j.line),
		func(std::move(j.func))
		,tags(std::move(j.tags))
	{
		g_job_count++;
	}

	job& operator = (job&&j){
		file = j.file;
		function_name = j.function_name;
		key = j.key;
		line = j.line;
		func = std::move(j.func);
		return *this;
	}
	void reset(){
		key = 0;
		file = "";
		function_name = "";
		line = 0;
		func = nullptr;
		tags.clear();
	}

	job& operator = (const job &j) = delete;
	job(const job&j) = delete;

	const char *file = 0;
	const char *function_name = "";
	int line= 0;
	struct list_head node;
	std::function<void()> func;
	std::list<job_atomic_tag> tags;

	~job() {
		g_job_count--;
	}
};

#define JOBK(func,key) job(std::move(func),key,__FILE__,__LINE__,__FUNCTION__)
#define JOB(func) JOBK(func,0)
#define JOB_DEBUG __FILE__,__LINE__,__FUNCTION__

struct job_spending_time
{
	uint64_t min;
	uint64_t max;
	uint64_t sum;
	int32_t  call_count;
};


class jobq {
	friend class job_atomic;
public:
	jobq();
	virtual ~jobq();

public:
	job *running = 0;

	int poll(void)
	{
		int n = 0;
		struct list_head h;
		INIT_LIST_HEAD(&h);
		this->job_lock.lock();
		if (list_empty(&this->job_list)){
			this->job_lock.unlock();
			return 0;
		}
		list_splice(&this->job_list, &h);
		INIT_LIST_HEAD(&this->job_list);
		n = this->_size;
		this->_size = 0;
		this->job_lock.unlock();
		job *j = 0;
		job *tmp_next = 0;
		list_for_each_entry_safe_t(j, tmp_next,&h, job,node)
		{
			if (j->tags.size()>0){
				bool is_all_ok = true;
				for (auto &i:j->tags){
					if (!i.test_take()){
						is_all_ok = false;
						list_del_init( &j->node );
						i.add_suspended_job(j);
						break;
					}
				}
				if (!is_all_ok){
					continue;
				}
				for (auto &i:j->tags){
					i.take();
				}
			}
			if (j->func) {
				running_job_file = j->file;
				running_job_line = j->line;
				if (g_show_running_job) {
					std::cout << "run job:" << running_job_file << ":" << running_job_line << " " << j->function_name << std::endl;
				}
				try{
					this->running = j;
					j->func();
				}
				catch (std::exception &e){
					std::cerr << "ERROR job:" << running_job_file << ":" << running_job_line << " exception:" << e.what() << std::endl;
				} catch (...){
					std::cerr << "ERROR job:" << running_job_file << ":" << running_job_line << " unkown exception..." << std::endl;
				}
				this->running = 0;
				if (g_sts_running_job) {
					std::stringstream ss;
					ss << "f" << running_job_file << ":" << running_job_line;
				}

				if (j->tags.size()>0){
					for (auto &i:j->tags){
						i.give( *this );
					}
				}

				try{
					j->reset();
				}
				catch (std::exception &e){
					std::cerr << "ERROR jobreset:" << running_job_file << ":" << running_job_line << " exception:" << e.what() << std::endl;
				} catch (...){
					std::cerr << "ERROR jobreset:" << running_job_file << ":" << running_job_line << " unkown exception..." << std::endl;
				}
			}
		}
		this->job_lock.lock();
		list_splice_tail(&h, &this->job_free);
		n = this->_size;
		this->job_lock.unlock();
		return n;
	}

public:
	void add(job j)
	{
		std::lock_guard<std::mutex> lk(this->job_lock);
		auto p = list_node_alloc<job>(&this->job_free);
		*p = std::move(j);
		list_add_tail( &p->node, &this->job_list );
		this->_size++;
	}

	void add(job j, std::list<job_atomic_tag> &tags)
	{
		std::lock_guard<std::mutex> lk(this->job_lock);
		auto p = list_node_alloc<job>(&this->job_free);
		*p = std::move(j);
		p->tags = std::move(tags);
		list_add_tail( &p->node, &this->job_list );
		this->_size++;
	}
	void add(std::function<void()> f, std::list<job_atomic_tag> &tags)
	{
		std::lock_guard<std::mutex> lk(this->job_lock);
		auto p = list_node_alloc<job>(&this->job_free);
		p->func = std::move(f);
		p->tags = std::move(tags);
		list_add_tail( &p->node, &this->job_list );
		this->_size++;
	}

	void add(job j, const job_atomic_tag &tag)
	{
		if (!tag.p_jm){
			std::string msg("empty-tag name=");
			msg.append(tag.name);
			throw std::runtime_error(msg);
		}
		std::list<job_atomic_tag> tags;
		tags.push_back(tag);
		std::lock_guard<std::mutex> lk(this->job_lock);
		auto p = list_node_alloc<job>(&this->job_free);
		*p = std::move(j);
		p->tags = std::move(tags);
		list_add_tail( &p->node, &this->job_list );
		this->_size++;
	}
	void add(std::function<void()> f, const job_atomic_tag &tag)
	{
		if (!tag.p_jm){
			std::string msg("empty-tag name=");
			msg.append(tag.name);
			throw std::runtime_error(msg);
		}
		std::list<job_atomic_tag> tags;
		tags.push_back(tag);
		std::lock_guard<std::mutex> lk(this->job_lock);
		auto p = list_node_alloc<job>(&this->job_free);
		p->func = std::move(f);
		p->tags = std::move(tags);
		list_add_tail( &p->node, &this->job_list );
		this->_size++;
	}


	void add(std::function<void()> f)
	{
		std::lock_guard<std::mutex> lk(this->job_lock);
		auto p = list_node_alloc<job>(&this->job_free);
		p->func = std::move(f);
		list_add_tail( &p->node, &this->job_list );
		this->_size++;
	}


	int size(){
		std::lock_guard<std::mutex> lk(this->job_lock);
		return this->_size;
	}

	const char *running_job_file; // for debug
	int         running_job_line; // for debug
	std::unordered_map<std::string, struct job_spending_time> log_time;

private:
	mutable std::mutex job_lock;
	struct list_head job_list;
	struct list_head job_free;
	int _size;
};


#endif /* JOBQ_HPP_ */
