
#ifndef COMMON_CDBI_H_
#define COMMON_CDBI_H_
#include <memory>
#include <string>
#include <functional>
#include "Poco/Data/Session.h"
#include "Poco/Data/SessionPool.h"
#include "Poco/MongoDB/MongoDB.h"
#include "Poco/MongoDB/Connection.h"
#include "Poco/MongoDB/Database.h"
#include "Poco/MongoDB/Cursor.h"
#include "Poco/MongoDB/Array.h"
#include "Poco/MongoDB/PoolableConnectionFactory.h"

#include "Poco/ObjectPool.h"
#include "Poco/Redis/AsyncReader.h"
#include "Poco/Redis/Command.h"
#include "Poco/Redis/PoolableConnectionFactory.h"
#include "Poco/Redis/Type.h"
#include "Poco/Redis/Redis.h"
#include "Poco/Redis/Array.h"
#include "Poco/Redis/PoolableConnectionFactory.h"
#include "Poco/Delegate.h"

#include "Poco/JSON/Object.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Query.h"

class db_buffer;

class cdbi {
private:
	cdbi();
	virtual ~cdbi();

public:
	static void init_common(std::string connect);
	static void init(std::string connect);
	static void un_init();
	static void exec_async( std::function<void(Poco::Data::Session &s)> sqls );
	static bool is_valid();
	static cdbi &get();
	static Poco::ObjectPool<Poco::MongoDB::Connection, Poco::MongoDB::Connection::Ptr> &mongodb_pool();

	static std::string &mongodb_name();

	static std::string mongodb_eval( Poco::MongoDB::Connection &c, const std::string &js);

	template<class T>
	T mongodb_eval( Poco::MongoDB::Connection &c, const std::string &js){
		std::string retval = mongodb_eval(c,js);
		if (!retval.empty()){
			if (typeid(T) == typeid(Poco::JSON::Object::Ptr)){
				auto obj_ptr = Poco::JSON::Parser().parse(retval).extract<Poco::JSON::Object::Ptr>();
				return obj_ptr;
			}else {
				Poco::Dynamic::Var v(retval);
				return v.extract<T>();
			}
		} else {
			return T(0);
		}
	}

	/*
	 * exsample:
	 *
			Poco::MongoDB::PooledConnection ccc(cglobal::mongodb_pool());
			Poco::MongoDB::Connection &connection = *Poco::MongoDB::Connection::Ptr(ccc);
			Poco::MongoDB::Array::Ptr p = new Poco::MongoDB::Array();
			p->addNewDocument("").addNewDocument("$match");
			p->addNewDocument("").addNewDocument("$lookup").add("localField", "president").add("from", "users").add("foreignField", "_id").add("as", "user");
			p->addNewDocument("").addNewDocument("$project").add("_id", 0).add("name", 1).add("president",1).add("user.name",1).add("user.id",1).add("user.pwd",1).add("user.portrait",1);
			p->addNewDocument("").addNewDocument("$project").add("nameAAA", "$name").add("president", "$user");
			std::cout << p->toString(4) << std::endl;
			Poco::MongoDB::Database db("local");
			auto p_cmd = db.createCommand();
			p_cmd->setNumberToReturn(-1);
			p_cmd->selector().add("aggregate", "dfriends_o").add("pipeline", p)
					.add("explain", false);

			Poco::MongoDB::ResponseMessage respsss;
			connection.sendRequest(*p_cmd, respsss);
			Poco::MongoDB::Document::Ptr err_doc = db.getLastErrorDoc(connection);

			std::cout << "Done...\n"
					<< "----------------------------------"
					<< p_cmd->selector().toString()
					<< "----------------------------------"
					<< err_doc->toString(4) << std::endl;
			std::cout << "\n\n+++++respsss.documents().size" << respsss.documents().size() << "\n";

			for (Poco::MongoDB::Document::Vector::const_iterator it = respsss.documents().begin(); it != respsss.documents().end(); ++it)
			{
				std::cout << (*it)->toString(4) << std::endl;
			}
	 */

	Poco::Data::SessionPool &pool();

	void exec( std::function<void(Poco::Data::Session &s)> sqls );

	/*
	 * redis pool
	 */
	static Poco::ObjectPool<Poco::Redis::Client,Poco::Redis::Client::Ptr> &redis_object_pool();

#ifndef DEF_REDIS
#define DEF_REDIS(var,t) Poco::Redis::PooledConnection __p_d_c_##var(cdbi::redis_object_pool(), t);Poco::Redis::Client::Ptr var(__p_d_c_##var)
#endif

	static void redis_publish(const std::string &topic, const Poco::JSON::Array &arr);
	static void redis_publish(const std::string &topic, const Poco::JSON::Object &obj);
	static void redis_publish(const std::string &topic, const std::string &str);

private:
	std::shared_ptr<db_buffer> p_db;
};



#endif /* COMMON_CDBI_H_ */
