#include "DLLCompiler.h"
#include "CL_Log.h"
#include "C_Vector.h"
#include "C_Hash.h"
#include "C_Stream.h"
#include "C_FilePath.h"
#include "C_MemBlock.h"
#include "C_Utils.h"

#include "mips_def.h"
#include "DefsFile.h"
#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"
#include "elfio/elfio_symbols.hpp"
#include "elfio/elf_types.hpp"

namespace DLLCompiler_private
{
    struct MemoryHelper
    {
        struct Section
        {
            string mName;
            uint32 mVirtualAddr = 0;
            uint32 mPhysicalAddr = 0;
            uint32 mSize = 0;
            const char* mData = NULL;
        };

        void AllocSections(int aNum)
        {
            mSections.Resize(aNum);
        }

        void CreateSection(int aId, uint32 aVirtualAddress, uint32 aPhysicalAddress, uint32 aSize)
        {
            Section& sec = mSections[aId];
            sec.mVirtualAddr = aVirtualAddress;
            sec.mPhysicalAddr = aPhysicalAddress;
            sec.mSize = aSize;
            WAR_CHECK(sec.mVirtualAddr != 0);
        }

        void CreateSection(int aId, const ELFIO::section* aSec)
        {
            Section& sec = mSections[aId];
            sec.mVirtualAddr = aSec->get_address();
            sec.mPhysicalAddr = aSec->get_offset();
            sec.mSize = aSec->get_size();
            sec.mData = aSec->get_data();
            sec.mName = aSec->get_name();
            WAR_CHECK(sec.mVirtualAddr != 0);
        }

        void CreateSection(int aId, const Section& aSec)
        {
            mSections[aId] = aSec;
            WAR_CHECK(aSec.mVirtualAddr != 0);
        }

        void RelocateSection(int aId, uint32 aNewPhysicalAddress)
        {
            mSections[aId].mPhysicalAddr = aNewPhysicalAddress;
        }

        bool IsValidPointer(uint32 aAddr) const
        {
            if (aAddr == 0)
                return false;

            for (const Section& sec : mSections)
            {
                if (aAddr >= sec.mVirtualAddr
                    && aAddr <= sec.mVirtualAddr + sec.mSize)
                    return true;
            }

            return false;
        }

        uint32 MakeRelativeOffset(int aNewSection, uint32 aAddr) const
        {
            const uint32 fileOffset = VirtualToPhysicalAddress(aAddr);
            WAR_CHECK(fileOffset > 0);

            return MakeRelativeFileOffset(aNewSection, fileOffset);
        }

        uint32 MakeRelativeFileOffset(int aNewSection, uint32 aFileOffset, bool aCanFail = false) const
        {
            const uint32 baseOffset = GetPhysicalAddress(aNewSection);

            if (!aCanFail)
                WAR_ASSERT(aFileOffset >= baseOffset, "section %i can not be used as base for offs 0x%08X", aNewSection, aFileOffset);
            else if (aFileOffset < baseOffset)
                return uint32(-1);

            return aFileOffset - baseOffset;
        }

        const Section* FindVirtualSection(uint32 aAddr) const
        {
            for (const Section& sec : mSections)
            {
                if (aAddr >= sec.mVirtualAddr
                    && aAddr <= sec.mVirtualAddr + sec.mSize)
                {
                    return &sec;
                }
            }

            return NULL;
        }

        uint32 VirtualToPhysicalAddress(uint32 aAddr) const
        {
            const Section* sec = FindVirtualSection(aAddr);
            WAR_CHECK(sec);

            uint32 offsInSec = aAddr - sec->mVirtualAddr;
            uint32 fileOffset = sec->mPhysicalAddr + offsInSec;
            return fileOffset;
        }

        const char* GetAddressData(uint32 aAddr) const
        {
            const Section* sec = FindVirtualSection(aAddr);
            WAR_CHECK(sec);
            uint32 addr = VirtualToPhysicalAddress(aAddr);
            return sec->mData + (addr - sec->mPhysicalAddr);
        }

