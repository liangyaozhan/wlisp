#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <algorithm>
#include <thread>
#include <fstream>
#include <chrono>
#include <Windows.h>
#include "TLHelp32.h"

#include "Poco/Process.h"
#include "Poco/NamedEvent.h"
#include "Poco/Util/Units.h"
#include "Poco/Pipe.h"

#include "wlisp.hpp"
#undef min

class w_process:public lisp::user_data{
public:
	Poco::Pipe pipe_stdout, pipe_stderr, pipe_stdin;
	std::shared_ptr<Poco::ProcessHandle> ptr_handle;
	std::string name;
	std::string cwd;
	std::string command;
	Poco::Process::Args args;

	w_process(){

	}
	~w_process(){
		//std::cerr << "process release '" << command << "'" << std::endl;
	}

	lisp::Value try_read(Poco::Pipe &pipe, lisp::Environment &env){
		DWORD dwReadLen = 0;
		DWORD dwStdLen = 0;
		
		auto hPipeRead = pipe.readHandle();
		
		std::array<char, 8192> buffer = {};
		DWORD bytesRead = 0;
		DWORD bytesAvailable = 0;
		if ( !::PeekNamedPipe(hPipeRead, &buffer[0], static_cast<DWORD>(buffer.size()), &bytesRead, &bytesAvailable, NULL)) {
			auto err = ::GetLastError();
			if (err != ERROR_BROKEN_PIPE)
			{
				throw lisp::Error(lisp::Value::string("process-read"), env, (std::string("system error, reading pipe last-error:") + std::to_string(err)).c_str());
			}
			return lisp::Value::nil();
		}
		if (bytesAvailable > 0 || bytesRead > 0){
			char *buff = new char [bytesAvailable+1];
			if (ReadFile(hPipeRead, buff, bytesAvailable, &dwStdLen, NULL))
			{
				std::string str(buff, dwStdLen);
				delete buff;
				return lisp::Value::string(str);
			} else {
				delete buff;
				auto err = ::GetLastError();
				if (err != ERROR_BROKEN_PIPE)
				{
					throw lisp::Error(lisp::Value::string("process-read"), env, "system error, reading pipe");
				}
				return lisp::Value::nil();
			}
		}
		return lisp::Value::nil();
	}

	void write(const std::string &str){
		this->pipe_stdin.writeBytes(str.data(), str.length());
	}

	bool join(){
		if (this->ptr_handle){
			Poco::Process::wait( *this->ptr_handle  );
		}
		return false;
	}
	bool is_running(){
		if (this->ptr_handle){
			return Poco::Process::isRunning(this->ptr_handle->id());
		}
		return false;
	}

	bool run(bool same_stdout_stderr, bool no_pipe, const std::string &cwd ){
		if (no_pipe){
			if (cwd.length()>0){
				auto handle = Poco::Process::launch(this->command, this->args, cwd);
				this->ptr_handle = std::make_shared<Poco::ProcessHandle>(handle);
			} else {
				auto handle = Poco::Process::launch(this->command, this->args);
				this->ptr_handle = std::make_shared<Poco::ProcessHandle>(handle);
			}
			return true;
		} else {
			if (cwd.length()>0){
				if (same_stdout_stderr){
					auto handle = Poco::Process::launch(this->command, this->args, cwd, &this->pipe_stdin, &this->pipe_stdout, &this->pipe_stdout);
					this->ptr_handle = std::make_shared<Poco::ProcessHandle>(handle);
				} else {
					auto handle = Poco::Process::launch(this->command, this->args, cwd, &this->pipe_stdin, &this->pipe_stdout, &this->pipe_stderr);
					this->ptr_handle = std::make_shared<Poco::ProcessHandle>(handle);
				}
			} else {
				if (same_stdout_stderr){
					auto handle = Poco::Process::launch(this->command, this->args, &this->pipe_stdin, &this->pipe_stdout, &this->pipe_stdout);
					this->ptr_handle = std::make_shared<Poco::ProcessHandle>(handle);
				} else {
					auto handle = Poco::Process::launch(this->command, this->args, &this->pipe_stdin, &this->pipe_stdout, &this->pipe_stderr);
					this->ptr_handle = std::make_shared<Poco::ProcessHandle>(handle);
				}
			}
			return true;
		}
		return false;
	}
	bool run_shell(){
		return false;
	}

