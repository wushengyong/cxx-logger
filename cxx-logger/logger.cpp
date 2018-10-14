#include "pch.h"
#include "logger.h"

using namespace app_logger;

logger_backend::logger_backend(const std::string& path)
{
	this->path = path;
	
	fs = 0;

#if ENABLE_ASYNC_LOGGER == 1
	running = false;
#endif
}


logger_backend::~logger_backend()
{
#if ENABLE_ASYNC_LOGGER == 1
	stop_thread();
#endif

	if (f.is_open()) {
		f.flush();
	}
}

void logger_backend::Start()
{
	f.open(this->path, std::ios_base::app);

	if (f.is_open()) {
		f.seekp(0, f.end);
		fs = f.tellp();
	}

#if ENABLE_ASYNC_LOGGER == 1
	cur_write_b = std::shared_ptr<std::string>(new std::string());
	next_write_b = std::shared_ptr<std::string>(new std::string());
	start_thread();
#endif
}

bool logger_backend::is_must_backup() const
{
	return fs >= MAX_LOG_SIZE;
}

void logger_backend::write_log(const std::string& log)
{
	// �ж��ļ���С
	if (!f.is_open())return;
	if (is_must_backup()) {
		// ��־�ļ����������ƣ�����
		f.close(); // �ȹر��ļ�
		std::string new_path = path + "." + app_times::times::systime("%Y-%m-%d-%H-%M-%S");
		rename(path.c_str(), new_path.c_str());
		f.open(path, std::ios_base::app);
		fs = 0;
		if (!f.is_open())return;
	}
	f << log;
	fs += log.size();
}
#if ENABLE_ASYNC_LOGGER == 0
void logger_backend::WriteLog(const std::string& log, const logger_level& l)
{
	if (l < logger_factory::LOGGER_LEVEL) return;
	std::unique_lock<std::mutex> locker(lock);
	write_log(log);
	f.flush(); // ����������ļ�
}
#else
void logger_backend::WriteLog(const std::string& log, const logger_level& l)
{
	if (l < logger_factory::LOGGER_LEVEL) return;
	std::unique_lock<std::mutex> locker(lock);

	if (running == false) return; /// �Ѿ�û�����첽д�߳��ˣ�������д��־

	if (cur_write_b->size() < MAX_BUF_SIZE) {
		cur_write_b->append(log);
	}
	else {
		b.push_back(cur_write_b);

		if (next_write_b) {
			cur_write_b = next_write_b;
			next_write_b = std::shared_ptr<std::string>();
		}
		else {
			cur_write_b = std::shared_ptr<std::string>(new std::string());
		}
		cur_write_b->append(log);
		c.notify_one();
	}
}

void logger_backend::write_thread()
{
	std::shared_ptr<std::string> b1(new std::string());
	std::shared_ptr<std::string> b2(new std::string());
	std::vector<std::shared_ptr<std::string>> bufs2write;

	bool lastrecycle = false; // �Ƿ�ִ��������һ��д

	while (running || !lastrecycle)
	{
		{
			std::unique_lock<std::mutex> locker(lock);
			if (b.empty()) {
				c.wait_for(locker, std::chrono::milliseconds(ASYNC_THREAD_INTERVAL));
			}
			/// ����Ѿ����ˣ�����ǰ����������������������д��
			b.push_back(cur_write_b);
			cur_write_b.swap(b1);
			bufs2write.swap(b);

			if (!next_write_b) {
				next_write_b.swap(b2);
			}
		}
		for (size_t i = 0; i < bufs2write.size(); ++i) {
			if (bufs2write[i]->empty())continue;
			write_log(*bufs2write[i]);
		}
		f.flush(); // ����������ļ�

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
	c.notify_all();
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
	loggers = std::shared_ptr<logger_map>(new logger_map);
}

logger_factory::~logger_factory()
{

}

logger_backend& logger_factory::GetLoggerBackend(const std::string& path)
{
	logger_map_ptr loggers;

	{
		std::unique_lock<std::mutex> locker(g_inst.lock);
		loggers = g_inst.loggers;
	}

	if (loggers->find(path) != loggers->end()) {
		return (*(*loggers)[path]);
	}

	/// �Ҳ���������Ҫ����һ��logger_backend
	logger_ptr logger(new logger_backend(path));

	{
		std::unique_lock<std::mutex> locker(g_inst.lock);
		// ȷ���Ƿ����������½���ͬ��һ��ģ��
		if (g_inst.loggers->find(path) != g_inst.loggers->end()) {
			return (*(*g_inst.loggers)[path]);
		}

		if (g_inst.loggers.unique()) {
			(*g_inst.loggers)[path] = logger;
		}
		else {
			/// ����һ��������ͬʱ�����������ڵ�
			loggers = std::shared_ptr<logger_map>(new logger_map(*g_inst.loggers));
			(*loggers)[path] = logger;
			g_inst.loggers = loggers;
		}
	}

	logger->Start();
	return (*logger);
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