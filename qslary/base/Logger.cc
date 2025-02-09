#include "qslary/base/Config.h"
#include "qslary/base/Logger.h"
#include <boost/algorithm/string.hpp>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <time.h>
namespace qslary
{

const char *LogLevel::ToString(LogLevel::Level level)
{

    switch (level)
    {
#define QSLARY_LOGLEVEL_HELP(name)                                             \
    case LogLevel::name:                                                       \
        return #name;                                                          \
        break;
        QSLARY_LOGLEVEL_HELP(DEBUG);
        QSLARY_LOGLEVEL_HELP(INFO);
        QSLARY_LOGLEVEL_HELP(WARRING);
        QSLARY_LOGLEVEL_HELP(ERROR);
        QSLARY_LOGLEVEL_HELP(FATAL);
#undef QSLARY_LOGLEVEL_HELP
    default:
        return "UNKNOW";
    }
    return "UNKNOE";
};

LogLevel::Level LogLevel::FromString(const std::string &str)
{
/* #define XX(name) {\
//     std::string s=boost::to_upper(str); \
//     if(s == #name){ \
//         return LogLevel::name; \
//     }\
// }
//     XX(DEBUG);
//     XX(INFO);
//     XX(WARRING);
//     XX(ERROR);
//     XX(FATAL);
//     return LogLevel::UNKONW;
// #undef XX
*/
#define XX(level, v)                                                           \
    if (str == #v)                                                             \
    {                                                                          \
        return LogLevel::level;                                                \
    }

    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARRING, warring);
    XX(ERROR, error);
    XX(FATAL, fatal);
    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARRING, WARRING);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKONW;
#undef XX
}
// %m -- message
// %l -- line
// %n -- new line
// %p -- level
// %d -- day&time
// %t -- threadId
// %f -- fileName
// %r -- startTime
// %c -- logerName
class MessageFormatItem : public LoggerFormatter::FormatItem
{
public:
    MessageFormatItem(const std::string &str = ""){};
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getContent();
    }
};

class LevelFormatItem : public LoggerFormatter::FormatItem
{
public:
    LevelFormatItem(const std::string &str = ""){};
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LoggerFormatter::FormatItem
{
public:
    ElapseFormatItem(const std::string &str = ""){};
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getElpase();
    }
};

class LoggerNameFormatItem : public LoggerFormatter::FormatItem
{
public:
    LoggerNameFormatItem(const std::string &str = ""){};
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getLogger()->getName();
    }
};

class ThreadNameFormatItem : public LoggerFormatter::FormatItem
{
public:
    ThreadNameFormatItem(const std::string &str = "") {}

    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getThreadName();
    }
};

class ThreadIdFormatItem : public LoggerFormatter::FormatItem
{
public:
    ThreadIdFormatItem(const std::string &str = ""){};
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getThreadID();
    }
};

class FiberIdFormatItem : public LoggerFormatter::FormatItem
{
public:
    FiberIdFormatItem(const std::string &str = ""){};
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getFiberID();
    }
};

class DateTimeFormatItem : public LoggerFormatter::FormatItem
{
public:
    DateTimeFormatItem(const std::string &date_format = "%Y-%m-%d %H:%M:%S")
        : m_format(date_format)
    {
        if (m_format.empty())
        {
            m_format = ("%Y-%m-%d %H:%M:%S");
        }
    };
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        // os << event->getTime();
        os << buf;
    }

private:
    std::string m_format;
};

class LineFormatItem : public LoggerFormatter::FormatItem
{
public:
    LineFormatItem(const std::string &str = ""){};
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LoggerFormatter::FormatItem
{
public:
    NewLineFormatItem(const std::string &str = ""){};
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << std::endl;
    }
};

class FileNameFormatItem : public LoggerFormatter::FormatItem
{
public:
    FileNameFormatItem(const std::string &str = ""){};
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << event->getFile();
    }
};

class StringFormatItem : public LoggerFormatter::FormatItem
{
public:
    StringFormatItem(const std::string &str) : m_string(str){};
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << m_string;
    }

private:
    std::string m_string;
};

