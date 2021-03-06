#ifndef YASM_RDFOBJECT_H
#define YASM_RDFOBJECT_H
//
// Relocatable Dynamic Object File Format (RDOFF) version 2 format
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
#include "yasmx/ObjectFormat.h"


namespace yasm
{
class DiagnosticsEngine;
class DirectiveInfo;

namespace objfmt
{

class YASM_STD_EXPORT RdfObject : public ObjectFormat
{
public:
    /// Constructor.
    /// To make object format truly usable, set_object()
    /// needs to be called.
    RdfObject(const ObjectFormatModule& module, Object& object)
        : ObjectFormat(module, object)
    {}

    /// Destructor.
    ~RdfObject();

    void AddDirectives(Directives& dirs, StringRef parser);

    bool Read(SourceManager& sm, DiagnosticsEngine& diags);
    void Output(raw_fd_ostream& os,
                bool all_syms,
                DebugFormat& dbgfmt,
                DiagnosticsEngine& diags);

    Section* AddDefaultSection();
    Section* AppendSection(StringRef name,
                           SourceLocation source,
                           DiagnosticsEngine& diags);

    static StringRef getName()
    { return "Relocatable Dynamic Object File Format (RDOFF) v2.0"; }
    static StringRef getKeyword() { return "rdf"; }
    static StringRef getExtension() { return ".rdf"; }
    static unsigned int getDefaultX86ModeBits() { return 32; }
    static StringRef getDefaultDebugFormatKeyword() { return "null"; }
    static std::vector<StringRef> getDebugFormatKeywords();
    static bool isOkObject(Object& object) { return true; }
    static bool Taste(const MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine);

private:
    void DirSection(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirLibrary(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirModule(DirectiveInfo& info, DiagnosticsEngine& diags);

    void AddLibOrModule(StringRef name,
                        bool lib,
                        SourceLocation name_source,
                        DiagnosticsEngine& diags);

    std::vector<std::string> m_module_names;
    std::vector<std::string> m_library_names;
};

}} // namespace yasm::objfmt

#endif
