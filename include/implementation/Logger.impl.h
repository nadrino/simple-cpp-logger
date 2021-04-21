//
// Created by Nadrino on 24/08/2020.
//

#ifndef SIMPLE_CPP_LOGGER_LOGGER_IMPL_H
#define SIMPLE_CPP_LOGGER_LOGGER_IMPL_H

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <string>
#include <cstring>
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

#if defined(_WIN32)
// Windows
#include <windows.h>
#include <psapi.h>
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#elif defined(__APPLE__) && defined(__MACH__)
// MacOS
#include <unistd.h>
#include <sys/resource.h>
#include <mach/mach.h>
#include <sys/ioctl.h>
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
// Linux
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <sys/ioctl.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
// AIX and Solaris
#include <unistd.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <procfs.h>
#include <sys/ioctl.h>

#else
// Unsupported
#endif


namespace LoggerUtils{

  class LastCharBuffer : public std::streambuf {

  public:
    LastCharBuffer() = default;
    void setStreamBuffer(std::streambuf* buf_){
      _streamBufferPtr_ = buf_;
      _lastChar_ = traits_type::eof();
      // no buffering, overflow on every char
      setp(nullptr, nullptr);
      _isInitialized_ = true;
    }
    const char& getLastChar() const { return _lastChar_; }
    bool getIsInitialized() const { return _isInitialized_; }

    int_type overflow(int_type c) override {
      if( _streamBufferPtr_ != nullptr ) _streamBufferPtr_->sputc(char(c));
      _lastChar_ = char(c);
      return c;
    }

