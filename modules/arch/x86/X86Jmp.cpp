//
// x86 jump bytecode
//
//  Copyright (C) 2001-2007  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include "X86Jmp.h"

#include "util.h"

#include <libyasmx/BytecodeContainer.h>
#include <libyasmx/BytecodeOutput.h>
#include <libyasmx/Bytecode.h>
#include <libyasmx/Bytes.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/Expr.h>
#include <libyasmx/IntNum.h>
#include <libyasmx/marg_ostream.h>
#include <libyasmx/Symbol.h>

#include "X86Common.h"
#include "X86Opcode.h"


namespace yasm
{
namespace arch
{
namespace x86
{

class X86Jmp : public Bytecode::Contents
{
public:
    X86Jmp(const X86Common& common,
           JmpOpcodeSel op_sel,
           const X86Opcode& shortop,
           const X86Opcode& nearop,
           std::auto_ptr<Expr> target);
    ~X86Jmp();

    void put(marg_ostream& os) const;
    void finalize(Bytecode& bc);
    unsigned long calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span);
    bool expand(Bytecode& bc,
                unsigned long& len,
                int span,
                long old_val,
                long new_val,
                /*@out@*/ long* neg_thres,
                /*@out@*/ long* pos_thres);
    void output(Bytecode& bc, BytecodeOutput& bc_out);

    X86Jmp* clone() const;

private:
    X86Common m_common;
    X86Opcode m_shortop, m_nearop;

    Value m_target;             // jump target

    // which opcode are we using?
    // The *FORCED forms are specified in the source as such
    JmpOpcodeSel m_op_sel;
};

X86Jmp::X86Jmp(const X86Common& common,
               JmpOpcodeSel op_sel,
               const X86Opcode& shortop,
               const X86Opcode& nearop,
               std::auto_ptr<Expr> target)
    : Bytecode::Contents(),
      m_common(common),
      m_shortop(shortop),
      m_nearop(nearop),
      m_target(0, target),
      m_op_sel(op_sel)
{
    m_target.set_jump_target();
    m_target.set_signed();
}

X86Jmp::~X86Jmp()
{
}

void
X86Jmp::put(marg_ostream& os) const
{
    os << "_Jump_\n";

    os << "Target:\n";
    ++os;
    os << m_target;
    --os;
    /* FIXME
    fprintf(f, "%*sOrigin=\n", indent_level, "");
    yasm_symrec_print(jmp->origin, f, indent_level+1);
    */

    os << "\nShort Form:\n";
    ++os;
    if (m_shortop.get_len() == 0)
        os << "None\n";
    else
        os << m_shortop;
    --os;

    os << "Near Form:\n";
    ++os;
    if (m_nearop.get_len() == 0)
        os << "None\n";
    else
        os << m_nearop;
    --os;

    os << "OpSel=";
    switch (m_op_sel)
    {
        case JMP_NONE:
            os << "None";
            break;
        case JMP_SHORT:
            os << "Short";
            break;
        case JMP_NEAR:
            os << "Near";
            break;
        default:
            os << "UNKNOWN!!";
            break;
    }

    os << m_common;
}

void
X86Jmp::finalize(Bytecode& bc)
{
    if (!m_target.finalize())
        throw TooComplexError(N_("jump target expression too complex"));
    if (m_target.is_complex_rel())
        throw ValueError(N_("invalid jump target"));

    // Need to adjust target to the end of the instruction.
    // However, we don't know the instruction length yet (short/near).
    // So just adjust to the start of the instruction, and handle the
    // difference in calc_len() and tobytes().
    Location sub_loc = {&bc, bc.get_fixed_len()};
    m_target.sub_rel(bc.get_container()->get_object(), sub_loc);
    m_target.set_ip_rel();

    Location target_loc;
    if (m_target.is_relative()
        && (!m_target.get_rel()->get_label(&target_loc)
            || target_loc.bc->get_container() != bc.get_container()))
    {
        // External or out of segment, so we can't check distance.
        // Default to near (if explicitly overridden, we never get to
        // this function anyway).
        m_op_sel = JMP_NEAR;
    }
    else
    {
        // Default to short jump
        m_op_sel = JMP_SHORT;
    }
}

unsigned long
X86Jmp::calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    unsigned long len = 0;

