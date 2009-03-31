#ifndef YASM_EXPR_H
#define YASM_EXPR_H
///
/// @file
/// @brief Expression interface.
///
/// @license
///  Copyright (C) 2001-2007  Michael Urman, Peter Johnson
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
#include <assert.h>
#include <iosfwd>
#include <memory>
#include <vector>

#include "export.h"
#include "IntNum.h"
#include "Location.h"
#include "Op.h"
#include "SymbolRef.h"


namespace yasm
{

class Bytecode;
class Expr;
class ExprTest;
class FloatNum;
class Register;
class Symbol;

/// An term inside an expression.
class YASM_LIB_EXPORT ExprTerm
{
public:
    /// Note loc must be used carefully (in a-b pairs), as only symrecs
    /// can become the relative term in a #Value.
    /// Testing uses bit comparison (&) so these have to be in bitmask form.
    enum Type
    {
        NONE = 0,       ///< Nothing (temporary placeholder only).
        REG = 1<<0,     ///< Register.
        INT = 1<<1,     ///< Integer (IntNum).
        SUBST = 1<<2,   ///< Substitution value.
        FLOAT = 1<<3,   ///< Float (FloatNum).
        SYM = 1<<4,     ///< Symbol.
        LOC = 1<<5,     ///< Direct Location ref (rather than via symbol).
        OP = 1<<6       ///< Operator.
    };

    /// Substitution value.
    struct Subst
    {
        explicit Subst(unsigned int v) : subst(v) {}
        unsigned int subst;
    };

    ExprTerm() : m_type(NONE), m_depth(0) {}
    explicit ExprTerm(const Register& reg, int depth=0)
        : m_type(REG), m_depth(depth)
    {
        m_data.reg = &reg;
    }
    explicit ExprTerm(IntNum intn, int depth=0)
        : m_type(INT), m_depth(depth)
    {
        m_data.intn.m_type = IntNumData::INTNUM_L;
        intn.swap(static_cast<IntNum&>(m_data.intn));
    }
    explicit ExprTerm(const Subst& subst, int depth=0)
        : m_type(SUBST), m_depth(depth)
    {
        m_data.subst = subst.subst;
    }
    explicit ExprTerm(SymbolRef sym, int depth=0)
        : m_type(SYM), m_depth(depth)
    {
        m_data.sym = sym;
    }
    explicit ExprTerm(Location loc, int depth=0)
        : m_type(LOC), m_depth(depth)
    {
        m_data.loc = loc;
    }
    // Depth must be explicit to avoid conflict with IntNum constructor.
    ExprTerm(Op::Op op, int nchild, int depth)
        : m_type(OP), m_depth(depth)
    {
        m_data.op.op = op;
        m_data.op.nchild = nchild;
    }

    // auto_ptr constructors

    ExprTerm(std::auto_ptr<IntNum> intn, int depth=0);
    ExprTerm(std::auto_ptr<FloatNum> flt, int depth=0);

    /// Assignment operator.
    ExprTerm& operator= (const ExprTerm& rhs)
    {
        if (this != &rhs)
            ExprTerm(rhs).swap(*this);
        return *this;
    }

    /// Copy constructor.
    ExprTerm(const ExprTerm& term);

    /// Destructor.
    ~ExprTerm() { clear(); }

    /// Exchanges this term with another term.
    /// @param oth      other term
    void swap(ExprTerm& oth);

    /// Clear the term.
    void clear();

    /// Make the term zero.
    void zero();

    /// Is term cleared?
    bool is_empty() const { return (m_type == NONE); }

    /// Comparison used for sorting; assumes TermTypes are in sort order.
    bool operator< (const ExprTerm& other) const
    {
        return (m_type < other.m_type);
    }

    /// Match type.  Can take an OR'ed combination of TermTypes.
    bool is_type(int type) const { return (m_type & type) != 0; }

    /// Get the type.
    Type get_type() const { return m_type; }

