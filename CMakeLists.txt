
project(goap CXX)

cmake_minimum_required(VERSION 3.1)

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

	process/win/process.cpp

	threading.cpp

	buildin/jobq.cpp
	buildin/w_joqq.cpp
    )
    
add_library(goap ${src})
target_compile_options(goap PUBLIC -g3 -O0 -Wall)
target_include_directories(goap PUBLIC . goap)

add_executable(goap_exe goap/demo.cpp)


target_link_libraries(goap_exe PUBLIC goap)


add_executable(wlisp main.cpp wlisp.cpp winit.cpp )
target_link_libraries(wlisp PUBLIC goap)

add_executable(helloworld helloworld.cpp)


