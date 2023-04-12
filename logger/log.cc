#include "log.h"

#include <unistd.h>
#include <syscall.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

namespace util {

static pid_t g_pid = 0;
static thread_local pid_t g_tid = 0;

static util::Logger::ptr g_logger;

void initLog(const char *file_name, const char *file_path /*= "./"*/, int max_size /*= 5 MB*/, 
             int sync_interval /*= 500 ms*/, LogLevel level /*= DEBUG*/) {
    
    if(g_logger) return ;
    
    g_logger = std::make_shared<Logger>();
    setLogLevel(level);
    g_logger->init(file_name, file_path, max_size, sync_interval);
    g_logger->start();
}

void CoredumpHandler(int signal_no) {
    LOG_ERROR << "progress received invalid signal, will exit";
    
    g_logger->stop();
    g_logger->getAsyncLogger()->join();

    ::signal(signal_no, SIG_DFL);
    ::raise(signal_no);
}

pid_t getpid() {
    if(g_pid == 0) {
        g_pid = ::getpid();
    }
    return g_pid;
}

pid_t gettid() {
    if(g_tid == 0) {
        g_tid = ::syscall(SYS_gettid);
    }
    return g_tid;
}

bool openLog() {
    if(!g_logger) return false;
    return true;
}

void setLogLevel(LogLevel level) {
    g_log_level = level;
}

LogLevel stringToLevel(const std::string &str) {
    if(str == "DEBUG") return LogLevel::DEBUG;
    if(str == "INFO") return LogLevel::INFO;
    if(str == "WARN") return LogLevel::WARN;
    if(str == "ERROR") return LogLevel::ERROR;
    if(str == "NONE") return LogLevel::NONE;
}

std::string levelToString(LogLevel level) {
    std::string str = "DEBUG";

    switch (level) {
        case LogLevel::DEBUG:
            str = "DEBUG";
            break;
        case LogLevel::INFO:
            str = "INFO";
            break;
        case LogLevel::WARN:
            str = "WARN";
            break;
        case LogLevel::ERROR:
            str = "ERROR";
            break;
        case LogLevel::NONE:
            str = "NONE";
            break;
        default:
            break;
    }

    return str;
}

// LogEvent ---------------------------------------------- 
LogEvent::LogEvent(LogLevel level, const char *file_name, int line, const char *func_name)
    : m_level(level),
      m_pid(0),
      m_tid(0),
      m_file_name(file_name),
      m_line(line),
      m_func_name(func_name) {}

void LogEvent::log() {
    m_ss << "\n";
    if(m_level >= g_log_level) {
        g_logger->push(m_ss.str());
    }
}

std::stringstream &LogEvent::getStringStream() {
    ::gettimeofday(&m_timeval, nullptr);

    struct tm time;
    ::localtime_r(&(m_timeval.tv_sec), &time);

    const char *format = "%Y-%m-%d %H:%M:%S";
    char buf[128];
    ::strftime(buf, sizeof(buf), format, &time);
    m_ss << "[" << buf << "." << m_timeval.tv_usec << "]\t";

    std::string s_level = levelToString(m_level);
    m_ss << "[" << s_level << "]\t";

    m_pid = getpid();
    m_tid = gettid();

    m_ss << "[" << m_pid << "]\t[" << m_tid << "]\t[" 
         << m_file_name << ":" << m_line << "]\t";

    return m_ss;
}


// AsyncLogger ---------------------------------------------- 

AsyncLogger::AsyncLogger(const char *file_name, const char *file_path, int max_size)
    : m_file_name(file_name),
      m_file_path(file_path),
      m_max_size(max_size),
      m_no(0),
      m_stop(false),
      m_need_reopen(false),
      m_file_handle(nullptr) {

    int rt = sem_init(&m_sem, 0, 0);
    assert(rt == 0);

    rt = ::pthread_create(&m_thread, nullptr, &AsyncLogger::execute, this);
    assert(rt == 0);

    rt = ::sem_wait(&m_sem);
    assert(rt == 0);

    sem_destroy(&m_sem);
}

void AsyncLogger::push(std::vector<std::string> &buffer) {
    if(buffer.empty()) return ;

    ::pthread_mutex_lock(&m_mutex);
    m_tasks.push(buffer);
    ::pthread_mutex_unlock(&m_mutex);

    ::pthread_cond_signal(&m_cond);
}

void AsyncLogger::flush() {
    if(m_file_handle) {
        ::fflush(m_file_handle);
    }
}

void AsyncLogger::stop() {
    if(!m_stop) {
        m_stop = true;
        ::pthread_cond_signal(&m_cond);
    }
}

void AsyncLogger::join() {
    ::pthread_join(m_thread, nullptr);
}

void *AsyncLogger::execute(void *arg) {
    AsyncLogger *ptr = reinterpret_cast<AsyncLogger *>(arg);
    int rt = ::pthread_cond_init(&ptr->m_cond, nullptr);
    assert(rt == 0);

    rt = ::sem_post(&ptr->m_sem);
    assert(rt == 0);

    while(1) {
        ::pthread_mutex_lock(&ptr->m_mutex);

        while(ptr->m_tasks.empty() && !ptr->m_stop) {
            ::pthread_cond_wait(&ptr->m_cond, &ptr->m_mutex);
        }
        bool is_stop = ptr->m_stop;
        if(is_stop && ptr->m_tasks.empty()) {
            ::pthread_mutex_unlock(&ptr->m_mutex);
            break;
        }

        std::vector <std::string> tmp;
        tmp.swap(ptr->m_tasks.front());
        ptr->m_tasks.pop();

        ::pthread_mutex_unlock(&ptr->m_mutex);

        timeval now;
        ::gettimeofday(&now, nullptr);

        struct tm now_time;
        ::localtime_r(&(now.tv_sec), &now_time);

        const char *format = "%Y%m%d";
        char date[32];
        ::strftime(date, sizeof(date), format, &now_time);
        if(ptr->m_date != std::string(date)) {
            ptr->m_no = 0;
            ptr->m_date = std::string(date);
            ptr->m_need_reopen = true;
        }

        if(!ptr->m_file_handle) {
            ptr->m_need_reopen = true;
        }

        std::stringstream ss;
        ss << ptr->m_file_path << ptr->m_file_name << "_" 
           << ptr->m_date << "_" << ptr->m_no << ".log";
        std::string full_file_name = ss.str();

        if(ptr->m_need_reopen) {
            if(ptr->m_file_handle) {
                ::fclose(ptr->m_file_handle);
            }

            ptr->m_file_handle = ::fopen(full_file_name.c_str(), "a");
            if(ptr->m_file_handle == nullptr) {
                printf("open fail errno = %d reason = %s \n", errno, strerror(errno));
                exit(-1);
            }
            ptr->m_need_reopen = false;
        }

        if(::ftell(ptr->m_file_handle) > ptr->m_max_size) {
            ::fclose(ptr->m_file_handle);

            ptr->m_no++;
            std::stringstream ss2;
            ss2 << ptr->m_file_path << ptr->m_file_name << "_" 
               << ptr->m_date << "_" << ptr->m_no << ".log";
            full_file_name = ss2.str();
        
            ptr->m_file_handle = ::fopen(full_file_name.c_str(), "a");
            if(ptr->m_file_handle == nullptr) {
                printf("open fail errno = %d reason = %s \n", errno, strerror(errno));
                exit(-1);
            }
            ptr->m_need_reopen = false;
        }

        for(auto s : tmp) {
            if(!s.empty()) {
                ::fwrite(s.c_str(), 1, s.length(), ptr->m_file_handle);
            }
        }
        tmp.clear();
        ::fflush(ptr->m_file_handle);

        if(is_stop) break;
    }
    
    if (ptr->m_file_handle) {
        ::fclose(ptr->m_file_handle);
        ptr->m_file_handle = nullptr;
    }

    return nullptr;
}


// Logger ---------------------------------------------- 
void Logger::init(const char *file_name, const char *file_path, 
                  int max_size, int sync_interval) {
    
    if(!m_is_init) {
        m_sync_interval = sync_interval;
        m_async_logger = std::make_shared<AsyncLogger>(file_name, file_path, max_size);

        ::signal(SIGSEGV, CoredumpHandler);
        ::signal(SIGABRT, CoredumpHandler);
        ::signal(SIGTERM, CoredumpHandler);
        ::signal(SIGKILL, CoredumpHandler);
        ::signal(SIGINT, CoredumpHandler);
        ::signal(SIGSTKFLT, CoredumpHandler);
        ::signal(SIGPIPE, SIG_IGN);   /* ignore */

        m_is_init = true;
    }
}

void *Logger::collect(void *arg) {
    Logger* ptr = reinterpret_cast<Logger*>(arg);
    ptr->m_stop = false;
    __useconds_t s_time = ptr->m_sync_interval * 1000;

    while(!ptr->m_stop) {
        ::usleep(s_time);
        ptr->loopFunc();
    }

    return nullptr;
}

void Logger::start() {
    /* tmp method */
    int rt = ::pthread_create(&m_thread, nullptr, &Logger::collect, this);
    assert(rt == 0);
}

void Logger::stop() {
    /* tmp method */
    m_stop = true;

    loopFunc();
    join();
    m_async_logger->stop();
    m_async_logger->flush();
}

void Logger::loopFunc() {
    std::vector<std::string> tmp;

    ::pthread_mutex_lock(&m_mutex);
    tmp.swap(m_buffer);
    ::pthread_mutex_unlock(&m_mutex);

    m_async_logger->push(tmp);
}

void Logger::push(const std::string &msg) {
    ::pthread_mutex_lock(&m_mutex);

    m_buffer.push_back(std::move(msg));
    
    ::pthread_mutex_unlock(&m_mutex);
}

void Exit(int code) {
    g_logger->stop();
    g_logger->getAsyncLogger()->join();

    _exit(code);
}

}   // namespace util