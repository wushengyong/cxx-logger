#pragma once
#include <sstream>
#include <mutex>
#include <map>
#include <memory>
#include <vector>

#include "strings.h"
#include "times.h"

namespace app_logger
{
#define ENABLE_ASYNC_LOGGER    1    // 是否启用异步线程写入日志

	enum logger_delimter {
		logger_endl
	};
	enum logger_level {
		logger_debug,
		logger_info,
		logger_warn,
		logger_err,
	};

	/// 真正写入日志的后端
	class logger_backend
	{
	public:
		logger_backend();
		~logger_backend();

#if ENABLE_ASYNC_LOGGER == 0
		void WriteLog(const std::string& path, const std::string& log, const logger_level& l);
#else 
		void WriteLog(const std::string& path, const std::string& log, const logger_level& l);
		
	private:
		void write_thread(); // 异步写线程
		void stop_thread(); // 停止写线程
		void start_thread(); // 启动异步写线程
	private:
		std::thread t; // 线程
		std::condition_variable c; // 条件变量
		bool running; // 是否正在运行线程

	private:
		typedef std::map<std::string, std::string> path_log_map;

		struct logger_info_struct {
			path_log_map logs_;
			size_t size_;

		public:
			logger_info_struct() { size_ = 0; }

			void clear() {
				logs_.clear();
				size_ = 0;
			}
			void append(const std::string& path, const std::string& log) {
				if (logs_.find(path) == logs_.end()) {
					logs_[path] = "";
				}
				logs_[path].append(log);
				size_ += log.size();
			}
			size_t size() const { return size_; }
			const path_log_map& logs() const { return logs_; }
		};

		typedef std::shared_ptr<logger_info_struct> logger_info_struct_ptr;
		typedef std::vector<logger_info_struct_ptr> logger_info_struct_ptr_vec;

		logger_info_struct_ptr_vec b; // 缓冲区，放置待写入的日志
		logger_info_struct_ptr cur_write_b; // 当前的写入缓冲区
		logger_info_struct_ptr next_write_b; // 下一个可用缓冲区

	private:
		static const int ASYNC_THREAD_INTERVAL = 2 * 1000; // 2s
#endif
	private:
		void write_log(std::ofstream& f, size_t& fs,
			const std::string&path, const std::string& log);
		bool is_must_backup(int fs) const;
	private:
		std::mutex lock; // 写锁
	private:
		static const size_t MAX_LOG_SIZE = 1024 * 1024 * 10; // 10M
		static const size_t MAX_BUF_SIZE = 1024 * 4; // 4K
	};

	/// 获取对应后端的日志模块
	class logger_factory
	{
	public:
		static const logger_level LOGGER_LEVEL = logger_debug;

		static void Log(const std::string& path, const std::string& log, const logger_level& level);
	private:
		logger_factory();
		~logger_factory();

		logger_backend logger;

		static logger_factory g_inst;
	};


	/// 作为格式化日志的实体，并不真正写日志到文件中去
	class logger_front
	{
	public:
		logger_front(const std::string& path);
		logger_front(const std::wstring& path);

		~logger_front();

		template <typename T>
		logger_front& operator << (const T & t) {
			std::ostringstream s;
			s << t;
			info += s.str();
			return *this;
		}

		template <>
		logger_front& operator << (const std::wstring& t) {
			std::string s = app_strings_convertion::strings::ws2s(t);
			return (*this) << s;
		}

		logger_front& operator << (const wchar_t* t) {
			return (*this) << (std::wstring(t));
		}
		// 设置级别
		template <>
		logger_front& operator<< (const logger_level& t) {
			level = t;
			return *this;
		}
		// 推送到后端进行实际的写入
		template <>
		logger_front& operator << (const logger_delimter& t) {
			/// 写入换行，以及直接写入文件
			info += "\n";
			if (!path.empty()) {
				logger_factory::Log(path, info, level);
				info = "";
			}
			return *this;
		}

	private:
		std::string path;
		std::string info;
		logger_level level;
	};


#define LOG_MACRO(path,level,levelname) \
	app_logger::logger_front(path) \
	<< level \
	<< "[" << app_times::times::systime() << "][" levelname "]"\
	<< "[L" << __LINE__ << "]" \
	<< "[" << __FUNCTION__ << "] "

#define LOG_DEBUG(path)    LOG_MACRO(path, logger_debug, "DEBUG")
#define LOG_INFO(path)     LOG_MACRO(path, logger_info, "INFO ")
#define LOG_WARN(path)     LOG_MACRO(path, logger_warn, "WARN ")
#define LOG_ERR(path)      LOG_MACRO(path, logger_err, "ERR  ")
}


