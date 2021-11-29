#include <iostream>
#include "../qslary/logger.h"
#include "../qslary/util.h"
int main(){

    qslary::Logger::ptr logger(new qslary::Logger);
    // logger->addAppender(qslary::LoggerAppender::ptr(new qslary::StdoutLogAppender));

    qslary::FileLogAppender::ptr fileappender(new qslary::FileLogAppender("./log.txt"));
    fileappender->setLevel(qslary::LogLevel::ERROR);
    logger->addAppender(fileappender);
    QSLARY_LOG_INFO(logger)<<"test macro";
    QSLARY_LOG_ERROR(logger) << "test error macro";


    return 0;
}