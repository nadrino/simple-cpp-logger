![](resources/logo/simple-cpp-logger-logo.png)

## Description 

This simple header-only library written in C++ provides logging features with handy options.
Because it's written as a header-only library, you can easily add it to your current project. 
A cmake example file is shipped within this repository.

## Showcase

Here is a screenshot of first output lines you get while running the example.

![](resources/screenshot/example_showcase.png)

If you want to see all the features embedded in the example, feel free to build and run it yourself: 

```bash
git clone https://github.com/nadrino/simple-cpp-logger.git
mkdir simple-cpp-logger/build
cd simple-cpp-logger/build
cmake ../
make
./LoggerExample
```

## Requirements

- **C++11 or higher is required to use this logger.**


## Getting Setup

You can add simple-cpp-logger to your existing project as a submodule for example.
In this example we assume that your project is using CMake.
In your CMakeLists.txt, add the following lines:

```cmake
include_directories(path/to/simple-cpp-logger/include)
```

Preprocessor variables are defined in `Logger.h`, and can be changed by adding the following lines to CMakeLists.txt:

```cmake
add_definitions( -D LOGGER_MAX_LOG_LEVEL_PRINTED=6 )
add_definitions( -D LOGGER_PREFIX_LEVEL=2 )
add_definitions( -D LOGGER_ENABLE_COLORS=1 )
add_definitions( -D LOGGER_ENABLE_COLORS_ON_USER_HEADER=0 )
```

Now you can include the header in your source file:

```cpp
#include <Logger.h>
```

Then you're all set!


## Implementation Guidelines

Here are simple example lines of implementation. 
**If you want to know more about the available features, you can refer to "example/main.cpp".**

Let's print a simple line:

```cpp
LogInfo << "My very important message" << std::endl;
```

You have just printed a line on screen with the tag "Info". 
`LogInfo` is actually a preprocessor macros which automatically passes the **filename** and the **line number** from where it's called.

It exists different variants of this same call: `LogFatal`, `LogError`, `LogAlert`, `LogWarning`, `LogInfo`, `LogDebug` and `LogTrace`.
Each of these will display their proper verbosity tag.

These calls can as well be made in the style of the C function `printf()`:

```cpp
LogInfo("The variable is: %i", my_int);
```

Few user parameters can be access from the Logger class.
For example let's change the highest printed log level:

```cpp
Logger::setMaxLogLevel(Logger::LogLevel::INFO);
```

From this moment, all following Log calls will be printed if their associated log level is lower than `INFO`.
This means that `LogFatal`, `LogError`, `LogAlert`, `LogWarning` and `LogInfo` will be printed, but `LogDebug` and `LogTrace` will be muted. 

Keep in mind that every parameter you set this way will be only be applied for the current source file.
For a global effect, you need to set the associated preprocessor variables accordingly (cf. CMakeLists.txt).