	static lisp::Value make_value(std::vector<lisp::Value> &args, lisp::Environment &env) {
		auto ptr = std::make_shared<w_process>();
        if (!ptr){
            throw std::runtime_error("no memory");
        };
		std::string name;
		ptr->name = name;
		ptr->command = args[0].as_string();
		for (int i=1; i<args.size(); i++){
			ptr->args.push_back(args[i].as_string());
		}
		lisp::Value ret(name, ptr);
		return ret;
	}
    lisp::Value value(){
        auto p = this->shared_from_this();
        return lisp::Value("", p);
    }
    static std::shared_ptr<w_process> ptr_from_value(lisp::Value &v, lisp::Environment &env){
        auto p = std::dynamic_pointer_cast<w_process>(v.as_user_data());
        if (!p){
            throw lisp::Error(lisp::Value::string("ptr casting result is nullptr"), env, "ptr casting result is nullptr");
        }
        return p;
    }
	
	std::string display() const {
		return std::string("<process>" + this->name);
	}
};

struct process_struct
{
    DWORD   dwSize;
    DWORD   cntUsage;
    DWORD   th32ProcessID;          // this process
    ULONG_PTR th32DefaultHeapID;
    DWORD   th32ModuleID;           // associated exe
    DWORD   cntThreads;
    DWORD   th32ParentProcessID;    // this process's parent process
    LONG    pcPriClassBase;         // Base priority of process's threads
    DWORD   dwFlags;
    std::string exefile;    // Path
};

bool process_list(std::map<std::string,process_struct> &list) {
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		std::cout << "Create Toolhelp32Snapshot Error!" << std::endl;
		return false;
	}

	BOOL bResult = Process32First(hProcessSnap, &pe32);

	while (bResult)
	{
		std::string name = pe32.szExeFile;
		int id = pe32.th32ProcessID;
		struct process_struct ps;
		ps.dwSize = pe32.dwSize;
		ps.cntUsage = pe32.cntUsage;
		ps.th32ProcessID = pe32.th32ProcessID;
		ps.th32DefaultHeapID = pe32.th32DefaultHeapID;
		ps.th32ModuleID = pe32.th32ModuleID;
		ps.cntThreads = pe32.cntThreads;
		ps.th32ParentProcessID = pe32.th32ParentProcessID;
		ps.pcPriClassBase = pe32.pcPriClassBase;
		ps.dwFlags = pe32.dwFlags;
		ps.exefile = name;

		list.insert(std::pair<std::string, process_struct>(name, ps));
		bResult = Process32Next(hProcessSnap, &pe32);
	}

	CloseHandle(hProcessSnap);
	return true;
}

BOOL TerminateMyProcess(DWORD dwProcessId, UINT uExitCode)
{
    DWORD dwDesiredAccess = PROCESS_TERMINATE;
    BOOL  bInheritHandle  = FALSE;
    HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
    if (hProcess == NULL)
        return FALSE;

    BOOL result = TerminateProcess(hProcess, uExitCode);
    CloseHandle(hProcess);
    return result;
}

