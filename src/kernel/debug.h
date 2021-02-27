#ifndef DEBUG_H
#define DEBUG_H

namespace debug {
    bool enabled();
    bool try_to_enable();
    bool write_str(const char * str);
    void printf(const char * fmt, ...) __attribute__((format (printf, 1, 2)));
}

#endif