        uint32 GetVirtualAddress(int aId) const { return mSections[aId].mVirtualAddr; }
        uint32 GetPhysicalAddress(int aId) const { return mSections[aId].mPhysicalAddr; }
        int32 GetHdrPhysicalAddress(int aId) const { return mSections[aId].mPhysicalAddr > 0 ? mSections[aId].mPhysicalAddr : -1; }

        C_Vector<Section> mSections;
    };

    struct AssemblyPatcher
    {
        AssemblyPatcher(void* aData, int aSize)
        {
            mNumInstr = aSize / 4;
            mInstructions = (uint32*)aData;
            LoadInstr();
        }

        ~AssemblyPatcher()
        {
            WriteInstr();
        }

        bool Next()
        {
            WAR_CHECK(mCurInstr < mNumInstr);

            WriteInstr();

            mIsReadDirty = false;
            ++mCurInstr;

            if (mCurInstr >= mNumInstr)
                return false;

            mIsReadDirty = true;
            LoadInstr();
        }

        int GetOp()
        {
            return mHandled ? mOp : -1;
        }

        void SetOp(int aOp)
        {
            mOp = aOp;
            mIsWriteDirty = true;
        }

        int GetRs()
        {
            return mRs;
        }

        void SetRs(int aRs)
        {
            mRs = aRs;
            mIsWriteDirty = true;
        }

        int GetRt()
        {
            return mRt;
        }

        void SetRt(int aRt)
        {
            mRt = aRt;
            mIsWriteDirty = true;
        }

        int GetAddr()
        {
            return mAddr;
        }

        void SetAddr(int aAddr)
        {
            mAddr = aAddr;
            mIsWriteDirty = true;
        }

        void SetInstrI(int aOp, int aRs, int aRt, int aAddr)
        {
            mOp = aOp;
            mRs = aRs;
            mRt = aRt;
            mAddr = aAddr;
            mIsWriteDirty = true;
        }

        void SetInstrIAdv(int aOp, int aRs, int aRt, int aAddr)
        {
            SetInstrI(aOp, aRs, aRt, aAddr);
            Next();
        }

        void SetNopAdv()
        {
            mIsNop = true;
            mIsWriteDirty = true;
            Next();
        }

    private:
        void LoadInstr()
        {
            if (!mIsReadDirty)
                return;

            mIsReadDirty = false;
            mHandled = false;

            uint32 instr = WAR_BYTESWAP_UINT32(mInstructions[mCurInstr]);
            mIsNop = instr == 0;

            if (!mIsNop)
            {
                mOp = (instr >> 26) & 0x3F;

                switch (mOp)
                {
                    // only care about I-types for now rip
                    // I-type
                    case OP_addi:
                    case OP_addiu:
                    case OP_andi:
                    case OP_beq:
                    case OP_bne:
                    case OP_lb:
                    case OP_lbu:
                    case OP_lh:
                    case OP_lhu:
                    case OP_lui:
                    case OP_lw:
                    case OP_ori:
                    case OP_slti:
                    case OP_sltiu:
                    case OP_sb:
                    case OP_sh:
                    case OP_sw:
                    {
                        mRs = (instr >> 21) & 0x1F;
                        mRt = (instr >> 16) & 0x1F;
                        mAddr = (instr >> 0) & 0xFFFF;
                        mHandled = true;
                        break;
                    }
                }
            }
        }

