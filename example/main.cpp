//
// Created by Nadrino on 24/08/2020.
//

#include <future>
#include "ClassExample.h"

#include "Logger.h"

int main(){

  LogInfo << "-----------------------------------------" << std::endl;
  LogInfo << "Welcome to the simple-cpp-logger example." << std::endl;
  LogInfo << "-----------------------------------------" << std::endl;

  Logger::quietLineJump();


  LogInfo << "-----------------------------------------" << std::endl;
  LogInfo << "Here are all the different log levels:" << std::endl;
  LogInfo << "-----------------------------------------" << std::endl;
  LogFatal    << "This is a FATAL (0) message." << std::endl;
  LogError    << "This is a ERROR (1) message." << std::endl;
  LogAlert    << "This is a ALERT (2) message." << std::endl;
  LogWarning  << "This is a WARNING (3) message." << std::endl;
  LogInfo     << "This is a INFO (4) message." << std::endl;
  LogDebug    << "This is a DEBUG (5) message." << std::endl;
  LogTrace    << "This is a TRACE (6) message." << std::endl;

  LogInfo.quietLineJump();


  LogInfo << "-----------------------------------------" << std::endl;
  LogInfo << "Specifying the prefix level:" << std::endl;
  LogInfo << "-----------------------------------------" << std::endl;
  Logger::setPrefixLevel(Logger::PrefixLevel::NONE);
  LogDebug << "NONE (0) -> No prefix at all. (or only the Header string if you set it before)" << std::endl;
  Logger::setPrefixLevel(Logger::PrefixLevel::MINIMAL);
  LogDebug << "MINIMAL (1) -> Only the log level." << std::endl;
  Logger::setPrefixLevel(Logger::PrefixLevel::PRODUCTION);
  LogDebug << "PRODUCTION (2) -> Add the clock." << std::endl;
  Logger::setPrefixLevel(Logger::PrefixLevel::DEBUG);
  LogDebug << "DEBUG (3) -> Add the filename and the line number." << std::endl;
  Logger::setPrefixLevel(Logger::PrefixLevel::FULL);
  LogDebug << "FULL (4) -> Add the current thread id." << std::endl;
  Logger::setPrefixLevel(Logger::PrefixLevel::DEBUG);
  LogInfo << "For a global application, set it with cmake:" << std::endl;
  LogInfo << "\"-D LOGGER_PREFIX_LEVEL=3\" -> DEBUG by default" << std::endl;

  Logger::quietLineJump();


  LogInfo << "-----------------------------------------" << std::endl;
  LogInfo << "You can use printf() style calls too:" << std::endl;
  LogInfo << "-----------------------------------------" << std::endl;
  int *i_ptr = new int();
  *i_ptr = 99;
  LogInfo << R"(-> LogInfo("Here is a pointer \"%p\", followed by its int value: %i", i_ptr, *i_ptr);)" << std::endl;
  LogInfo("Here is a pointer \"%p\", followed by its int value: %i", i_ptr, *i_ptr);
  delete i_ptr;

  LogInfo.quietLineJump();


  LogInfo << "-----------------------------------------" << std::endl;
  LogInfo << "A header for each calls in this source file can be added:" << std::endl;
  LogInfo << "-----------------------------------------" << std::endl;
  Logger::setUserHeaderStr("[LoggerExample]");
  LogWarning << "There you go :)." << std::endl;
  ClassExample().printMessageFromAnotherSourceFile(); // header is only defined for one source file

  LogInfo.quietLineJump();


  LogInfo << "-----------------------------------------" << std::endl;
  LogInfo << "Colors can be propagated on the header" << std::endl;
  LogInfo << "-----------------------------------------" << std::endl;
  Logger::setPropagateColorsOnUserHeader(true);
  LogInfo << "Here it is!" << std::endl;
  LogInfo << "For a global application, set it with cmake:" << std::endl;
  LogInfo << "\"-D LOGGER_ENABLE_COLORS=1\"" << std::endl;

  LogInfo.quietLineJump();


  LogInfo << "-----------------------------------------" << std::endl;
  LogInfo << "You can set the maximum log level of a given source file." << std::endl;
  LogInfo << "-----------------------------------------" << std::endl;
  Logger::setMaxLogLevel(LogInfo);
  LogInfo << "Here for example, we have set the max log level to INFO:" << std::endl;
  LogInfo << "It means than all printouts from FATAL (0) to INFO (4) will be printed," << std::endl;
  LogInfo << "but not the DEBUG (5) and TRACE (6)." << std::endl;
  LogWarning << "-> You can see me :) (but not the next message which is a DEBUG)" << std::endl;
  LogDebug << "-> You cannot see me :(" << std::endl;
  LogAlert << "* Note that Logger::setMaxLogLevel will only set the max log level for this source file." << std::endl;
  LogAlert << "* This is because simple-cpp-logger is a header-only library." << std::endl;
  LogAlert << "* In fact, all (static) members of the Logger are defined within a given source file." << std::endl;
  LogAlert << "* To apply this parameter globally, set the variable in your cmake file." << std::endl;
  LogTrace.setMaxLogLevel();
  LogInfo << "For a global application, set it with cmake:" << std::endl;
  LogInfo << "\"-D LOGGER_MAX_LOG_LEVEL_PRINTED=6\" -> TRACE by default" << std::endl;

  LogInfo.quietLineJump();


  LogInfo << "-----------------------------------------" << std::endl;
  LogInfo << "Printing strings containing multiple lines:" << std::endl;
  LogInfo << "-----------------------------------------" << std::endl;
  std::stringstream ss;
  ss << "You can also print" << std::endl;
  ss << "multiple lines";
  LogTrace(ss.str().c_str());
  ss.str("");
  ss << "from a string which contains:" << std::endl;
  ss << "\"\\n\"";
  LogTrace << ss.str() << std::endl;
  LogTrace << "-> It also works with strings literals,\na.k.a. \"const char[N]\" (or hard coded strings)" << std::endl;

  Logger::quietLineJump();


  LogInfo << "-----------------------------------------" << std::endl;
  LogInfo << "Disabling colors:" << std::endl;
  LogInfo << "-----------------------------------------" << std::endl;
  Logger::setEnableColors(false);
  LogInfo << "Here is a colorless message" << std::endl;

  Logger::quietLineJump();


