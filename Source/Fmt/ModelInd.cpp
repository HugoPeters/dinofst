#include "C_Stream.h"
#include "ROMFST.h"
#include "C_DataPack.h"

namespace FormatsInternal
{
    bool ExportMODELIND(FSTContext* aCtx)
    {
        C_Stream& handle = aCtx->GetFileStream(ROMFST::MAPINFO_BIN);

        C_DataPack pack;
        int id = 0;

        while (handle.GetRemaining() > 0)
        {
            uint16 entry;
            handle >> entry;

			if (entry == 0xFFFF)
				break;

            pack.Set(id++, entry);
        }

        aCtx->WriteJson(pack, "MAPINFO.json");
        aCtx->MarkFileHandled(ROMFST::MAPINFO_BIN);

        return true;
    }

    bool CompileMODELIND(FSTContext* aCtx)
    {
        C_DataPack pack;

        if (!aCtx->ReadJson(pack, "MODELIND.json"))
            return false;

		C_Stream& handle = aCtx->GetFileStream(ROMFST::MODELIND_BIN);

        for (int i = 0; i < pack.NumEntries(); ++i)
        {
			uint16 entry;
            pack.Get(i, entry);

			handle << entry;
        }

		handle << uint16(0xFFFF);

        aCtx->MarkFileHandled(ROMFST::MODELIND_BIN);

        return true;
    }
}