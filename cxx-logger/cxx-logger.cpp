// cxx-logger.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "logger.h"
#include <thread>

using namespace std;
using namespace app_logger;

int main()
{
	std::string log_f1 = "log1.log";
	std::string log_f2 = "log2.log";

	std::vector<std::thread> threads;

	for (size_t i = 0; i < 1000; i++) {
		if (i % 2) {
			threads.push_back(std::thread([&] {
				for (int j = 0; j < 100; j++) {
					LOG_DEBUG(log_f1) << "thread id: " << std::this_thread::get_id() << logger_endl;
					LOG_INFO(log_f1) << "thread id: " << std::this_thread::get_id() << logger_endl;
					LOG_WARN(log_f1) << "thread id: " << std::this_thread::get_id() << logger_endl;
					LOG_ERR(log_f1) << "thread id: " << std::this_thread::get_id() << logger_endl;
				}
			}));
		}
		else {
			threads.push_back(std::thread([&] {
				for (int j = 0; j < 100; j++) {
					LOG_DEBUG(log_f2) << "thread id: " << std::this_thread::get_id() << logger_endl;
					LOG_INFO(log_f2) << "thread id: " << std::this_thread::get_id() << logger_endl;
					LOG_WARN(log_f2) << "thread id: " << std::this_thread::get_id() << logger_endl;
					LOG_ERR(log_f2) << "thread id: " << std::this_thread::get_id() << logger_endl;
				}
			}));
		}
	}

	for (size_t i = 0; i < threads.size(); i++) {
		threads[i].join();
	}
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
