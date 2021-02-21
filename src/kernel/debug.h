#ifndef DEBUG_H
#define DEBUG_H

namespace debug {
    bool write_str(const char * str);
    void printf(const char * fmt, ...) __attribute__((format (printf, 1, 2)));
}

#endif
