//
// Created by Nadrino on 24/08/2020.
//

#ifndef SIMPLE_CPP_LOGGER_LOGGER_H
#define SIMPLE_CPP_LOGGER_LOGGER_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

#include "implementation/LoggerParameters.h"
#include "implementation/LoggerUtils.h"

#include <mutex>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_set>


// Here is what you want to use
#define LogFatal                (LogFatalImpl( true, false ))
#define LogError                (LogErrorImpl( true, false ))
#define LogAlert                (LogAlertImpl( true, false ))
#define LogWarning              (LogWarningImpl( true, false ))
#define LogInfo                 (LogInfoImpl( true, false ))
#define LogDebug                (LogDebugImpl( true, false ))
#define LogTrace                (LogTraceImpl( true, false ))

// conditional
#define LogFatalIf(isPrint_)    (LogFatalImpl( isPrint_, false ))
#define LogErrorIf(isPrint_)    (LogErrorImpl( isPrint_, false ))
#define LogAlertIf(isPrint_)    (LogAlertImpl( isPrint_, false ))
#define LogWarningIf(isPrint_)  (LogWarningImpl( isPrint_, false ))
#define LogInfoIf(isPrint_)     (LogInfoImpl( isPrint_, false ))
#define LogDebugIf(isPrint_)    (LogDebugImpl( isPrint_, false ))
#define LogTraceIf(isPrint_)    (LogTraceImpl( isPrint_, false ))

// once
#define LogFatalOnce            (LogFatalImpl( true, true ))
#define LogErrorOnce            (LogErrorImpl( true, true ))
#define LogAlertOnce            (LogAlertImpl( true, true ))
#define LogWarningOnce          (LogWarningImpl( true, true ))
#define LogInfoOnce             (LogInfoImpl( true, true ))
#define LogDebugOnce            (LogDebugImpl( true, true ))
#define LogTraceOnce            (LogTraceImpl( true, true ))

// To make assertions
#define LogThrowIf2(isThrowing_, errorMessage_)  if(isThrowing_){(LogError << "(" << __PRETTY_FUNCTION__ << "): "<< errorMessage_ << std::endl).throwError(#isThrowing_);}
#define LogThrowIf1(isThrowing_) LogThrowIf2(isThrowing_, #isThrowing_)
#define LogThrowIf(...) GET_OVERLOADED_MACRO2(__VA_ARGS__, LogThrowIf2, LogThrowIf1)(__VA_ARGS__)
#define LogAssert(assertion_, errorMessage_)    LogThrowIf(not (assertion_), errorMessage_)
#define LogThrow(errorMessage_)                 LogThrowIf(true, errorMessage_)

// Within loops
#define LogContinueIf2(isContinue_, continueMessage_)  if(isContinue_){(LogWarning << "(" << __PRETTY_FUNCTION__ << "): "<< continueMessage_ << std::endl); continue; }
#define LogContinueIf1(isContinue_)  LogContinueIf2(isContinue_, #isContinue_)
#define LogContinueIf(...) GET_OVERLOADED_MACRO2(__VA_ARGS__, LogContinueIf2, LogContinueIf1)(__VA_ARGS__)

// Within functions
#define LogReturnIf3(isReturn_, returnMessage_, returnObj_)  if(isReturn_){(LogWarning << "(" << __PRETTY_FUNCTION__ << "): "<< returnMessage_ << std::endl); return returnObj_; }
#define LogReturnIf2(isReturn_, returnMessage_)  LogReturnIf3(isReturn_, returnMessage_, )
#define LogReturnIf1(isReturn_)  LogReturnIf2(isReturn_, (#isReturn_))
#define LogReturnIf(...) GET_OVERLOADED_MACRO3(__VA_ARGS__, LogReturnIf3, LogReturnIf2, LogReturnIf1)(__VA_ARGS__)

#define LogScopeIndent Logger::ScopedIndent MAKE_VARNAME_LINE(scopeIndentTempObj);
#define LogIndent Logger::setIndentStr(Logger::getIndentStr() + "  ")
#define LogUnIndent Logger::setIndentStr(Logger::getIndentStr().substr(0, Logger::getIndentStr().size()-2))

// To set up the logger in a given source file
#define LoggerInit( lambdaInit ) LoggerInitializerImpl( lambdaInit )

// Implementation
namespace {

  class Logger {

  public:

