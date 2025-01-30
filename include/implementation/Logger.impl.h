//
// Created by Nadrino on 24/08/2020.
//

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
      if (Logger::getStreamBufferSupervisorPtr() != nullptr) Logger::getStreamBufferSupervisorPtr()->flush(); \
      std::cerr << "Error occurred during LoggerInit within the lamda instruction. Please check." << std::endl; \
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

  // Getters
  inline std::string Logger::getPrefixString() {
    buildCurrentPrefix();
    return _currentPrefix_;
  }
  inline std::string Logger::getPrefixString(const Logger& loggerConstructor){
    // Calling the constructor will automatically update the fields
    return Logger::getPrefixString();
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
  inline void Logger::printNewLine(){
    *_streamBufferSupervisorPtr_ << std::endl;
    triggerNewLine();
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

  // C-tor D-tor
  inline Logger::Logger(const LogLevel logLevel_, char const *fileName_, const int lineNumber_, bool once_) {

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

  inline void Logger::throwError(const std::string errorStr_) {
    std::stringstream ss;
    ss << "exception thrown by the logger at " << _currentFileName_ << ":" << _currentLineNumber_;
    ss << (errorStr_.empty()? "." : ": " + errorStr_);
    if (Logger::getStreamBufferSupervisorPtr() != nullptr) Logger::getStreamBufferSupervisorPtr()->flush();
    throw std::runtime_error( ss.str() );
  }
  inline void Logger::triggerExit( const std::string errorStr_ ){
    std::cout << "std::exit() called by the logger at " << _currentFileName_ << ":" << _currentLineNumber_;
    std::cout << (errorStr_.empty()? "." : ": " + errorStr_) << std::endl;
    std::exit( EXIT_FAILURE );
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

    std::stringstream ssBuffer;

    // RESET THE PREFIX
    _currentPrefix_ = "";

    // Nothing else -> NONE level
    if( Logger::_prefixLevel_ == Logger::PrefixLevel::NONE ){
      if( not _userHeaderSs_.str().empty() ){
        Logger::generateUserHeader(_currentPrefix_);
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
    ssBuffer.str("");
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
      ssBuffer << ss.str();
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{TIME}", ssBuffer.str());

    // {FILE} and {LINE} -> at least DEBUG level
    ssBuffer.str("");
    if(Logger::_prefixLevel_ >= Logger::PrefixLevel::DEBUG){
      ssBuffer << (_enableColors_ ? LOGGER_STR_COLOR_LIGHT_GREY : "");
      ssBuffer << _currentFileName_ << ":" << std::to_string(_currentLineNumber_);
      ssBuffer << (_enableColors_ ? LOGGER_STR_COLOR_RESET : "");
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{FILELINE}", ssBuffer.str());

    // {FILENAME} -> at least PRODUCTION level
    ssBuffer.str("");
    if(Logger::_prefixLevel_ >= Logger::PrefixLevel::PRODUCTION){
      ssBuffer << (_enableColors_ ? LOGGER_STR_COLOR_LIGHT_GREY : "");
      ssBuffer << _currentFileName_.substr(0, _currentFileName_.find_last_of('.'));
      ssBuffer << (_enableColors_ ? LOGGER_STR_COLOR_RESET : "");
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{FILENAME}", ssBuffer.str());

    // "{THREAD}" -> at least FULL level
    ssBuffer.str("");
    if(Logger::_prefixLevel_ >= Logger::PrefixLevel::FULL){
      ssBuffer << (_enableColors_ ? LOGGER_STR_COLOR_LIGHT_GREY : "");
      ssBuffer << "(thread: " << std::this_thread::get_id() << ")";
      ssBuffer << (_enableColors_ ? LOGGER_STR_COLOR_RESET : "");
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{THREAD}", ssBuffer.str());


    if( _userHeaderSs_.str().empty() ){
      LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{USER_HEADER}", "");
    }

    // Remove extra spaces left by non-applied tags
    LoggerUtils::removeRepeatedCharInsideInputStr(_currentPrefix_, " ");
    // Remove extra spaces on the left
    while(_currentPrefix_[0] == ' ') _currentPrefix_ = _currentPrefix_.substr(1, _currentPrefix_.size());

    // "{USER_HEADER}" -> User prefix can have doubled spaces and spaces on the left
    if( not _userHeaderSs_.str().empty() ){
      ssBuffer.str("");
      ssBuffer << Logger::generateUserHeader();
      LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{USER_HEADER}", ssBuffer.str());
    }

    // {SEVERITY} -> at least MINIMAL level
    ssBuffer.str("");
    if( Logger::_prefixLevel_ >= Logger::PrefixLevel::MINIMAL ) {
      ssBuffer << (_enableColors_ ? Logger::getLogLevelColorStr(_currentLogLevel_) : "");
      ssBuffer << LoggerUtils::padString(getLogLevelStr(_currentLogLevel_), 5);
      ssBuffer << (_enableColors_ ? LOGGER_STR_COLOR_RESET : "");
    }
    LoggerUtils::replaceSubstringInsideInputString(_currentPrefix_, "{SEVERITY}", ssBuffer.str());

    // cleanup (make sure there's no trailing spaces)
    while(_currentPrefix_[_currentPrefix_.size()-1] == ' ') _currentPrefix_ = _currentPrefix_.substr(0, _currentPrefix_.size()-1);

    // Add ": " to separate the header from the message
    if (not _currentPrefix_.empty()){
      _currentPrefix_ += ": ";
    }
  }
  inline void Logger::generateUserHeader(std::string &strBuffer_) {
    if( not _userHeaderSs_.str().empty() ){
      if(_enableColors_ and _propagateColorsOnUserHeader_) strBuffer_ += getLogLevelColorStr(_currentLogLevel_);
      strBuffer_ += _userHeaderSs_.str();
      if(_enableColors_ and _propagateColorsOnUserHeader_) strBuffer_ += LOGGER_STR_COLOR_RESET;
    }
  }
  inline std::string Logger::getLogLevelColorStr(const LogLevel &selectedLogLevel_) {
    switch (selectedLogLevel_) {
      case Logger::LogLevel::FATAL:   return LOGGER_STR_COLOR_RED_BG;
      case Logger::LogLevel::ERROR:   return LOGGER_STR_COLOR_RED;
      case Logger::LogLevel::ALERT:   return LOGGER_STR_COLOR_MAGENTA;
      case Logger::LogLevel::WARNING: return LOGGER_STR_COLOR_YELLOW;
      case Logger::LogLevel::INFO:    return LOGGER_STR_COLOR_GREEN;
      case Logger::LogLevel::DEBUG:   return LOGGER_STR_COLOR_LIGHT_BLUE;
      case Logger::LogLevel::TRACE:   return LOGGER_STR_COLOR_CYAN;
      default:                        return {};
    }
  }
  inline std::string Logger::getLogLevelStr(const LogLevel &selectedLogLevel_) {
    switch (selectedLogLevel_) {
      case Logger::LogLevel::FATAL:   return "FATAL";
      case Logger::LogLevel::ERROR:   return "ERROR";
      case Logger::LogLevel::ALERT:   return "ALERT";
      case Logger::LogLevel::WARNING: return "WARN";
      case Logger::LogLevel::INFO:    return "INFO";
      case Logger::LogLevel::DEBUG:   return "DEBUG";
      case Logger::LogLevel::TRACE:   return "TRACE";
      default:                        return {};
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
        *_streamBufferSupervisorPtr_ << LoggerUtils::formatString( LOGGER_STR_COLOR_RESET );
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
//  std::string Logger::_userHeaderSs_;
//  std::string Logger::_prefixFormat_;

//  std::string Logger::_currentPrefix_;
//  Logger::LogLevel Logger::_currentLogLevel_{Logger::LogLevel::TRACE};
//  std::string Logger::_currentFileName_;
//  int Logger::_currentLineNumber_{-1};
//  bool Logger::_isNewLine_{true};
//  std::mutex Logger::_loggerMutex_;
//  LoggerUtils::StreamBufferSupervisor* Logger::_streamBufferSupervisorPtr_{nullptr};
//  std::string Logger::_outputFileName_;

}

//#endif //SIMPLE_CPP_LOGGER_LOGGER_IMPL_H
