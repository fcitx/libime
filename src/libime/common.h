#ifndef LIBIME_TYPES_H
#define LIBIME_TYPES_H

#include <stdint.h>
#include <vector>

#define LIBIME_API __attribute__ ((visibility("default")))

#define LIBIME_UNUSED(expr) do { (void)(expr); } while (0)

namespace libime
{
    typedef std::vector<uint8_t> Bytes;
}

#endif // LIBIME_TYPES_H
