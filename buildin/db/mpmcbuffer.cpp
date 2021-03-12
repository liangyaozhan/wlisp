#include "mpmcbuffer.h"

#include <sstream>
#include <atomic>

#include "Poco/Data/SessionFactory.h"
#include "Poco/LocalDateTime.h"
#include "Poco/DateTime.h"

#include "Poco/ThreadPool.h"

namespace Poco {
namespace Data {
class Session;
}
;
}
;

std::atomic<int32_t> g_dbexes_monitor_count_total(0);
std::atomic<int32_t> g_dbexes_monitor_count(0);
std::atomic<int32_t> g_dbexes_monitor_count_ok(0);

std::atomic<int32_t> g_dbexes_count_total(0);
std::atomic<int32_t> g_dbexes_count(0);
std::atomic<int32_t> g_dbexes_count_ok(0);


class mpmc_runnable: public Poco::Runnable
{
	std::vector<struct db_buffer::exec_para> exes;
	public:
	bool have_transfer;
	Poco::Data::SessionPool &pool;
	Poco::Logger &logger;
	mpmc_runnable(Poco::Data::SessionPool &_pool, std::vector<struct db_buffer::exec_para> sqls, Poco::Logger &_logger)
					:
					  exes(std::move(sqls)),
					  pool(_pool),
					  logger(_logger)
	{
		have_transfer = true;
	}

	void run()
	{
		while (true) {
			Poco::Data::Session sec(pool.get());
			if (sec.isConnected()) {
#ifndef NDEBUG
				int32_t v = -1;
				sec << "select @@AutoCommit", Poco::Data::Keywords::into(v),
								Poco::Data::Keywords::now;
				if (v != 1){
					std::cout << "*********************************** bad config for mysql, autocommit!=1, it is:" << v << std::endl;
					sec << "set @@AutoCommit=1",Poco::Data::Keywords::now;
					sec << "select @@AutoCommit", Poco::Data::Keywords::into(v),
								Poco::Data::Keywords::now;
					std::cout << "*********************************** failed set @@AutoCommit=1, it is:" << v << std::endl;
				}
#endif
				this->run_it(sec);
				break;
			}
			sec.close();
		}
		delete this;
	}

	void run_it(Poco::Data::Session &sec)
	{

		bool have_transfer = this->have_transfer;
		std::vector<struct db_buffer::exec_para> &sqls = this->exes;
		int retry_count = 0;

		have_transfer = false;
		while (sqls.size())
		{
			try
			{
				Poco::DateTime t0;

				if (have_transfer)
				{
					sec.begin();
				}

				for (auto &sql : sqls)
				{
					if (sql.exec)
					{
						g_dbexes_count_total++;
						g_dbexes_count++;
						if (!sql.exec(sec))
						    {
							if (sql.done)
							{
								try
								{
									g_dbexes_count_ok++;
									sql.done(false);
								} catch (std::exception &e)
								{
									poco_fatal_f1(logger, "bug! done() should not throw exception. what:%s", std::string(e.what()));
								}
							}
							sql.exec = nullptr;
							sql.done = nullptr;
						} else
						{
							g_dbexes_count_ok++;
							if (!have_transfer)
							{
								if (sql.done)
								{
									try
									{
										sql.done(true);
									} catch (std::exception &e)
									{
										poco_fatal_f1(logger, "bug! done() should not throw exception. what:%s", std::string(e.what()));
									}
								}
								sql.exec = nullptr;
								sql.done = nullptr;
							}
						}
					}
				}

				if (have_transfer)
				{
					sec.commit();
				}

				if (have_transfer)
				{
					Poco::Timespan diff = Poco::DateTime() - t0;
					for (auto &sql : sqls)
					{
						if (sql.done)
						{
							try
							{
								sql.done(true);
							} catch (std::exception &e)
							{
								poco_fatal_f1(logger, "bug! done() should not throw exception. what:%s", std::string(e.what()));
							}
						}
					}
					poco_debug_f2(logger, "commiting %d spending %dms", (int)sqls.size(), (int)diff.totalMilliseconds());
				}
				break;
			} catch (Poco::Exception &ex)
			{
				if (have_transfer)
				{
					sec.rollback();
				}
				Poco::Thread::sleep(20);
				std::cerr << "bug! db op retry 20times, all failed.(" << ex.displayText() << ")" << std::endl;

				if ((++retry_count % 20) == 0) {
					poco_fatal_f1(logger, "bug! db op retry 20times, all failed.(%s)", ex.displayText());
					if (have_transfer) {
						for (auto &sql : sqls)
						{
							if (sql.done)
							{
								try
								{
									sql.done(false);
								} catch (std::exception &e)
								{
									poco_fatal_f1(logger, "bug! done() should not throw exception. what:%s", std::string(e.what()));
								}
							}
						}
					}
					goto done_exit;
				}
			}
		}

		done_exit: ;
	}
};

