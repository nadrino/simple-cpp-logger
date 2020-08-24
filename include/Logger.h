//
// Created by Nadrino on 24/08/2020.
//

#ifndef SIMPLE_CPP_LOGGER_LOGGER_H
#define SIMPLE_CPP_LOGGER_LOGGER_H


#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <string>
#include <cstring>
#include <utility>
#include <vector>
#include <thread>
#include <map>
#include <type_traits>
#include <sstream>
#include <iostream>
#include <iomanip>


#ifndef DEFAULT_LOG_LEVEL
#define DEFAULT_LOG_LEVEL   6
#endif

#ifndef DEFAULT_PREFIX_LEVEL
#define DEFAULT_PREFIX_LEVEL   1
#endif

#ifndef DEFAULT_ENABLE_COLORS
#define DEFAULT_ENABLE_COLORS   1
#endif

#define LogFatal       (Logger{Logger::LogLevel::FATAL,    __FILE__, __LINE__})
#define LogError       (Logger{Logger::LogLevel::ERROR,    __FILE__, __LINE__})
#define LogAlert       (Logger{Logger::LogLevel::ALERT,    __FILE__, __LINE__})
#define LogWarning     (Logger{Logger::LogLevel::WARNING,  __FILE__, __LINE__})
#define LogInfo        (Logger{Logger::LogLevel::INFO,     __FILE__, __LINE__})
#define LogDebug       (Logger{Logger::LogLevel::DEBUG,    __FILE__, __LINE__})
#define LogTrace       (Logger{Logger::LogLevel::TRACE,    __FILE__, __LINE__})


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
      TRACE   = 6
    };
    enum class PrefixLevel {
      NONE        = 0,
      MINIMAL     = 1,
      PRODUCTION  = 2,
      DEBUG       = 3,
      FULL        = 4
    };

    Logger(LogLevel level, char const * file, int line){
      if (level != _currentLogLevel_) _isNewLine_ = true; // force reprinting the prefix if the verbosity has changed

      _currentLogLevel_ = level;
      _currentFileName_ = Logger::splitString(file, "/").back();
      _currentLineNumber_ = line;
    }

    // User parameters
    static void setMaxLogLevel(int maxLogLevel_){
      setMaxLogLevel(getLogLevel(maxLogLevel_));
    }
    static void setMaxLogLevel(LogLevel maxLogLevel_) {
      _maxLogLevel_ = maxLogLevel_;
    }
    static void setEnableColors(bool enableColors_) {
      _enableColors_ = enableColors_;
    }
    static void setPrefixLevel(PrefixLevel prefixLevel_) {
      _prefixLevel_ = prefixLevel_;
    }
    static void setUserPrefixStr(std::string userPrefixStr_){
      _userPrefixStr_ = std::move(userPrefixStr_);
    }

    static LogLevel getLogLevel(int logLevelInt_){
      switch(logLevelInt_){
        case 0:   return(LogLevel::FATAL);
        case 1:   return(LogLevel::ERROR);
        case 2:   return(LogLevel::ALERT);
        case 3:   return(LogLevel::WARNING);
        case 4:   return(LogLevel::INFO);
        case 5:   return(LogLevel::DEBUG);
        default:  return(LogLevel::TRACE);
      }
    }
    static PrefixLevel getPrefixLevel(int prefixLevelInt_){
      switch(prefixLevelInt_){
        case 0:   return(PrefixLevel::NONE);
        case 1:   return(PrefixLevel::MINIMAL);
        case 2:   return(PrefixLevel::PRODUCTION);
        case 3:   return(PrefixLevel::DEBUG);
        default:  return(PrefixLevel::FULL);
      }
    }
    static int getMaxLogLevelInt(){
      switch (_maxLogLevel_) {
        case LogLevel::FATAL:   return 0;
        case LogLevel::ERROR:   return 1;
        case LogLevel::ALERT:   return 2;
        case LogLevel::WARNING: return 3;
        case LogLevel::INFO:    return 4;
        case LogLevel::DEBUG:   return 5;
        default:                return 6;

      }
    }
    static LogLevel getMaxLogLevel() {
      return _maxLogLevel_;
    }
    static std::string getPrefixString(){
      buildCurrentPrefix();
      return _currentPrefix_;
    }

    static void quietLineJump(){
      _outputStream_ << std::endl;
    }

    // For printf-style calls
    template <typename... TT> void operator()(TT && ... args) {
      printFormat(std::forward<TT>(args)...);
    }

    // For std::cout-style calls
    template<typename T> Logger &operator<<(const T &data) {

      if (_currentLogLevel_ > _maxLogLevel_) return *this;

      // If new line -> print prefix
      if (_isNewLine_) {
        Logger::buildCurrentPrefix();
        _outputStream_ << _currentPrefix_;
        _isNewLine_ = false;
      }

      if (_enableColors_ and _currentLogLevel_ == LogLevel::FATAL) _outputStream_ << getTagColorStr(LogLevel::FATAL);
      _outputStream_ << data;
      if (_enableColors_ and _currentLogLevel_ == LogLevel::FATAL) _outputStream_ << "\033[0m";

      return *this;
    }
    Logger &operator<<(std::ostream &(*f)(std::ostream &)) {

      if (_currentLogLevel_ > _maxLogLevel_) return *this;

      _outputStream_ << f;
      _isNewLine_ = true;

      return *this;
    }
    template<std::size_t N> Logger& operator<< ( const char (&data) [N] ){
      std::string s(data);
      _disablePrintFormatLineJump_ = true;
      printFormat(s.c_str());
      _disablePrintFormatLineJump_ = false;
      return *this;
    }


  protected:

    static void buildCurrentPrefix() {
      _currentPrefix_ = ""; // reset

      // Time
      if (Logger::_prefixLevel_ >= Logger::PrefixLevel::PRODUCTION) {
        time_t rawTime = std::time(nullptr);
        struct tm timeInfo = *localtime(&rawTime);
        std::stringstream ss;
        ss << std::put_time(&timeInfo, "%H:%M:%S");
        _currentPrefix_ += ss.str();
      }

      if(not _userPrefixStr_.empty()){
        if(not _currentPrefix_.empty()) _currentPrefix_ += " ";
        _currentPrefix_ += _userPrefixStr_;
      }

      // Severity Tag
      if (Logger::_prefixLevel_ >= Logger::PrefixLevel::MINIMAL){
        if (not _currentPrefix_.empty()) _currentPrefix_ += " ";
        if (_enableColors_) _currentPrefix_ += getTagColorStr(_currentLogLevel_);
        char buffer[6];
        snprintf(buffer, 6, "%5.5s", getTagStr(_currentLogLevel_).c_str());
        _currentPrefix_ += buffer;
        if (_enableColors_) _currentPrefix_ += "\033[0m";
      }

      // Filename and Line#
      if (Logger::_prefixLevel_ >= Logger::PrefixLevel::DEBUG) {
        _currentPrefix_ += " ";
        if (_enableColors_) _currentPrefix_ += "\x1b[90m";
        _currentPrefix_ += _currentFileName_;
        _currentPrefix_ += ":";
        _currentPrefix_ += std::to_string(_currentLineNumber_);
        if (_enableColors_) _currentPrefix_ += "\033[0m";
      }

      if (Logger::_prefixLevel_ >= Logger::PrefixLevel::FULL){
        _currentPrefix_ += " ";
        if (_enableColors_) _currentPrefix_ += "\x1b[90m";
        _currentPrefix_ += "(thread: ";
        std::stringstream ss;
        ss << std::this_thread::get_id();
        _currentPrefix_ += ss.str();
        _currentPrefix_ += ")";
        if (_enableColors_) _currentPrefix_ += "\033[0m";
      }

      if (not _currentPrefix_.empty()){
        _currentPrefix_ += ": ";
      }
    }

    static std::string getTagColorStr(LogLevel selectedLogLevel_) {

      switch (selectedLogLevel_) {

        case Logger::LogLevel::FATAL:
          return "\033[41m";
        case Logger::LogLevel::ERROR:
          return "\033[31m";
        case Logger::LogLevel::ALERT:
          return "\033[35m";
        case Logger::LogLevel::WARNING:
          return "\033[33m";
        case Logger::LogLevel::INFO:
          return "\x1b[32m";
        case Logger::LogLevel::DEBUG:
          return "\x1b[94m";
        case Logger::LogLevel::TRACE:
          return "\x1b[36m";
        default:
          return "";

      }

    }
    static std::string getTagStr(LogLevel selectedLogLevel_) {

      switch (selectedLogLevel_) {

        case Logger::LogLevel::FATAL:
          return "FATAL";
        case Logger::LogLevel::ERROR:
          return "ERROR";
        case Logger::LogLevel::ALERT:
          return "ALERT";
        case Logger::LogLevel::WARNING:
          return "WARN";
        case Logger::LogLevel::INFO:
          return "INFO";
        case Logger::LogLevel::DEBUG:
          return "DEBUG";
        case Logger::LogLevel::TRACE:
          return "TRACE";
        default:
          return "";

      }
    }

    template<typename ... Args> static void printFormat(const char *fmt_str, Args ... args ){
      if (_currentLogLevel_ > _maxLogLevel_) return;

      std::string tempStr(fmt_str);
      if (doesStringContainsSubstring(tempStr, "\n")){
        auto splitedString = splitString(tempStr, "\n");
        for(int i_line = 0 ; i_line < int(splitedString.size()) ; i_line++){
          printFormat(splitedString[i_line].c_str());
          if( _disablePrintFormatLineJump_
              and i_line != splitedString.size()-1 // let the last line jump be handle by the user
              ) _outputStream_ << std::endl;
        }
        return;
      }
      else{
        Logger::buildCurrentPrefix();
        if(_isNewLine_) _outputStream_ << _currentPrefix_;

        if (_enableColors_ and _currentLogLevel_ == LogLevel::FATAL)
          _outputStream_ << formatString("%s", getTagColorStr(LogLevel::FATAL).c_str());

        _outputStream_ << formatString(fmt_str, std::forward<Args>(args)...);

        if (_enableColors_ and _currentLogLevel_ == LogLevel::FATAL)
          _outputStream_ << formatString("\033[0m");
      }

      if(not _disablePrintFormatLineJump_){
        _outputStream_ << std::endl;
        _isNewLine_ = true; // always new line after a printf style (useful when using << calls after)
      }
    }

    // Generic Functions
    static std::vector<std::string> splitString(const std::string& input_string_, const std::string& delimiter_) {

      std::vector<std::string> output_splited_string;

      const char *src = input_string_.c_str();
      const char *next = src;

      std::string out_string_piece;

      while ((next = std::strstr(src, delimiter_.c_str())) != nullptr) {
        out_string_piece = "";
        while (src != next) {
          out_string_piece += *src++;
        }
        output_splited_string.emplace_back(out_string_piece);
        /* Skip the delimiter_ */
        src += delimiter_.size();
      }

      /* Handle the last token */
      out_string_piece = "";
      while (*src != '\0')
        out_string_piece += *src++;

      output_splited_string.emplace_back(out_string_piece);

      return output_splited_string;

    }
    template<typename ... Args> static std::string formatString( const char *fmt_str, Args ... args )  {
      size_t size = snprintf( nullptr, 0, fmt_str, args ... ) + 1; // Extra space for '\0'
      if( size <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
      std::unique_ptr<char[]> buf( new char[ size ] );
      snprintf( buf.get(), size, fmt_str, args ... );
      return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
    }
    static bool doesStringContainsSubstring(std::string string_, std::string substring_, bool ignoreCase_ = false){
      if(substring_.size() > string_.size()) return false;
      if(ignoreCase_){
        string_ = toLowerCase(string_);
        substring_ = toLowerCase(substring_);
      }
      if(string_.find(substring_) != std::string::npos) return true;
      else return false;
    }
    static std::string toLowerCase(std::string& inputStr_){
      std::string output_str(inputStr_);
      std::transform(output_str.begin(), output_str.end(), output_str.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      return output_str;
    }

  private:
    // internal
    static LogLevel _currentLogLevel_;
    static std::string _currentFileName_;
    static int _currentLineNumber_;
    static std::string _currentPrefix_;
    static bool _isNewLine_;
    static std::ostream& _outputStream_;

    // parameters
    static bool _enableColors_;
    static bool _disablePrintFormatLineJump_;
    static LogLevel _maxLogLevel_;
    static PrefixLevel _prefixLevel_;
    static std::string _userPrefixStr_;

  };

  bool Logger::_enableColors_ = DEFAULT_ENABLE_COLORS;
  Logger::LogLevel Logger::_maxLogLevel_ = Logger::getLogLevel(DEFAULT_LOG_LEVEL);
  Logger::PrefixLevel Logger::_prefixLevel_ = Logger::getPrefixLevel(DEFAULT_PREFIX_LEVEL);
  std::string Logger::_userPrefixStr_;

  std::string Logger::_currentPrefix_;
  Logger::LogLevel Logger::_currentLogLevel_ = Logger::LogLevel::TRACE;
  std::string Logger::_currentFileName_;
  int Logger::_currentLineNumber_ = -1;
  bool Logger::_isNewLine_ = true;
  bool Logger::_disablePrintFormatLineJump_ = false;
  std::ostream& Logger::_outputStream_ = std::cout;

  // template specialization
  template <> Logger& Logger::operator<< <std::string>  ( std::string const &data){
    _disablePrintFormatLineJump_ = true;
    printFormat(data.c_str()); // printFormat will split the string wrt \n
    _disablePrintFormatLineJump_ = false;
    return *this;
  }
//  template <> Logger& Logger::operator<< <const char[]> ( char const (&data) []){
//    _disablePrintFormatLineJump_ = true;
//    printFormat(data); // printFormat will split the string wrt \n
//    _disablePrintFormatLineJump_ = false;
//    return *this;
//  }
//  template <> Logger& Logger::operator<< <const char *> ( const char* const &data ){
//    _disablePrintFormatLineJump_ = true;
//    printFormat(data); // printFormat will split the string wrt \n
//    _disablePrintFormatLineJump_ = false;
//    return *this;
//  }


}


#endif //SIMPLE_CPP_LOGGER_LOGGER_H
