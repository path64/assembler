//
// YASM bytecode implementation
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
#include "bytecode.h"

#include "util.h"

#include <iomanip>
#include <ostream>

#include "bytes.h"
#include "errwarn.h"
#include "errwarns.h"
#include "expr.h"
#include "intnum.h"
#include "location_util.h"
#include "operator.h"
#include "symbol.h"
#include "value.h"


namespace yasm {

void
Bytecode::set_multiple(std::auto_ptr<Expr> e)
{
    m_multiple = e;
}

void
Bytecode::multiply_multiple(std::auto_ptr<Expr> e)
{
    if (m_multiple.get() != 0)
        m_multiple.reset(new Expr(m_multiple, Op::MUL, e, e->get_line()));
    else
        m_multiple = e;
}

bool
Bytecode::Contents::expand(Bytecode& bc, unsigned long& len, int span,
                           long old_val, long new_val,
                           /*@out@*/ long& neg_thres,
                           /*@out@*/ long& pos_thres)
{
    throw InternalError(N_("bytecode does not have any dependent spans"));
}

/*@null@*/ const Expr*
Bytecode::Contents::reserve_numitems(/*@out@*/ unsigned int& itemsize) const
{
    itemsize = 0;
    return 0;
}

Bytecode::Contents::SpecialType
Bytecode::Contents::get_special() const
{
    return SPECIAL_NONE;
}

void
Bytecode::transform(std::auto_ptr<Contents> contents)
{
    m_contents.reset(contents.release());
}

Bytecode::Bytecode(std::auto_ptr<Contents> contents, unsigned long line)
    : m_contents(contents),
      m_section(0),
      m_multiple(0),
      m_len(0),
      m_mult_int(1),
      m_line(line),
      m_offset(~0UL),   // obviously incorrect / uninitialized value
      m_index(~0UL)
{
}

Bytecode::Bytecode()
    : m_contents(0),
      m_section(0),
      m_multiple(0),
      m_len(0),
      m_mult_int(1),
      m_line(0),
      m_offset(~0UL),   // obviously incorrect / uninitialized value
      m_index(~0UL)
{
}

Bytecode::Bytecode(const Bytecode& oth)
    : m_contents(oth.m_contents->clone()),
      m_section(oth.m_section),
      m_multiple(oth.m_multiple->clone()),
      m_len(oth.m_len),
      m_mult_int(oth.m_mult_int),
      m_line(oth.m_line),
      m_offset(oth.m_offset),
      m_index(oth.m_index),
      m_symbols(oth.m_symbols)
{}

Bytecode::~Bytecode()
{
}

Bytecode&
Bytecode::operator= (const Bytecode& oth)
{
    if (this != &oth) {
        if (oth.m_contents.get() != 0)
            m_contents.reset(oth.m_contents->clone());
        else
            m_contents.reset(0);
        m_section = oth.m_section;
        if (oth.m_multiple.get() != 0)
            m_multiple.reset(oth.m_multiple->clone());
        else
            m_multiple.reset(0);
        m_len = oth.m_len;
        m_mult_int = oth.m_mult_int;
        m_line = oth.m_line;
        m_offset = oth.m_offset;
        m_index = oth.m_index;
        m_symbols = oth.m_symbols;
    }
    return *this;
}

void
Bytecode::put(std::ostream& os, int indent_level) const
{
    if (m_contents.get() != 0)
        m_contents->put(os, indent_level);
    else
        os << std::setw(indent_level) << "" << "EMPTY\n";
    os << std::setw(indent_level) << "" << "Multiple=";
    if (m_multiple.get() == 0)
        os << "nil (1)";
    else
        os << *m_multiple;
    os << '\n';
    os << std::setw(indent_level) << "" << "Length=" << m_len << '\n';
    os << std::setw(indent_level) << "" << "Line Index=" << m_line << '\n';
    os << std::setw(indent_level) << "" << "Offset=" << m_offset << '\n';
}

void
Bytecode::finalize()
{
    for (std::vector<Fixup>::iterator i=m_fixed_fixups.begin(),
         end=m_fixed_fixups.end(); i != end; ++i) {
        Location loc = {this, i->get_off()};
        if (i->finalize(loc)) {
            if (i->m_jump_target)
                throw TooComplexError(i->get_line(),
                                      N_("jump target expression too complex"));
            else
                throw TooComplexError(i->get_line(),
                                      N_("expression too complex"));
        }
        if (i->m_jump_target) {
            if (i->m_seg_of || i->m_rshift || i->m_curpos_rel)
                throw ValueError(i->get_line(), N_("invalid jump target"));
            i->set_curpos_rel(*this, false);
        }
        warn_update_line(i->get_line());
    }

    if (m_contents.get() == 0)
        return;
    m_contents->finalize(*this);
    if (m_multiple.get() != 0) {
        Location loc = {this, 0};
        Value val(0, std::auto_ptr<Expr>(m_multiple->clone()));

        if (val.finalize(loc))
            throw TooComplexError(N_("multiple expression too complex"));
        else if (val.is_relative())
            throw NotAbsoluteError(N_("multiple expression not absolute"));
        // Finalize creates NULL output if value=0, but bc->multiple is NULL
        // if value=1 (this difference is to make the common case small).
        // However, this means we need to set bc->multiple explicitly to 0
        // here if val.abs is NULL.
        if (const Expr* e = val.get_abs())
            m_multiple.reset(e->clone());
        else
            m_multiple.reset(new Expr(new IntNum(0), m_line));
    }
}

void
Bytecode::finalize(Errwarns& errwarns)
{
    try {
        finalize();
    } catch (Error& err) {
        errwarns.propagate(m_line, err);
    }
    errwarns.propagate(m_line);     // propagate warnings
}

void
Bytecode::calc_len(AddSpanFunc add_span)
{
    if (m_contents.get() == 0) {
        m_len = 0;
        return;
    }
    m_len = m_contents->calc_len(*this, add_span);

    // Check for multiples
    m_mult_int = 1;
    if (m_multiple.get() != 0) {
        if (const IntNum* num = m_multiple->get_intnum()) {
            if (num->sign() < 0) {
                m_len = 0;
                throw ValueError(N_("multiple is negative"));
            } else
                m_mult_int = num->get_int();
        } else {
            if (m_multiple->contains(Expr::FLOAT)) {
                m_len = 0;
                throw ValueError(
                    N_("expression must not contain floating point value"));
            } else {
                Value value(0, Expr::Ptr(m_multiple->clone()));
                add_span(*this, 0, value, 0, 0);
                m_mult_int = 0;     // assume 0 to start
            }
        }
    }
}

void
Bytecode::calc_len(AddSpanFunc add_span, Errwarns& errwarns)
{
    try {
        calc_len(add_span);
    } catch (Error& err) {
        errwarns.propagate(m_line, err);
    }
    errwarns.propagate(m_line);     // propagate warnings
}

bool
Bytecode::expand(int span, long old_val, long new_val,
                 /*@out@*/ long& neg_thres, /*@out@*/ long& pos_thres)
{
    if (span == 0) {
        m_mult_int = new_val;
        return true;
    }
    if (m_contents.get() == 0)
        return false;
    return m_contents->expand(*this, m_len, span, old_val, new_val, neg_thres,
                              pos_thres);
}

bool
Bytecode::expand(int span, long old_val, long new_val,
                 /*@out@*/ long& neg_thres, /*@out@*/ long& pos_thres,
                 Errwarns& errwarns)
{
    bool retval = false;
    try {
        retval = expand(span, old_val, new_val, neg_thres, pos_thres);
    } catch (Error& err) {
        errwarns.propagate(m_line, err);
    }
    errwarns.propagate(m_line);     // propagate warnings
    return retval;
}

void
Bytecode::to_bytes(Bytes& bytes, /*@out@*/ unsigned long& gap,
                   OutputValueFunc output_value,
                   /*@null@*/ OutputRelocFunc output_reloc)
{
    unsigned int last = 0;
    for (std::vector<Fixup>::iterator i=m_fixed_fixups.begin(),
         end=m_fixed_fixups.end(); i != end; ++i) {
        unsigned int off = i->get_off();
        Location loc = {this, off};
        bytes.insert(bytes.end(), m_fixed.begin() + last,
                     m_fixed.begin() + off);
        output_value(*i, bytes, i->m_size/8, loc, i->m_sign ? -1 : 1);
        last = off + i->m_size/8;
        warn_update_line(i->get_line());
    }
    bytes.insert(bytes.end(), m_fixed.begin() + last, m_fixed.end());

    // handle tail contents
    gap = 0;

    m_mult_int = get_multiple(true);
    if (m_mult_int == 0)
        return; // nothing to output

    if (m_contents.get() == 0)
        return; // nothing to output

    // special case for reserve bytecodes
    if (m_contents->get_special() == Contents::SPECIAL_RESERVE) {
        gap = m_len * m_mult_int;
        return;
    }

    Bytes lb;
    lb.reserve(m_len);
    for (long i=0; i<m_mult_int; i++) {
        m_contents->to_bytes(*this, lb, output_value, output_reloc);

        if (lb.size() != m_len)
            throw InternalError(
                N_("written length does not match optimized length"));

        bytes.insert(bytes.end(), lb.begin(), lb.end());
        lb.clear();
    }
}

long
Bytecode::get_multiple(bool calc_dist)
{
    if (m_multiple.get() != 0) {
        if (calc_dist)
            m_multiple->level_tree(true, true, true, xform_calc_dist);
        const IntNum* num = m_multiple->get_intnum();
        if (!num)
            throw ValueError(N_("could not determine multiple"));
        if (num->sign() < 0)
            throw ValueError(N_("multiple is negative"));
        return num->get_int();
    }
    return 1;
}

unsigned long
Bytecode::update_offset(unsigned long offset)
{
    if (m_contents.get() != 0 &&
        m_contents->get_special() == Contents::SPECIAL_OFFSET) {
        // Recalculate/adjust len of offset-based bytecodes here
        long neg_thres = 0;
        long pos_thres = (long)next_offset();
        expand(1, 0, (long)offset, neg_thres, pos_thres);
    }
    m_offset = offset;
    return next_offset();
}

unsigned long
Bytecode::update_offset(unsigned long offset, Errwarns& errwarns)
{
    unsigned long retval;
    try {
        retval = update_offset(offset);
    } catch (Error& err) {
        errwarns.propagate(m_line, err);
        retval = offset + m_len*m_mult_int;
    }
    errwarns.propagate(m_line);     // propagate warnings
    return retval;
}

void
Bytecode::append_fixed(const Value& val)
{
    m_fixed_fixups.push_back(Fixup(m_fixed.size(), val, m_line));
    m_fixed.write(val.m_size/8, 0);
}

void
Bytecode::append_fixed(unsigned int size, std::auto_ptr<Expr> e)
{
    m_fixed_fixups.push_back(Fixup(m_fixed.size(), Value(size*8, e), m_line));
    m_fixed.write(size, 0);
}

Bytecode::Fixup::Fixup(unsigned int off, const Value& val, unsigned long line)
    : Value(val), m_line(line), m_off(off)
{
}

} // namespace yasm
