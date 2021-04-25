//
// Created by Nadrino on 24/08/2020.
//

#ifndef SIMPLE_CPP_LOGGER_LOGGER_IMPL_H
#define SIMPLE_CPP_LOGGER_LOGGER_IMPL_H

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <map>
#include <mutex>
#include <type_traits>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <memory>    // For std::unique_ptr


namespace {

  // Setters
  void Logger::setMaxLogLevel(int maxLogLevel_) {
    Logger::setMaxLogLevel(static_cast<Logger::LogLevel>(maxLogLevel_));
  }
  void Logger::setMaxLogLevel(LogLevel maxLogLevel_) {
    _maxLogLevel_ = maxLogLevel_;
  }
  void Logger::setEnableColors(bool enableColors_) {
    _enableColors_ = enableColors_;
  }
  void Logger::setPropagateColorsOnUserHeader(bool propagateColorsOnUserHeader_) {
    _propagateColorsOnUserHeader_ = propagateColorsOnUserHeader_;
  }
  void Logger::setPrefixLevel(PrefixLevel prefixLevel_) {
    _prefixLevel_ = prefixLevel_;
  }
  void Logger::setUserHeaderStr(const std::string &userHeaderStr_) {
    _userHeaderStr_ = userHeaderStr_;
  }
  void Logger::setPrefixFormat(const std::string &prefixFormat_) {
    _prefixFormat_ = prefixFormat_;
  }

  // Getters
  int Logger::getMaxLogLevelInt() {
    return static_cast<int>(_maxLogLevel_);
  }
  const Logger::LogLevel & Logger::getMaxLogLevel() {
    return _maxLogLevel_;
  }
  std::string Logger::getPrefixString() {
    buildCurrentPrefix();
    return _currentPrefix_;
  }
  std::string Logger::getPrefixString(const Logger& loggerConstructor){
    // Calling the constructor will automatically update the fields
    return Logger::getPrefixString();
  }


  // User Methods
  void Logger::quietLineJump() {
    _outputStream_ << std::endl;
  }

  // C-tor D-tor
  Logger::Logger(LogLevel logLevel_, char const *fileName_, int lineNumber_) {

    hookStreamBuffer(); // hook the stream buffer to an object we can handle
    if (logLevel_ != _currentLogLevel_) _isNewLine_ = true; // force reprinting the prefix if the verbosity has changed

    // Lock while this object is created
    _loggerMutex_.lock();

    // static members
    _currentLogLevel_ = logLevel_;
    _currentFileName_ = fileName_;
    _currentLineNumber_ = lineNumber_;
  }
  Logger::~Logger() {
    _loggerMutex_.unlock();
  }


  template<typename... TT> void Logger::operator()(const char *fmt_str, TT &&... args) {

    if (_currentLogLevel_ > _maxLogLevel_) return;

    { // guard
//      std::lock_guard<std::mutex> guard(_loggerMutex_);
      printFormat(fmt_str, std::forward<TT>(args)...);
      if (not _disablePrintfLineJump_ and fmt_str[strlen(fmt_str) - 1] != '\n') {
        _outputStream_ << std::endl;
        _isNewLine_ = true;
      }
    } // guard

  }
  template<typename T> Logger &Logger::operator<<(const T &data) {

    if (_currentLogLevel_ > _maxLogLevel_) return *this;

    std::stringstream dataStream;
    dataStream << data;
    {
//      std::lock_guard<std::mutex> guard(_loggerMutex_);
      /* do whatever necessary with the shared data */
      printFormat(dataStream.str().c_str());
    }

    return *this;
  }
  Logger &Logger::operator<<(std::ostream &(*f)(std::ostream &)) {

    // Handling std::endl
    if (_currentLogLevel_ > _maxLogLevel_) return *this;

    _outputStream_ << f;
    _isNewLine_ = true;

    return *this;
  }

