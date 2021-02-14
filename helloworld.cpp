/*
 * main.cpp
 *
 *  Created on: Jan 7, 2021
 *      Author: Administrator
 */
#include <iostream>

#include <chrono>
#include <thread>
 
//D:\code\cpp\wisp-main\goap\build\Debug\helloworld.exe


int main(int argc, const char **argv)
{
	std::chrono::duration<int> d;
	for (int i=0; i<10; i++){
		std::cout << "hello world i=" << i << std::endl;
		std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
	}

	std::cout << "final ext output:";
	for (int i=0; i<10; i++){
		std::cout << "loop i=" << i << "\n";
	}
	std::cout << std::endl;

	return 0;
}


