#include "DefsFile.h"
#include "C_FileSystem.h"
#include "C_MemBlock.h"
#include "C_Stream.h"
#include "C_TextReader.h"
#include "CL_Log.h"
#include "C_Hash.h"

bool DefsFile::Read(const char* fpath)
{
    if (!C_FileSystem::Exists(fpath))
        return false;

    C_Ptr<C_MemBlock> defsFile = C_FileSystem::ReadFile(fpath);

    if (!defsFile)
        return false;
    C_MemoryStream defsStrm(defsFile);
    C_TextReader rdr(defsStrm);

    char strbuff[512];
    WAR_ZeroMem(strbuff);

    int lineId = 0;

    while (const char* line = rdr.ReadLine())
    {
        if (!line || strlen(line) == 0)
            break;

        uint32 offs;
        if (sscanf(line, "%08X\t%s", &offs, strbuff) != 2)
        {
            WAR_LOG_ERROR(CAT_GENERAL, "Failed to parse DLLSIMPORTTAB.def, error at line %i (%s)", lineId, line);
            return false;
        }
        ++lineId;

        bool hasName = false;
        int strbuffLen = strlen(strbuff);

        if (strbuffLen > 0 && strbuff[0] != '?')
            hasName = true;

        Entry& e = mEntries.Add();
        e.mAddr = offs;
        e.mName = hasName ? strbuff : "";
        e.mNameHash = C_Hash(e.mName.c_str());
    }

    return true;
}

DefsFile::Entry* DefsFile::FindEntry(uint32 aAddr)
{
    for (Entry& e : mEntries)
        if (e.mAddr == aAddr)
            return &e;
    return NULL;
}

DefsFile::Entry& DefsFile::GetOrInsert(uint32 aAddr)
{
    Entry* existingEntry = FindEntry(aAddr);

    if (!existingEntry)
    {
        Entry& e = mEntries.Add();
        e.mAddr = aAddr;
        return e;
    }

    return *existingEntry;
}

void DefsFile::ReadBinaryAddresses(C_Stream& handle)
{
    mEntries.Clear();

    while (handle.GetRemaining() >= 4)
    {
        uint32 offs;
        handle >> offs;
        Entry& e = mEntries.Add();
        e.mAddr = offs;
    }
}

void DefsFile::WriteBinaryAddresses(C_Stream& handle)
{
    for (Entry& e : mEntries)
    {
        handle << e.mAddr;
    }
}

void DefsFile::TryAnnotate(uint32 aAddr, const char* aDesc)
{
    if (Entry* e = FindEntry(aAddr))
    {
        e->mName = aDesc;
        e->mNameHash = C_Hash(aDesc);
    }
}

void DefsFile::Write(const char* fpath)
{
    C_String defs;

    for (Entry& e : mEntries)
    {
        C_Strfmt<512> line("%08X\t%s\n", e.mAddr, e.mName.length() > 0 ? e.mName.c_str() : "?");
        defs.append(line);
    }

    C_FileSystem::WriteFile(fpath, (void*)defs.data(), defs.size()); //fixme, writefile should take const data
}

int DefsFile::FindByName(const char* aSym)
{
    uint32 hash = C_Hash(aSym);

    for (int i = 0; i < mEntries.Count(); ++i)
        if (mEntries[i].mNameHash == hash)
            return i;

    return -1;
}
