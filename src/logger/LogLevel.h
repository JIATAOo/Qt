#ifndef __LOG_LEVEL_H__
#define __LOG_LEVEL_H__

namespace Logger {

enum class Level {
    Debug = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3,
    Off   = 4
};

enum class Output {
    Console = 0,
    File    = 1,
    Both    = 2
};

} // namespace Logger

#endif // __LOG_LEVEL_H__
