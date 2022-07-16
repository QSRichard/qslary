#ifndef __QSLARY_LOGGER_H__
#define __QSLARY_LOGGER_H__

#include "qslary/base/Mutex.h"
#include "qslary/base/singleton.h"
#include "qslary/base/util.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#define QSLARY_LOG_LEVEL(logger, level)                                        \
    if (logger->getLevel() <= level)                                           \
    qslary::LogEventWarp(                                                      \
        qslary::LogEvent::ptr(new qslary::LogEvent(                            \
            logger, level, __FILE__, __LINE__, 0,                              \
            qslary::detail::getThreadId(), qslary::detail::getFiberId(),       \
            time(0), qslary::CurrentThread::name())))                          \
        .getSS()

#define QSLARY_LOG_DEBUG(logger)                                               \
    QSLARY_LOG_LEVEL(logger, qslary::LogLevel::DEBUG)
#define QSLARY_LOG_INFO(logger) QSLARY_LOG_LEVEL(logger, qslary::LogLevel::INFO)
#define QSLARY_LOG_WARING(logger)                                              \
    QSLARY_LOG_LEVEL(logger, qslary::LogLevel::WARING)
#define QSLARY_LOG_ERROR(logger)                                               \
    QSLARY_LOG_LEVEL(logger, qslary::LogLevel::ERROR)
#define QSLARY_LOG_FATAL(logger)                                               \
    QSLARY_LOG_LEVEL(logger, qslary::LogLevel::FATAL)

#define QSLARY_LOG_ROOT() qslary::LoggerMgr::getInstance()->getRoot()
#define QSLARY_LOG_NAME(name) qslary::LoggerMgr::getInstance()->getLogger(name);

namespace qslary
{
class Logger;
class LogManger;

class LogLevel
{
public:
    enum Level
    {
        UNKONW = 0,
        DEBUG = 1,
        INFO = 2,
        WARRING = 3,
        ERROR = 4,
        FATAL = 5
    };
    static const char *ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string &str);
};
class LogEvent
{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
             const char *file, uint32_t line, uint32_t elpase,
             uint32_t threadId, uint32_t fiberId, uint64_t time,
             const std::string &thread_name);
    const char *getFile() const { return m_file; };
    uint32_t getLine() const { return m_line; };
    uint32_t getThreadID() const { return m_thread_id; };
    uint32_t getFiberID() const { return m_fiber_id; };
    uint64_t getTime() const { return m_time; };
    uint32_t getElpase() const { return m_elpase; };
    const std::string &getThreadName() const { return thread_name_; };

    const std::string getContent() const { return m_ss.str(); };
    std::shared_ptr<Logger> getLogger() const { return m_logger; };
    LogLevel::Level getLevel() const { return m_level; };
    std::stringstream &getSS() { return m_ss; };

private:
    const char *m_file = nullptr; // 文件名
    uint32_t m_line = 0;          // 行号
    uint32_t m_elpase = 0;        // 毫秒数
    uint32_t m_thread_id = 0;     // 线程id
    uint32_t m_fiber_id = 0;      // 协程id
    uint64_t m_time;              // 时间戳
    std::string thread_name_;     // 线程名称
    std::string m_content;
    std::stringstream m_ss;
    // TAG 注意Logger类在LogEvent之后
    // Logger::ptr m_logger;
    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};

class LogEventWarp
{
public:
    LogEventWarp(LogEvent::ptr event);
    ~LogEventWarp();
    std::stringstream &getSS();

private:
    LogEvent::ptr m_event;
};

class LoggerFormatter
{
public:
    typedef std::shared_ptr<LoggerFormatter> ptr;
    LoggerFormatter(const std::string &partten);
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                       LogEvent::ptr event);
    void init();
    void setPartten(std::string &val) { m_partten = val; };
    std::string getPartten() const { return m_partten; };

public:
    class FormatItem
    {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem(){};
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    bool isError() const { return m_error == true; };

private:
    std::string m_partten;
    std::vector<FormatItem::ptr> m_items;
    bool m_error = false;
};

// 日志输出地
class LoggerAppender
{
    friend class Logger;

public:
    typedef std::shared_ptr<LoggerAppender> ptr;
    typedef qslary::MutexLock MutexType;
    // FIXME LoggerAppender 的构造析构函数 以及子类的构造析构函数
    // LoggerAppender(){};
    virtual ~LoggerAppender(){};

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) = 0;

    virtual std::string toYamlString() = 0;

    void setFormatter(LoggerFormatter::ptr formater);

    LoggerFormatter::ptr getForamter();

    void setLevel(LogLevel::Level level) { m_level = level; };

    LogLevel::Level getLevel() { return m_level; };

protected:
    /// 日志级别
    LoggerFormatter::ptr m_formater;
    /// 是否有自己的日志格式器
    bool m_hasFormatter = false;

    MutexType mutex_;

    // FIXME LoggerAppender的LogLevel 以及构造和析构
    LogLevel::Level m_level = LogLevel::DEBUG;
};

class StdoutLogAppender : public LoggerAppender
{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) override;
    virtual std::string toYamlString() override;
};

class FileLogAppender : public LoggerAppender
{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string &filename);

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) override;
    virtual std::string toYamlString() override;

    bool reopen();

public:
    // std::string m_name;
    std::string m_filename;
    std::ofstream m_filestream;
    uint64_t lastTime_ = 0;
};

// 日志器
class Logger : public std::enable_shared_from_this<Logger>
{
    friend class LogManger;

public:
    typedef std::shared_ptr<Logger> ptr;
    typedef qslary::MutexLock MutexType;

    Logger(const std::string &name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
    void addAppender(LoggerAppender::ptr appender);
    void delAppender(LoggerAppender::ptr appender);
    void clearAppender();
    LogLevel::Level getLevel() const { return m_level; };
    void setLevel(LogLevel::Level val) { m_level = val; };
    std::string getName() const { return m_name; };
    void setRootLogger(Logger::ptr root) { m_root = root; };
    Logger::ptr getRootLogger() const { return m_root; };
    void setFormatter(LoggerFormatter::ptr val);
    void setFormatter(const std::string &val);
    LoggerFormatter::ptr getFormatter();
    std::string toYamlString();

private:
    std::string m_name;
    LogLevel::Level m_level;

    MutexType mutex_;
    std::list<LoggerAppender::ptr> m_appenders;
    LoggerFormatter::ptr m_formatter;
    Logger::ptr m_root;
};

class LogManger
{
public:
    typedef qslary::MutexLock MutexType;
    LogManger();
    Logger::ptr getLogger(const std::string &name);
    void init();
    Logger::ptr getRoot() const { return m_root; };
    std::string toYamlString();

private:
    MutexType mutex_;
    std::map<std::string, Logger::ptr> m_loggers;

    Logger::ptr m_root;
};

typedef qslary::Singleton<LogManger> LoggerMgr;
} // namespace qslary

#endif