void process_init()
{
    lisp::global_set( "process", lisp::Value("process", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()==0) {
            throw lisp::Error(lisp::Value::string("process"), env, "need more args");
        }
		std::string cwd;
		int has_cwd = int(args.size())-2;
		if (has_cwd > 0 && args[has_cwd].is_atom() && args[has_cwd].as_atom() == "CD"){
			cwd = args[has_cwd+1].eval(env).as_string();
			args.resize(has_cwd);
		}
		lisp::eval_args(args, env);
        auto v   = w_process::make_value(args, env);
		auto ptr = w_process::ptr_from_value(v, env);
		ptr->cwd = cwd;
		return v;
    }), "<command> [arg1] [arg2] ... [argN] [CD <dir>]");
    lisp::global_set( "process-start", lisp::Value("process-start", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() < 1) {
            throw lisp::Error(lisp::Value::string("process-start"), env, "need more args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_process::ptr_from_value(args[0], env);
		if (args.size() == 1){
			auto ok = ptr->run( true, false, ptr->cwd );
			if (!ok){
				throw lisp::Error(lisp::Value::string("process-start"), env, (std::string("failed to start process '")+ptr->command+ std::string("'")).c_str());
			}
		} else if ( args[1].as_string() == "no-pipe" ) {
			auto ok = ptr->run( true, true, ptr->cwd );
			if (!ok){
				throw lisp::Error(lisp::Value::string("process-start"), env, (std::string("failed to start process '")+ptr->command + std::string("' by shell.")).c_str());
			}
		}

		return lisp::Value(1);
    }), "<proc> [\"no-pipe\"]");
    lisp::global_set( "process-read", lisp::Value("process-read", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()!=1) {
            throw lisp::Error(lisp::Value::string("process-read"), env, "need more args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_process::ptr_from_value(args[0], env);
		return ptr->try_read(ptr->pipe_stdout, env);
    }), "<proc> -> nil|string;  poll mode used.");
    lisp::global_set( "process-join", lisp::Value("process-join", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()!=1) {
            throw lisp::Error(lisp::Value::string("process-join"), env, "need more args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_process::ptr_from_value(args[0], env);
		int ok = ptr->join();
		return lisp::Value((int)ok);
    }));
    lisp::global_set( "process-is-running", lisp::Value("process-is-running", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()!=1) {
            throw lisp::Error(lisp::Value::string("process-is-running"), env, "need 1 args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_process::ptr_from_value(args[0], env);
		int ok = ptr->is_running();
		return lisp::Value((int)ok);
    }));
    lisp::global_set( "process-list", lisp::Value("process-list", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        lisp::eval_args(args, env);
		std::map<std::string,process_struct> result;
		process_list(result);
		std::vector<lisp::Value> ret;
		if (result.size()){
			std::vector<lisp::Value> item;
			item.push_back(lisp::Value::string("dwSize"));
			// item.push_back(lisp::Value::string("cntUsage"));
			item.push_back(lisp::Value::string("th32ProcessID"));
			// item.push_back(lisp::Value::string("th32DefaultHeapID"));
			// item.push_back(lisp::Value::string("th32ModuleID"));
			item.push_back(lisp::Value::string("cntThreads"));
			item.push_back(lisp::Value::string("th32ParentProcessID"));
			// item.push_back(lisp::Value::string("pcPriClassBase"));
			// item.push_back(lisp::Value::string("dwFlags"));
			item.push_back(lisp::Value::string("exefile"));
			ret.push_back( lisp::Value(item));
		}

		for (auto &r:result){
			auto &ps = r.second;
			std::vector<lisp::Value> item;
			item.push_back(lisp::Value(int(ps.dwSize)));
			// item.push_back(lisp::Value(int(ps.cntUsage)));
			item.push_back(lisp::Value(int(ps.th32ProcessID)));
			// item.push_back(lisp::Value(int(ps.th32DefaultHeapID)));
			// item.push_back(lisp::Value(int(ps.th32ModuleID)));
			item.push_back(lisp::Value(int(ps.cntThreads)));
			item.push_back(lisp::Value(int(ps.th32ParentProcessID)));
			// item.push_back(lisp::Value(int(ps.pcPriClassBase)));
			// item.push_back(lisp::Value(int(ps.dwFlags)));
			item.push_back(lisp::Value::string(ps.exefile));
			ret.push_back( lisp::Value(item));
		}
		return ret;
    }));
    lisp::global_set( "process-pid", lisp::Value("process-pid", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 0){
			throw lisp::Error(lisp::Value::string("process-pid"), env, "need 1 args <pid> [code]");
		}
		return lisp::Value(int(Poco::Process::id()));
    }));
    lisp::global_set( "process-kill", lisp::Value("process-kill", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1 && args.size() != 2){
			throw lisp::Error(lisp::Value::string("process-kill"), env, "need 1 args <pid> [code]");
		}
        lisp::eval_args(args, env);

		int pid = args[0].as_int();
		int code = -1;
		if (args.size() == 2){
			code = args[1].as_int();
		}
		bool ok = TerminateMyProcess(pid, code);
		return lisp::Value(int(ok));

    }));

    lisp::global_set( "poco-termination-event-name", lisp::Value("poco-termination-event-name", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1){
			throw lisp::exception(env, "syntax", "poco-termination-event-name need 1 arg");
		}
		int pid = args[0].eval(env).as_int();
		return lisp::Value::string(Poco::ProcessImpl::terminationEventName(pid));
    }), "<process-id> -> name:string");

    lisp::global_set( "named-event", lisp::Value("named-event", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1){
			throw lisp::exception(env, "syntax", "named-event need 1 arg");
		}
		std::string name = args[0].eval(env).as_string();
		auto ptr = lisp::extend<std::shared_ptr<Poco::NamedEvent>>::make_value();
		ptr->cxx_value = std::make_shared<Poco::NamedEvent>(name);
		if (!ptr->cxx_value){
			throw lisp::exception(env, "os", "named-event, no memory");
		}
		return ptr->value();
    }), "<name> -> obj");

    lisp::global_set( "named-event-set", lisp::Value("named-event-set", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1){
			throw lisp::exception(env, "syntax", "named-event-set need 1 arg");
		}
		auto ptr = lisp::extend<std::shared_ptr<Poco::NamedEvent>>::ptr_from_value(args[0], env);
		auto &event = *ptr->cxx_value;
		event.set();
		return args[0];
    }), "<obj> -> args[0]:obj");

    lisp::global_set( "named-event-wait", lisp::Value("named-event-wait", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
		if (args.size() != 1){
			throw lisp::exception(env, "syntax", "named-event-set need 1 arg");
		}
		auto ptr = lisp::extend<std::shared_ptr<Poco::NamedEvent>>::ptr_from_value(args[0], env);
		auto &event = *ptr->cxx_value;
		event.wait();
		return args[0];
    }), "<obj> -> args[0]:obj");
    
}