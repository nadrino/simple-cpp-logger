//
// Created by Nadrino on 02/09/2024.
//

#pragma once


#ifndef HAS_CPP_17
#define HAS_CPP_17 (__cplusplus >= 201703L)
#endif // HAS_CPP_17

#ifndef HAS_CPP_14
#define HAS_CPP_14 (__cplusplus >= 201300L)
#endif // HAS_CPP_14

#ifndef HAS_CPP_11
#define HAS_CPP_11 (__cplusplus >= 201103L)
#endif // HAS_CPP_11


#define CAT_(a, b) a ## b
#define CAT(a, b) CAT_(a, b)
#define MAKE_VARNAME_LINE(Var) CAT(Var, __LINE__)


#define LogDispatcher( logLevel_, isPrint_, isOnce_ )  (Logger{((!(isPrint_) || Logger::isMuted()) ? Logger::LogLevel::INVALID : logLevel_), FILENAME, __LINE__, isOnce_})

#define LogFatalImpl( isPrint_, isOnce_)     (LogDispatcher(Logger::LogLevel::FATAL,   isPrint_, isOnce_))
#define LogErrorImpl( isPrint_, isOnce_ )     (LogDispatcher(Logger::LogLevel::ERROR,   isPrint_, isOnce_))
#define LogAlertImpl( isPrint_, isOnce_ )     (LogDispatcher(Logger::LogLevel::ALERT,   isPrint_, isOnce_))
#define LogWarningImpl( isPrint_, isOnce_ )     (LogDispatcher(Logger::LogLevel::WARNING, isPrint_, isOnce_))
#define LogInfoImpl( isPrint_, isOnce_)     (LogDispatcher(Logger::LogLevel::INFO,    isPrint_, isOnce_))
#define LogDebugImpl( isPrint_, isOnce_ )     (LogDispatcher(Logger::LogLevel::DEBUG,   isPrint_, isOnce_))
#define LogTraceImpl( isPrint_, isOnce_ )     (LogDispatcher(Logger::LogLevel::TRACE,   isPrint_, isOnce_))


#define GET_OVERLOADED_MACRO2(_1,_2,NAME,...) NAME
#define GET_OVERLOADED_MACRO3(_1,_2,_3,NAME,...) NAME


// prompt color
#define LOGGER_STR_COLOR_RESET "\x1b[0m"

#define LOGGER_STR_COLOR_RED     "\x1b[31m"
#define LOGGER_STR_COLOR_GREEN   "\x1b[32m"
#define LOGGER_STR_COLOR_YELLOW  "\x1b[33m"
#define LOGGER_STR_COLOR_BLUE    "\x1b[34m"
#define LOGGER_STR_COLOR_MAGENTA "\x1b[35m"
#define LOGGER_STR_COLOR_CYAN    "\x1b[36m"
#define LOGGER_STR_COLOR_WHITE   "\x1b[37m"

#define LOGGER_STR_COLOR_LIGHT_RED     "\x1b[91m"
#define LOGGER_STR_COLOR_LIGHT_GREEN   "\x1b[92m"
#define LOGGER_STR_COLOR_LIGHT_YELLOW  "\x1b[93m"
#define LOGGER_STR_COLOR_LIGHT_BLUE    "\x1b[94m"
#define LOGGER_STR_COLOR_LIGHT_MAGENTA "\x1b[95m"
#define LOGGER_STR_COLOR_LIGHT_CYAN    "\x1b[96m"
#define LOGGER_STR_COLOR_LIGHT_WHITE   "\x1b[97m"
#define LOGGER_STR_COLOR_LIGHT_GREY    "\x1b[90m"

#define LOGGER_STR_COLOR_RED_BG     "\x1b[41m"
#define LOGGER_STR_COLOR_GREEN_BG   "\x1b[42m"
#define LOGGER_STR_COLOR_YELLOW_BG  "\x1b[43m"
#define LOGGER_STR_COLOR_BLUE_BG    "\x1b[44m"
#define LOGGER_STR_COLOR_MAGENTA_BG "\x1b[45m"
#define LOGGER_STR_COLOR_CYAN_BG    "\x1b[46m"
#define LOGGER_STR_COLOR_GREY_BG    "\x1b[47m"