        void WriteInstr()
        {
            if (!mIsWriteDirty)
                return;

            mIsWriteDirty = false;

            if (mIsNop)
            {
                mInstructions[mCurInstr] = 0;
            }
            else
            {
                uint32 instr = 0;

                switch (mOp)
                {
                    // only care about I-types for now rip
                    // I-type
                    case OP_addi:
                    case OP_addiu:
                    case OP_andi:
                    case OP_beq:
                    case OP_bne:
                    case OP_lb:
                    case OP_lbu:
                    case OP_lh:
                    case OP_lhu:
                    case OP_lui:
                    case OP_lw:
                    case OP_ori:
                    case OP_slti:
                    case OP_sltiu:
                    case OP_sb:
                    case OP_sh:
                    case OP_sw:
                    {
                        instr |= (mOp << 26);
                        instr |= ((mRs & 0x1F) << 21);
                        instr |= ((mRt & 0x1F) << 16);
                        instr |= (mAddr & 0xFFFF);
                        mInstructions[mCurInstr] = WAR_BYTESWAP_UINT32(instr);
                        break;
                    }
                }
            }
        }

        uint32 mNumInstr = 0;
        uint32 mCurInstr = 0;
        uint32* mInstructions = 0;
        bool mIsReadDirty = true;
        bool mIsWriteDirty = false;

        int mOp = 0;
        int mRs = 0;
        int mRt = 0;
        int mAddr = 0;
        bool mIsNop = false;
        bool mHandled = false;
    };

    struct ElfSymbol
    {
        std::string         name;
        ELFIO::Elf64_Addr   value;
        ELFIO::Elf_Xword    size;
        unsigned char       bind;
        unsigned char       type;
        ELFIO::Elf_Half     section_index;
        unsigned char       other;
    };

    void ExtractSymbols(const ELFIO::elfio& elf, const ELFIO::section* sec, C_Vector<ElfSymbol>& aOut)
    {
        ELFIO::const_symbol_section_accessor symbols(elf, sec);

        aOut.Resize((int)symbols.get_symbols_num());

        for (int i = 0; i < symbols.get_symbols_num(); ++i)
        {
            ElfSymbol& sym = aOut[i];
            symbols.get_symbol(i, sym.name, sym.value, sym.size, sym.bind, sym.type, sym.section_index, sym.other);
        }
    }

    const ELFIO::segment* FindSegmentFromSectionId(const ELFIO::elfio& elf, ELFIO::Elf_Half aId)
    {
        if (aId == SHN_XINDEX)
        {
            WAR_LOG_ERROR(CAT_GENERAL, "SHN_XINDEX not supported");
            return NULL;
        }

        uint8 sectionIndex = aId & 0xFF;

        for (int i = 0; i < elf.segments.size(); ++i)
        {
            const ELFIO::segment* seg = elf.segments[i];

            for (int j = 0; j < seg->get_sections_num(); ++j)
            {
                if (seg->get_section_index_at(j) == aId)
                    return seg;
            }
        }

        return NULL;
    }

    enum FunctionType
    {
        FUNC_ONLOAD,
        FUNC_ONUNLOAD,

        FUNC_USER
    };

    struct ReservedFunctionInfo
    {
        ReservedFunctionInfo(const char* aName, bool aRequired)
        {
            mName = aName;
            mNameHash = C_Hash(mName);
            mIsRequired = aRequired;
        }

        string mName;
        uint32 mNameHash;
        bool mIsRequired;
    };

    static const ReservedFunctionInfo sReservedFuncs[] =
    {
        { "onLoad",   true },
        { "onUnload", true },
    };

    const FunctionType GetFunctionType(const char* aSym)
    {
        uint32 symHash = C_Hash(aSym);

        for (int i = 0; i < WAR_ARRAY_SIZE(sReservedFuncs); ++i)
        {
            if (sReservedFuncs[i].mNameHash == symHash)
                return (FunctionType)i;
        }

        return FUNC_USER;
    }

    struct Func
    {
        ElfSymbol mSymbol;
        FunctionType mFuncType = FUNC_USER;
        uint32 mPhysicalAddress = 0;
        void* mData = NULL;
        bool mUses_GP_disp = false;

        bool Uses_GP_disp() const { return mUses_GP_disp; }
    };

