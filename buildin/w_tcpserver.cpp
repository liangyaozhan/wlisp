
#include <chrono>

#include "Poco/Net/TCPServer.h"
#include "Poco/Net/TCPServerConnection.h"
#include "chd_msg.h"

#include "wlisp.hpp"


template <class S>
class TCPServerConnectionFactoryImplLisp: public Poco::Net::TCPServerConnectionFactory
	/// This template provides a basic implementation of
	/// TCPServerConnectionFactory.
{
public:
	lisp::Environment env;
	lisp::Value lamdba;

	TCPServerConnectionFactoryImplLisp()
	{
	}
	
	~TCPServerConnectionFactoryImplLisp()
	{
	}
	
	Poco::Net::TCPServerConnection* createConnection(const Poco::Net::StreamSocket& socket)
	{
		auto ptr = new S(socket);
		if (ptr){
			ptr->env.combine(this->env);
			ptr->lambda = this->lamdba;
		}
		return ptr;
	}
};
class tcpserver_connection;

class tcpserver_connection:public Poco::Net::TCPServerConnection{
public:
	lisp::Environment env;
	lisp::Value lambda;

	tcpserver_connection(const Poco::Net::StreamSocket& socket):Poco::Net::TCPServerConnection(socket){
	}
	~tcpserver_connection(){
	}

	void run()
	{
		std::vector<lisp::Value> args;
		auto ptr = lisp::extend<Poco::Net::StreamSocket>::make_value();
		ptr->cxx_value = this->socket();
		args.push_back(ptr->value());
		this->lambda.apply(args, env);
	}
};

class w_tcpserver:public lisp::user_data{
public:
	Poco::Net::TCPServer server;

	w_tcpserver(Poco::Net::TCPServerConnectionFactory::Ptr pFactory,
		const Poco::Net::ServerSocket& socket, Poco::Net::TCPServerParams::Ptr pParams = 0)
			:server(pFactory, socket, pParams)
		{
		}

	static lisp::Value make_value(std::vector<lisp::Value> args, lisp::Environment &env) {
		std::string ip;
		int port = 0;
		auto var = args[0].eval(env).as_list();
		if (var.size() != 2){
			throw lisp::Error(lisp::Value::string("tcpserver"), env, "ip/port list len should be 2");
		}
		ip = var[0].eval(env).as_string();
		port = var[1].eval(env).as_int();
		auto captures = args[1].as_list();
		auto p_factory = new TCPServerConnectionFactoryImplLisp<tcpserver_connection>;
		for (auto &c:captures){
			if (!c.is_atom()){
				throw lisp::Error(lisp::Value::string("tcpserver"), env, "atom expected on capture list");
			}
			std::string atom = c.as_atom();
			p_factory->env.set(atom, env.get(atom) );
		}
		auto lambda = args[2].eval(env);
		if (!lambda.is_lambda()){
			throw lisp::Error(lisp::Value::string("tcpserver"), env, "lambda expected");
		}
		p_factory->lamdba = lambda;
		auto ptr = std::make_shared<w_tcpserver>(
				p_factory
				, Poco::Net::ServerSocket(Poco::Net::SocketAddress(ip, port)) );
        if (!ptr){
            throw std::runtime_error("no memory");
        }
		lisp::Value ret("", ptr);
		return ret;
	}
    lisp::Value value(){
        auto p = this->shared_from_this();
        return lisp::Value("<tcpserver>", p);
    }
    static std::shared_ptr<w_tcpserver> ptr_from_value(lisp::Value &v, lisp::Environment &env){
        auto p = std::dynamic_pointer_cast<w_tcpserver>(v.as_user_data());
        if (!p){
            throw lisp::Error(lisp::Value::string("ptr casting result is nullptr"), env, "ptr casting result is nullptr");
        }
        return p;
    }
	
	std::string display() const {
		return std::string("<tcpserver>") + std::to_string((int64_t)this);
	}
};

void w_tcpserver_init()
{
	lisp::global_set( "tcpserver", lisp::Value("tcpserver", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 3) {
			throw lisp::Error(lisp::Value::string("tcpserver"), env, "need 3 args");
		}
		//lisp::eval_args(args, env);
		return w_tcpserver::make_value(args, env);
	}), "<(ip port)> <(catpture-list)> <lambda-new-connection (stream-socket)> -> obj:tcpserver");
	lisp::global_set( "tcpserver-start", lisp::Value("tcpserver-start", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("tcpserver-start"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = w_tcpserver::ptr_from_value(args[0], env);
		ptr->server.start();
		return args[0];
	}), "<obj> -> <obj>");
}

