SET (PLUGIN_CPP)
IF (NOT BUILD_STATIC)
    SET (PLUGIN_CPP yasmx/plugin.cpp)
ENDIF (NOT BUILD_STATIC)

SET (XML_CPP)
IF (WITH_XML)
    SET (XML_CPP
        pugixml/pugixml.cpp
        yasmx/DebugDumper.cpp)
ENDIF (WITH_XML)

SET(LIBYASMX_SOURCES
    llvm/Support/Allocator.cpp
    llvm/Support/APFloat.cpp
    llvm/Support/APInt.cpp
    llvm/Support/Atomic.cpp
    llvm/Support/circular_raw_ostream.cpp
    llvm/Support/CommandLine.cpp
    llvm/Support/CrashRecoveryContext.cpp
    llvm/Support/Debug.cpp
    llvm/Support/Errno.cpp
    llvm/Support/ErrorHandling.cpp
    llvm/Support/FoldingSet.cpp
    llvm/Support/Hashing.cpp
    llvm/Support/Locale.cpp
    llvm/Support/raw_ostream.cpp
    llvm/Support/ManagedStatic.cpp
    llvm/Support/Memory.cpp
    llvm/Support/MemoryBuffer.cpp
    llvm/Support/Mutex.cpp
    llvm/Support/Path.cpp
    llvm/Support/PathV2.cpp
    llvm/Support/PrettyStackTrace.cpp
    llvm/Support/Process.cpp
    llvm/Support/Program.cpp
    llvm/Support/Signals.cpp
    llvm/Support/SmallPtrSet.cpp
    llvm/Support/SmallVector.cpp
    llvm/Support/Statistic.cpp
    llvm/Support/strdup.c
    llvm/Support/StringExtras.cpp
    llvm/Support/StringMap.cpp
    llvm/Support/StringRef.cpp
    llvm/Support/system_error.cpp
    llvm/Support/Threading.cpp
    llvm/Support/ThreadLocal.cpp
    llvm/Support/Timer.cpp
    llvm/Support/TimeValue.cpp
    llvm/Support/Twine.cpp
    llvm/Support/Valgrind.cpp
    ${XML_CPP}
    yasmx/Basic/ConvertUTF.c
    yasmx/Basic/ConvertUTFWrapper.cpp
    yasmx/Basic/Diagnostic.cpp
    yasmx/Basic/DiagnosticIDs.cpp
    yasmx/Basic/FileManager.cpp
    yasmx/Basic/FileSystemStatCache.cpp
    yasmx/Basic/SourceLocation.cpp
    yasmx/Basic/SourceManager.cpp
    yasmx/Frontend/DiagnosticRenderer.cpp
    yasmx/Frontend/OffsetDiagnosticPrinter.cpp
    yasmx/Frontend/TextDiagnostic.cpp
    yasmx/Frontend/TextDiagnosticPrinter.cpp
    yasmx/Parse/Directive.cpp
    yasmx/Parse/DirHelpers.cpp
    yasmx/Parse/HeaderSearch.cpp
    yasmx/Parse/IdentifierTable.cpp
    yasmx/Parse/Lexer.cpp
    yasmx/Parse/NameValue.cpp
    yasmx/Parse/NumericParser.cpp
    yasmx/Parse/Parser.cpp
    yasmx/Parse/ParserImpl.cpp
    yasmx/Parse/Preprocessor.cpp
    yasmx/Parse/PPCaching.cpp
    yasmx/Parse/PPLexerChange.cpp
    yasmx/Parse/TokenLexer.cpp
    yasmx/Support/MD5.cpp
    yasmx/Support/phash.cpp
    yasmx/Support/registry.cpp
    yasmx/AlignBytecode.cpp
    yasmx/Arch.cpp
    yasmx/Assembler.cpp
    yasmx/AssocData.cpp
    yasmx/BytecodeContainer.cpp
    yasmx/BytecodeOutput.cpp
    yasmx/Bytecode.cpp
    yasmx/Bytes.cpp
    yasmx/Bytes_util.cpp
    yasmx/Bytes_leb128.cpp
    yasmx/DataBytecode.cpp
    yasmx/DebugFormat.cpp
    yasmx/EffAddr.cpp
    yasmx/Expr.cpp
    yasmx/Expr_util.cpp
    yasmx/IncbinBytecode.cpp
    yasmx/InputBuffer.cpp
    yasmx/Insn.cpp
    yasmx/IntNum.cpp
    yasmx/Leb128Bytecode.cpp
    yasmx/ListFormat.cpp
    yasmx/Location.cpp
    yasmx/Location_util.cpp
    yasmx/Module.cpp
    yasmx/MultipleBytecode.cpp
    yasmx/NumericOutput.cpp
    yasmx/Object.cpp
    yasmx/ObjectFormat.cpp
    yasmx/Object_util.cpp
    yasmx/OrgBytecode.cpp
    yasmx/Op.cpp
    yasmx/Optimizer.cpp
    ${PLUGIN_CPP}
    yasmx/Reloc.cpp
    yasmx/Section.cpp
    yasmx/StringTable.cpp
    yasmx/Symbol.cpp
    yasmx/Symbol_util.cpp
    yasmx/SymbolRef.cpp
    yasmx/Value.cpp
    )

INCLUDE(CreateSourceGroups)
CreateSourceGroups("Source Files" "" "${LIBYASMX_SOURCES}")

YASM_ADD_LIBRARY(libyasmx SHARED ${LIBYASMX_SOURCES})

SET_TARGET_PROPERTIES(libyasmx PROPERTIES
    OUTPUT_NAME "yasmx"
    )
IF(NOT BUILD_STATIC)
    TARGET_LINK_LIBRARIES(libyasmx ${LIBDL} ${LIBPSAPI} ${LIBIMAGEHLP})
    SET_TARGET_PROPERTIES(libyasmx PROPERTIES
	VERSION "0.0.0"
	SOVERSION 0
	)
ENDIF(NOT BUILD_STATIC)
ADD_DEPENDENCIES(libyasmx DiagnosticIncludes)

IF(INSTALL_GPUASM)
    IF(NOT BUILD_STATIC)
        INSTALL(TARGETS libyasmx ${INSTALL_TARGETS_DEFAULT_ARGS})
    ENDIF(NOT BUILD_STATIC)
ENDIF()