    int FindFuncInTable(const C_Vector<Func>& aTable, int aType)
    {
        for (int i = 0; i < aTable.Count(); ++i)
            if (aTable[i].mFuncType == aType)
                return i;

        return -1;
    }

    bool ValidateFuncTable(const C_Vector<Func>& aTable)
    {
        for (int i = 0; i < WAR_ARRAY_SIZE(sReservedFuncs); ++i)
        {
            if (sReservedFuncs[i].mIsRequired)
            {
                if (-1 == FindFuncInTable(aTable, i))
                {
                    WAR_LOG_ERROR(CAT_GENERAL, "DLL does not contain required func: %s", sReservedFuncs[i].mName.c_str());
                    return false;
                }
            }
        }

        return true;
    }

    void BuildFunctionTable(const MemoryHelper& mem, const C_Vector<ElfSymbol>& aSymbols, C_Vector<Func>& aOut)
    {
        for (const ElfSymbol& sym : aSymbols)
        {
            if (sym.type != STT_FUNC)
                continue;

            if (sym.size == 0 || sym.bind == STB_WEAK || sym.section_index == SHN_UNDEF)
                continue;

            Func& func = aOut.Add();
            func.mSymbol = sym;
            func.mFuncType = GetFunctionType(func.mSymbol.name.c_str());
            func.mPhysicalAddress = mem.VirtualToPhysicalAddress(func.mSymbol.value);
            func.mData = (void*)mem.GetAddressData(func.mSymbol.value);
            WAR_CHECK(func.mData);

            if (func.mSymbol.size > 4)
            {
                AssemblyPatcher rdr(func.mData, func.mSymbol.size);

                if (rdr.GetOp() == OP_lui
                    && rdr.GetRt() == R_gp
                    && rdr.GetAddr() == 0)
                {
                    func.mUses_GP_disp = true;
                }
            }
        }
    }

    C_MemBlock* ScanMultiSections(ELFIO::elfio& elf, const char* aPrefix, MemoryHelper::Section& aOutMergedSection, C_Vector<MemoryHelper::Section>& aOutSections)
    {
        int numSections = 0;

        C_Vector<uint32> offsets;

        for (int i = 0; i < elf.sections.size(); ++i)
        {
            const ELFIO::section* sec = elf.sections[i];

            if (C_StringUtils::StartsWith(aPrefix, sec->get_name().c_str()))
            {
                MemoryHelper::Section& newSection = aOutSections.Add();
                newSection.mVirtualAddr = sec->get_address();
                newSection.mPhysicalAddr = sec->get_offset();
                newSection.mData = sec->get_data();
                newSection.mSize = sec->get_size();
                newSection.mName = sec->get_name();

                if (aOutMergedSection.mVirtualAddr == 0)
                {
                    aOutMergedSection = newSection;
                    offsets.Add(0);
                }
                else
                {
                    uint32 offs = sec->get_address() - aOutMergedSection.mVirtualAddr;
                    offsets.Add(offs);
                }

                ++numSections;
            }
            else
            {
                if (numSections > 0)
                    break;
            }
        }

        uint32 totalSize = 0;

        for (int i = 0; i < aOutSections.Count(); ++i)
        {
            totalSize = C_Max(totalSize, offsets[i] + aOutSections[i].mSize);
        }

        aOutMergedSection.mSize = totalSize;
        aOutMergedSection.mName = C_Strfmt<256>("%s_merged", aPrefix).GetBuffer();

        C_Ptr<C_MemBlock> newSecData = WAR_MemBlockAlloc(totalSize);
        C_MemoryStream strm(newSecData);

        for (int i = 0; i < aOutSections.Count(); ++i)
        {
            strm.Seek(C_FileSystem::SeekSet, offsets[i]);
            strm.WriteBytes(aOutSections[i].mData, aOutSections[i].mSize);
        }

        return newSecData.Detach();
    }

