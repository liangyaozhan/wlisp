/*
 * jobq.cpp
 *
 *  Created on: May 13, 2017
 */

#include "jobq.hpp"
//#include "timer-wheel.h"
namespace tw {
std::atomic <int> g_tw_allocate_count(0);
}

std::atomic <int32_t> g_job_count(0);

jobq::jobq()
{
    // TODO Auto-generated constructor stub
	INIT_LIST_HEAD(&this->job_free);
	INIT_LIST_HEAD(&this->job_list);
	_size = 0;
	running_job_file = "";
	running_job_line = 0;
}

jobq::~jobq()
{

	job *pos = 0;
	job *n = 0;
	list_for_each_entry_safe_t(pos, n, &this->job_free, job, node)
	{
		delete pos;
	}
	list_for_each_entry_safe_t(pos, n, &this->job_list, job, node)
	{
		delete pos;
	}
}

volatile bool g_show_running_job = false;
volatile bool g_sts_running_job = false;


void job_atomic::add_suspended_job(const job_atomic_tag &tag, job *p)
{
	auto key =int64_t(tag.p_owner.get());
	auto found = this->jobs_pending.find(key);
	if (found == jobs_pending.end()) {
		auto &item = jobs_pending[key];
		INIT_LIST_HEAD(&item);
		list_add_tail( &p->node, &item);
	} else {
		list_add_tail( &p->node, &found->second);
	}
}

int job_atomic::extract_first_suspended_jobs(jobq &joq){
	int n = 0;
	if (!this->is_idle()){
		return 0;
	}
	for (auto it = this->jobs_pending.begin(); it !=this->jobs_pending.end(); ){
		auto &h = it->second;
		if (list_empty(&h)){
			it = this->jobs_pending.erase(it);
			continue;
		}
		auto next_ptr = list_entry(h.next, job, node);
		if (next_ptr->tags.size() <=0 || (!next_ptr->tags.begin()->p_jm) ){
			std::cerr << "BUG! try extract_suspended_jobs found empty tags." << std::endl;
			++it;
			break;
		}
		if (!next_ptr->tags.begin()->take()){
			std::cerr << "BUG! textract_first_suspended_jobs tags.take failed." << std::endl;
			break;
		}
		joq.job_lock.lock();
		list_splice_tail(&h, &joq.job_list);
		joq.job_lock.unlock();
		n++;
		it = this->jobs_pending.erase(it);
		break;
	}
	return n;
}

void job_atomic_tag::inherit(const job_atomic_tag &o)
{
	if (p_jm){
		std::string msg("job_atomic p_jm already has owner(inherit). owner:");
		msg.append(p_jm->name);
		throw std::runtime_error(msg);
	}
	*this = o;
	this->tag_type = 0;
}

void job_atomic_tag::inherit_end(const job_atomic_tag &o)
{
	if (p_jm){
		std::string msg("job_atomic p_jm already has owner(inherit).");
		msg.append(p_jm->name);
		throw std::runtime_error(msg);
	}
	*this = o;
	this->tag_type = END;
}

void job_atomic_tag::add_suspended_job(job *p)
{
	if (p_jm){
		p_jm->add_suspended_job(*this, p);
	} else {
		std::string msg("BUG, job_atomic_tag::add_suspended_job p_jm should not be empty");
		throw std::runtime_error(msg);
	}
}

bool job_atomic_tag::take()const
{
	if ((tag_type & 1) == 0) {
		return test_take();
	}
	if (!this->p_jm) {
		std::string msg("BUG, take p_jm should not be empty");
		throw std::runtime_error(msg);
	}
	job_atomic& m(*this->p_jm);
	if (m.p_owner == nullptr){
		m.p_owner = this->p_owner;
		m.owner_name = this->name;
		m._recursive = 1;
		m.seq++;
		this->p_owner->seq = m.seq; /* bind-seq */
		//Poco::Logger::root().debug("tag +++ take +++ sem:%s tag:%s ",m.name, std::string(this->name));
		return true;
	}
	if (m.p_owner != this->p_owner){
		//Poco::Logger::root().debug("tag ??? take ??? sem:%s tag:%s owner:%s",m.name, std::string(this->name), m.owner_name);
		std::cerr << "tag ??? take ??? sem:" << m.name << " tag:" << std::string(this->name) << " owner:" << m.owner_name << std::endl;
		return false;
	}
	m._recursive++;
	return true;
}

bool job_atomic_tag::test_take()const
{
	if (!this->p_jm) {
		throw std::runtime_error("BUG, test_take p_jm should not be empty");
	}
	job_atomic& m(*this->p_jm);
	if (m.p_owner == nullptr){
		return true;
	}
	if (m.p_owner != this->p_owner){
		std::cerr << "tag ??? take ??? sem:" << m.name << " tag:" << std::string(this->name) << " owner:" << m.owner_name << std::endl;
		return false;
	}
	return true;
}


void job_atomic_tag::give(jobq &joq)const
{
	this->give_call_count++;

	if ( (tag_type & 2) == 0){
		return ;
	}
	if (!this->p_jm) {
		throw std::runtime_error("BUG, give p_jm should not be empty");
	}
	job_atomic& m(*this->p_jm);
	if (this->is_mime()){
		m._recursive = 0;
		m.p_owner = nullptr;
		m.owner_name = "";
		m.seq++;
		// Poco::Logger::root().debug("tag --- give --- sem:%s tag:%s ",m.name, std::string(this->name));
		m.extract_first_suspended_jobs(joq);
	}
}

bool job_atomic_tag::is_mime()const
{
	if (!this->p_jm) {
		throw std::runtime_error("BUG, is_mime p_jm should not be empty");
	}
	job_atomic& m(*this->p_jm);
	if (this->p_owner && this->p_owner == m.p_owner && (this->p_owner->seq == m.seq ) ){
		return true;
	}
	return false;
}

