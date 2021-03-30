#include "C_Stream.h"
#include "ROMFST.h"
#include "C_DataPack.h"

namespace FormatsInternal
{
    struct GlobalMapEntry
    {
        int16 mCoordX = -1;
        int16 mCoordZ = -1;
        int16 mUnused = 0;
        int16 mMapIndex = -1;
        int16 mNum0 = -1;
        int16 mNum1 = -1;

        void Read(C_Stream& handle)
        {
            handle >> mCoordX;
            handle >> mCoordZ;
            handle >> mUnused;
            handle >> mMapIndex;
            handle >> mNum0;
            handle >> mNum1;
        }

        void Write(C_Stream& handle) const
        {
            handle << mCoordX;
            handle << mCoordZ;
            handle << mUnused;
            handle << mMapIndex;
            handle << mNum0;
            handle << mNum1;
        }

        void Pack(C_DataPack& pack) const
        {
            pack.Set("CoordX", mCoordX);
            pack.Set("CoordZ", mCoordZ);
            pack.Set("Unk0", mUnused);
            pack.Set("MapIndex", mMapIndex);
            pack.Set("Unk1", mNum0);
            pack.Set("Unk2", mNum1);
        }

        void Unpack(const C_DataPack& pack)
        {
            pack.Get("CoordX", mCoordX);
            pack.Get("CoordZ", mCoordZ);
            pack.Get("Unk0", mUnused);
            pack.Get("MapIndex", mMapIndex);
            pack.Get("Unk1", mNum0);
            pack.Get("Unk2", mNum1);
        }
    };

    bool ExportGlobalMap(FSTContext* aCtx)
    {
        C_Stream& handle = aCtx->GetFileStream(ROMFST::GLOBALMAP_BIN);

        C_DataPack pack;
        int id = 0;

        while (true)
        {
            GlobalMapEntry entry;
            handle >> entry;

            if (entry.mMapIndex == -1)
                break;

            C_DataPack packEntry;
            entry.Pack(packEntry);
            pack.Set(id++, packEntry);
        }

        aCtx->WriteJson(pack, "GLOBALMAP.json");
        aCtx->MarkFileHandled(ROMFST::GLOBALMAP_BIN);

        return true;
    }

    bool CompileGlobalMap(FSTContext* aCtx)
    {
        C_DataPack pack;

        if (!aCtx->ReadJson(pack, "GLOBALMAP.json"))
            return false;

        C_Vector<GlobalMapEntry> entries;
        for (int i = 0; i < pack.NumEntries(); ++i)
        {
            GlobalMapEntry& mapEntry = entries.Add();

            C_DataPack packEntry;
            pack.Get(i, packEntry);
            mapEntry.Unpack(packEntry);
        }

        C_Stream& handle = aCtx->GetFileStream(ROMFST::GLOBALMAP_BIN);

        for (GlobalMapEntry& entry : entries)
        {
            entry.Write(handle);
        }

        GlobalMapEntry eofEntry;
        handle << eofEntry;

        aCtx->MarkFileHandled(ROMFST::GLOBALMAP_BIN);

        return true;
    }
}