    class DLLFile
    {
    public:
        struct BinInfo;

        enum RelocateSectionType
        {
            RELSEC_TEXT,
            RELSEC_GOT,
            RELSEC_RODATA,
            RELSEC_DATA,
            RELSEC_BSS,

            RELSEC_NUM,
        };

        bool LoadFromElf(ELFIO::elfio& elf)
        {
            mMem.AllocSections(RELSEC_NUM);

            if (const ELFIO::section* sec = elf.sections[".text"])
            {
                mMem.CreateSection(RELSEC_TEXT, sec);
                mTEXT = sec->get_data();
                mTEXTSize = sec->get_size();
            }

            if (const ELFIO::section* sec = elf.sections[".bss"])
            {
                mMem.CreateSection(RELSEC_BSS, sec);
                mBSSSize = sec->get_size();
            }

            if (const ELFIO::section* sec = elf.sections[".got"])
            {
                mMem.CreateSection(RELSEC_GOT, sec);
                C_MemoryStream strm((void*)sec->get_data(), sec->get_size());
                strm.SetEndianSwap(true);
                for (int i = 0; i < sec->get_size() / 4; ++i)
                    mGOT.Add(strm.ReadUInt32());
            }

            MemoryHelper::Section rodataSec;
            mRODATAMergedSections = ScanMultiSections(elf, ".rodata", rodataSec, mRODATASections);
            if (mRODATASections.Count() > 0)
            {
                mMem.CreateSection(RELSEC_RODATA, rodataSec);
            }

            if (const ELFIO::section* sec = elf.sections[".data"])
            {
                mMem.CreateSection(RELSEC_DATA, sec);
                mDATA = sec->get_data();
                mDATASize = sec->get_size();
            }

            if (const ELFIO::section* sec = elf.sections[".dynamic"])
            {
                C_MemoryStream strm((void*)sec->get_data(), sec->get_size());
                strm.SetEndianSwap(true);
                for (int i = 0; i < sec->get_size() / 8; ++i)
                {
                    DynamicEntry& e = mDynamic.Add();
                    strm >> e.mTag >> e.mValue;
                }
            }

            if (const ELFIO::section* sec = elf.sections[".symtab"])
            {
                ExtractSymbols(elf, sec, mSymbols);
            }

            if (const ELFIO::section* sec = elf.sections[".dynsym"])
            {
                ExtractSymbols(elf, sec, mDynSymbols);
            }

            BuildFunctionTable(mMem, mSymbols, mFuncs);

            if (!ValidateFuncTable(mFuncs))
                return false;

            return true;
        }

        void Write(C_Stream& handle, DefsFile& defs)
        {
            BinInfo info;
            info.mFuncs.Resize(NumFuncs());
            info.mMem = mMem;

            // pass 1 reserve write
            WriteSections(handle, info);

            // resolve sections 
            PatchFunctions();
            ResolveGOT(info, defs);

            // pass 2 rewrite resolved sections
            handle.Seek(C_FileSystem::SeekSet, 0);
            WriteSections(handle, info);

            // custom, write BSS size here, and pick it up in the fst compiler
            handle << mBSSSize;
        }

        void WriteSections(C_Stream& handle, BinInfo& info)
        {
            WriteHeader(handle, info);
            WriteExports(handle, info);
            WriteTEXT(handle, info);
            WriteGOT(handle, info);
            WriteRODATA(handle, info);
            WriteDATA(handle, info);
            WriteBSS(handle, info);
        }

        void WriteHeader(C_Stream& handle, BinInfo& info)
        {
            handle << info.mMem.GetHdrPhysicalAddress(RELSEC_TEXT);
            handle << info.mMem.GetHdrPhysicalAddress(RELSEC_DATA);
            handle << info.mMem.GetHdrPhysicalAddress(RELSEC_GOT);
            handle << int16(NumUserExportFuncs());
            handle << int16(0);
        }

