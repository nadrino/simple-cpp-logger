//
// Created by Nadrino on 24/08/2020.
//

//#ifndef SIMPLE_CPP_LOGGER_LOGGER_IMPL_H
//#define SIMPLE_CPP_LOGGER_LOGGER_IMPL_H

#pragma once

#include "LoggerParameters.h"

#include <map>
#include <mutex>
#include <cmath>        // std::abs
#include <ctime>
#include <cstdio>
#include <memory>    // For std::unique_ptr
#include <string>
#include <vector>
#include <thread>
#include <cstdio>
#include <cstdarg>
#include <utility>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <type_traits>


#define LoggerInitializerImpl( lambdaInit ) \
  static void* MAKE_VARNAME_LINE(LoggerInitPlaceHolder) = []{ \
    try{ lambdaInit(); }         \
    catch( ... ){                  \
      std::cout << "Error occurred during LoggerInit within the lamda instruction. Please check." << std::endl; \
      throw std::runtime_error("Error occurred during LoggerInit"); \
    } \
    return nullptr; \
  }()


namespace {

  // Setters
  inline void Logger::setMaxLogLevel(const Logger& logger_){
    // _currentLogLevel_ is set by the constructor,
    // so when you provide "LogDebug" as an argument the _currentLogLevel_ is automatically updated
    // Stricto sensu: the argument is just a placeholder for silently updating _currentLogLevel_
    _maxLogLevel_ = _currentLogLevel_;
  }
  inline void Logger::setMaxLogLevel(){
    // same technique as other, but this time with no arguments
    _maxLogLevel_ = _currentLogLevel_;
  }
  inline void Logger::setIsMuted(bool isMuted_){
    _isMuted_ = isMuted_;
  }
  inline void Logger::setEnableColors(bool enableColors_) {
    _enableColors_ = enableColors_;
  }
  inline void Logger::setCleanLineBeforePrint(bool cleanLineBeforePrint) {
    _cleanLineBeforePrint_ = cleanLineBeforePrint;
  }
  inline void Logger::setPropagateColorsOnUserHeader(bool propagateColorsOnUserHeader_) {
    _propagateColorsOnUserHeader_ = propagateColorsOnUserHeader_;
  }
  inline void Logger::setPrefixLevel(const PrefixLevel &prefixLevel_) {
    _prefixLevel_ = prefixLevel_;
  }
  inline void Logger::setUserHeaderStr(const std::string &userHeaderStr_) {
    _userHeaderStr_.str(userHeaderStr_);
  }
  inline void Logger::setPrefixFormat(const std::string &prefixFormat_) {
    _prefixFormat_ = prefixFormat_;
  }
  inline void Logger::setIndentStr(const std::string &indentStr_){
    _indentStr_ = indentStr_;
  }
  inline std::stringstream& Logger::getUserHeader(){
    return _userHeaderStr_;
  }

  // Getters
  inline bool Logger::isCleanLineBeforePrint() {
    return _cleanLineBeforePrint_;
  }
  inline bool Logger::isMuted(){
    return _isMuted_;
  }
  inline int Logger::getMaxLogLevelInt() {
    return static_cast<int>(_maxLogLevel_);
  }
  inline const Logger::LogLevel & Logger::getMaxLogLevel() {
    return _maxLogLevel_;
  }
  inline std::string Logger::getPrefixString() {
    buildCurrentPrefix();
    return _currentPrefix_;
  }
  inline std::string Logger::getPrefixString(const Logger& loggerConstructor){
    // Calling the constructor will automatically update the fields
    return Logger::getPrefixString();
  }
  inline LoggerUtils::StreamBufferSupervisor *Logger::getStreamBufferSupervisorPtr() {
    return _streamBufferSupervisorPtr_;
  }
  inline const std::string& Logger::getIndentStr(){
    return _indentStr_;
  }


