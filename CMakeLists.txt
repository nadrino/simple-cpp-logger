cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

project(simple-cpp-logger)

# This logger is written considering C++11
set (CMAKE_CXX_STANDARD 11)

# If Clang (Apple) disable extra-warnings about format security
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # using regular Clang or AppleClang
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-format-security")
endif()

# Include the header folder
include_directories(./include)

# These variables defines the default variable all source files will include from
# CAVEAT: Keep in mind each time you modify the verbosity in the code, it will be applied only on the given source file
# Use these calls instead to set any variables globally
# Or use cmake command line arguments. Example: cmake -D LOGGER_MAX_LOG_LEVEL_PRINTED=4 ../
add_definitions( -D LOGGER_MAX_LOG_LEVEL_PRINTED=6 )
add_definitions( -D LOGGER_PREFIX_LEVEL=2 )
add_definitions( -D LOGGER_ENABLE_COLORS=1 )
add_definitions( -D LOGGER_ENABLE_COLORS_ON_USER_HEADER=0 )


add_executable(LoggerExample example/main.cpp)

