//
// Created by Nadrino on 25/04/2021.
//

#ifndef SIMPLE_CPP_LOGGER_LOGGERUTILS_H
#define SIMPLE_CPP_LOGGER_LOGGERUTILS_H

#include <thread>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <functional>


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




// Header
namespace LoggerUtils{

  class StreamBufferSupervisor : public std::streambuf {
    // An external class is need to keep track of the last char printed.
    // It can't be handled by the Logger since each time a logger is called, it is deleted after the ";"
  public:
    StreamBufferSupervisor(){
      _streamBufferPtr_ = _outputStream_->rdbuf();   // back up cout's streambuf
      _outputStream_->flush();
      setp(nullptr, nullptr);
//      _streamBufferSupervisorPtr_->setStreamBuffer(cbuf);
      _outputStream_->rdbuf(this);          // reassign your streambuf to cout
    }
    ~StreamBufferSupervisor() override {
      if( _outFileStream_.is_open() ){
        _outFileStream_.close();
      }
      _outputStream_->rdbuf(_streamBufferPtr_);
    }

    void setStreamBuffer(std::streambuf* buf_){
      _streamBufferPtr_ = buf_;
      // no buffering, overflow on every char
      setp(nullptr, nullptr);
    }
    const char& getLastChar() const { return _lastChar_; }
    int_type overflow(int_type c) override {
      if( _streamBufferPtr_ != nullptr ) _streamBufferPtr_->sputc(char(c));
      _lastChar_ = char(c);
      return c;
    }

    void openOutFileStream(const std::string& outFilePath_){
      _outFileStream_.open(outFilePath_);
    }
    template<typename T> StreamBufferSupervisor& operator<<(const T& something){
      (*_outputStream_) << something;
      if( _outFileStream_.is_open() ) _outFileStream_ << something;
      return *this;
    }
    StreamBufferSupervisor &operator<<(std::ostream &(*f)(std::ostream &)){
      (*_outputStream_) << f;
      if( _outFileStream_.is_open() ) _outFileStream_ << f;
      return *this;
    }

  private:
    std::streambuf* _streamBufferPtr_{nullptr};
    std::ofstream _outFileStream_;
    std::ostream* _outputStream_ = &std::cout;
    char _lastChar_{static_cast<char>(traits_type::eof())};
  };

  //! String Utils
  inline bool doesStringContainsSubstring(const std::string &string_, const std::string &substring_, bool ignoreCase_ = false);
  inline std::string padString(const std::string& inputStr_, const unsigned int &padSize_, const char& padChar = ' ');
  inline std::string toLowerCase(const std::string &inputStr_);
  inline std::string stripStringUnicode(const std::string &inputStr_);
  inline std::string repeatString(const std::string &inputStr_, int amount_);
  inline std::vector<std::string> splitString(const std::string& input_string_, const std::string& delimiter_);
  inline std::string formatString( const std::string& strToFormat_ ); // 0 args overrider
  template<typename ... Args> inline std::string formatString( const std::string& strToFormat_, const Args& ... args );

  inline void removeRepeatedCharInsideInputStr(std::string &inputStr_, const std::string &doubledChar_);
  inline void replaceSubstringInsideInputString(std::string &input_str_, const std::string &substr_to_look_for_, const std::string &substr_to_replace_);

  // Hardware Utils
  inline int getTerminalWidth();
  inline std::string getExecutableName();

  // hash Utils
  template <class T> inline void hashCombine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
  }

}

#include "LoggerUtils.impl.h"

#endif //SIMPLE_CPP_LOGGER_LOGGERUTILS_H
