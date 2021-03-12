/*
 * cdbi.cpp
 *
 *  Created on: 2018年1月11日
 *      Author: xxxxxx
 */
#include <assert.h>

#include "mpmcbuffer.h"
#include "cdbi.h"

static cdbi *g_cdbi_instance;
static Poco::ObjectPool<Poco::MongoDB::Connection, Poco::MongoDB::Connection::Ptr> *g_mongodb_connection_pool;
static Poco::PoolableObjectFactory<Poco::MongoDB::Connection, Poco::MongoDB::Connection::Ptr> *pg_factory_mongodb;
static std::string g_mongodb_name;

static Poco::ObjectPool<Poco::Redis::Client,Poco::Redis::Client::Ptr> *g_op = nullptr;

static void _redis_init(const std::string &ip, int port){
	auto addr = new Poco::Net::SocketAddress(ip, port);
	auto factory = new Poco::PoolableObjectFactory<Poco::Redis::Client,Poco::Redis::Client::Ptr>(*addr);
	g_op = new Poco::ObjectPool<Poco::Redis::Client,Poco::Redis::Client::Ptr>(*factory, 5,15);
}

cdbi::cdbi()
{
	// TODO Auto-generated constructor stub
}

cdbi::~cdbi()
{

	// TODO Auto-generated destructor stub
}

void cdbi::init(std::string connect)
{
	if (g_cdbi_instance){
		return;
	}

	_redis_init("127.0.0.1", 6379);

	g_cdbi_instance = new cdbi();
	g_cdbi_instance->p_db = std::make_shared<db_buffer>(connect.c_str());
	g_cdbi_instance->p_db->set_transaction(false);
}

void cdbi::init_common(std::string connect)
{
	if (g_cdbi_instance){
		return;
	}
	g_cdbi_instance = new cdbi();
	g_cdbi_instance->p_db = std::make_shared<db_buffer>(connect.c_str());
	g_cdbi_instance->p_db->set_transaction(false);
}

void cdbi::un_init()
{
	if (g_cdbi_instance)
		delete g_cdbi_instance;
}

bool cdbi::is_valid()
{
	return !!g_cdbi_instance;
}


void cdbi::exec_async(std::function<void(Poco::Data::Session& s)> sqls)
{
	if (!sqls)
	{
		return;
	}

	assert(g_cdbi_instance);

	if (!g_cdbi_instance){
		std::cout << "g_cdbi_instance is null" << std::endl;
		return ;
	}

	auto &db = g_cdbi_instance->p_db;

	db->add([sqls](Poco::Data::Session &sec)->bool
	{
		sqls(sec);
		return true;
	}, nullptr);
}


Poco::ObjectPool<Poco::MongoDB::Connection, Poco::MongoDB::Connection::Ptr> &cdbi::mongodb_pool()
{
	return *g_mongodb_connection_pool;
}

Poco::Data::SessionPool &cdbi::pool()
{
	return this->p_db->pool;
}

cdbi& cdbi::get()
{
	if (!g_cdbi_instance){
		throw std::runtime_error("cdbi not inited.");
	}
	return *g_cdbi_instance;
}

void cdbi::exec(std::function<void(Poco::Data::Session& s)> sqls)
{
		if (!sqls)
	{
		return;
	}

	auto &db = this->p_db;

	db->add([sqls](Poco::Data::Session &sec)->bool
	{
		sqls(sec);
		return true;
	}, nullptr);
}

std::string& cdbi::mongodb_name()
{
	return g_mongodb_name;
}

std::string cdbi::mongodb_eval( Poco::MongoDB::Connection &c, const std::string &js)
{
	Poco::MongoDB::Database db(mongodb_name());
	auto p_cmd = db.createCommand();
	p_cmd->setNumberToReturn(-1);
	p_cmd->setNumberToSkip(100);
	p_cmd->selector().add("eval", js);
	Poco::MongoDB::ResponseMessage respsss;
	c.sendRequest(*p_cmd, respsss);
	Poco::MongoDB::Document::Ptr err_doc = db.getLastErrorDoc(c);
	if (!err_doc){
		throw Poco::Exception("poco mongo no memory");
	}
	int ok = (int)err_doc->getInteger("ok");
	if (ok !=1){
		throw Poco::Exception(err_doc->toString(4));
	}
	if (respsss.documents().size()>0){
		if (respsss.documents()[0]->exists("retval")){
			return respsss.documents()[0]->get<std::string>("retval");
		} else {
			throw Poco::Exception(respsss.documents()[0]->toString(4));
		}
	} else {
		return "";
	}
}


Poco::ObjectPool<Poco::Redis::Client,Poco::Redis::Client::Ptr> &cdbi::redis_object_pool()
{
	return *g_op;
}

void cdbi::redis_publish(const std::string &topic, const Poco::JSON::Array &arr)
{
	std::stringstream ss;
	arr.stringify(ss);
	std::string str(ss.str());
	cdbi::redis_publish(topic, str);
}

void cdbi::redis_publish(const std::string &topic, const Poco::JSON::Object &obj)
{
	std::stringstream ss;
	obj.stringify(ss);
	std::string str(ss.str());
	cdbi::redis_publish(topic, str);
}

void cdbi::redis_publish(const std::string &topic, const std::string &str)
{
	Poco::Redis::Command c("publish");
	c << topic << str;
	DEF_REDIS(redis, 5000);
	if (redis) {
		redis->sendCommand(c);
	}
}