void w_stream_socket_init()
{
	
	lisp::global_set( "stream-socket", lisp::Value("stream-socket", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("stream-socket"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		std::string ip = args[0].as_string();
		int port = args[1].as_int();
		auto ptr = lisp::extend<Poco::Net::StreamSocket>::make_value();
		ptr->cxx_value = Poco::Net::StreamSocket(Poco::Net::SocketAddress(ip, port));
		return ptr->value();
	}), "<ip> <port> -> obj");
	
	lisp::global_set( "stream-socket-set-receive-buffer-size", lisp::Value("stream-socket-set-receive-buffer-size", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("stream-socket-set-receive-buffer-size"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Net::StreamSocket>::ptr_from_value(args[0], env);
		auto size = args[1].as_int();
		ptr->cxx_value.setReceiveBufferSize(size);
		return args[0];
	}), "<stream-socket> <size> -> <stream-socket>");
	lisp::global_set( "stream-socket-set-send-buffer-size", lisp::Value("stream-socket-set-send-buffer-size", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("stream-socket-set-send-buffer-size"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Net::StreamSocket>::ptr_from_value(args[0], env);
		auto size = args[1].as_int();
		ptr->cxx_value.setSendBufferSize(size);
		return args[0];
	}), "<stream-socket> <size> -> <stream-socket>");

	lisp::global_set( "stream-socket-set-receive-timeout", lisp::Value("stream-socket-set-receive-timeout", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("stream-socket-set-receive-timeout"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Net::StreamSocket>::ptr_from_value(args[0], env);
		auto ms = args[1].as_int();
		ptr->cxx_value.setReceiveTimeout(Poco::Timespan( 1000ULL * ms));
		return args[0];
	}), "<stream-socket> <ms> -> <stream-socket>");
	lisp::global_set( "stream-socket-set-send-timeout", lisp::Value("stream-socket-set-send-timeout", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("stream-socket-set-send-timeout"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Net::StreamSocket>::ptr_from_value(args[0], env);
		auto ms = args[1].as_int();
		ptr->cxx_value.setSendTimeout(Poco::Timespan( 1000ULL * ms));
		return args[0];
	}), "<stream-socket> <ms> -> <stream-socket>");

	lisp::global_set( "stream-socket-send-clob", lisp::Value("stream-socket-clob", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("stream-socket-send-clob"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Net::StreamSocket>::ptr_from_value(args[0], env);
		auto ptr_clob = lisp::extend<Poco::Data::CLOB>::ptr_from_value(args[1], env);
		auto clob = ptr_clob->cxx_value;
		int size = clob.size();
		int sent = 0;
		do {
			int s = ptr->cxx_value.sendBytes( clob.rawContent()+ sent, size-sent);
			if (s > 0){
				sent += s;
			} else if (s < 0){
				throw Poco::Exception("socket.sendBytes failed.");
			}
		}while (sent < size);
		
		return lisp::Value::string("");
	}), "<stream-socket> <clob> -> <stream-socket>");

	lisp::global_set( "stream-socket-receive-clob", lisp::Value("stream-socket-receive-clob", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1) {
			throw lisp::Error(lisp::Value::string("stream-socket-receive-clob"), env, "need 1 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Net::StreamSocket>::ptr_from_value(args[0], env);
		auto &socket = ptr->cxx_value;
		int available = socket.available();
		auto ptr_clob = lisp::extend<Poco::Data::CLOB>::make_value();
		if (available > 0){
			std::vector<char> buff(available+1, 0);
			int rx = socket.receiveBytes(&buff[0], available );
			if (rx > 0){
				ptr_clob->cxx_value.assignRaw(&buff[0], rx);
				return ptr_clob->value();
			}
		}
		return ptr_clob->value();
	}), "<stream-socket> -> clob");
	lisp::global_set( "stream-socket-receive-fifo-buffer", lisp::Value("stream-receive-read-fifo-buffer", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("stream-receive-read-fifo-buffer"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Net::StreamSocket>::ptr_from_value(args[0], env);
		auto ptr_fifo = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::ptr_from_value(args[1], env);
		auto &socket = ptr->cxx_value;
		if (!ptr_fifo->cxx_value){
			throw lisp::Error(lisp::Value::string("stream-receive-read-fifo-buffer"), env, "BUG! FifoBuffer emptry");
		}
		int rx_count = socket.receiveBytes(*ptr_fifo->cxx_value);
		return lisp::Value(int(rx_count));
	}), "<stream-socket> <fifo-buffer> -> size:int");
	lisp::global_set( "stream-socket-send-fifo-buffer", lisp::Value("stream-socket-send-fifo-buffer", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 2) {
			throw lisp::Error(lisp::Value::string("stream-socket-send-fifo-buffer"), env, "need 2 args");
		}
		lisp::eval_args(args, env);
		auto ptr = lisp::extend<Poco::Net::StreamSocket>::ptr_from_value(args[0], env);
		auto ptr_fifo = lisp::extend<std::shared_ptr<Poco::FIFOBuffer>>::ptr_from_value(args[1], env);
		auto &socket = ptr->cxx_value;
		if (!ptr_fifo->cxx_value){
			throw lisp::Error(lisp::Value::string("stream-socket-send-fifo-buffer"), env, "BUG! FifoBuffer emptry");
		}
		int rx_count = socket.sendBytes(*ptr_fifo->cxx_value);
		return lisp::Value(int(rx_count));
	}), "<stream-socket> <fifo-buffer> -> size-sent:int");
}

void w_dialog_socket()
{

}