  // User Methods
  inline void Logger::quietLineJump() {
    Logger::setupStreamBufferSupervisor(); // in case it was not
    printNewLine();
  }
  inline void Logger::moveTerminalCursorBack(int nLines_, bool clearLines_ ){
    if( nLines_ <= 0 ) return;
    Logger::setupStreamBufferSupervisor(); // in case it was not

    // VT100 commands
    if( not clearLines_ ){
      *_streamBufferSupervisorPtr_ << static_cast<char>(27) << "[" << nLines_ << "F";
    }
    else{
      for( int iLine = 0 ; iLine < nLines_ ; iLine++ ){
        Logger::moveTerminalCursorBack(1);
        Logger::clearLine();
      }
    }
  }
  inline void Logger::moveTerminalCursorForward(int nLines_, bool clearLines_ ){
    if( nLines_ <= 0 ) return;
    Logger::setupStreamBufferSupervisor(); // in case it was not

    // VT100 commands
    if( not clearLines_ ){
      *_streamBufferSupervisorPtr_ << static_cast<char>(27) << "[" << nLines_ << ";1E";
    }
    else{
      for( int iLine = 0 ; iLine < nLines_ ; iLine++ ){
        Logger::moveTerminalCursorForward(1);
        Logger::clearLine();
      }
    }
  }
  inline void Logger::clearLine(){
    Logger::setupStreamBufferSupervisor(); // in case it was not
    *_streamBufferSupervisorPtr_ << static_cast<char>(27) << "[2K" << "\r";
  }
  inline void Logger::triggerNewLine(){
    _isNewLine_ = true;
  }
  inline void Logger::printNewLine(){
    if( _currentColor_ != Logger::Color::RESET ) *_streamBufferSupervisorPtr_ << getColorEscapeCode(Logger::Color::RESET);
    *_streamBufferSupervisorPtr_ << std::endl;
    triggerNewLine();
  }
  inline std::string Logger::getColorEscapeCode(Logger::Color color_){
    if( color_ == Logger::Color::RESET ) return {"\x1b[0m"};
    if( color_ == Logger::Color::BG_RED ) return {"\x1b[41m"};
    if( color_ == Logger::Color::BG_GREEN ) return {"\x1b[42m"};
    if( color_ == Logger::Color::BG_YELLOW ) return {"\x1b[43m"};
    if( color_ == Logger::Color::BG_BLUE ) return {"\x1b[44m"};
    if( color_ == Logger::Color::BG_MAGENTA ) return {"\x1b[45m"};
    if( color_ == Logger::Color::BG_GREY ) return {"\x1b[46m"};
    return {};
  }

  //! Non-static Methods
  // For printf-style calls
  template<typename... TT> inline void Logger::operator()(const char *fmt_str, TT &&... args) {

    if (_currentLogLevel_ > _maxLogLevel_) return;

    Logger::printFormat(fmt_str, std::forward<TT>(args)...);
    if (not _disablePrintfLineJump_ and fmt_str[strlen(fmt_str) - 1] != '\n') { printNewLine(); }

  }
  template<typename T> inline Logger &Logger::operator<<(const T &data) {

    if (_currentLogLevel_ > _maxLogLevel_) return *this;

    std::stringstream dataStream;
    dataStream << data;

    if( dataStream.str().empty() ) return *this; // Don't even print the header

    {
      /* do whatever necessary with the shared data */
      printFormat(dataStream.str().c_str());
    }

    return *this;
  }
  inline Logger &Logger::operator<<(std::ostream &(*f)(std::ostream &)) {

    // Handling std::endl
    if (_currentLogLevel_ > _maxLogLevel_) return *this;

    if( _currentColor_ != Logger::Color::RESET ) *_streamBufferSupervisorPtr_ << getColorEscapeCode(Logger::Color::RESET);
    *_streamBufferSupervisorPtr_ << f;
    triggerNewLine();

    return *this;
  }
  inline Logger &Logger::operator<<(Logger& l_){
    return *this;
  }
  inline Logger &Logger::operator()(bool condition_){
    if( not condition_ ) Logger::_currentLogLevel_ = LogLevel::INVALID;
    return *this;
  }
  inline Logger &Logger::operator()(Logger::Color printColor_){
    _currentColor_ = printColor_;
    return *this;
  }

