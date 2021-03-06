#ifndef YASM_SUPPORT_ENDIANSTATE_H
#define YASM_SUPPORT_ENDIANSTATE_H
///
/// @file
/// @brief Endian state interface.
///
/// @license
///  Copyright (C) 2009  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
namespace yasm
{

class EndianState
{
public:
    EndianState() : m_bigendian(false) {}

    void setBigEndian() { m_bigendian = true; }
    void setLittleEndian() { m_bigendian = false; }
    bool isBigEndian() const { return m_bigendian; }
    bool isLittleEndian() const { return !m_bigendian; }

    void setEndian(EndianState oth)
    {
        m_bigendian = oth.m_bigendian;
    }

    void swap(EndianState& oth)
    {
        bool t = m_bigendian;
        m_bigendian = oth.m_bigendian;
        oth.m_bigendian = t;
    }

private:
    bool m_bigendian;
};

} // namespace yasm

#endif
