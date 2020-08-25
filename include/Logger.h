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
#include <algorithm>


#ifndef LOGGER_MAX_LOG_LEVEL_PRINTED
#define LOGGER_MAX_LOG_LEVEL_PRINTED   6
#endif

#ifndef LOGGER_PREFIX_LEVEL
#define LOGGER_PREFIX_LEVEL   1
#endif

#ifndef LOGGER_ENABLE_COLORS
#define LOGGER_ENABLE_COLORS   1
#endif

#ifndef LOGGER_ENABLE_COLORS_ON_USER_HEADER
#define LOGGER_ENABLE_COLORS_ON_USER_HEADER   0
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
    static void setPropagateColorsOnUserHeader(bool propagateColorsOnUserHeader_){
      _propagateColorsOnUserHeader_ = propagateColorsOnUserHeader_;
    }
    static void setPrefixLevel(PrefixLevel prefixLevel_) {
      _prefixLevel_ = prefixLevel_;
    }
    static void setUserHeaderStr(std::string userHeaderStr_){
      _userHeaderStr_ = std::move(userHeaderStr_);
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

      if (_currentLogLevel_ > _maxLogLevel_) return *this;

      std::string s(data);
      _disablePrintfLineJump_ = true;
      if(_isNewLine_){
        Logger::buildCurrentPrefix();
        _outputStream_ << _currentPrefix_;
      }
      printFormat(s.c_str());
      _disablePrintfLineJump_ = false;
      if(data[N-1] != '\n') _isNewLine_ = false;
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

      // User header
      if(not _userHeaderStr_.empty()){
        if(not _currentPrefix_.empty()) _currentPrefix_ += " ";
        if(_enableColors_ and _propagateColorsOnUserHeader_) _currentPrefix_ += getTagColorStr(_currentLogLevel_);
        _currentPrefix_ += _userHeaderStr_;
        if(_enableColors_ and _propagateColorsOnUserHeader_) _currentPrefix_ += "\033[0m";
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

      // Check if there is multiple lines
      std::string tempStr(fmt_str);
      if (doesStringContainsSubstring(tempStr, "\n")){
        auto splitedString = splitString(tempStr, "\n");
        for(int i_line = 0 ; i_line < int(splitedString.size()) ; i_line++){

          // If the last line is empty, don't print it since a \n will be added.
          // Let the end of this function do it.
          if(i_line == splitedString.size()-1 and splitedString[i_line].empty()){
            break;
          }

          if(_disablePrintfLineJump_ and i_line > 0){
            Logger::buildCurrentPrefix();
            _outputStream_ << _currentPrefix_;
          }
          // Recurse
          printFormat(splitedString[i_line].c_str());

          // let the last line jump be handle by the user
          if(_disablePrintfLineJump_ and i_line != splitedString.size()-1){
            _outputStream_ << std::endl;
          }
        }
        return;
      }
      else{
        if(not _disablePrintfLineJump_){
          Logger::buildCurrentPrefix();
          if(_isNewLine_) _outputStream_ << _currentPrefix_;
        }

        if (_enableColors_ and _currentLogLevel_ == LogLevel::FATAL)
          _outputStream_ << formatString("%s", getTagColorStr(LogLevel::FATAL).c_str());

        _outputStream_ << formatString(fmt_str, std::forward<Args>(args)...);

        if (_enableColors_ and _currentLogLevel_ == LogLevel::FATAL)
          _outputStream_ << formatString("\033[0m");
      }

      if(not _disablePrintfLineJump_){
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
    static bool _disablePrintfLineJump_;
    static bool _propagateColorsOnUserHeader_;
    static LogLevel _maxLogLevel_;
    static PrefixLevel _prefixLevel_;
    static std::string _userHeaderStr_;

  };

  bool Logger::_enableColors_ = LOGGER_ENABLE_COLORS;
  bool Logger::_propagateColorsOnUserHeader_ = LOGGER_ENABLE_COLORS_ON_USER_HEADER;
  bool Logger::_disablePrintfLineJump_ = false;
  Logger::LogLevel Logger::_maxLogLevel_ = Logger::getLogLevel(LOGGER_MAX_LOG_LEVEL_PRINTED);
  Logger::PrefixLevel Logger::_prefixLevel_ = Logger::getPrefixLevel(LOGGER_PREFIX_LEVEL);
  std::string Logger::_userHeaderStr_;

  std::string Logger::_currentPrefix_;
  Logger::LogLevel Logger::_currentLogLevel_ = Logger::LogLevel::TRACE;
  std::string Logger::_currentFileName_;
  int Logger::_currentLineNumber_ = -1;
  bool Logger::_isNewLine_ = true;
  std::ostream& Logger::_outputStream_ = std::cout;

  // template specialization for strings
  template <> Logger& Logger::operator<< <std::string>  ( std::string const &data){

    if (_currentLogLevel_ > _maxLogLevel_) return *this;

    _disablePrintfLineJump_ = true;
    if(_isNewLine_){
      Logger::buildCurrentPrefix();
      _outputStream_ << _currentPrefix_;
    }
    printFormat(data.c_str()); // printFormat will split the string wrt \n
    _disablePrintfLineJump_ = false;
    if(data[data.size()-1] != '\n') _isNewLine_ = false;
    return *this;
  }


}


#endif //SIMPLE_CPP_LOGGER_LOGGER_H
