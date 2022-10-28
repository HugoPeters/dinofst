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

    bool ExportMPEG(FSTContext*);
    bool CompileMPEG(FSTContext*);

	bool ExportMODELS(FSTContext*);
	bool CompileMODELS(FSTContext*);

	bool ExportMODELIND(FSTContext*);
	bool CompileMODELIND(FSTContext*);

    static const FormatInfo sFormats[Formats::NUM_FMTS] =
    {
        { ExportDLLs, CompileDLLs, "DLLS" },
        { ExportDLLSIMPORTTAB, CompileDLLSIMPORTTAB, "DLLSIMPORTTAB" },
        { ExportGlobalMap, CompileGlobalMap, "GLOBALMAP" },
        { ExportMAPINFO, CompileMAPINFO, "MAPINFO" },
		{ ExportMODELS, CompileMODELS, "MODELS" },
		{ ExportMODELIND, CompileMODELIND, "MODELIND" },
		//{ ExportMPEG, CompileMPEG, "MPEG" }
    };
}

const FormatInfo& Formats::GetFormatInfo(int aType)
{
    return FormatsInternal::sFormats[aType];
}
