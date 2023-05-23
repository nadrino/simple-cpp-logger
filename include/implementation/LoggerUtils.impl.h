//
// Created by Nadrino on 25/04/2021.
//

#ifndef SIMPLE_CPP_LOGGER_LOGGERUTILS_IMPL_H
#define SIMPLE_CPP_LOGGER_LOGGERUTILS_IMPL_H

#include <cmath>
#include <vector>
#include <string>
#include <memory>
#include <cstring> // strrchr
#include <fstream>
#include <iostream>
#include <algorithm>

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
#include <Logger.h>

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

#include <cstdio>
#include <cstdlib>
extern char* __progname;

// stripping the full path
#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Implementation:
namespace LoggerUtils{

// String Utils
  inline bool doesStringContainsSubstring(const std::string &string_, const std::string &substring_, bool ignoreCase_){
    if(substring_.size() > string_.size()) return false;
    if(ignoreCase_){
      if(toLowerCase(string_).find(toLowerCase(substring_)) != std::string::npos){
        return true;
      }
    }
    else if(string_.find(substring_) != std::string::npos){
      return true;
    }
    return false;
  }
  inline std::string padString(const std::string& inputStr_, const unsigned int &padSize_, const char& padChar){
    std::string outputString;
    int padDelta = int(inputStr_.size()) - int(padSize_);
    while( padDelta < 0 ){
      // add extra chars if needed
      outputString += padChar;
      padDelta++;
    }
    outputString += inputStr_;
    return outputString.substr(0, outputString.size() - padDelta);
  }
  inline std::string toLowerCase(const std::string &inputStr_){
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
      auto splitOutputStr = LoggerUtils::splitString(outputStr, "\033");
      for(const auto& sliceStr : splitOutputStr){
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
  inline void removeRepeatedCharInsideInputStr(std::string &inputStr_, const std::string &doubledChar_){
    std::string doubledCharStr = doubledChar_+doubledChar_;
    std::string lastStr;
    do{
      lastStr = inputStr_;
      LoggerUtils::replaceSubstringInsideInputString(inputStr_, doubledCharStr, doubledChar_);
    } while( lastStr != inputStr_ );
  }
  inline void replaceSubstringInsideInputString(std::string &input_str_, const std::string &substr_to_look_for_, const std::string &substr_to_replace_){
    size_t index = 0;
    while ((index = input_str_.find(substr_to_look_for_, index)) != std::string::npos) {
      input_str_.replace(index, substr_to_look_for_.length(), substr_to_replace_);
      index += substr_to_replace_.length();
    }
  }
  inline std::vector<std::string> splitString(const std::string& input_string_, const std::string& delimiter_) {

    std::vector<std::string> output_split_string;

    const char *src = input_string_.c_str();
    const char *next;

    std::string out_string_piece;

    while ((next = std::strstr(src, delimiter_.c_str())) != nullptr) {
      out_string_piece = "";
      while (src != next) {
        out_string_piece += *src++;
      }
      output_split_string.emplace_back(out_string_piece);
      /* Skip the delimiter_ */
      src += delimiter_.size();
    }

    /* Handle the last token */
    out_string_piece = "";
    while (*src != '\0')
      out_string_piece += *src++;

    output_split_string.emplace_back(out_string_piece);

    return output_split_string;

  }
  inline std::string formatString( const std::string& strToFormat_ ){
    return strToFormat_;
  }
  template<typename ... Args> inline std::string formatString( const std::string& strToFormat_, const Args& ... args ) {
    size_t size = snprintf(nullptr, 0, strToFormat_.c_str(), args ...) + 1; // Extra space for '\0'
    if (size <= 0) { throw std::runtime_error("Error during formatting."); }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, strToFormat_.c_str(), args ...);
    return {buf.get(), buf.get() + size - 1}; // We don't want the '\0' inside
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
  inline std::string getExecutableName(){
    std::string outStr;
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) //check defines for your setup
    std::ifstream("/proc/self/comm") >> outStr;
#elif defined(_WIN32)
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    outStr = buf;
#elif defined(__SWITCH__)
    outStr = "UNDEFINED";
#else
    outStr = __progname;
#endif
    return outStr;
  }

}

#endif //SIMPLE_CPP_LOGGER_LOGGERUTILS_IMPL_H