        void WriteExports(C_Stream& handle, BinInfo& info)
        {
            handle << info.mMem.MakeRelativeFileOffset(RELSEC_TEXT, info.GetOffsetCTOR(*this), true);
            handle << info.mMem.MakeRelativeFileOffset(RELSEC_TEXT, info.GetOffsetDTOR(*this), true);
            handle << uint32(0);

            for (int i = 0; i < mFuncs.Count(); ++i)
                if (IsUserExport(mFuncs[i]))
                    handle << info.mMem.MakeRelativeFileOffset(RELSEC_TEXT, info.GetSrcFuncOffs(i), true);

            handle << uint32(0);
        }

        void WriteTEXT(C_Stream& handle, BinInfo& info)
        {
            info.mMem.RelocateSection(RELSEC_TEXT, handle.GetPosition());
            info.mNumWrittenFuncs = 0;

            // cannot write individual functions because of PC-relative instructions like BAL, so TEXT has to match
            handle.WriteBytes(mTEXT, mTEXTSize);

            for (int i = 0; i < mFuncs.Count(); ++i)
            {
                const Func& func = mFuncs[i];

                BinFuncInfo& ofunc = info.mFuncs[info.mNumWrittenFuncs++];
                ofunc.mOffset = info.mMem.VirtualToPhysicalAddress(func.mSymbol.value);
                ofunc.mSrcFuncId = i;
            }
        }

        void WriteGOT(C_Stream& handle, BinInfo& info)
        {
            info.mMem.RelocateSection(RELSEC_GOT, handle.GetPosition());

            for (int i = 0; i < mGOT.Count(); ++i)
            {
                handle << mGOT[i];
            }

            handle << int32(-2);

            // $gp patch funcs
            for (int i = 0; i < info.mFuncs.Count(); ++i)
            {
                const Func& funcDef = mFuncs[info.mFuncs[i].mSrcFuncId];
                if (!funcDef.Uses_GP_disp())
                    continue;

                uint32 relOffset = info.mMem.MakeRelativeFileOffset(RELSEC_TEXT, info.mFuncs[i].mOffset);
                handle << relOffset;
            }

            handle << int32(-3);

            // TODO: determine pointers in data and make relative

            handle << int32(-1);
        }

        void WriteRODATA(C_Stream& handle, BinInfo& info)
        {
            if (mRODATASections.Count() == 0)
                return;

            info.mMem.RelocateSection(RELSEC_RODATA, handle.GetPosition());
            handle.WriteBytes(mRODATAMergedSections->mBlock, mRODATAMergedSections->mSize);
        }

        void WriteDATA(C_Stream& handle, BinInfo& info)
        {
            if (mDATASize == 0)
                return;

            info.mMem.RelocateSection(RELSEC_DATA, handle.GetPosition());
            handle.WriteBytes(mDATA, mDATASize);
        }

        void WriteBSS(C_Stream& handle, BinInfo& info)
        {
            if (mBSSSize == 0)
                return;

            info.mMem.RelocateSection(RELSEC_BSS, handle.GetPosition());
            // game will attach BSS at end of DLL at runtime
            // seems to still write a (S)BSS of 4 bytes into the main DLL? should we handle this?
        }

        void WriteFunction(C_Stream& handle, BinInfo& info, int idx)
        {
            const Func& func = mFuncs[idx];

            const bool printFuncName = false;

            if (printFuncName)
            {
                handle.WriteBytes(func.mSymbol.name.c_str(), func.mSymbol.name.length());
            }

            BinFuncInfo& ofunc = info.mFuncs[info.mNumWrittenFuncs++];
            ofunc.mOffset = handle.GetPosition();
            ofunc.mSrcFuncId = idx;

            handle.WriteBytes(func.mData, func.mSymbol.size);
        }