    enum class LogLevel {
      FATAL   = 0,
      ERROR   = 1,
      ALERT   = 2,
      WARNING = 3,
      INFO    = 4,
      DEBUG   = 5,
      TRACE   = 6,
      INVALID = 7
    };
    enum class PrefixLevel {
      NONE        = 0,
      MINIMAL     = 1,
      PRODUCTION  = 2,
      DEBUG       = 3,
      FULL        = 4
    };
    enum class Color {
      RESET    = 0,
      BG_RED,
      BG_GREEN,
      BG_YELLOW,
      BG_BLUE,
      BG_MAGENTA,
      BG_GREY
    };

    //! Setters
    // Keep in mind that every parameter you set will be applied only in the context of the source file you're in
    // It is an inherent feature as a **header-only** library
    inline static void setMaxLogLevel(const Logger& logger_);  // Example: Logger::setMaxLogLevel(LogDebug);
    inline static void setMaxLogLevel();                       // Example: LogDebug.setMaxLogLevel();
    inline static void setIsMuted(bool isMuted_);
    inline static void setEnableColors(bool enableColors_);
    inline static void setCleanLineBeforePrint(bool cleanLineBeforePrint);
    inline static void setPropagateColorsOnUserHeader(bool propagateColorsOnUserHeader_);
    inline static void setPrefixLevel(const PrefixLevel &prefixLevel_);
    inline static void setUserHeaderStr(const std::string &userHeaderStr_);
    inline static void setPrefixFormat(const std::string &prefixFormat_);
    inline static void setIndentStr(const std::string &indentStr_);
    inline static std::stringstream& getUserHeader();
    inline static std::string indent(){ LogIndent; return {}; }
    inline static std::string unIndent(){ LogUnIndent; return {}; }

    //! Getters
    inline static bool isCleanLineBeforePrint();
    inline static bool isMuted();
    inline static int getMaxLogLevelInt();
    inline static const LogLevel & getMaxLogLevel();
    inline static std::string getPrefixString();                                // LogWarning.getPrefixString()
    inline static std::string getPrefixString(const Logger& loggerConstructor); // Logger::getPrefixString(LogWarning)
    inline static LoggerUtils::StreamBufferSupervisor *getStreamBufferSupervisorPtr();
    inline static const std::string& getIndentStr();

    //! Misc
    inline static void quietLineJump();
    inline static void moveTerminalCursorBack(int nLines_, bool clearLines_ = false );
    inline static void moveTerminalCursorForward(int nLines_, bool clearLines_ = false );
    inline static void clearLine();
    inline static void triggerNewLine();
    inline static void printNewLine();
    inline static std::string getColorEscapeCode(Logger::Color color_);

    //! Non-static Methods
    // For printf-style calls
    template <typename... TT> inline void operator()(const char *fmt_str, TT && ... args);
    // For std::cout-style calls
    template<typename T> inline Logger &operator<<(const T &data);
    inline Logger &operator<<(std::ostream &(*f)(std::ostream &));
    inline Logger &operator<<(Logger& l_);
    inline Logger &operator()(bool condition_);
    inline Logger &operator()(Logger::Color printColor_);

    // Macro-Related Methods
    // Those intended to be called using the above preprocessor macros
    inline Logger(const LogLevel &logLevel_, char const * fileName_, const int &lineNumber_, bool once_=false);
    virtual inline ~Logger();

    inline static void throwError(const std::string& errorStr_ = "");

    // Deprecated (left here for compatibility)
    inline static void setMaxLogLevel(int maxLogLevel_);
    inline static void setMaxLogLevel(const LogLevel &maxLogLevel_);

  protected:

    inline static void buildCurrentPrefix();
    inline static void formatUserHeaderStr(std::string &strBuffer_);
    inline static std::string getLogLevelColorStr(const LogLevel &selectedLogLevel_);
    inline static std::string getLogLevelStr(const LogLevel &selectedLogLevel_);
    template<typename ... Args> inline static void printFormat(const char *fmt_str, Args ... args );

    // Setup Methods
    inline static void setupStreamBufferSupervisor();
    inline static void setupOutputFile();

  private:

#if HAS_CPP_17
    // C++17 code

    // parameters
    static inline bool _enableColors_{LOGGER_ENABLE_COLORS};
    static inline bool _disablePrintfLineJump_{false};
    static inline bool _propagateColorsOnUserHeader_{LOGGER_ENABLE_COLORS_ON_USER_HEADER};
    static inline bool _cleanLineBeforePrint_{LOGGER_WRITE_OUTFILE};
    static inline bool _writeInOutputFile_{false};
    static inline bool _isMuted_{false};
    static inline std::string _prefixFormat_{};
    static inline std::string _indentStr_{};
    static inline std::stringstream _userHeaderStr_{};
    static inline LogLevel _maxLogLevel_{static_cast<Logger::LogLevel>(LOGGER_MAX_LOG_LEVEL_PRINTED)};
    static inline PrefixLevel _prefixLevel_{static_cast<Logger::PrefixLevel>(LOGGER_PREFIX_LEVEL)};