    /// Match operator.  Does not match non-operators.
    /// @param op       operator to match
    /// @return True if operator is op, false if non- or other operator.
    bool is_op(Op::Op op) const
    {
        return (m_type == OP && m_data.op.op == op);
    }

    /// Test if term is an operator.
    /// @return True if term is an operator, false otherwise.
    bool is_op() const { return (m_type == OP); }

    /// Change operator.  Term must already be an operator.
    /// Maintains existing depth.
    void set_op(Op::Op op)
    {
        assert(m_type == OP);
        m_data.op.op = op;
    }

    // Helper functions to make it easier to get specific types.

    const Register* get_reg() const
    {
        return (m_type == REG ? m_data.reg : 0);
    }

    const IntNum* get_int() const
    {
        return (m_type == INT ? static_cast<const IntNum*>(&m_data.intn) : 0);
    }

    IntNum* get_int()
    {
        return (m_type == INT ? static_cast<IntNum*>(&m_data.intn) : 0);
    }

    void set_int(IntNum intn)
    {
        clear();
        m_type = INT;
        m_data.intn.m_type = IntNumData::INTNUM_L;
        intn.swap(static_cast<IntNum&>(m_data.intn));
    }

    const unsigned int* get_subst() const
    {
        return (m_type == SUBST ? &m_data.subst : 0);
    }

    FloatNum* get_float() const
    {
        return (m_type == FLOAT ? m_data.flt : 0);
    }

    SymbolRef get_sym() const
    {
        return SymbolRef(m_type == SYM ? m_data.sym : 0);
    }

    const Location* get_loc() const
    {
        return (m_type == LOC ? &m_data.loc : 0);
    }

    Location* get_loc()
    {
        return (m_type == LOC ? &m_data.loc : 0);
    }

    Op::Op get_op() const
    {
        return (m_type == OP ? m_data.op.op : Op::NONNUM);
    }

    int get_nchild() const
    {
        return (m_type == OP ? m_data.op.nchild : 0);
    }

    void add_nchild(int delta)
    {
        assert(m_type == OP);
        m_data.op.nchild += delta;
    }

private:
    /// Expression item data.  Correct value depends on type.
    union Data
    {
        const Register *reg;    ///< Register (#REG)
        IntNumData intn;        ///< Integer value (#INT)
        unsigned int subst;     ///< Subst placeholder (#SUBST)
        FloatNum *flt;          ///< Floating point value (#FLOAT)
        Symbol *sym;            ///< Symbol (#SYM)
        Location loc;           ///< Direct bytecode ref (#LOC)
        struct
        {
            Op::Op op;          ///< Operator
            int nchild;         ///< Number of children
        }
        op;                     ///< Operator (#OP)
    } m_data;                   ///< Data.
    Type m_type;                ///< Type.

public:
    int m_depth;                ///< Depth in tree.
};

typedef std::vector<ExprTerm> ExprTerms;

/// An expression.
///
/// Expressions are n-ary trees.  Most operators are either unary or binary,
/// but associative operators such as Op::ADD and Op::MUL may have more than
/// two children.
///
/// Expressions are stored as a vector of terms (ExprTerm) in
/// reverse polish notation (highest operator last).  Each term may be an
/// operator or a value (such as an integer or register) and has an
/// associated depth.  Operator terms also keep track of the number of
/// immediate children they have.
///
/// Examples of expressions stored in vector format:
///
/// <pre>
/// Infix: (a+b)*c
/// Index [0] [1] [2] [3] [4]
/// Depth  2   2   1   1   0
/// Data   a   b   +   c   *
///
/// Infix: (a+b+c)+d+(e*f)
/// Index [0] [1] [2] [3] [4] [5] [6] [7] [8]
/// Depth  2   2   2   1   1   2   2   1   0
/// Data   a   b   c   +   d   e   f   *   +
///
/// Infix: a
/// Index [0]
/// Depth  0
/// Data   a
///
/// Infix: a+(((b+c)*d)-e)+f
/// Index [0] [1] [2] [3] [4] [5] [6] [7] [8] [9]
/// Depth  1   4   4   3   3   2   2   1   1   0
/// Data   a   b   c   +   d   *   e   -   f   +
/// </pre>
///
/// General Expr usage does not need to be aware of this internal data format,
/// but it is key to doing advanced expression manipulation.  Due to the
/// RPN storage style, most processing occurs going from back-to-front within
/// the terms vector.
class YASM_LIB_EXPORT Expr
{
    friend class ExprTest;

public:
    typedef std::auto_ptr<Expr> Ptr;

