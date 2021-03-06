#ifndef PHASH_H
#define PHASH_H
// Modified for use with yasm by Peter Johnson.
//
// ---------------------------------------------------------------------------
// By Bob Jenkins, September 1996.
// lookupa.h, a hash function for table lookup, same function as lookup.c.
// Use this code in any way you wish.  Public Domain.  It has no warranty.
// Source is http://burtleburtle.net/bob/c/lookupa.h
// ---------------------------------------------------------------------------

#include <cstddef>

#include "yasmx/Config/export.h"

namespace phash
{

YASM_LIB_EXPORT
unsigned long lookup(const char* k, size_t length, unsigned long level);
YASM_LIB_EXPORT
void checksum(const char* k, size_t length, unsigned long* state);

} // namespace phash

#endif
