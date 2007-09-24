#ifndef YASM_INSN_H
#define YASM_INSN_H
///
/// @file libyasm/insn.h
/// @brief YASM mnenomic instruction.
///
/// @license
///  Copyright (C) 2002-2007  Peter Johnson
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
#include <iosfwd>
#include <memory>

#include <boost/noncopyable.hpp>

#include "value.h"

namespace yasm {

class SegmentRegister;

/// Base class for an effective address.
/// Architectures should derive their own implementation from this.
class EffAddr : private boost::noncopyable {
public:
    /// Constructor.
    EffAddr(std::auto_ptr<Expr> e);

    /// Destructor.
    virtual ~EffAddr();

    /// Set segment override for an effective address.
    /// Some architectures (such as x86) support segment overrides on
    /// effective addresses.  A override of an override will result in a
    /// warning.
    /// @param segreg       segment register (0 if none)
    void set_segreg(const SegmentRegister* segreg);

    Value m_disp;       ///< address displacement

    /// Segment register override (0 if none).
    const SegmentRegister* m_segreg;

    /// 1 if length of disp must be >0.
    bool m_need_nonzero_len:1;

    /// 1 if a displacement should be present in the output.
    bool m_need_disp:1;

    /// 1 if reg*2 should not be split into reg+reg. (0 if not).
    /// This flag indicates (for architectures that support complex effective
    /// addresses such as x86) if various types of complex effective addresses
    /// can be split into different forms in order to minimize instruction
    /// length.
    bool m_nosplit:1;

    /// 1 if effective address is /definitely/ an effective address.
    /// This is used in e.g. the GAS parser to differentiate
    /// between "expr" (which might or might not be an effective address) and
    /// "expr(,1)" (which is definitely an effective address).
    bool m_strong:1;

    /// Print an effective address.  For debugging purposes.
    /// @param os           output stream
    /// @param indent_level indentation level
    virtual void put(std::ostream& os, int indent_level) const = 0;
};

} // namespace yasm

#endif