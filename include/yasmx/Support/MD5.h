#ifndef YASM_MD5_H
#define YASM_MD5_H

/* See md5.c for explanation and copyright information.  */

/*
 * $Id: md5.h 1825 2007-04-22 03:32:46Z peter $
 */

#include "yasmx/Config/export.h"


namespace yasm
{

/* Unlike previous versions of this code, uint32 need not be exactly
   32 bits, merely 32 bits or more.  Choosing a data type which is 32
   bits instead of 64 is not important; speed is considerably more
   important.  ANSI guarantees that "unsigned long" will be big enough,
   and always using it seems to have few disadvantages.  */

class YASM_LIB_EXPORT MD5
{
    MD5();

    void Init();
    void Update(const unsigned char* buf, unsigned long len);
    void Final(unsigned char digest[16]);

private:
    static void Transform(unsigned long buf[4], const unsigned char in[64]);

    unsigned long m_buf[4];
    unsigned long m_bits[2];
    unsigned char m_in[64];
};

inline
MD5::MD5()
{
    Init();
}

} // namespace yasm

#endif
