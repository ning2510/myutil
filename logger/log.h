#ifndef _LOG_H
#define _LOG_H

#include <sstream>
#include <string>
#include <memory>
#include <time.h>
#include <sys/time.h>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <semaphore.h>

namespace util {

enum LogLevel {
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    NONE = 5
};

static LogLevel g_log_level = DEBUG;

#define LOG_DEBUG \
    if(util::openLog() && util::LogLevel::DEBUG >= util::g_log_level) \
        util::LogTmp(util::LogEvent::ptr(new util::LogEvent(util::LogLevel::DEBUG, __FILE__, __LINE__, __func__))).getStringStream()

#define LOG_INFO \
    if(util::openLog() && util::LogLevel::INFO >= util::g_log_level)  \
        util::LogTmp(util::LogEvent::ptr(new util::LogEvent(util::LogLevel::INFO, __FILE__, __LINE__, __func__))).getStringStream()

#define LOG_WARN \
    if(util::openLog() && util::LogLevel::WARN >= util::g_log_level)  \
        util::LogTmp(util::LogEvent::ptr(new util::LogEvent(util::LogLevel::WARN, __FILE__, __LINE__, __func__))).getStringStream()

#define LOG_ERROR \
    if(util::openLog() && util::LogLevel::ERROR >= util::g_log_level) \
        util::LogTmp(util::LogEvent::ptr(new util::LogEvent(util::LogLevel::ERROR, __FILE__, __LINE__, __func__))).getStringStream()


/* 5MB 500ms */
void initLog(const char *file_name, const char *file_path = "./", int max_size = 5 * 1024 * 1024, int sync_interval = 500, LogLevel level = DEBUG);

pid_t getpid();
pid_t gettid();
bool openLog();

void setLogLevel(LogLevel level);
LogLevel stringToLevel(const std::string &str);
std::string levelToString(LogLevel level);

class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;

    LogEvent(LogLevel level, const char *file_name, int line, const char *func_name);
    ~LogEvent() {}

    void log();
    std::stringstream &getStringStream();
    
private:
    timeval m_timeval;
    LogLevel m_level;

    pid_t m_pid;
    pid_t m_tid;

    int m_line;
    std::string m_file_name;
    std::string m_func_name;

    std::stringstream m_ss;
};


class LogTmp {
public:
    explicit LogTmp(LogEvent::ptr event) : m_event(event) {}
    ~LogTmp() {
        m_event->log();
    }

    std::stringstream &getStringStream() {
        return m_event->getStringStream();
    }

private:
    LogEvent::ptr m_event;
};


class AsyncLogger {
public:
    typedef std::shared_ptr<AsyncLogger> ptr;

    AsyncLogger(const char *file_name, const char *file_path, int max_size);
    ~AsyncLogger() {
        pthread_cond_destroy(&m_cond);
        pthread_mutex_destroy(&m_mutex);
    }

    void push(std::vector<std::string> &buffer);
    void flush();
    void stop();
    void join();

    static void *execute(void *arg);

private:
    std::string m_file_name;
    std::string m_file_path;
    std::string m_date;

    int m_max_size;
    int m_no;

    bool m_stop;
    bool m_need_reopen;
    FILE *m_file_handle;

    sem_t m_sem;
    pthread_mutex_t m_mutex;
    pthread_t m_thread;
    pthread_cond_t m_cond;
    
    std::queue <std::vector<std::string>> m_tasks;
};


class Logger {
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger() : m_is_init(false), m_stop(false) {
        ::pthread_mutex_init(&m_mutex, nullptr);
    }
    ~Logger() {
        stop();
        m_async_logger->join();
        ::pthread_mutex_destroy(&m_mutex);
    }

    void init(const char *file_name, const char *file_path, 
              int max_size, int sync_interval);

    
    /* tmp method */
    static void *collect(void *arg);
    void join() {
        ::pthread_join(m_thread, nullptr);
    }

    void start();
    void stop();
    void loopFunc();
    void push(const std::string &msg);

    AsyncLogger::ptr getAsyncLogger() {
        return m_async_logger;
    }

    std::vector<std::string> m_buffer;

private:
    /* tmp method */
    bool m_stop;
    pthread_t m_thread;

    pthread_mutex_t m_mutex;
    bool m_is_init;
    int m_sync_interval;

    AsyncLogger::ptr m_async_logger;
};

void Exit(int code);

}   // namespace util

#endif