        void PatchFunctions()
        {
            for (Func& func : mFuncs)
            {
                if (!func.Uses_GP_disp())
                    continue;

                AssemblyPatcher patch(func.mData, func.mSymbol.size);
                patch.SetInstrIAdv(OP_lui,  0,      R_gp,  0);
                patch.SetInstrIAdv(OP_ori,  R_gp,   R_gp,  0);
                patch.SetNopAdv();
            }
        }

        void ResolveGOT(BinInfo& info, DefsFile& defs)
        {
            if (mGOT.Count() == 0)
                return;

            uint32 localgotno = GetDynamicValue(DT_MIPS_LOCAL_GOTNO);
            WAR_CHECK(localgotno > 0);

            uint32 symtabno = GetDynamicValue(DT_MIPS_SYMTABNO);
            uint32 gotsym = GetDynamicValue(DT_MIPS_GOTSYM);

            if (gotsym > 0)
            {
                // why can this happen?
                /*if (localgotno > gotsym)
                    localgotno = gotsym;*/

                WAR_CHECK(symtabno > gotsym);
            }

            int entry = 0;

            // reserved
            mGOT[entry++] = 0;

            // GOT[1] can have GNU module pointer with MSB set, we don't want this,
            // as the game uses MSB to import from DLLSIMPORTTAB
            uint32& vMP = mGOT[entry];

            if ((vMP >> 31) & 1)
                mGOT[entry++] = 0;

            // locals
            for (int i = 0; entry < localgotno; ++i, ++entry)
            {
                uint32& v = mGOT[entry];

                if (v == 0)
                    continue;

                if (info.mMem.IsValidPointer(v))
                {
                    v = FixGOTPointer(info, v);
                }
                else
                {
                    const char* secName = NULL;
                    const char* symName = NULL;

                    const MemoryHelper::Section* sec = mMem.FindVirtualSection(v);
                    secName = sec ? sec->mName.c_str() : "UNKSEC";

                    for (const ElfSymbol& sym : mSymbols)
                        if (sym.value == v)
                            symName = sym.name.c_str();

                    symName = (symName && *symName) ? symName : "UNKSYM";

                    // can happen with -O0 somehow, need to investigate, GOT seems to include symbols that are 0 size
                    WAR_LOG_WARNING(CAT_GENERAL, "Unknown GOT entry: 0x%08X (%s, %s)", v, secName, symName);
                }
            }

            // globals
            for (int i = 0; i < (symtabno - gotsym); ++i, ++entry)
            {
                if (entry >= mGOT.Count())
                    break;

                uint32& v = mGOT[entry];

                const ElfSymbol& sym = mDynSymbols[gotsym + i];

                if (sym.bind == STB_WEAK)
                {
                    int importId = defs.FindByName(sym.name.c_str());

                    if (importId == -1)
                    {
                        WAR_LOG_WARNING(CAT_GENERAL, "Import [%s] could not be resolved from DLLSIMPORTTAB defs!", sym.name.c_str());
                        v = 0x7000D;
                    }
                    else
                    {
                        importId += 1; // game does -4, but why
                        v = ((1U << 31) | importId);
                    }
                }
                else
                {
                    if (v == 0)
                        continue;

                    if (info.mMem.IsValidPointer(v))
                    {
                        v = FixGOTPointer(info, v);
                    }
                    else
                    {
                        WAR_LOG_WARNING(CAT_GENERAL, "Unknown GOT entry: 0x%08X (%s)", v, sym.name.c_str());
                    }
                }
            }
        }

        uint32 FixGOTPointer(BinInfo& info, uint32 v)
        {
            int funcId = FindFuncId(v);

            if (funcId != -1)
            {
                const uint32 funcOffset = info.GetSrcFuncOffs(funcId);
                return info.mMem.MakeRelativeFileOffset(RELSEC_TEXT, funcOffset);
            }

            return info.mMem.MakeRelativeOffset(RELSEC_TEXT, v);
        }

        int NumFuncs() const { return mFuncs.Count(); }
        
