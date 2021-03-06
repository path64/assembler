#ifndef YASM_DWARFSECTION_H
#define YASM_DWARFSECTION_H
//
// DWARF debugging format per-section data
//
//  Copyright (C) 2006-2007  Peter Johnson
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
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Support/ptr_vector.h"
#include "yasmx/AssocData.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location.h"
#include "yasmx/SymbolRef.h"


namespace yasm {
class Section;

namespace dbgfmt {

/// .loc directive data
struct YASM_STD_EXPORT DwarfLoc
{
    DwarfLoc(Location loc_,
             SourceLocation source_,
             unsigned long file_,
             unsigned long line_);

    SourceLocation source;    // source location of .loc directive

    // source information
    unsigned long file;     // index into table of filenames
    unsigned long line;     // source line number
    unsigned long column;   // source column
    IntNum discriminator;
    bool isa_change;
    unsigned long isa;
    enum IsStmt
    {
        IS_STMT_NOCHANGE = 0,
        IS_STMT_SET,
        IS_STMT_CLEAR
    };
    IsStmt is_stmt;
    bool basic_block;
    bool prologue_end;
    bool epilogue_begin;

    Location loc;           // location following
};

/// Per-section DWARF data
class YASM_STD_EXPORT DwarfSection : public AssocData
{
public:
    static const char* key;

    DwarfSection();
    ~DwarfSection();
#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    /// The locations set by the .loc directives in this section, in assembly
    /// source order.
    typedef stdx::ptr_vector<DwarfLoc> Locs;
    Locs locs;
};

}} // namespace yasm::dbgfmt

#endif
