#include "Formats.h"
#include "ROMFST.h"
#include "C_DataPack.h"
#include "C_Vector.h"
#include "C_Stream.h"

namespace FormatsInternal
{
    bool ExportDLLs(FSTContext*);
    bool CompileDLLs(FSTContext*);

    bool ExportGlobalMap(FSTContext*);
    bool CompileGlobalMap(FSTContext*);

    bool ExportDLLSIMPORTTAB(FSTContext*);
    bool CompileDLLSIMPORTTAB(FSTContext*);

    bool ExportMAPINFO(FSTContext*);
    bool CompileMAPINFO(FSTContext*);

    static const FormatInfo sFormats[Formats::NUM_FMTS] =
    {
        { ExportDLLs, CompileDLLs },
        { ExportDLLSIMPORTTAB, CompileDLLSIMPORTTAB },
        { ExportGlobalMap, CompileGlobalMap },
        { ExportMAPINFO, CompileMAPINFO }
    };
}

const FormatInfo& Formats::GetFormatInfo(int aType)
{
    return FormatsInternal::sFormats[aType];
}