void db_buffer::add(std::function<bool(Poco::Data::Session &sec)> exec,
                std::function<void(bool)> done) // can be called by multi thread.
{
	this->lock.lock();
	execs.push_back(exec_para(exec, done));
	this->lock.unlock();
}

std::vector<struct db_buffer::exec_para> db_buffer::get()
{
	this->lock.lock();
	std::vector<struct db_buffer::exec_para> sqls;
	sqls = std::move(execs);
	execs.clear();
	this->lock.unlock();
	return sqls;
}


void db_buffer::run()
{
	poco_debug(log, "logger works.");
	while (!this->_stop)
	{
		this->lock.lock();
		if (execs.size() == 0) {
			this->lock.unlock();
			Poco::Thread::sleep(10);
			continue;
		}
		mpmc_runnable *p = new mpmc_runnable(
		    this->pool,
		    std::move(execs),
		    log);
		execs.clear();
		this->lock.unlock();
		if (!p) {
			poco_fatal(log, "bug! no memory for mpmc_runnable obj");
			break;
		}
		p->have_transfer = this->have_transfer;
		again: try
		{
			Poco::DateTime t0;
			if (p)
			{
				g_dbexes_monitor_count_total++;
				g_dbexes_monitor_count++;
				Poco::ThreadPool::defaultPool().start(*p);
				g_dbexes_monitor_count_ok++;
			}
			Poco::DateTime t1;
			int32_t ms = (int32_t) (t1 - t0).totalMilliseconds();
			if (ms < this->min_buffer_time)
			                {
				Poco::Thread::sleep(this->min_buffer_time - ms);
			}
		} catch (Poco::NoThreadAvailableException &e)
		{
			(void)e;
			poco_debug(log, e.displayText());
			Poco::Thread::sleep(20);
			goto again;
		} catch (Poco::Exception &e)
		{
			poco_error(log, e.displayText());
			goto again;
		} catch (std::exception &e) {
			poco_error(log, std::string(e.what()));
			goto again;
		}
	}

	Poco::ThreadPool::defaultPool().stopAll();
}

#include "Poco/Data/SessionFactory.h"
#include "Poco/Data/MySQL/Connector.h"
#include "Poco/Data/MySQL/Utility.h"
#include "Poco/Data/MySQL/MySQLException.h"

int register_mysql()
{
	static bool is_register = false;

	if (!is_register)
	{
		Poco::Data::MySQL::Connector::registerConnector();
		is_register = true;
	}
	return 0;
}

db_buffer::db_buffer(const char *connect_str, bool reg, int min,int max)
				:
				  debug_msg_on(false),
				  min_buffer_time(30),
				  max_buffer_time(1000),
				  have_transfer(true),
				  _stop(0),
				  log(Poco::Logger::root().get("db")),
				  pool(((reg?register_mysql():1), Poco::Data::MySQL::Connector::KEY), connect_str, min, max )
{
	my_thread = new Poco::Thread();
	auto& p = my_thread;
	p->start(*this);
	this->pool.setFeature("autoCommit", true);
}

void db_buffer::set_transaction(bool tf)
{
	this->have_transfer = tf;
}