  private:
    std::streambuf* _streamBufferPtr_{nullptr};
    char _lastChar_{0};
    bool _isInitialized_{false};
  };

// String Utils
  inline bool doesStringContainsSubstring(std::string string_, std::string substring_, bool ignoreCase_){
    if(substring_.size() > string_.size()) return false;
    if(ignoreCase_){
      string_ = toLowerCase(string_);
      substring_ = toLowerCase(substring_);
    }
    if(string_.find(substring_) != std::string::npos) return true;
    else return false;
  }
  inline std::string toLowerCase(std::string& inputStr_){
    std::string output_str(inputStr_);
    std::transform(output_str.begin(), output_str.end(), output_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return output_str;
  }
  inline std::string stripStringUnicode(const std::string &inputStr_){
    std::string outputStr(inputStr_);

    if(LoggerUtils::doesStringContainsSubstring(outputStr, "\033")){
      // remove color
      std::string tempStr;
      auto splitOuputStr = LoggerUtils::splitString(outputStr, "\033");
      for(const auto& sliceStr : splitOuputStr){
        if(sliceStr.empty()) continue;
        if(tempStr.empty()){
          tempStr = sliceStr;
          continue;
        }
        // look for a 'm' char that determines the end of the color code
        bool mCharHasBeenFound = false;
        for(const char& c : sliceStr){
          if(not mCharHasBeenFound){
            if(c == 'm'){
              mCharHasBeenFound = true;
            }
          }
          else{
            tempStr += c;
          }
        }
      }
      outputStr = tempStr;
    }

    outputStr.erase(
      remove_if(
        outputStr.begin(), outputStr.end(),
        [](const char& c){return !isprint( static_cast<unsigned char>( c ) );}
      ),
      outputStr.end()
    );

    return outputStr;
  }
  inline std::string repeatString(const std::string &inputStr_, int amount_){
    std::string outputStr;
    if(amount_ <= 0) return outputStr;
    for(int i_count = 0 ; i_count < amount_ ; i_count++){
      outputStr += inputStr_;
    }
    return outputStr;
  }
  inline std::string removeRepeatedCharacters(const std::string &inputStr_, const std::string &doubledChar_) {
    std::string doubledCharStr = doubledChar_+doubledChar_;
    std::string outStr = inputStr_;
    std::string lastStr;
    do{
      lastStr = outStr;
      outStr = LoggerUtils::replaceSubstringInString(outStr, doubledCharStr, doubledChar_);
    } while( lastStr != outStr );
    return outStr;
  }
  inline std::string replaceSubstringInString(const std::string &input_str_, const std::string &substr_to_look_for_, const std::string &substr_to_replace_) {
    std::string stripped_str = input_str_;
    size_t index = 0;
    while ((index = stripped_str.find(substr_to_look_for_, index)) != std::string::npos) {
      stripped_str.replace(index, substr_to_look_for_.length(), substr_to_replace_);
      index += substr_to_replace_.length();
    }
    return stripped_str;
  }
  inline std::vector<std::string> splitString(const std::string& input_string_, const std::string& delimiter_) {

    std::vector<std::string> output_splited_string;

    const char *src = input_string_.c_str();
    const char *next;

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
  inline std::string formatString( const std::string& strToFormat_ ){
    return strToFormat_;
  }
  template<typename ... Args> inline std::string formatString( const std::string& strToFormat_, const Args& ... args ) {
    size_t size = snprintf(nullptr, 0, strToFormat_.c_str(), args ...) + 1; // Extra space for '\0'
    if (size <= 0) { throw std::runtime_error("Error during formatting."); }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, strToFormat_.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
  }

  // Hardware related tools
  inline int getTerminalWidth(){
    int outWith;
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
      GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
      outWith = (int)(csbi.dwSize.X);
  //    outWith = (int)(csbi.dwSize.Y);
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) \
    || (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__))) \
    || (defined(__APPLE__) && defined(__MACH__))
    struct winsize w{};
    ioctl(fileno(stdout), TIOCGWINSZ, &w);
    outWith = (int)(w.ws_col);
    //    outWith = (int)(w.ws_row);
#endif // Windows/Linux
    return outWith;
  }

  static LastCharBuffer _lastCharKeeper_;

}

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
    switch (_maxLogLevel_) {
      case LogLevel::FATAL:
        return 0;
      case LogLevel::ERROR:
        return 1;
      case LogLevel::ALERT:
        return 2;
      case LogLevel::WARNING:
        return 3;
      case LogLevel::INFO:
        return 4;
      case LogLevel::DEBUG:
        return 5;
      default:
        return 6;

    }
  }
  Logger::LogLevel Logger::getMaxLogLevel() {
    return _maxLogLevel_;
  }
  std::string Logger::getPrefixString() {
    buildCurrentPrefix();
    return _currentPrefix_;
  }
  std::string Logger::getPrefixString(Logger loggerConstructor){
    // Calling the constructor will automatically update the fields
    return Logger::getPrefixString();
  }


  // User Methods
  void Logger::quietLineJump() {
    _outputStream_ << std::endl;
  }


  // Macro-Related Methods
  Logger::Logger(LogLevel logLevel_, char const *fileName_, int lineNumber_) {
    hookStreamBuffer();
    if (logLevel_ != _currentLogLevel_) _isNewLine_ = true; // force reprinting the prefix if the verbosity has changed

    // static members
    _currentLogLevel_ = logLevel_;
//    _currentFileName_ = LoggerUtils::splitString(fileName_, "/").back();
    _currentFileName_ = fileName_;
    _currentLineNumber_ = lineNumber_;
  }
  template<typename... TT> void Logger::operator()(const char *fmt_str, TT &&... args) {

    if (_currentLogLevel_ > _maxLogLevel_) return;

    { // guard
      std::lock_guard<std::mutex> guard(_loggerMutex_);
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
      std::lock_guard<std::mutex> guard(_loggerMutex_);
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

    // default:
    // _prefixFormat_ = "{TIME} {USER_HEADER} {SEVERITY} {FILELINE} {THREAD}";
    if( _prefixFormat_.empty() ) _prefixFormat_ = LOGGER_PREFIX_FORMAT;

    // Reset the prefix
    _currentPrefix_ = LoggerUtils::stripStringUnicode(_prefixFormat_); // remove potential colors
    std::string contentStrBuffer;

    // "{THREAD}"
    if(Logger::_prefixLevel_ >= Logger::PrefixLevel::FULL){
      std::stringstream ss;
      ss << "\x1b[90m(thread: " << std::this_thread::get_id() << ")\033[0m";
      contentStrBuffer = ss.str();
    }
    _currentPrefix_ = LoggerUtils::replaceSubstringInString(_currentPrefix_, "{THREAD}", contentStrBuffer);
    contentStrBuffer = "";

    // {FILE} and {LINE}
    if(Logger::_prefixLevel_ >= Logger::PrefixLevel::DEBUG){
      contentStrBuffer = "\x1b[90m";
      contentStrBuffer += _currentFileName_;
      contentStrBuffer += ":";
      contentStrBuffer += std::to_string(_currentLineNumber_);
      contentStrBuffer += "\033[0m";
    }
    _currentPrefix_ = LoggerUtils::replaceSubstringInString(_currentPrefix_, "{FILELINE}", contentStrBuffer);
    contentStrBuffer = "";

    // {TIME}
    if (Logger::_prefixLevel_ >= Logger::PrefixLevel::PRODUCTION) {
      time_t rawTime = std::time(nullptr);
      struct tm timeInfo = *localtime(&rawTime);
      std::stringstream ss;
      ss << std::put_time(&timeInfo, "%H:%M:%S");
      contentStrBuffer = ss.str();
    }
    _currentPrefix_ = LoggerUtils::replaceSubstringInString(_currentPrefix_, "{TIME}", contentStrBuffer);
    contentStrBuffer = "";

    // {SEVERITY}
    if (Logger::_prefixLevel_ >= Logger::PrefixLevel::MINIMAL) {
      if (_enableColors_) contentStrBuffer += getTagColorStr(_currentLogLevel_);
      char buffer[6];
      snprintf(buffer, 6, "%5.5s", getTagStr(_currentLogLevel_).c_str());
      contentStrBuffer += buffer;
      if (_enableColors_) contentStrBuffer += "\033[0m";
    }
    _currentPrefix_ = LoggerUtils::replaceSubstringInString(_currentPrefix_, "{SEVERITY}", contentStrBuffer);
    contentStrBuffer = "";

    // Remove extra spaces left by non-applied tags
    _currentPrefix_ = LoggerUtils::removeRepeatedCharacters(_currentPrefix_, " ");

    // cleanup
    while(_currentPrefix_[_currentPrefix_.size()-1] == ' ') _currentPrefix_ = _currentPrefix_.substr(0, _currentPrefix_.size()-1);
    while(_currentPrefix_[0] == ' ') _currentPrefix_ = _currentPrefix_.substr(1, _currentPrefix_.size());

    // {USER_HEADER} -> can contain multiple spaces
    if(not _userHeaderStr_.empty()){
      if(_enableColors_ and _propagateColorsOnUserHeader_) contentStrBuffer += getTagColorStr(_currentLogLevel_);
      contentStrBuffer += _userHeaderStr_;
      if(_enableColors_ and _propagateColorsOnUserHeader_) contentStrBuffer += "\033[0m";
      _currentPrefix_ = LoggerUtils::replaceSubstringInString(_currentPrefix_, "{USER_HEADER}", contentStrBuffer);
    }
    else{
      _currentPrefix_ = LoggerUtils::replaceSubstringInString(_currentPrefix_, "{USER_HEADER}", contentStrBuffer);
      _currentPrefix_ = LoggerUtils::removeRepeatedCharacters(_currentPrefix_, " ");
      while(_currentPrefix_[0] == ' ') _currentPrefix_ = _currentPrefix_.substr(1, _currentPrefix_.size());
    }
    contentStrBuffer = "";

    // Add ": " to separate the header from the message
    if (not _currentPrefix_.empty()){
      _currentPrefix_ += ": ";
    }
  }
  std::string Logger::getTagColorStr(LogLevel selectedLogLevel_) {

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
  std::string Logger::getTagStr(LogLevel selectedLogLevel_) {

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
    if(LoggerUtils::_lastCharKeeper_.getIsInitialized()) return;
    std::streambuf* cbuf = _outputStream_.rdbuf();   // back up cout's streambuf
    _outputStream_.flush();
    LoggerUtils::_lastCharKeeper_.setStreamBuffer(cbuf);
    _outputStream_.rdbuf(&LoggerUtils::_lastCharKeeper_);          // reassign your streambuf to cout
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
      if( LoggerUtils::_lastCharKeeper_.getLastChar() == '\r' ){
        // Clean the line if the option is enabled and the terminal width is measurable
        if( _cleanLineBeforePrint_ and LoggerUtils::getTerminalWidth() != 0){
          _outputStream_ << LoggerUtils::repeatString(" ", LoggerUtils::getTerminalWidth()-1) << "\r";
        }
        _isNewLine_ = true;
      }

      // Start printing
      if(_isNewLine_ or LoggerUtils::_lastCharKeeper_.getLastChar() == '\n'){
        Logger::buildCurrentPrefix();
        _outputStream_ << _currentPrefix_;
        _isNewLine_ = false;
      }

      if (_enableColors_ and _currentLogLevel_ == LogLevel::FATAL)
        _outputStream_ << LoggerUtils::formatString("%s", getTagColorStr(LogLevel::FATAL).c_str());

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
  Logger::LogLevel Logger::_currentLogLevel_ = Logger::LogLevel::TRACE;
  std::string Logger::_currentFileName_;
  int Logger::_currentLineNumber_ = -1;
  bool Logger::_isNewLine_ = true;
  std::ostream& Logger::_outputStream_ = std::cout;
  std::mutex Logger::_loggerMutex_;

}

#endif //SIMPLE_CPP_LOGGER_LOGGER_IMPL_H