class TabFormatItem : public LoggerFormatter::FormatItem
{
public:
    TabFormatItem(const std::string &str = ""){};
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override
    {
        os << "\t";
    }
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   const char *file, uint32_t line, uint32_t elpase,
                   uint32_t threadId, uint32_t fiberId, uint64_t time,
                   const std::string &thread_name)
    : m_file(file), m_line(line), m_elpase(elpase), m_thread_id(threadId),
      m_fiber_id(fiberId), m_time(time), thread_name_(thread_name),
      m_logger(logger), m_level(level){};

LogEventWarp::LogEventWarp(LogEvent::ptr event) : m_event(event) {}

LogEventWarp::~LogEventWarp()
{
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream &LogEventWarp::getSS() { return m_event->getSS(); }

Logger::Logger(const std::string &name)
    : m_name(name), m_level(LogLevel::Level::DEBUG)
{
    // TAG LoggerFormatter default          // 时间 Tab 线程ID Tab 线程名 Tab
    // 协程ID Tab Level Tab Logger Tab File:line Tab message \n
    m_formatter.reset(new LoggerFormatter(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    // if(name=="root"){
    //     m_appenders.push_back(StdoutLogAppender::ptr(new StdoutLogAppender));
    // }
}

void Logger::setFormatter(LoggerFormatter::ptr val)
{
    // qslary::LoggerFormatter::ptr new_val(new qslary::LoggerFormatter(val));
    // qslary::LoggerFormatter::ptr new_val=val;
    // if (new_val->isError())
    // {
    //     std::cout << "Logger setFormatter name = " << m_name << " value = "
    //     << val->getPartten() << " invalid formatter" << std::endl; return;
    // }
    // m_formatter = new_val;

    qslary::MutexLockGuard lock(mutex_);
    m_formatter = val;

    for (auto &i : m_appenders)
    {
        qslary::MutexLockGuard appenderlock(i->mutex_);
        if (!i->m_hasFormatter)
        {
            i->m_formater = m_formatter;
        }
    }
}
void Logger::setFormatter(const std::string &val)
{
    // std::cout << "---" << val << std::endl;
    qslary::LoggerFormatter::ptr new_val(new qslary::LoggerFormatter(val));
    if (new_val->isError())
    {
        std::cout << "Logger setFormatter name=" << m_name << " value=" << val
                  << " invalid formatter" << std::endl;
        return;
    }
    // m_formatter = new_val;
    setFormatter(new_val);
    // m_formatter.reset(new qslary::LoggerFormatter(val));
}
LoggerFormatter::ptr Logger::getFormatter()
{
    qslary::MutexLockGuard lock(mutex_);
    return m_formatter;
}

std::string Logger::toYamlString()
{

    qslary::MutexLockGuard lock(mutex_);
    YAML::Node node;
    node["name"] = m_name;

    if (m_level != LogLevel::UNKONW)
    {
        node["level"] = LogLevel::ToString(m_level);
    }

    if (m_formatter)
    {
        node["formatter"] = m_formatter->getPartten();
    }
    for (auto &i : m_appenders)
    {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void Logger::addAppender(LoggerAppender::ptr appender)
{
    qslary::MutexLockGuard lock(mutex_);
    if (!appender->getForamter())
    {
        qslary::MutexLockGuard appenderLock(appender->mutex_);
        appender->m_formater = m_formatter;
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LoggerAppender::ptr appender)
{
    qslary::MutexLockGuard lock(mutex_);
    for (auto it = m_appenders.begin(); it != m_appenders.end(); it++)
    {
        if (*it == appender)
        {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppender()
{
    qslary::MutexLockGuard lock(mutex_);
    m_appenders.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event)
{
    // FIXME enable_from_this()
    if (level >= m_level)
    {
        qslary::MutexLockGuard lock(mutex_);
        auto self = shared_from_this();
        if (!m_appenders.empty())
        {
            for (auto &it : m_appenders)
            {
                it->log(self, level, event);
            }
        }
        else if (m_root)
        {
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event) { log(LogLevel::Level::DEBUG, event); }

void Logger::info(LogEvent::ptr event) { log(LogLevel::Level::INFO, event); }

void Logger::error(LogEvent::ptr event) { log(LogLevel::Level::ERROR, event); }

void Logger::fatal(LogEvent::ptr event) { log(LogLevel::Level::FATAL, event); }

LoggerFormatter::ptr LoggerAppender::getForamter()
{

    qslary::MutexLockGuard lock(mutex_);
    return m_formater;
}

void LoggerAppender::setFormatter(LoggerFormatter::ptr val)
{
    qslary::MutexLockGuard lock(mutex_);
    m_formater = val;
    if (m_formater)
    {
        m_hasFormatter = true;
    }
    else
    {
        m_hasFormatter = false;
    }
}

FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename)
{
    reopen();
};

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                          LogEvent::ptr event)
{
    // printf("FileLogAppender::log()\n");

    if (level >= m_level)
    {
        // FIXME thread safety && now == lastTime_???
        uint64_t now = time(0);
        if (now != lastTime_)
        {
            reopen();
            lastTime_ = now;
        }
        qslary::MutexLockGuard lock(mutex_);
        m_filestream << m_formater->format(logger, level, event);
    }
}

bool FileLogAppender::reopen()
{
    qslary::MutexLockGuard lock(mutex_);
    if (m_filestream)
    {
        m_filestream.close();
    }
    // printf("open file stream\n");
    m_filestream.open(m_filename);
    return !m_filestream;
}

std::string FileLogAppender::toYamlString()
{
    qslary::MutexLockGuard lock(mutex_);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if (m_level != LogLevel::UNKONW)
    {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_hasFormatter && m_formater)
    {
        node["formatter"] = m_formater->getPartten();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event)
{
    if (level >= m_level)
    {
        qslary::MutexLockGuard lock(mutex_);
        // std::cout<<"in StdoutLogAppender::log()"<<std::endl;
        std::cout << m_formater->format(logger, level, event);
    }
}

std::string StdoutLogAppender::toYamlString()
{
    qslary::MutexLockGuard lock(mutex_);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if (m_level != LogLevel::UNKONW)
    {
        node["level"] = qslary::LogLevel::ToString(m_level);
    }
    if (m_hasFormatter && m_formater)
    {
        node["formatter"] = m_formater->getPartten();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LoggerFormatter::LoggerFormatter(const std::string &partten)
    : m_partten(partten)
{
    init();
}
std::string LoggerFormatter::format(std::shared_ptr<Logger> logger,
                                    LogLevel::Level level, LogEvent::ptr event)
{
    std::stringstream ss;
    for (auto &it : m_items)
    {
        it->format(ss, logger, level, event);
    }
    return ss.str();
}

// TODO 优化这个函数
void LoggerFormatter::init()
{
    std::vector<std::tuple<std::string, std::string, int>> vec;

    std::string nstr;
    for (size_t i = 0; i < m_partten.size(); i++)
    {
        if (m_partten[i] != '%')
        {
            nstr.append(1, m_partten[i]);
            continue;
        }
        if ((i + 1) < m_partten.size() && m_partten[i + 1] == '%')
        {
            nstr.append(1, '%');
            continue;
        }
        size_t format_begin = i + 1;
        size_t j = format_begin;
        size_t type_begin = -1;
        int status = 0;
        std::string format_str;
        std::string type_str;
        while (j < m_partten.size())
        {
            // 如果目前没有碰到过'{'符号 并且当前既不是字母也不是'{' '}'
            // 此时是%xxx? ?表示非字母，非'{' '}'
            if (!status && !isalpha(m_partten[j]) && m_partten[j] != '{' &&
                m_partten[j] != '}')
            {
                format_str = m_partten.substr(format_begin, j - format_begin);
                j--;
                break;
            }
            // 开始解析type
            if (status == 0 && m_partten[j] == '{')
            {
                format_str = m_partten.substr(format_begin, j - format_begin);
                type_begin = ++j;
                // 改变状态 标识进入解析type阶段
                status = 1;
                continue;
            }
            // type解析成功
            if (status == 1 && m_partten[j] == '}')
            {
                //
                type_str = m_partten.substr(type_begin, j - type_begin);
                // 标识状态 表示解析成功
                // FIXME %xxx{xxx}xxx?? break?
                status = 0;
                break;
            }
            j++;
        }
        i = j;
        if (status == 0)
        {
            if (!nstr.empty())
            {
                vec.push_back(std::make_tuple(nstr, "", 0));
                nstr.clear();
            }
            // 这个判断是因为当 *%x
            // 这种情况时导致j到达partten的末尾退出while循环
            // 从而使得format_str没有被赋值
            if (j == m_partten.size())
            {
                format_str = m_partten.substr(format_begin, j - format_begin);
            }
            vec.push_back(std::make_tuple(format_str, type_str, 1));
        }
        else if (status == 1)
        {
            std::cout << "pattern parse error: " << m_partten << " - "
                      << m_partten.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern error>>", format_str, 2));
        }
    }
    if (!nstr.empty())
    {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    static std::map<std::string,
                    std::function<FormatItem::ptr(const std::string &str)>>
        s_format_items = {

#define XX(str, C)                                                             \
    {                                                                          \
#str,                                                                  \
            [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
    }

            XX(m, MessageFormatItem),
            XX(p, LevelFormatItem),
            XX(r, ElapseFormatItem),
            XX(c, LoggerNameFormatItem),
            XX(t, ThreadIdFormatItem),
            XX(n, NewLineFormatItem),
            XX(d, DateTimeFormatItem),
            XX(f, FileNameFormatItem),
            XX(l, LineFormatItem),
            XX(T, TabFormatItem),
            XX(F, FiberIdFormatItem),
            XX(N, ThreadNameFormatItem)
#undef XX
        };
    // printf("------vec.size = %ld------\n",vec.size());
    // for(auto& i:vec){
    //     std::cout<<std::get<2>(i)<<"   ";
    //     std::cout<<std::get<0>(i)<<"  ";
    //     std::cout<<std::get<0>(i).size()<<std::endl;
    // }
    int index = 0;
    for (auto &i : vec)
    {
        index++;
        if (std::get<2>(i) == 0)
        {
            // printf("entry if ok\n");
            m_items.push_back(
                FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }
        else
        {

            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end())
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(
                    "<<error format %" + std::get<0>(i) + ">>")));
                m_error = true;
                printf("\nHello error else %d \n", index);
            }
            else
            {
                // printf("entry else ok\n");
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
    }
}
LogManger::LogManger()
{
    m_root.reset(new Logger);
    m_root->addAppender(LoggerAppender::ptr(new StdoutLogAppender));
    m_loggers[m_root->getName()] = m_root;
    init();
}

Logger::ptr LogManger::getLogger(const std::string &name)
{
    qslary::MutexLockGuard lock(mutex_);
    auto it = m_loggers.find(name);
    if (it != m_loggers.end())
    {
        return it->second;
    }
    Logger::ptr logger(new Logger(name));
    logger->setRootLogger(m_root);
    m_loggers[name] = logger;
    return logger;
}

std::string LogManger::toYamlString()
{
    qslary::MutexLockGuard lock(mutex_);
    YAML::Node node;
    // node["name"]=
    for (auto &i : m_loggers)
    {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

struct LogAppenderDefine
{
    int type = 0; // 1 Stdout 2 File
    LogLevel::Level level = LogLevel::UNKONW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine &oth) const
    {
        return type == oth.type && level == oth.level &&
               formatter == oth.formatter && file == oth.file;
    }
};

struct LogDefine
{
    std::string name;
    LogLevel::Level level = LogLevel::UNKONW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine &oth) const
    {
        return name == oth.name && level == oth.level &&
               formatter == oth.formatter && appenders == oth.appenders;
    }

    bool operator<(const LogDefine &oth) const { return name < oth.name; }
};

template <> class LexicalCast<std::string, LogDefine>
{
public:
    LogDefine operator()(const std::string &v)
    {
        YAML::Node n = YAML::Load(v);
        LogDefine ld;
        if (!n["name"].IsDefined())
        {
            std::cout << "log config error: name is null, " << n << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = n["name"].as<std::string>();
        ld.level = LogLevel::FromString(
            n["level"].IsDefined() ? n["level"].as<std::string>() : "");
        if (n["formatter"].IsDefined())
        {
            ld.formatter = n["formatter"].as<std::string>();
        }

        if (n["appenders"].IsDefined())
        {
            // std::cout << "==" << ld.name << " = " << n["appenders"].size() <<
            // std::endl;
            for (size_t x = 0; x < n["appenders"].size(); ++x)
            {
                auto a = n["appenders"][x];
                if (!a["type"].IsDefined())
                {
                    std::cout << "log config error: appender type is null, "
                              << a << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if (type == "FileLogAppender")
                {
                    lad.type = 2;
                    if (!a["file"].IsDefined())
                    {
                        std::cout
                            << "log config error: fileappender file is null, "
                            << a << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if (a["formatter"].IsDefined())
                    {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                }
                else if (type == "StdoutLogAppender")
                {
                    lad.type = 1;
                    if (a["formatter"].IsDefined())
                    {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                }
                else
                {
                    std::cout << "log config error: appender type is invalid, "
                              << a << std::endl;
                    continue;
                }

                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template <> class LexicalCast<LogDefine, std::string>
{
public:
    std::string operator()(const LogDefine &i)
    {
        YAML::Node n;
        n["name"] = i.name;
        if (i.level != LogLevel::UNKONW)
        {
            n["level"] = LogLevel::ToString(i.level);
        }
        if (!i.formatter.empty())
        {
            n["formatter"] = i.formatter;
        }

        for (auto &a : i.appenders)
        {
            YAML::Node na;
            if (a.type == 2)
            {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            }
            else if (a.type == 1)
            {
                na["type"] = "StdoutLogAppender";
            }
            if (a.level != LogLevel::UNKONW)
            {
                na["level"] = LogLevel::ToString(a.level);
            }

            if (!a.formatter.empty())
            {
                na["formatter"] = a.formatter;
            }

            n["appenders"].push_back(na);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

// // 偏特化
// template<>
// class LexicalCast<std::string,LogDefine>{
//     public:
//         LogDefine operator()(const std::string& v){
//             YAML::Node node=YAML::Load(v);
//             LogDefine ret;

//             ret.name=node["name"].as<std::string>();
//             ret.level=LogLevel::FromString(node["level"].as<std::string>());
//             ret.formatter=node["formatter"].as<std::string>();

//             // for appenders
//             for(size_t i=0;i<node["appenders"].size();i++){
//                 auto t=node["appenders"][i];
//                 LogAppenderDefine def;
//                 std::string type=t["type"].as<std::string>();
//                 if(type=="FileLogAppender"){
//                     def.type=2;
//                     def.file=t["file"].as<std::string>();
//                     if(t["formatter"].IsDefined()){
//                         def.formatter=t["formatter"].as<std::string>();
//                     }
//                 }
//                 else if(type=="StdoutLogAppender"){
//                     def.type=1;
//                 }
//                 else{
//                     std::cout<<"Log config error appenser type is invalid,
//                     "<<type<<std::endl; continue;
//                 }
//                 ret.appenders.push_back(def);
//             }

//             return ret;
//         }
// };

// template<>
// class LexicalCast<LogDefine,std::string>{
//     public:
//         std::string operator()(const LogDefine& v){
//             YAML::Node node;
//             node["name"]=v.name;
//             node["level"]=qslary::LogLevel::ToString(v.level);
//             node["formatter"]=v.formatter;
//             for(size_t i=0;i<v.appenders.size();i++){
//                 YAML::Node na;
//                 if(v.appenders[i].type==1){
//                     na["type"]="StdoutLogAppender";
//                 }else if(v.appenders[i].type==2){
//                     na["type"]="FileLogAppender";
//                     na["file"]=v.appenders[i].file;
//                 }
//                 na["level"]=qslary::LogLevel::ToString(v.appenders[i].level);
//                 node["appenders"].push_back(na);
//             }
//             std::stringstream ss;
//             ss<<node;
//             return ss.str();
//         }
// };

// // 针对std::set<LogDefine>偏特化
// template<>
// class LexicalCast<std::string,std::set<LogDefine>>{
//     public:
//         std::set<LogDefine> operator()(const std::string& v){
//             YAML::Node node=YAML::Load(v);
//             std::set<LogDefine> ret;
//             for(size_t i=0;i<node.size();i++){
//                 auto n=node[i];
//                 LogDefine ld;
//                 ld.name=n["name"].as<std::string>();
//                 ld.level=LogLevel::FromString(n["level"].as<std::string>());
//                 if(n["formatter"].IsDefined()){
//                     ld.formatter=n["formatter"].as<std::string>();
//                 }
//                 if(n["appenders"].IsDefined()){
//                     for(size_t j=0;j<n["appenders"].size();j++){
//                         auto t=n["appenders"][j];
//                         LogAppenderDefine lad;
//                         std::string type=t["type"].as<std::string>();
//                         if(type=="FileLogAppender"){
//                             lad.type=2;
//                             lad.file=t["file"].as<std::string>();
//                         }else if(type=="StdoutLogAppender"){
//                             lad.type=1;
//                         }
//                         else{
//                             std::cout<<"log config error: appender type is
//                             invalid "<<type<<std::endl; continue;
//                         }
//                         lad.level=qslary::LogLevel::FromString(t["level"].as<std::string>());
//                         lad.formatter=t["formatter"].as<std::string>();
//                         ld.appenders.push_back(lad);
//                     }
//                 }
//                 ret.insert(ld);
//             }
//             return ret;
//         }
// };
// // 针对std::set<LogDefine>偏特化
// template<>
// class LexicalCast<std::set<LogDefine>,std::string>{
//     public:
//         std::string operator()(const std::set<LogDefine>& v){
//             YAML::Node node;
//             for(auto& i:v){
//                 node["name"]=i.name;
//                 node["level"]=qslary::LogLevel::ToString(i.level);
//                 node["formatter"]=i.formatter;
//                 for(size_t j=0;j<i.appenders.size();j++){
//                     YAML::Node na;
//                     if(i.appenders[j].type==2){
//                         na["type"]="FileLogAppender";
//                         na["file"]=i.appenders[j].file;
//                     }else if(i.appenders[j].type==1){
//                         na["type"]="StdoutLogAppender";
//                     }
//                     na["level"]=qslary::LogLevel::ToString(i.appenders[j].level);
//                     na["formatter"]=i.appenders[j].formatter;
//                     node["appenders"].push_back(na);
//                 }
//             }
//             std::stringstream ss;
//             ss<<node;
//             return ss.str();
//         }
// };

// TAG 在这里执行了lookup()函数
// 会在GetData()得到的map<string,ConfigBase&>中加入一个entry
qslary::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
    qslary::Config::lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter
{
    LogIniter()
    {
        g_log_defines->addListener([](const std::set<LogDefine> &old_value,
                                      const std::set<LogDefine> &new_value) {
            QSLARY_LOG_INFO(QSLARY_LOG_ROOT()) << " on_logger_config change";
            for (auto &i : new_value)
            {
                auto it = old_value.find(i);
                qslary::Logger::ptr logger;
                if (it == old_value.end())
                {
                    // 新增的logger
                    logger = QSLARY_LOG_NAME(i.name);
                }
                else
                {
                    if (!(i == *it))
                    {
                        logger = QSLARY_LOG_NAME(i.name);
                    }
                }

                logger->setLevel(i.level);
                if (!i.formatter.empty())
                {
                    logger->setFormatter(i.formatter);
                }
                logger->clearAppender();
                for (auto &a : i.appenders)
                {
                    qslary::LoggerAppender::ptr ap;
                    if (a.type == 1)
                    {
                        ap.reset(new StdoutLogAppender);
                    }
                    else if (a.type == 2)
                    {
                        // FIXME a or i ?
                        ap.reset(new FileLogAppender(a.file));
                    }
                    ap->setLevel(a.level);
                    if (!a.formatter.empty())
                    {
                        LoggerFormatter::ptr fmt(
                            new LoggerFormatter(a.formatter));
                        if (!fmt->isError())
                        {
                            // std::cout<<"00000000000000000000000"<<std::endl;
                            ap->setFormatter(fmt);
                        }
                        else
                        {
                            std::cout << "logger.name = " << i.name
                                      << " appender type = " << a.type
                                      << " formatter = " << a.formatter
                                      << "is invalib" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }
            for (auto &i : old_value)
            {
                auto it = new_value.find(i);
                if (it == new_value.end())
                {
                    // 删除logger
                    auto logger = QSLARY_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)100);
                    logger->clearAppender();
                }
            }
        });
    }
};

// TAG 全局静态变量 会在这里执行构造函数 在构造函数中对
// g_log_defines加入一个监听器 监听器会在setValue()函数中被调用
static LogIniter __log_init;

// TODO 完成init
void LogManger::init() {}
} // namespace qslary