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


#include "wlisp.hpp"
#undef min

#define MY_PIPE_BUFFER_SIZE 1024

class w_process:public lisp::user_data{
public:

	HANDLE hPipeRead;
	HANDLE hPipeWrite;
	SECURITY_ATTRIBUTES saOutPipe;
	std::string name;
	std::string program;
	std::string args;

	PROCESS_INFORMATION processInfo;
	STARTUPINFOA startupInfo;
	std::string output;

	w_process(){
		::ZeroMemory(&saOutPipe, sizeof(saOutPipe));
		::ZeroMemory(&processInfo, sizeof(processInfo));
		::ZeroMemory(&startupInfo, sizeof(startupInfo));

		saOutPipe.nLength = sizeof(SECURITY_ATTRIBUTES);
		saOutPipe.lpSecurityDescriptor = NULL;
		saOutPipe.bInheritHandle = TRUE;
		startupInfo.cb = sizeof(STARTUPINFO);
		startupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		startupInfo.wShowWindow = SW_SHOWNORMAL;

	}

	std::string read(lisp::Environment &env){
		DWORD dwReadLen = 0;
		DWORD dwStdLen = 0;
		char szPipeOut[MY_PIPE_BUFFER_SIZE*2+1];
		::ZeroMemory(szPipeOut, sizeof(szPipeOut));
		this->output = "";
		while (1)
		{
        	std::array<char, 256> buffer = {};
			DWORD bytesRead = 0;
			DWORD bytesAvailable = 0;
			if ( !::PeekNamedPipe(hPipeRead, &buffer[0], static_cast<DWORD>(buffer.size()), &bytesRead, &bytesAvailable, NULL)) {
				auto err = ::GetLastError();
				if (err != ERROR_BROKEN_PIPE)
				{
					throw lisp::Error(lisp::Value::string("process-read"), env, (std::string("system error, reading pipe last-error:") + std::to_string(err)).c_str());
				}
				break;
			}
			if (bytesAvailable > 0 || bytesRead > 0){
				if (ReadFile(hPipeRead, szPipeOut, std::min(int(bytesAvailable), int(sizeof(szPipeOut)-1)), &dwStdLen, NULL))
				{
					szPipeOut[dwStdLen] = 0;
					output += std::string(szPipeOut);
					dwStdLen = 0;
				} else {
					auto err = ::GetLastError();
					if (err != ERROR_BROKEN_PIPE)
					{
						throw lisp::Error(lisp::Value::string("process-read"), env, "system error, reading pipe");
					}
					break;
				}
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
		}
		return output;
	}


	void redirect(lisp::Environment &env, const std::string &filename){
		DWORD dwReadLen = 0;
		DWORD dwStdLen = 0;
		char szPipeOut[MY_PIPE_BUFFER_SIZE*2+1];
		::ZeroMemory(szPipeOut, sizeof(szPipeOut));
		std::ofstream out(filename, std::ios::binary);
		if (!out.is_open()){
			throw lisp::Error(lisp::Value::string("process-redirect"), env, (std::string("find cannot open for write. ") + filename).c_str());
		}

		while (1)
		{
        	std::array<char, 256> buffer = {};
			DWORD bytesRead = 0;
			DWORD bytesAvailable = 0;
			if ( !::PeekNamedPipe(hPipeRead, &buffer[0], static_cast<DWORD>(buffer.size()), &bytesRead, &bytesAvailable, NULL)) {
				auto err = ::GetLastError();
				if (err != ERROR_BROKEN_PIPE)
				{
					throw lisp::Error(lisp::Value::string("process-read"), env, (std::string("system error, reading pipe last-error:") + std::to_string(err)).c_str());
				}
				break;
			}
			if (bytesAvailable > 0 || bytesRead > 0){
				if (ReadFile(hPipeRead, szPipeOut, std::min(int(bytesAvailable), int(sizeof(szPipeOut)-1)), &dwStdLen, NULL))
				{
					szPipeOut[dwStdLen] = 0;
					out << std::string(szPipeOut);
					dwStdLen = 0;
				} else {
					auto err = ::GetLastError();
					if (err != ERROR_BROKEN_PIPE)
					{
						throw lisp::Error(lisp::Value::string("process-read"), env, "system error, reading pipe");
					}
					break;
				}
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
		}
	}

	void read(lisp::Environment &env, lisp::Value &lambda){
		DWORD dwReadLen = 0;
		DWORD dwStdLen = 0;
		char szPipeOut[MY_PIPE_BUFFER_SIZE*2+1];
		::ZeroMemory(szPipeOut, sizeof(szPipeOut));

		while (1)
		{
        	std::array<char, 256> buffer = {};
			DWORD bytesRead = 0;
			DWORD bytesAvailable = 0;
			if ( !::PeekNamedPipe(hPipeRead, &buffer[0], static_cast<DWORD>(buffer.size()), &bytesRead, &bytesAvailable, NULL)) {
				auto err = ::GetLastError();
				if (err != ERROR_BROKEN_PIPE)
				{
					throw lisp::Error(lisp::Value::string("process-read"), env, (std::string("system error, reading pipe last-error:") + std::to_string(err)).c_str());
				}
				break;
			}
			if (bytesAvailable > 0 || bytesRead > 0){
				if (ReadFile(hPipeRead, szPipeOut, std::min(int(bytesAvailable), int(sizeof(szPipeOut)-1)), &dwStdLen, NULL))
				{
					szPipeOut[dwStdLen] = 0;
					std::vector<lisp::Value> a;
					a.push_back( lisp::Value::string(szPipeOut));
					lambda.apply(a, env);
					dwStdLen = 0;
				} else {
					auto err = ::GetLastError();
					if (err != ERROR_BROKEN_PIPE)
					{
						throw lisp::Error(lisp::Value::string("process-read"), env, "system error, reading pipe");
					}
					break;
				}
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
		}
	}

	bool join(){
		if (processInfo.hProcess)
		{
			if (WAIT_TIMEOUT != WaitForSingleObject(processInfo.hProcess, 3000))
			{
			}
			CloseHandle(processInfo.hProcess);
		}
		if (processInfo.hThread)
		{
			CloseHandle(processInfo.hThread);
		}
		CloseHandle(hPipeRead);
		//CloseHandle(hPipeWrite);
		return true;
	}
	bool is_running(){
		if (processInfo.hProcess)
		{
			if (WAIT_TIMEOUT == WaitForSingleObject(processInfo.hProcess, 0))
			{
				return true;
			}
		}
		return false;
	}

	bool run(){
		if (CreatePipe(&hPipeRead, &hPipeWrite, &saOutPipe, MY_PIPE_BUFFER_SIZE))
		{
			startupInfo.hStdOutput = hPipeWrite;
			startupInfo.hStdError = hPipeWrite;

			if (::CreateProcessA(program.c_str(), (LPSTR)args.c_str(),
				NULL,  // process security
				NULL,  // thread security
				TRUE,  //inheritance
				0,     //no startup flags
				NULL,  // no special environment
				NULL,  //default startup directory
				&startupInfo,
				&processInfo))
			{
				CloseHandle(hPipeWrite);
				return true;
			}
		}
		CloseHandle(hPipeWrite);
		return false;
	}
	bool run_shell(){
		if (CreatePipe(&hPipeRead, &hPipeWrite, &saOutPipe, MY_PIPE_BUFFER_SIZE))
		{
			startupInfo.hStdOutput = hPipeWrite;
			startupInfo.hStdError = hPipeWrite;

			if (::CreateProcessA(NULL, (LPSTR)program.c_str(),
				NULL,  // process security
				NULL,  // thread security
				TRUE,  //inheritance
				0,     //no startup flags
				NULL,  // no special environment
				NULL,  //default startup directory
				&startupInfo,
				&processInfo))
			{
				CloseHandle(hPipeWrite);
				return true;
			}
		}
		CloseHandle(hPipeWrite);
		return false;
	}

	static lisp::Value make_value(std::vector<lisp::Value> &args, lisp::Environment &env) {
		auto ptr = std::make_shared<w_process>();
        if (!ptr){
            throw std::runtime_error("no memory");
        };
		std::string name;
		ptr->name = name;
		ptr->program = args[0].as_string();
		for (int i=1; i<args.size(); i++){
			ptr->args += args[i].as_string();
			if (i+1 < args.size()){
				ptr->args += " ";
			}
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
        lisp::eval_args(args, env);
        return w_process::make_value(args, env);
    }));
    lisp::global_set( "process-start", lisp::Value("process-start", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size() < 1) {
            throw lisp::Error(lisp::Value::string("process-start"), env, "need more args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_process::ptr_from_value(args[0], env);
		if (args.size() == 1){
			auto ok = ptr->run();
			if (!ok){
				throw lisp::Error(lisp::Value::string("process-start"), env, (std::string("failed to start process '")+ptr->program + std::string("'")).c_str());
			}
		} else if ( args[1].as_bool() ) {
			auto ok = ptr->run_shell();
			if (!ok){
				throw lisp::Error(lisp::Value::string("process-start"), env, (std::string("failed to start process '")+ptr->program + std::string("' by shell.")).c_str());
			}
		}

		return lisp::Value(1);
    }));
    lisp::global_set( "process-read", lisp::Value("process-read", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()!=1) {
            throw lisp::Error(lisp::Value::string("process-read"), env, "need more args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_process::ptr_from_value(args[0], env);
		return lisp::Value::string(ptr->read(env));
    }));
    lisp::global_set( "process-read-f", lisp::Value("process-read-f", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()!=2) {
            throw lisp::Error(lisp::Value::string("process-read-f"), env, "need more args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_process::ptr_from_value(args[0], env);
		ptr->read(env, args[1]);
		return args[0];
    }));
    lisp::global_set( "process-redirect", lisp::Value("process-redirect", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
        if (args.size()!=2) {
            throw lisp::Error(lisp::Value::string("process-redirect"), env, "need more args");
        }
        lisp::eval_args(args, env);
        auto ptr = w_process::ptr_from_value(args[0], env);
		std::string filename = args[1].as_string();
		ptr->redirect(env, filename);
		return args[0];
    }));
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

		
    
}