
project(goap CXX)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_PREFIX_PATH D:/PocoBin/static)

find_package(Poco 1.9 CONFIG REQUIRED COMPONENTS Foundation Data DataSQLite DataMySQL  Net Util XML JSON Redis MongoDB)
if(NOT Poco_FOUND)
	message(STATUS "Poco not found. you should set CMAKE_PREFIX_PATH first in cache file.")
endif()
set(poco_libs  Poco::Foundation Poco::Data Poco::DataMySQL Poco::Redis Poco::MongoDB Poco::Net Poco::Util Poco::XML Poco::JSON Poco::DataSQLite )


set(src 
	goap/Action.cpp
	goap/Action.h
	goap/Node.cpp
	goap/Node.h
	goap/Planner.cpp
	goap/Planner.h
	goap/WorldState.cpp
	goap/WorldState.h
	goap/statemachine.cpp
	goap/statemachine.h
	wlisp.cpp
	wlisp.hpp
	math.cpp

	#process/win/process.cpp
	winit.cpp
	buildin/process.cpp
	threading.cpp
	buildin/jobq.cpp
	buildin/w_joqq.cpp
	buildin/files.cpp
	buildin/exception.cpp
	buildin/w_global_auto_init.hpp
	buildin/w_global_auto_init.cpp
	buildin/w_data.cpp
	buildin/w_tcpserver.cpp
	buildin/db/cdbi.cpp
	buildin/db/cdbi.h
	buildin/db/mpmcbuffer.cpp
	buildin/db/mpmcbuffer.h

    )
    

add_executable(helloworld helloworld.cpp)

add_library(lisp SHARED ${src} )
target_include_directories(lisp PUBLIC goap . buildin buildin/db)
target_link_libraries(lisp PUBLIC ${poco_libs} )
target_compile_definitions(lisp PRIVATE WLISP_API_Exports=1 LISP_BUILD_LIB=1 LOAD_DEFAULT_LIB=1 )
#target_compile_definitions(lisp PUBLIC  )


add_library(lisp_static STATIC ${src} )
target_include_directories(lisp_static PUBLIC goap . buildin )
target_link_libraries(lisp_static PUBLIC ${poco_libs} )
target_compile_definitions(lisp_static PRIVATE  LISP_BUILD_LIB=1 LOAD_DEFAULT_LIB=1 BUILD_STATIC_LIB=1 )

add_executable(ilisp main.cpp )
target_link_libraries(ilisp PUBLIC lisp_static )
target_compile_definitions(ilisp PRIVATE  BUILD_STATIC_LIB=1 )


add_executable(dlisp main.cpp )
target_link_libraries(dlisp PUBLIC lisp )