  // C-tor D-tor
  inline Logger::Logger(const LogLevel &logLevel_, char const *fileName_, const int &lineNumber_, bool once_) {

    setupStreamBufferSupervisor(); // hook the stream buffer to an object we can handle
    if (logLevel_ != _currentLogLevel_) triggerNewLine(); // force reprinting the prefix if the verbosity has changed

    // static members
    _currentLogLevel_ = logLevel_;
    _currentFileName_ = fileName_;
    _currentLineNumber_ = lineNumber_;

    if( once_ ){
      size_t instanceHash{(size_t) lineNumber_};
      LoggerUtils::hashCombine(instanceHash, fileName_);
      if( _onceLogList_.find( instanceHash ) != _onceLogList_.end() ){
        // mute
        Logger::_currentLogLevel_ = LogLevel::INVALID;
      }
      else{
        // will be printed only this time:
        // dirty trick (better way??): get unique identifier out of file name and lineNumber.
        _onceLogList_.insert( instanceHash );
      }
    }
  }
  inline Logger::~Logger() {
    _currentColor_ = Logger::Color::RESET;
  }

  inline void Logger::throwError(const std::string& errorStr_) {
    if( errorStr_.empty() ) throw std::runtime_error("exception thrown by the logger.");
    else throw std::runtime_error("exception thrown by the logger: " + errorStr_);
  }

  // Deprecated (left here for compatibility)
  inline void Logger::setMaxLogLevel(int maxLogLevel_) {
    Logger::setMaxLogLevel(static_cast<Logger::LogLevel>(maxLogLevel_));
  }
  inline void Logger::setMaxLogLevel(const LogLevel &maxLogLevel_) {
    _maxLogLevel_ = maxLogLevel_;
  }


