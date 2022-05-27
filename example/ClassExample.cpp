//
// Created by Nadrino on 25/04/2021.
//

#include "ClassExample.h"

#include "Logger.h"

LoggerInit([]{
  // Careful with this! segfault won't be caught since it is run in a context of a global variable
  Logger::setUserHeaderStr("[ClassExample]");
} );

ClassExample::ClassExample() {
  Logger::setPrefixLevel(Logger::PrefixLevel::DEBUG);
}

void ClassExample::printMessageFromAnotherSourceFile() {
  LogWarning << __PRETTY_FUNCTION__  << ": " << this << std::endl;
  LogWarning << "This message is printed from the ClassExample source file." << std::endl;
  LogWarning << "(header-only feature)" << std::endl;
}
