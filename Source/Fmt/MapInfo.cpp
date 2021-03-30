#include "C_Stream.h"
#include "ROMFST.h"
#include "C_DataPack.h"

namespace FormatsInternal
{
    struct MapInfoEntry
    {
        string mName;
        uint8 mMapType = 0;
        uint8 mUnk0 = 0x45;
        uint16 mUnk1 = 0;

        void Read(C_Stream& handle)
        {
            char nameBuff[29] = { 0 };
            handle.ReadBytes(nameBuff, 28);
            mName = nameBuff;
            handle >> mMapType;
            handle >> mUnk0;
            handle >> mUnk1;
        }

        void Write(C_Stream& handle) const
        {
            char nameBuff[29] = { 0 };
            strncpy(nameBuff, mName.c_str(), sizeof(nameBuff));
            handle.WriteBytes(nameBuff, 28);
            handle << mMapType;
            handle << mUnk0;
            handle << mUnk1;
        }

        void Pack(C_DataPack& pack) const
        {
            pack.Set("Name", mName);
            pack.Set("Type", mMapType);
            pack.Set("Unk0", mUnk0);
            pack.Set("Unk1", mUnk1);
        }

        void Unpack(const C_DataPack& pack)
        {
            pack.Get("Name", mName);
            pack.Get("Type", mMapType);
            pack.Get("Unk0", mUnk0);
            pack.Get("Unk1", mUnk1);
        }
    };

    bool ExportMAPINFO(FSTContext* aCtx)
    {
        C_Stream& handle = aCtx->GetFileStream(ROMFST::MAPINFO_BIN);

        C_DataPack pack;
        int id = 0;

        while (handle.GetRemaining() > 0)
        {
            MapInfoEntry entry;
            handle >> entry;

            C_DataPack packEntry;
            entry.Pack(packEntry);
            pack.Set(id++, packEntry);
        }

        aCtx->WriteJson(pack, "MAPINFO.json");
        aCtx->MarkFileHandled(ROMFST::MAPINFO_BIN);

        return true;
    }

    bool CompileMAPINFO(FSTContext* aCtx)
    {
        C_DataPack pack;

        if (!aCtx->ReadJson(pack, "MAPINFO.json"))
            return false;

        C_Vector<MapInfoEntry> entries;
        for (int i = 0; i < pack.NumEntries(); ++i)
        {
            MapInfoEntry& mapEntry = entries.Add();

            C_DataPack packEntry;
            pack.Get(i, packEntry);
            mapEntry.Unpack(packEntry);
        }

        C_Stream& handle = aCtx->GetFileStream(ROMFST::MAPINFO_BIN);

        for (MapInfoEntry& entry : entries)
        {
            entry.Write(handle);
        }

        aCtx->MarkFileHandled(ROMFST::MAPINFO_BIN);

        return true;
    }
}