    len += m_common.get_len();

    if (m_op_sel == JMP_NEAR)
    {
        len += m_nearop.get_len();
        len += (m_common.m_opersize == 16) ? 2 : 4;
    }
    else
    {
        // Short or maybe long; generate span
        len += m_shortop.get_len() + 1;
        add_span(bc, 1, m_target, -128+len, 127+len);
    }
    return len;
}

bool
X86Jmp::expand(Bytecode& bc,
               unsigned long& len,
               int span,
               long old_val,
               long new_val,
               /*@out@*/ long* neg_thres,
               /*@out@*/ long* pos_thres)
{
    assert(span == 1 && "unrecognized span id");
    assert(m_op_sel != JMP_NEAR && "trying to expand an already-near jump");

    // Upgrade to a near jump
    m_op_sel = JMP_NEAR;
    len -= m_shortop.get_len() + 1;
    len += m_nearop.get_len();
    len += (m_common.m_opersize == 16) ? 2 : 4;

    return false;
}

void
X86Jmp::output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.get_scratch();

    // Prefixes
    m_common.to_bytes(bytes, 0);

    unsigned int size;
    if (m_op_sel == JMP_SHORT)
    {
        // 1 byte relative displacement
        size = 1;

        // Opcode
        m_shortop.to_bytes(bytes);
    }
    else
    {
        // 2/4 byte relative displacement (depending on operand size)
        size = (m_common.m_opersize == 16) ? 2 : 4;

        // Opcode
        m_nearop.to_bytes(bytes);
    }

    bc_out.output(bytes);

    // Adjust relative displacement to end of instruction
    m_target.add_abs(-static_cast<long>(bytes.size()+size));
    m_target.set_size(size*8);

    // Distance from displacement to end of instruction is always 0.
    m_target.set_next_insn(0);

    // Output displacement
    Location loc = {&bc, bc.get_fixed_len()+bytes.size()};
    Bytes& tbytes = bc_out.get_scratch();
    tbytes.resize(size);
    bc_out.output(m_target, tbytes, loc, 1);
}

X86Jmp*
X86Jmp::clone() const
{
    return new X86Jmp(*this);
}

void
append_jmp(BytecodeContainer& container,
           const X86Common& common,
           const X86Opcode& shortop,
           const X86Opcode& nearop,
           std::auto_ptr<Expr> target,
           unsigned long line,
           JmpOpcodeSel op_sel)
{
    Bytecode& bc = container.fresh_bytecode();

    if (shortop.get_len() == 0)
        op_sel = JMP_NEAR;
    if (nearop.get_len() == 0)
        op_sel = JMP_SHORT;

    // jump size not forced near or far, so variable size (need contents)
    // TODO: we can be a bit more optimal for backward jumps within the
    // same bytecode (as the distance is known)
    if (op_sel == JMP_NONE)
    {
        bc.transform(Bytecode::Contents::Ptr(new X86Jmp(
            common, op_sel, shortop, nearop, target)));
        bc.set_line(line);
        return;
    }

    // if jump size selected, generate bytes directly.
    // FIXME: if short jump out of range, this results in an overflow warning
    // instead of a "short jump out of range" error.
    Bytes& bytes = bc.get_fixed();
    common.to_bytes(bytes, 0);

    Value targetv(0, target);
    targetv.set_line(line);
    targetv.set_jump_target();
    targetv.set_ip_rel();
    targetv.set_signed();
    targetv.set_next_insn(0);   // always 0.

    if (op_sel == JMP_SHORT)
    {
        // Opcode
        shortop.to_bytes(bytes);

        // Adjust relative displacement to end of bytecode
        targetv.add_abs(-1);
        targetv.set_size(8);
    }
    else
    {
        // Opcode
        nearop.to_bytes(bytes);

        unsigned int i = (common.m_opersize == 16) ? 2 : 4;

        // Adjust relative displacement to end of bytecode
        targetv.add_abs(-static_cast<long>(i));
        targetv.set_size(i*8);
    }
    bc.append_fixed(targetv);
}

}}} // namespace yasm::arch::x86