    /// Empty constructor.
    Expr() {}

    /// Assign to expression.
    /// @param term     expression value
    template <typename T>
    Expr& operator= (const T& term)
    {
        m_terms.clear();
        m_terms.push_back(ExprTerm(term));
        return *this;
    }

    /// Assign to expression.
    /// @param term     expression value
    template <typename T>
    Expr& operator= (T& term)
    {
        m_terms.clear();
        m_terms.push_back(ExprTerm(term));
        return *this;
    }

    /// Copy constructor.
    /// @param e        expression to copy from
    Expr(const Expr& e);

    /// Single-term constructor for register.
    explicit Expr(const Register& reg)
    { m_terms.push_back(ExprTerm(reg)); }

    /// Single-term constructor for integer.
    explicit Expr(IntNum intn)
    { m_terms.push_back(ExprTerm(intn)); }

    /// Single-term constructor for symbol.
    explicit Expr(SymbolRef sym)
    { m_terms.push_back(ExprTerm(sym)); }

    /// Single-term constructor for location.
    explicit Expr(Location loc)
    { m_terms.push_back(ExprTerm(loc)); }

    /// Single-term constructor for IntNum auto_ptr.
    explicit Expr(std::auto_ptr<IntNum> intn);

    /// Single-term constructor for FloatNum auto_ptr.
    explicit Expr(std::auto_ptr<FloatNum> flt);

    /// Destructor.
    ~Expr();

    /// Determine if an expression is a specified operation (at the top
    /// level).
    /// @param op   operator
    /// @return True if the expression was the specified operation at the top
    ///         level.
    bool is_op(Op::Op op) const
    { return !is_empty() && m_terms.back().is_op(op); }

    /// Exchanges this expression with another expression.
    /// @param oth      other expression
    void swap(Expr& oth);

    /// Get an allocated copy.
    /// @return Newly allocated copy of expression.
    Expr* clone() const { return new Expr(*this); }

    /// Clear the expression.
    void clear();

    /// Is expression empty?
    bool is_empty() const { return m_terms.empty(); }

    /// Simplify an expression as much as possible.  Eliminates extraneous
    /// branches and simplifies integer-only subexpressions.  Does *not*
    /// expand EQUs; use expand_equ() in expr_util.h to first expand EQUs.
    /// @param simplify_reg_mul simplify REG*1 identities
    void simplify(bool simplify_reg_mul = true);

    /// Simplify an expression as much as possible, taking a functor for
    /// additional processing.  Calls level_op() both before and after the
    /// functor in post-order.  Functor is only called on operator terms.
    /// @param func             functor to call on each operator, bottom-up
    ///                         called as (Expr&, int pos)
    /// @param simplify_reg_mul simplify REG*1 identities
    template <typename T>
    void simplify(const T& func, bool simplify_reg_mul = true);

    /// Extract the segment portion of an expression containing SEG:OFF,
    /// leaving the offset.
    /// @return Empty expression if unable to extract a segment (expr does not
    ///         contain an Op::SEGOFF operator), otherwise the segment
    ///         expression.
    ///         The input expression is modified such that on return, it's
    ///         the offset expression.
    Expr extract_deep_segoff();

    /// Extract the segment portion of a SEG:OFF expression, leaving the
    /// offset.
    /// @return Empty expression if unable to extract a segment (OP::SEGOFF
    ///         not the top-level operator), otherwise the segment expression.
    ///         The input expression is modified such that on return, it's the
    ///         offset expression.
    Expr extract_segoff();

