#ifndef _DefsFile_h_
#define _DefsFile_h_

#include "C_Vector.h"

class C_Stream;

struct DefsFile
{
    struct Entry
    {
        uint32 mAddr;
        uint32 mNameHash = 0;
        string mName;
    };

    bool Read(const char* fpath);
    Entry* FindEntry(uint32 aAddr);
    Entry& GetOrInsert(uint32 aAddr);
    void ReadBinaryAddresses(C_Stream& handle);
    void WriteBinaryAddresses(C_Stream& handle);
    void TryAnnotate(uint32 aAddr, const char* aDesc);
    void Write(const char* fpath);
    int FindByName(const char* aSym);

    C_Vector<Entry> mEntries;
};

#endif // _DefsFile_h_
