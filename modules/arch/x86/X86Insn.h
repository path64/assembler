#ifndef YASM_X86INSN_H
#define YASM_X86INSN_H
//
// x86 instruction handling
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
#include <libyasmx/Insn.h>

#include "X86Arch.h"

namespace yasm
{
namespace arch
{
namespace x86
{

struct X86InfoOperand;
struct X86InsnInfo;
class X86Opcode;

class X86Insn : public Insn
{
public:
    X86Insn(const X86Arch& arch,
            const X86InsnInfo* group,
            const X86Arch::CpuMask& active_cpu,
            unsigned char mod_data0,
            unsigned char mod_data1,
            unsigned char mod_data2,
            unsigned int num_info,
            unsigned int mode_bits,
            unsigned int suffix,
            unsigned int misc_flags,
            X86Arch::ParserSelect parser,
            bool force_strict,
            bool default_rel);
    ~X86Insn();

    void put(marg_ostream& os) const;
    X86Insn* clone() const;

protected:
    void do_append(BytecodeContainer& container, unsigned long line);

private:
    void do_append_jmpfar(BytecodeContainer& container,
                          const X86InsnInfo& info,
                          unsigned long line);

    bool match_jmp_info(const X86InsnInfo& info,
                        unsigned int opersize,
                        X86Opcode& shortop,
                        X86Opcode& nearop) const;
    void do_append_jmp(BytecodeContainer& container,
                       const X86InsnInfo& jinfo,
                       unsigned long line);

    void do_append_general(BytecodeContainer& container,
                           const X86InsnInfo& info,
                           const unsigned int* size_lookup,
                           unsigned long line);

    const X86InsnInfo* find_match(const unsigned int* size_lookup, int bypass)
        const;
    bool match_info(const X86InsnInfo& info,
                    const unsigned int* size_lookup,
                    int bypass) const;
    bool match_operand(const Operand& op,
                       const X86InfoOperand& info_op,
                       const Operand& op0,
                       const unsigned int* size_lookup,
                       int bypass) const;
    void match_error(const unsigned int* size_lookup) const;

    // architecture
    const X86Arch& m_arch;

    // instruction parse group - NULL if empty instruction (just prefixes)
    /*@null@*/ const X86InsnInfo* m_group;

    // CPU feature flags enabled at the time of parsing the instruction
    X86Arch::CpuMask m_active_cpu;

    // Modifier data
    unsigned char m_mod_data[3];

    // Number of elements in the instruction parse group
    unsigned int m_num_info:8;

    // BITS setting active at the time of parsing the instruction
    unsigned int m_mode_bits:8;

    // Suffix flags
    unsigned int m_suffix:8;

    // Tests against BITS==64 and AVX
    unsigned int m_misc_flags:6;

    // Parser enabled at the time of parsing the instruction
    unsigned int m_parser:2;

    // Strict forced setting at the time of parsing the instruction
    unsigned int m_force_strict:1;

    // Default rel setting at the time of parsing the instruction
    unsigned int m_default_rel:1;
};

}}} // namespace yasm::arch::x86

#endif