    /// Extract the right portion (y) of a x WRT y expression, leaving the
    /// left portion (x).
    /// @return Empty expression if unable to extract (OP::WRT not the
    ///         top-level operator), otherwise the right side of the WRT
    ///         expression.
    ///         The input expression is modified such that on return, it's
    ///         the left side of the WRT expression.
    Expr extract_wrt();

    /// Get the float value of an expression if it's just a float.
    /// @return 0 if the expression is too complex; otherwise the float
    ///         value of the expression.
    /*@dependent@*/ /*@null@*/ FloatNum* get_float() const;

    /// Get the integer value of an expression if it's just an integer.
    /// @return 0 if the expression is too complex (contains anything other
    ///         than integers, ie floats, non-valued labels, registers);
    ///         otherwise the intnum value of the expression.
    /*@dependent@*/ /*@null@*/ const IntNum* get_intnum() const;

    /// Get the integer value of an expression if it's just an integer.
    /// @return 0 if the expression is too complex (contains anything other
    ///         than integers, ie floats, non-valued labels, registers);
    ///         otherwise the intnum value of the expression.
    /*@dependent@*/ /*@null@*/ IntNum* get_intnum();

    /// Get the symbol value of an expression if it's just a symbol.
    /// @return 0 if the expression is too complex; otherwise the symbol
    ///         value of the expression.
    SymbolRef get_symbol() const;

    /// Get the register value of an expression if it's just a register.
    /// @return 0 if the expression is too complex; otherwise the register
    ///         value of the expression.
    /*@dependent@*/ /*@null@*/ const Register* get_reg() const;

    bool contains(int type, int pos=-1) const;

    /// Substitute terms into expr SUBST terms (by index).
    /// @param terms        terms
    /// @return True on error (index out of range).
    bool substitute(const ExprTerms& terms);

    void calc(Op::Op op)
    {
        if (!is_empty())
            append_op(op, 1);
    }
    template <typename T>
    void calc(Op::Op op, const T& rhs)
    {
        bool was_empty = is_empty();
        append(rhs);
        if (!was_empty)
            append_op(op, 2);
    }

    /// @defgroup lowlevel Low Level Manipulators
    /// Functions to manipulate the innards of the expression terms.
    /// Use with caution.
    ///@{

    /// Get raw expression terms.
    /// @return Terms reference.
    ExprTerms& get_terms() { return m_terms; }

    /// Get raw expression terms (const version).
    /// @return Const Terms reference.
    const ExprTerms& get_terms() const { return m_terms; }

    /// Append an expression term to terms.
    /// @param term     expression term
    template <typename T>
    void append(const T& term)
    { m_terms.push_back(ExprTerm(term)); }

    /// Append an operator to terms.  Pushes down all current terms and adds
    /// operator term to end.
    /// @param op       operator
    /// @param nchild   number of children
    void append_op(Op::Op op, int nchild);

    /// Make expression an ident if it only has one term.
    /// @param pos      index of operator term, may be negative for "from end"
    void make_ident(int pos=-1);

    /// Levels an expression tree.
    /// a+(b+c) -> a+b+c
    /// (a+b)+(c+d) -> a+b+c+d
    /// Only levels operators that allow more than two operand terms.
    /// Folds (combines by evaluation) *integer* constant values.
    /// @note Only does *one* level of leveling.  Should be called
    ///       post-order on a tree to combine deeper levels.
    /// @param simplify_reg_mul simplify REG*1 identities
    /// @param pos              index of top-level operator term
    void level_op(bool simplify_reg_mul, int pos=-1);

    //@}

private:
    /// Terms of the expression.  The entire tree is stored here.
    ExprTerms m_terms;

    /// Clean up terms by removing all empty (ExprTerm::NONE) elements.
    void cleanup();