  // Protected Methods
  inline void Logger::buildCurrentPrefix() {

    std::string strBuffer;

    // RESET THE PREFIX
    _currentPrefix_ = "";

    // Nothing else -> NONE level
    if( Logger::_prefixLevel_ == Logger::PrefixLevel::NONE ){
      if( not _userHeaderStr_.str().empty() ){
        Logger::formatUserHeaderStr(_currentPrefix_);
        _currentPrefix_ += " "; // extra space
      }
      return;
    }

    // default:
    // _prefixFormat_ = "{TIME} {USER_HEADER} {SEVERITY} {FILELINE} {THREAD}";
    if( _prefixFormat_.empty() ) _prefixFormat_ = LOGGER_PREFIX_FORMAT;

    // reset the prefix
    _currentPrefix_ = LoggerUtils::stripStringUnicode(_prefixFormat_); // remove potential colors

    // {SEVERITY} -> at least MINIMAL level -> LATER, can introduce repeated space in the prefix!

    // {TIME} -> at least PRODUCTION level
    strBuffer = "";
    if (Logger::_prefixLevel_ >= Logger::PrefixLevel::PRODUCTION) {
      time_t rawTime = std::time(nullptr);
      struct tm timeInfo = *localtime(&rawTime);
      std::stringstream ss;
#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ <= 4)
      char buffer[128];
      std::strftime(buffer, sizeof(buffer), LOGGER_TIME_FORMAT, &timeInfo);
      ss << buffer;
#else
      ss << std::put_time(&timeInfo, LOGGER_TIME_FORMAT);
#endif
      strBuffer += ss.str();
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{TIME}", strBuffer);

    // {FILE} and {LINE} -> at least DEBUG level
    strBuffer = "";
    if(Logger::_prefixLevel_ >= Logger::PrefixLevel::DEBUG){
      if(_enableColors_) strBuffer += "\x1b[90m"; // grey
      strBuffer += _currentFileName_;
      strBuffer += ":";
      strBuffer += std::to_string(_currentLineNumber_);
      if(_enableColors_) strBuffer += "\033[0m";
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{FILELINE}", strBuffer);

    // "{THREAD}" -> at least FULL level
    strBuffer = "";
    if(Logger::_prefixLevel_ >= Logger::PrefixLevel::FULL){
      std::stringstream ss;
      if(_enableColors_) ss << "\x1b[90m";
      ss << "(thread: " << std::this_thread::get_id();
      if(_enableColors_) ss << ")\033[0m";
      strBuffer = ss.str();
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{THREAD}", strBuffer);


    if( _userHeaderStr_.str().empty() ){
      LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{USER_HEADER}", "");
    }

    // Remove extra spaces left by non-applied tags
    LoggerUtils::removeRepeatedCharInsideInputStr(_currentPrefix_, " ");
    // Remove extra spaces on the left
    while(_currentPrefix_[0] == ' ') _currentPrefix_ = _currentPrefix_.substr(1, _currentPrefix_.size());

    // "{USER_HEADER}" -> User prefix can have doubled spaces and spaces on the left
    if( not _userHeaderStr_.str().empty() ){
      strBuffer = "";
      Logger::formatUserHeaderStr(strBuffer);
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

    if( _currentColor_ != Logger::Color::RESET ){
      _currentPrefix_ += getColorEscapeCode(_currentColor_);
    }
  }
  inline void Logger::formatUserHeaderStr(std::string &strBuffer_) {
    if( not _userHeaderStr_.str().empty() ){
      if(_enableColors_ and _propagateColorsOnUserHeader_) strBuffer_ += getLogLevelColorStr(_currentLogLevel_);
      strBuffer_ += _userHeaderStr_.str();
      if(_enableColors_ and _propagateColorsOnUserHeader_) strBuffer_ += "\033[0m";
    }
  }
  inline std::string Logger::getLogLevelColorStr(const LogLevel &selectedLogLevel_) {
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
  inline std::string Logger::getLogLevelStr(const LogLevel &selectedLogLevel_) {

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
  template<typename ... Args> inline void Logger::printFormat(const char *fmt_str, Args ... args ){

    std::string formattedString;

    // If there no extra args, string formatting is not needed
    if(sizeof...(Args) == 0) formattedString = fmt_str;
    else formattedString = LoggerUtils::formatString(fmt_str, std::forward<Args>(args)...);

    // Check if there is multiple lines
    if( LoggerUtils::doesStringContainsSubstring(formattedString, "\n") ) {

      // Print each line individually
      auto slicedString = LoggerUtils::splitString(formattedString, "\n");
      for( auto& line : slicedString ){

        // If the last line is empty, don't print since a \n will be added.
        // Let the parent function do it.
        if( line.empty() and &line == &slicedString.back() ){
          if( formattedString.back() == '\n' ) { triggerNewLine(); }
          break;
        }

        if( &line != &slicedString.front() ){
          // The next printed line should contain the prefix
          triggerNewLine();
        }

        // Recurse
        printFormat(line.c_str());

        // jump now!
        if( &line != &slicedString.back() ){
          printNewLine();
        }
        else {} // let the last line jump be handled by the user
      } // for each line
    } // If multiline
    else if( LoggerUtils::doesStringContainsSubstring(formattedString, "\r") ){
      // Print each line individually
      auto slicedString = LoggerUtils::splitString(formattedString, "\r");
      for (size_t iLine = 0; iLine < slicedString.size(); iLine++) {

        // If the last line is empty, don't print since a \n will be added.
        // Let the parent function do it.
        if (iLine == (slicedString.size() - 1) and slicedString[iLine].empty()) {
          if( formattedString.back() == '\r' ){ triggerNewLine(); }
          break;
        }

        // The next printed line should contain the prefix
        triggerNewLine();

        // Recurse
        printFormat(slicedString[iLine].c_str());

        // let the last trail back be handled by the user (or the parent function)
        if (iLine != (slicedString.size() - 1)) {
          *_streamBufferSupervisorPtr_ << "\r";
        }

      } // for each line
    }
    else{

      // If '\r' is detected, trigger Newline to reprint the header
      if(  _streamBufferSupervisorPtr_->getLastChar() == '\r'
        or _streamBufferSupervisorPtr_->getLastChar() == '\n'
      ){
        triggerNewLine();
      }

      // Start printing
      if(_isNewLine_){
        if( _cleanLineBeforePrint_ ){ Logger::clearLine(); }
        Logger::buildCurrentPrefix();
        *_streamBufferSupervisorPtr_ << _currentPrefix_ << _indentStr_;
        _isNewLine_ = false;
      }

      if (_enableColors_ and _currentLogLevel_ == LogLevel::FATAL){
        *_streamBufferSupervisorPtr_ << LoggerUtils::formatString("%s", getLogLevelColorStr(LogLevel::FATAL).c_str());
      }

      *_streamBufferSupervisorPtr_ << formattedString;

      if (_enableColors_ and _currentLogLevel_ == LogLevel::FATAL)
        *_streamBufferSupervisorPtr_ << LoggerUtils::formatString("\033[0m");
    } // else multiline

  }

  // Setup Methods
  inline void Logger::setupStreamBufferSupervisor(){
    if(_streamBufferSupervisorPtr_ != nullptr) return;
    _streamBufferSupervisorPtr_ = new LoggerUtils::StreamBufferSupervisor(); // this object can't be deleted -> that's why we can't directly override with the logger class
    Logger::setupOutputFile();

  }
  inline void Logger::setupOutputFile(){
    if( not _writeInOutputFile_ or not _outputFileName_.empty() ){
      return;
    }
    _outputFileName_ = LOGGER_OUTFILE_FOLDER;
    _outputFileName_ += "/";
    _outputFileName_ += LOGGER_OUTFILE_NAME_FORMAT;
    LoggerUtils::replaceSubstringInsideInputString(_outputFileName_, "{EXE}", LoggerUtils::getExecutableName());
    time_t rawTime = std::time(nullptr);
    struct tm timeInfo = *localtime(&rawTime);
    std::stringstream ss;
#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ <= 4)
    char buffer[128];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &timeInfo);
    ss << buffer;
#else
    ss << std::put_time(&timeInfo, "%Y%m%d_%H%M%S");
#endif
    LoggerUtils::replaceSubstringInsideInputString(_outputFileName_, "{TIME}", ss.str());
    _streamBufferSupervisorPtr_->openOutFileStream(_outputFileName_);
  }

  // Private Members
//  bool Logger::_enableColors_ = LOGGER_ENABLE_COLORS;
//  bool Logger::_propagateColorsOnUserHeader_ = LOGGER_ENABLE_COLORS_ON_USER_HEADER;
//  bool Logger::_cleanLineBeforePrint_ = LOGGER_CLEAR_LINE_BEFORE_PRINT;
//  bool Logger::_writeInOutputFile_ = LOGGER_WRITE_OUTFILE;
//  bool Logger::_disablePrintfLineJump_ = false;
//  Logger::LogLevel Logger::_maxLogLevel_(static_cast<Logger::LogLevel>(LOGGER_MAX_LOG_LEVEL_PRINTED));
//  Logger::PrefixLevel Logger::_prefixLevel_(static_cast<Logger::PrefixLevel>(LOGGER_PREFIX_LEVEL));
//  std::string Logger::_userHeaderStr_;
//  std::string Logger::_prefixFormat_;

//  std::string Logger::_currentPrefix_;
//  Logger::LogLevel Logger::_currentLogLevel_{Logger::LogLevel::TRACE};
//  Logger::Color Logger::_currentColor_{Logger::Color::RESET};
//  std::string Logger::_currentFileName_;
//  int Logger::_currentLineNumber_{-1};
//  bool Logger::_isNewLine_{true};
//  std::mutex Logger::_loggerMutex_;
//  LoggerUtils::StreamBufferSupervisor* Logger::_streamBufferSupervisorPtr_{nullptr};
//  std::string Logger::_outputFileName_;

}

//#endif //SIMPLE_CPP_LOGGER_LOGGER_IMPL_H
