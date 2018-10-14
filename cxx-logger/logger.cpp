#include "pch.h"
#include "logger.h"
#include <fstream>

using namespace app_logger;

struct fs_info
{
	std::ofstream f;
	size_t size;
	fs_info() {
	}
	fs_info(const std::string& path) {
		f.open(path, std::ios_base::app);
		size = 0;

		if (f.is_open()) {
			f.seekp(0, f.end);
			size = (size_t)f.tellp(); 
		}
	}
};
logger_backend::logger_backend()
{
#if ENABLE_ASYNC_LOGGER == 1
	running = false;
	cur_write_b = logger_info_struct_ptr(new logger_info_struct);
	next_write_b = logger_info_struct_ptr(new logger_info_struct);
	start_thread();
#endif
}


logger_backend::~logger_backend()
{
#if ENABLE_ASYNC_LOGGER == 1
	stop_thread();
#endif
}


bool logger_backend::is_must_backup(int fs) const
{
	return fs >= MAX_LOG_SIZE;
}

void logger_backend::write_log(std::ofstream& f, size_t& fs, 
	const std::string&path, const std::string& log)
{
	if (!f.is_open())return;

	if (is_must_backup(fs)) {
		// 执行备份操作
		f.close(); // 先关闭文件
		std::string new_path = path + "." + app_times::times::systime("%Y-%m-%d-%H-%M-%S");
		rename(path.c_str(), new_path.c_str());
		f.open(path, std::ios_base::app);
		fs = 0;
		if (!f.is_open())return;
	}
	f << log;
}

#if ENABLE_ASYNC_LOGGER == 0
void logger_backend::WriteLog(const std::string& path, const std::string& log, const logger_level& l)
{
	if (l < logger_factory::LOGGER_LEVEL) return;
	std::unique_lock<std::mutex> locker(lock);
	fs_info info(path);
	write_log(info.f, info.size, path, log);
}
#else
void logger_backend::WriteLog(const std::string& path, const std::string& log, const logger_level& l)
{
	if (l < logger_factory::LOGGER_LEVEL) return;

	if (running == false) return; /// 已经没有在异步写线程了，不允许写日志
	
	std::unique_lock<std::mutex> locker(lock);

	if (cur_write_b->size() < MAX_BUF_SIZE) {
		cur_write_b->append(path, log);
	}
	else {
		b.push_back(cur_write_b);

		if (next_write_b) {
			cur_write_b = next_write_b;
			next_write_b.reset();
		}
		else {
			cur_write_b = logger_info_struct_ptr(new logger_info_struct);
		}
		cur_write_b->append(path, log);
		c.notify_one();
	}
}

void logger_backend::write_thread()
{
	logger_info_struct_ptr b1(new logger_info_struct);
	logger_info_struct_ptr b2(new logger_info_struct);

	logger_info_struct_ptr_vec bufs2write;

	bool lastrecycle = false; // 是否执行完成最后一次写

	while (running || !lastrecycle)
	{
		{
			std::unique_lock<std::mutex> locker(lock);
			if (b.empty()) {
				c.wait_for(locker, std::chrono::milliseconds(ASYNC_THREAD_INTERVAL));
			}
			/// 间隔已经过了，将当前缓冲区的内容拉过来进行写入
			b.push_back(cur_write_b);
			cur_write_b.swap(b1);
			bufs2write.swap(b);

			if (!next_write_b) {
				next_write_b.swap(b2);
			}
		}
		/// 写入日志
		std::map<std::string, fs_info> map_fs_info;

		for (auto& buf : bufs2write) {
			if (buf->size() == 0) continue;
			for (auto& log_info : buf->logs()) {
				if (map_fs_info.find(log_info.first) == map_fs_info.end()) {
					map_fs_info[log_info.first] = fs_info(log_info.first);
				}
				auto& fs_info = map_fs_info[log_info.first];
				write_log(fs_info.f, fs_info.size, log_info.first, log_info.second);
			}
		}


		if (!b1) {
			b1 = bufs2write.back();
			bufs2write.pop_back();
		}
		if (!b2) {
			b2 = bufs2write.back();
			bufs2write.pop_back();
		}
		b1->clear();
		b2->clear();
		bufs2write.clear();

		if (false == running) lastrecycle = true;
	}

}

void logger_backend::stop_thread()
{
	if (running == false) return;

	running = false;
	c.notify_one();
	t.join();
}

void logger_backend::start_thread()
{
	running = true;
	t = std::thread(std::bind(&logger_backend::write_thread, this));
}
#endif


logger_factory logger_factory::g_inst;

logger_factory::logger_factory()
{
}

logger_factory::~logger_factory()
{

}

void logger_factory::Log(const std::string& path, const std::string& log, const logger_level& level)
{
	g_inst.logger.WriteLog(path, log, level);
}

logger_front::logger_front(const std::string& path)
{
	this->path = path;
	this->level = logger_debug;
}

logger_front::logger_front(const std::wstring& path)
{
	this->path = app_strings_convertion::strings::ws2s(path);
	this->level = logger_debug;
}

logger_front::~logger_front()
{
	if (!info.empty() && !path.empty()) {
		(*this) << logger_endl;
	}
}