  // Protected Methods
  void Logger::buildCurrentPrefix() {

    std::string strBuffer;

    // RESET THE PREFIX
    _currentPrefix_ = "";

    // Nothing else -> NONE level
    if( Logger::_prefixLevel_ == Logger::PrefixLevel::NONE ){
      if( not _userHeaderStr_.empty() ){
        Logger::getFormattedUserHeaderStr(_currentPrefix_);
        _currentPrefix_ += " "; // extra space
      }
      return;
    }

    // default:
    // _prefixFormat_ = "{TIME} {USER_HEADER} {SEVERITY} {FILELINE} {THREAD}";
    if( _prefixFormat_.empty() ) _prefixFormat_ = LOGGER_PREFIX_FORMAT;

    // Reset the prefix
    _currentPrefix_ = LoggerUtils::stripStringUnicode(_prefixFormat_); // remove potential colors

    // {SEVERITY} -> at least MINIMAL level -> LATER, can introduce repeated space in the prefix!

    // {TIME} -> at least PRODUCTION level
    strBuffer = "";
    if (Logger::_prefixLevel_ >= Logger::PrefixLevel::PRODUCTION) {
      time_t rawTime = std::time(nullptr);
      struct tm timeInfo = *localtime(&rawTime);
      std::stringstream ss;
      ss << std::put_time(&timeInfo, "%H:%M:%S");
      strBuffer += ss.str();
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{TIME}", strBuffer);

    // {FILE} and {LINE} -> at least DEBUG level
    strBuffer = "";
    if(Logger::_prefixLevel_ >= Logger::PrefixLevel::DEBUG){
      strBuffer += "\x1b[90m"; // grey
      strBuffer += _currentFileName_;
      strBuffer += ":";
      strBuffer += std::to_string(_currentLineNumber_);
      strBuffer += "\033[0m";
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{FILELINE}", strBuffer);

    // "{THREAD}" -> at least FULL level
    strBuffer = "";
    if(Logger::_prefixLevel_ >= Logger::PrefixLevel::FULL){
      std::stringstream ss;
      ss << "\x1b[90m(thread: " << std::this_thread::get_id() << ")\033[0m";
      strBuffer = ss.str();
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{THREAD}", strBuffer);


    if( _userHeaderStr_.empty() ){
      LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{USER_HEADER}", "");
    }

    // Remove extra spaces left by non-applied tags
    LoggerUtils::removeRepeatedCharInsideInputStr(_currentPrefix_, " ");
    while(_currentPrefix_[0] == ' ') _currentPrefix_ = _currentPrefix_.substr(1, _currentPrefix_.size());

    // User prefix can have doubled spaces
    // "{USER_HEADER}" ->
    if( not _userHeaderStr_.empty() ){
      strBuffer = "";
      Logger::getFormattedUserHeaderStr(strBuffer);
      LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{USER_HEADER}", strBuffer);
    }

    // {SEVERITY} -> at least MINIMAL level
    strBuffer = "";
    if( Logger::_prefixLevel_ >= Logger::PrefixLevel::MINIMAL ) {
      if (_enableColors_){ strBuffer += getLogLevelColorStr(_currentLogLevel_); }
      strBuffer += LoggerUtils::padString(getLogLevelStr(_currentLogLevel_), 5);
      if (_enableColors_){ strBuffer += "\033[0m"; }
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{SEVERITY}", strBuffer);

    // cleanup (make sure there's no trailing spaces)
    while(_currentPrefix_[_currentPrefix_.size()-1] == ' ') _currentPrefix_ = _currentPrefix_.substr(0, _currentPrefix_.size()-1);

    // Add ": " to separate the header from the message
    if (not _currentPrefix_.empty()){
      _currentPrefix_ += ": ";
    }
  }
  void Logger::getFormattedUserHeaderStr(std::string &formattedUserHeaderBuffer_) {
    if( not _userHeaderStr_.empty() ){
      if(_enableColors_ and _propagateColorsOnUserHeader_) formattedUserHeaderBuffer_ += getLogLevelColorStr(_currentLogLevel_);
      formattedUserHeaderBuffer_ += _userHeaderStr_;
      if(_enableColors_ and _propagateColorsOnUserHeader_) formattedUserHeaderBuffer_ += "\033[0m";
    }
  }
  std::string Logger::getLogLevelColorStr(const LogLevel &selectedLogLevel_) {

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
  std::string Logger::getLogLevelStr(const LogLevel &selectedLogLevel_) {

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
  void Logger::hookStreamBuffer(){

    if(_lastCharKeeper_ != nullptr) return;
    _lastCharKeeper_ = new LoggerUtils::LastCharBuffer(); // this object can't be deleted -> that's why we can't directly override with the logger class
    std::streambuf* cbuf = _outputStream_.rdbuf();   // back up cout's streambuf
    _outputStream_.flush();
    _lastCharKeeper_->setStreamBuffer(cbuf);
    _outputStream_.rdbuf(_lastCharKeeper_);          // reassign your streambuf to cout

  }

  template<typename ... Args> void Logger::printFormat(const char *fmt_str, Args ... args ){

    std::string formattedString;

    // If there no extra args, string formatting is not needed
    if(sizeof...(Args) == 0) formattedString = fmt_str;
    else formattedString = LoggerUtils::formatString(fmt_str, std::forward<Args>(args)...);

    // Check if there is multiple lines
    if( LoggerUtils::doesStringContainsSubstring(formattedString, "\n") ) {

      // Print each line individually
      auto slicedString = LoggerUtils::splitString(formattedString, "\n");
      for (int i_line = 0; i_line < int(slicedString.size()); i_line++) {

        // If the last line is empty, don't print since a \n will be added.
        // Let the parent function do it.
        if (i_line == (slicedString.size()-1) and slicedString[i_line].empty()) {
          if(formattedString.back() == '\n') _isNewLine_ = true;
          break;
        }

        // The next printed line should contain the prefix
        _isNewLine_ = true;

        // Recurse
        printFormat(slicedString[i_line].c_str());

        // let the last line jump be handle by the user (or the parent function)
        if (i_line != (slicedString.size() - 1)) {
          _outputStream_ << std::endl;
        }

      } // for each line
    } // If multiline
    else{

      // If '\r' is detected, trigger Newline to reprint the header
      if( _lastCharKeeper_->getLastChar() == '\r' ){
        // Clean the line if the option is enabled and the terminal width is measurable
        if( _cleanLineBeforePrint_ and LoggerUtils::getTerminalWidth() != 0){
          _outputStream_ << LoggerUtils::repeatString(" ", LoggerUtils::getTerminalWidth()-1) << "\r";
        }
        _isNewLine_ = true;
      }

      // Start printing
      if(_isNewLine_ or _lastCharKeeper_->getLastChar() == '\n'){
        Logger::buildCurrentPrefix();
        _outputStream_ << _currentPrefix_;
        _isNewLine_ = false;
      }

      if (_enableColors_ and _currentLogLevel_ == LogLevel::FATAL)
        _outputStream_ << LoggerUtils::formatString("%s", getLogLevelColorStr(LogLevel::FATAL).c_str());

      _outputStream_ << formattedString;

      if (_enableColors_ and _currentLogLevel_ == LogLevel::FATAL)
        _outputStream_ << LoggerUtils::formatString("\033[0m");
    } // else multiline

  }


  // Private Members
  bool Logger::_enableColors_ = LOGGER_ENABLE_COLORS;
  bool Logger::_propagateColorsOnUserHeader_ = LOGGER_ENABLE_COLORS_ON_USER_HEADER;
  bool Logger::_cleanLineBeforePrint_ = LOGGER_CLEAR_LINE_BEFORE_PRINT;
  bool Logger::_disablePrintfLineJump_ = false;
  Logger::LogLevel Logger::_maxLogLevel_(static_cast<Logger::LogLevel>(LOGGER_MAX_LOG_LEVEL_PRINTED));
  Logger::PrefixLevel Logger::_prefixLevel_(static_cast<Logger::PrefixLevel>(LOGGER_PREFIX_LEVEL));
  std::string Logger::_userHeaderStr_;
  std::string Logger::_prefixFormat_;

  std::string Logger::_currentPrefix_;
  Logger::LogLevel Logger::_currentLogLevel_{Logger::LogLevel::TRACE};
  std::string Logger::_currentFileName_;
  int Logger::_currentLineNumber_{-1};
  bool Logger::_isNewLine_{true};
  std::ostream& Logger::_outputStream_ = std::cout;
  std::mutex Logger::_loggerMutex_;
  LoggerUtils::LastCharBuffer* Logger::_lastCharKeeper_{nullptr};

}

#endif //SIMPLE_CPP_LOGGER_LOGGER_IMPL_H