    // internal
    static inline bool _isNewLine_{true};
    static inline int _currentLineNumber_{-1};
    static inline std::string _currentFileName_{};
    static inline std::string _currentPrefix_{};
    static inline std::string _outputFileName_{};
    static inline std::mutex _loggerMutex_{};
    static inline std::unordered_set<size_t> _onceLogList_{};
    static inline Color _currentColor_{Logger::Color::RESET};
    static inline LogLevel _currentLogLevel_{Logger::LogLevel::TRACE};
    static inline LoggerUtils::StreamBufferSupervisor* _streamBufferSupervisorPtr_{nullptr};
    static inline LoggerUtils::StreamBufferSupervisor _streamBufferSupervisor_;

    // non-static
    std::scoped_lock<std::mutex> _lock_{_loggerMutex_}; // one logger can be created at a given time
#else
    // parameters
    static bool _enableColors_;
    static bool _disablePrintfLineJump_;
    static bool _propagateColorsOnUserHeader_;
    static bool _cleanLineBeforePrint_;
    static bool _writeInOutputFile_;
    static bool _isMuted_;
    static std::string _prefixFormat_;
    static std::string _indentStr_;
    static std::stringstream _userHeaderStr_;
    static LogLevel _maxLogLevel_;
    static PrefixLevel _prefixLevel_;

    // internal
    static bool _isNewLine_;
    static int _currentLineNumber_;
    static std::string _currentFileName_;
    static std::string _currentPrefix_;
    static std::string _outputFileName_;
    static std::mutex _loggerMutex_;
    static std::unordered_set<size_t> _onceLogList_;
    static Color _currentColor_;
    static LogLevel _currentLogLevel_;
    static LoggerUtils::StreamBufferSupervisor* _streamBufferSupervisorPtr_;
    static LoggerUtils::StreamBufferSupervisor _streamBufferSupervisor_;

    // non-static
    std::lock_guard<std::mutex> _lock_{_loggerMutex_};
#endif

  public:
    struct ScopedIndent{
      inline ScopedIndent(){ LogIndent; }
      inline ~ScopedIndent(){ LogUnIndent; }
    };

  };


#if HAS_CPP_17
#else
  // Out of line declaration of non-const static variable (before C++17 -> now "inline" member work)
  // Need to declare even default init variables to avoid warning "has internal linkage but is not defined"

  // parameters
  bool Logger::_enableColors_{LOGGER_ENABLE_COLORS};
  bool Logger::_propagateColorsOnUserHeader_{LOGGER_ENABLE_COLORS_ON_USER_HEADER};
  bool Logger::_cleanLineBeforePrint_{LOGGER_WRITE_OUTFILE};
  bool Logger::_disablePrintfLineJump_{false};
  bool Logger::_writeInOutputFile_{false};
  bool Logger::_isMuted_{false};
  Logger::LogLevel Logger::_maxLogLevel_{static_cast<Logger::LogLevel>(LOGGER_MAX_LOG_LEVEL_PRINTED)};
  Logger::PrefixLevel Logger::_prefixLevel_{static_cast<Logger::PrefixLevel>(LOGGER_PREFIX_LEVEL)};
  std::stringstream Logger::_userHeaderStr_{};
  std::string Logger::_prefixFormat_{};
  std::string Logger::_indentStr_{};

  // internal
  bool Logger::_isNewLine_{true};
  int Logger::_currentLineNumber_{-1};
  std::string Logger::_currentFileName_{};
  std::string Logger::_currentPrefix_{};
  std::string Logger::_outputFileName_{};
  std::mutex Logger::_loggerMutex_{};
  LoggerUtils::StreamBufferSupervisor* Logger::_streamBufferSupervisorPtr_{nullptr};
  LoggerUtils::StreamBufferSupervisor Logger::_streamBufferSupervisor_{};
  Logger::LogLevel Logger::_currentLogLevel_{Logger::LogLevel::TRACE};
  Logger::Color Logger::_currentColor_{Logger::Color::RESET};
  std::unordered_set<size_t> Logger::_onceLogList_{};


#endif

}

#include "implementation/Logger.impl.h"

#endif //SIMPLE_CPP_LOGGER_LOGGER_H