        int NumUserExportFuncs() const
        {
            int c = 0;
            for (const Func& f : mFuncs)
                if (IsUserExport(f))
                    ++c;

            return c;
        }

        uint32 GetBSSSize() const { return mBSSSize; }

    private:
        struct BinFuncInfo
        {
            int mSrcFuncId;
            uint32 mOffset;
        };

        struct BinInfo
        {
            MemoryHelper mMem;
            C_Vector<BinFuncInfo> mFuncs;
            int mNumWrittenFuncs = 0;

            uint32 FindFuncOffs(const DLLFile& f, int type) const
            {
                for (int i = 0; i < mFuncs.Count(); ++i)
                    if (f.mFuncs[mFuncs[i].mSrcFuncId].mFuncType == type)
                        return mFuncs[i].mOffset;

                return 0;
            }
            uint32 GetSrcFuncOffs(int aSrcFuncId) const
            {
                for (int i = 0; i < mFuncs.Count(); ++i)
                    if (mFuncs[i].mSrcFuncId == aSrcFuncId)
                        return mFuncs[i].mOffset;
                return 0;
            }
            uint32 GetOffsetCTOR(const DLLFile& f) const { return FindFuncOffs(f, FUNC_ONLOAD); }
            uint32 GetOffsetDTOR(const DLLFile& f) const { return FindFuncOffs(f, FUNC_ONUNLOAD); }
        };

        bool IsUserExport(const Func& f) const
        {
            return (f.mFuncType == FUNC_USER) && (f.mSymbol.bind == STB_GLOBAL);
        }

        uint32 GetDynamicValue(uint32 aTag) const
        {
            for (const DynamicEntry& e : mDynamic)
                if (e.mTag == aTag)
                    return e.mValue;
            return 0;
        }

        int FindFuncId(uint32 aAddr) const
        {
            for (int i = 0; i < mFuncs.Count(); ++i)
                if (aAddr == mFuncs[i].mSymbol.value)
                    return i;
            return -1;
        }

        struct DynamicEntry
        {
            uint32 mTag;
            uint32 mValue;
        };

        C_Vector<ElfSymbol> mSymbols;
        C_Vector<ElfSymbol> mDynSymbols;
        C_Vector<Func> mFuncs;
        C_Vector<uint32> mGOT;
        C_Vector<DynamicEntry> mDynamic;
        MemoryHelper mMem;
        uint32 mBSSSize = 0;
        C_Vector<MemoryHelper::Section> mRODATASections;
        C_Ptr<C_MemBlock> mRODATAMergedSections;
        const void* mDATA = NULL;
        uint32 mDATASize = 0;
        const void* mTEXT = NULL;
        uint32 mTEXTSize = 0;
    };
}

bool DLLCompiler::ConvertELFtoDLL(const char* aELFPath, const char* aDLLPath, const char* aDefsPath)
{
    using namespace DLLCompiler_private;

    ELFIO::elfio elf;
    if (!elf.load(aELFPath))
    {
        WAR_LOG_ERROR(CAT_GENERAL, "Failed to load ELF: %s", aELFPath);
        return false;
    }

    DLLFile dll;
    if (!dll.LoadFromElf(elf))
        return false;

    DefsFile defs;
    if (!defs.Read(aDefsPath))
        return false;

    C_FilePath ofname;
    C_PathUtils::GetFilenameWithoutExtension(aDLLPath, ofname);

    C_FilePath opath;
    C_PathUtils::GetDirectoryPath(aDLLPath, opath);

    opath.Combine(C_Strfmt<256>("%s.dll", ofname.GetBuffer()));

    C_FileHandle ohandle;
    if (!C_FileSystem::Open(ohandle, opath, C_FileSystem::FileWriteDiscard))
        return false;

    C_Stream ostrm(ohandle);
    ostrm.SetEndianSwap(true);

    dll.Write(ostrm, defs);

    return true;
}