    /// Reduce depth of a subexpression.
    /// @param pos      term index of subexpression operator
    /// @param delta    delta to reduce depth by
    void reduce_depth(int pos, int delta=1);

    /// Clear all terms of a subexpression, possibly keeping a single term.
    /// @param pos      term index of subexpression operator
    /// @param keep     term index of term to keep; -1 to clear all terms
    void clear_except(int pos, int keep=-1);

    /// Transform all Op::SUB and Op::NEG subexpressions into appropriate *-1
    /// variants.  This assists with operator leveling as it transforms the
    /// nonlevelable Op::SUB into the levelable Op::ADD.
    void xform_neg();

    /// LHS expression extractor.
    /// @param op   reverse iterator pointing at operator term to be extracted
    ///             from
    Expr extract_lhs(ExprTerms::reverse_iterator op);
};

/// Assign an expression.
/// @param e        expression
template <>
inline Expr&
Expr::operator= (const Expr& rhs)
{
    if (this != &rhs)
        Expr(rhs).swap(*this);
    return *this;
}

/// Assign an expression.
/// @param e        expression
template <>
inline Expr&
Expr::operator= (Expr& rhs)
{
    if (this != &rhs)
        Expr(rhs).swap(*this);
    return *this;
}

/// Assign an expression term.
/// @param term     expression term
template <>
inline Expr&
Expr::operator= (const ExprTerm& term)
{
    m_terms.clear();
    m_terms.push_back(term);
    return *this;
}

/// Assign an expression term.
/// @param term     expression term
template <>
inline Expr&
Expr::operator= (ExprTerm& term)
{
    m_terms.clear();
    m_terms.push_back(term);
    return *this;
}

/// Append an expression to terms.
/// @param e        expression
template <>
inline void Expr::append(const Expr& e)
{ m_terms.insert(m_terms.end(), e.m_terms.begin(), e.m_terms.end()); }

/// Append an expression term to terms.
/// @param term     expression term
template <>
inline void Expr::append(const ExprTerm& term)
{ m_terms.push_back(term); }

template <typename T>
void
Expr::simplify(const T& func, bool simplify_reg_mul)
{
    xform_neg();

    // Must re-call size() in conditional as it may change during execution.
    for (int pos = 0; pos < static_cast<int>(m_terms.size()); ++pos)
    {
        if (!m_terms[pos].is_op())
            continue;
        level_op(simplify_reg_mul, pos);

        if (!m_terms[pos].is_op())
            continue;
        func(*this, pos);

        if (!m_terms[pos].is_op())
            continue;
        level_op(simplify_reg_mul, pos);
    }

    cleanup();
}

/// Expression builder based on operator.
/// Allows building expressions with the syntax Expr e = ADD(0, sym, ...);
struct ExprBuilder
{
    Op::Op op;

    template <typename T1>
    Expr operator() (const T1& t1) const
    {
        Expr e;
        e.append(t1);
        e.append_op(op, 1);
        return e;
    }

    template <typename T1, typename T2>
    Expr operator() (const T1& t1, const T2& t2) const
    {
        Expr e;
        e.append(t1);
        e.append(t2);
        e.append_op(op, 2);
        return e;
    }

    template <typename T1, typename T2, typename T3>
    Expr operator() (const T1& t1, const T2& t2, const T3& t3) const
    {
        Expr e;
        e.append(t1);
        e.append(t2);
        e.append(t3);
        e.append_op(op, 3);
        return e;
    }

    template <typename T1, typename T2, typename T3, typename T4>
    Expr operator() (const T1& t1, const T2& t2, const T3& t3,
                     const T4& t4) const
    {
        Expr e;
        e.append(t1);
        e.append(t2);
        e.append(t3);
        e.append(t4);
        e.append_op(op, 4);
        return e;
    }

