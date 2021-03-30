#include "C_Stream.h"
#include "ROMFST.h"
#include "C_DataPack.h"
#include "BinUtils.h"
#include "C_HashMap.h"
#include "C_Utils.h"
#include "CL_Log.h"
#include "C_Hash.h"
#include "C_MemBlock.h"
#include "C_TextReader.h"
#include "C_OS.h"
#include "DefsFile.h"

namespace FormatsInternal
{
    bool ExportDLLSIMPORTTAB(FSTContext* aCtx)
    {
        C_Stream& handleTab = aCtx->GetFileStream(ROMFST::DLLSIMPORTTAB_BIN);

        C_FilePath defaultDefsPath;

        if (aCtx->mDefsPath.length() > 0)
        {
            defaultDefsPath = aCtx->mDefsPath.c_str();
        }
        else
        {
            defaultDefsPath = C_FilePath(C_OS::GetInstance()->GetWorkingDirectory());
            defaultDefsPath.Combine("DLLSIMPORTTAB.def");
        }

        DefsFile defaultDefs;
        defaultDefs.Read(defaultDefsPath);

        DefsFile expDefs;
        expDefs.ReadBinaryAddresses(handleTab);

        for (DefsFile::Entry& e : defaultDefs.mEntries)
        {
            if (e.mName.length() > 0)
                expDefs.TryAnnotate(e.mAddr, e.mName.c_str());
        }

        C_FilePath opath;
        aCtx->FixFilePath("DLLSIMPORTTAB.def", opath);
        expDefs.Write(opath);

        aCtx->MarkFileHandled(ROMFST::DLLSIMPORTTAB_BIN);

        return true;
    }

    bool CompileDLLSIMPORTTAB(FSTContext* aCtx)
    {
        C_FilePath ipath;
        aCtx->FixFilePath("DLLSIMPORTTAB.def", ipath);

        DefsFile defs;
        if (!defs.Read(ipath))
            return false;

        C_Stream& handle = aCtx->GetFileStream(ROMFST::DLLSIMPORTTAB_BIN);

        defs.WriteBinaryAddresses(handle);

        aCtx->MarkFileHandled(ROMFST::DLLSIMPORTTAB_BIN);

        return true;
    }
}