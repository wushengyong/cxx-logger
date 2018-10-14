# 使用VS2017编译

## 用法

std::string log_f1 = "log1.log";
LOG_DEBUG(log_f1) << "test " << 1 << logger_endl;
LOG_INFO(log_f1) << "test " << 1 << logger_endl;
LOG_WARN(log_f1) << "test " << 1 << logger_endl;
LOG_ERR(log_f1) << "test " << 1 << logger_endl;