    template <typename T1, typename T2, typename T3, typename T4, typename T5>
    Expr operator() (const T1& t1, const T2& t2, const T3& t3, const T4& t4,
                     const T5& t5) const
    {
        Expr e;
        e.append(t1);
        e.append(t2);
        e.append(t3);
        e.append(t4);
        e.append(t5);
        e.append_op(op, 5);
        return e;
    }

    template <typename T1, typename T2, typename T3, typename T4, typename T5,
              typename T6>
    Expr operator() (const T1& t1, const T2& t2, const T3& t3, const T4& t4,
                     const T5& t5, const T6& t6) const
    {
        Expr e;
        e.append(t1);
        e.append(t2);
        e.append(t3);
        e.append(t4);
        e.append(t5);
        e.append(t6);
        e.append_op(op, 6);
        return e;
    }
};

/// Specialization of ExprBuilder to allow terms to be passed.
/// This allows e.g. ADD(terms).
template <>
inline Expr ExprBuilder::operator() (const ExprTerms& terms) const
{
    Expr e;
    for (ExprTerms::const_iterator i=terms.begin(), end=terms.end(); i != end;
         ++i)
        e.append(*i);
    e.append_op(op, terms.size());
    return e;
}

extern YASM_LIB_EXPORT const ExprBuilder ADD;        ///< Addition (+).
extern YASM_LIB_EXPORT const ExprBuilder SUB;        ///< Subtraction (-).
extern YASM_LIB_EXPORT const ExprBuilder MUL;        ///< Multiplication (*).
extern YASM_LIB_EXPORT const ExprBuilder DIV;        ///< Unsigned division.
extern YASM_LIB_EXPORT const ExprBuilder SIGNDIV;    ///< Signed division.
extern YASM_LIB_EXPORT const ExprBuilder MOD;        ///< Unsigned modulus.
extern YASM_LIB_EXPORT const ExprBuilder SIGNMOD;    ///< Signed modulus.
extern YASM_LIB_EXPORT const ExprBuilder NEG;        ///< Negation (-).
extern YASM_LIB_EXPORT const ExprBuilder NOT;        ///< Bitwise negation.
extern YASM_LIB_EXPORT const ExprBuilder OR;         ///< Bitwise OR.
extern YASM_LIB_EXPORT const ExprBuilder AND;        ///< Bitwise AND.
extern YASM_LIB_EXPORT const ExprBuilder XOR;        ///< Bitwise XOR.
extern YASM_LIB_EXPORT const ExprBuilder XNOR;       ///< Bitwise XNOR.
extern YASM_LIB_EXPORT const ExprBuilder NOR;        ///< Bitwise NOR.
extern YASM_LIB_EXPORT const ExprBuilder SHL;        ///< Shift left (logical).
extern YASM_LIB_EXPORT const ExprBuilder SHR;        ///< Shift right (logical).
extern YASM_LIB_EXPORT const ExprBuilder LOR;        ///< Logical OR.
extern YASM_LIB_EXPORT const ExprBuilder LAND;       ///< Logical AND.
extern YASM_LIB_EXPORT const ExprBuilder LNOT;       ///< Logical negation.
extern YASM_LIB_EXPORT const ExprBuilder LXOR;       ///< Logical XOR.
extern YASM_LIB_EXPORT const ExprBuilder LXNOR;      ///< Logical XNOR.
extern YASM_LIB_EXPORT const ExprBuilder LNOR;       ///< Logical NOR.
extern YASM_LIB_EXPORT const ExprBuilder LT;         ///< Less than comparison.
extern YASM_LIB_EXPORT const ExprBuilder GT;         ///< Greater than.
extern YASM_LIB_EXPORT const ExprBuilder EQ;         ///< Equality comparison.
extern YASM_LIB_EXPORT const ExprBuilder LE;         ///< Less than or equal to.
extern YASM_LIB_EXPORT const ExprBuilder GE;         ///< Greater than or equal.
extern YASM_LIB_EXPORT const ExprBuilder NE;         ///< Not equal comparison.
/// SEG operator (gets segment portion of address).
extern YASM_LIB_EXPORT const ExprBuilder SEG;
/// WRT operator (gets offset of address relative to some other
/// segment).
extern YASM_LIB_EXPORT const ExprBuilder WRT;
/// The ':' in segment:offset.
extern YASM_LIB_EXPORT const ExprBuilder SEGOFF;

/// Overloaded assignment binary operators.
template <typename T> inline Expr& operator+=(Expr& lhs, const T& rhs)
{ lhs.calc(Op::ADD, rhs); return lhs; }
template <typename T> inline Expr& operator-=(Expr& lhs, const T& rhs)
{ lhs.calc(Op::SUB, rhs); return lhs; }
template <typename T> inline Expr& operator*=(Expr& lhs, const T& rhs)
{ lhs.calc(Op::MUL, rhs); return lhs; }
template <typename T> inline Expr& operator/=(Expr& lhs, const T& rhs)
{ lhs.calc(Op::DIV, rhs); return lhs; }
template <typename T> inline Expr& operator%=(Expr& lhs, const T& rhs)
{ lhs.calc(Op::MOD, rhs); return lhs; }
template <typename T> inline Expr& operator^=(Expr& lhs, const T& rhs)
{ lhs.calc(Op::XOR, rhs); return lhs; }
template <typename T> inline Expr& operator&=(Expr& lhs, const T& rhs)
{ lhs.calc(Op::AND, rhs); return lhs; }
template <typename T> inline Expr& operator|=(Expr& lhs, const T& rhs)
{ lhs.calc(Op::OR, rhs); return lhs; }
template <typename T> inline Expr& operator>>=(Expr& lhs, const T& rhs)
{ lhs.calc(Op::SHR, rhs); return lhs; }
template <typename T> inline Expr& operator<<=(Expr& lhs, const T& rhs)
{ lhs.calc(Op::SHL, rhs); return lhs; }

YASM_LIB_EXPORT
std::ostream& operator<< (std::ostream& os, const ExprTerm& term);
YASM_LIB_EXPORT
std::ostream& operator<< (std::ostream& os, const Expr& e);

/// Get left and right hand immediate children, or single immediate child.
/// @param e        Expression
/// @param lhs      Term index of left hand side (output)
/// @param rhs      Term index of right hand side (output)
/// @param pos      Term index of operator (root), both input and output.
/// Pos is updated before return with the term index following the tree.
/// For single children, pass lhs=NULL and the rhs output will be the single
/// child.  Passed-in pos may be negative to indicate index "from end".
/// @return False if too many or too few children found.
YASM_LIB_EXPORT
bool get_children(Expr& e, /*@out@*/ int* lhs, /*@out@*/ int* rhs, int* pos);

/// Determine if a expression subtree is of the form Symbol*-1.
/// @param e        Expression
/// @param sym      Term index of symbol term (output)
/// @param neg1     Term index of -1 term (output)
/// @param pos      Term index of operator (root), both input and output.
/// @param loc_ok   If true, Location*-1 is also accepted
/// @return True if subtree matches.
/// If the subtree matches, pos is updated before return with the term index
/// following the tree.
YASM_LIB_EXPORT
bool is_neg1_sym(Expr& e,
                 /*@out@*/ int* sym,
                 /*@out@*/ int* neg1,
                 int* pos,
                 bool loc_ok);

/// Specialized swap for Expr.
inline void
swap(Expr& left, Expr& right)
{
    left.swap(right);
}

/// Specialized swap for ExprTerm.
inline void
swap(ExprTerm& left, ExprTerm& right)
{
    left.swap(right);
}

} // namespace yasm

namespace std
{

/// Specialized std::swap for Expr.
template <>
inline void
swap(yasm::Expr& left, yasm::Expr& right)
{
    left.swap(right);
}

/// Specialized std::swap for ExprTerm.
template <>
inline void
swap(yasm::ExprTerm& left, yasm::ExprTerm& right)
{
    left.swap(right);
}

} // namespace std

#endif