#ifdef DEBUG_EXAMPLE
  Logger::setEnableColors(true);

  // dirty examples for debugging purposes
  LogInfo("test %i\nline %i\nnext %i\n", 0, 1, 2);
  LogInfo << "test \nlast line is\nempty\n";
  LogInfo << "lol " << 3.14 << " ptr=" << i_ptr << std::endl;
  LogInfo << "lol %i" << std::endl;

  std::cout << Logger::getPrefixString(LogWarning) << "Manual printout" << std::endl;

  LogInfo << "line that should not be seen..?\r";
  LogInfo << "overriding prev line" << std::endl;

  LogInfo << "test";
  std::cout << std::endl;
  LogInfo << "And now ?" << std::endl;

  int* ptrStopLoop = new int();
  *ptrStopLoop = 0;
  std::future<void> threadLoop = std::async( std::launch::async, [ptrStopLoop](){

    while( *ptrStopLoop == 0 ){
      std::this_thread::sleep_for(std::chrono::microseconds (1));
      LogInfo << "Message from ANOTHER thread..." << std::endl;
    }

  } );

  for( int iPrint = 0 ; iPrint < 5 ; iPrint++ ){
    std::this_thread::sleep_for(std::chrono::microseconds (1));
    LogWarning << "Message from the MAIN thread" << std::endl;
  }
  *ptrStopLoop = 1;
  threadLoop.get();

  Logger::quietLineJump();

#endif


  LogAlert << "-----------------------------------------" << std::endl;
  LogAlert << "Have Fun! :)" << std::endl;
  LogAlert << "-----------------------------------------" << std::endl;

}