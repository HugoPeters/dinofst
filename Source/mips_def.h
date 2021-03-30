#ifndef _mips_def_h_
#define _mips_def_h_

// Dynamic tags found in the PT_DYNAMIC segment.

enum DT
{
    DT_NULL = 0,
    DT_NEEDED = 1,
    DT_PLTRELSZ = 2,
    DT_PLTGOT = 3,
    DT_HASH = 4,
    DT_STRTAB = 5,
    DT_SYMTAB = 6,
    DT_RELA = 7,
    DT_RELASZ = 8,
    DT_RELAENT = 9,
    DT_STRSZ = 10,
    DT_SYMENT = 11,
    DT_INIT = 12,
    DT_FINI = 13,
    DT_SONAME = 14,
    DT_RPATH = 15,
    DT_SYMBOLIC = 16,
    DT_REL = 17,
    DT_RELSZ = 18,
    DT_RELENT = 19,
    DT_PLTREL = 20,
    DT_DEBUG = 21,
    DT_TEXTREL = 22,
    DT_JMPREL = 23,
    DT_BIND_NOW = 24,
    DT_INIT_ARRAY = 25,
    DT_FINI_ARRAY = 26,
    DT_INIT_ARRAYSZ = 27,
    DT_FINI_ARRAYSZ = 28,
    DT_RUNPATH = 29,
    DT_FLAGS = 30,

    // This is used to mark a range of dynamic tags.  It is not really
    // a tag value.
    DT_ENCODING = 32,

    DT_PREINIT_ARRAY = 32,
    DT_PREINIT_ARRAYSZ = 33,
    DT_LOOS = 0x6000000d,
    DT_HIOS = 0x6ffff000,
    DT_LOPROC = 0x70000000,
    DT_HIPROC = 0x7fffffff,

    // The remaining values are extensions used by GNU or Solaris.
    DT_VALRNGLO = 0x6ffffd00,
    DT_GNU_PRELINKED = 0x6ffffdf5,
    DT_GNU_CONFLICTSZ = 0x6ffffdf6,
    DT_GNU_LIBLISTSZ = 0x6ffffdf7,
    DT_CHECKSUM = 0x6ffffdf8,
    DT_PLTPADSZ = 0x6ffffdf9,
    DT_MOVEENT = 0x6ffffdfa,
    DT_MOVESZ = 0x6ffffdfb,
    DT_FEATURE = 0x6ffffdfc,
    DT_POSFLAG_1 = 0x6ffffdfd,
    DT_SYMINSZ = 0x6ffffdfe,
    DT_SYMINENT = 0x6ffffdff,
    DT_VALRNGHI = 0x6ffffdff,

    DT_ADDRRNGLO = 0x6ffffe00,
    DT_GNU_HASH = 0x6ffffef5,
    DT_TLSDESC_PLT = 0x6ffffef6,
    DT_TLSDESC_GOT = 0x6ffffef7,
    DT_GNU_CONFLICT = 0x6ffffef8,
    DT_GNU_LIBLIST = 0x6ffffef9,
    DT_CONFIG = 0x6ffffefa,
    DT_DEPAUDIT = 0x6ffffefb,
    DT_AUDIT = 0x6ffffefc,
    DT_PLTPAD = 0x6ffffefd,
    DT_MOVETAB = 0x6ffffefe,
    DT_SYMINFO = 0x6ffffeff,
    DT_ADDRRNGHI = 0x6ffffeff,

    DT_RELACOUNT = 0x6ffffff9,
    DT_RELCOUNT = 0x6ffffffa,
    DT_FLAGS_1 = 0x6ffffffb,
    DT_VERDEF = 0x6ffffffc,
    DT_VERDEFNUM = 0x6ffffffd,
    DT_VERNEED = 0x6ffffffe,
    DT_VERNEEDNUM = 0x6fffffff,

    DT_VERSYM = 0x6ffffff0,

    // Specify the value of _GLOBAL_OFFSET_TABLE_.
    DT_PPC_GOT = 0x70000000,

    // Specify whether various optimisations are possible.
    DT_PPC_OPT = 0x70000001,

    // Specify the start of the .glink section.
    DT_PPC64_GLINK = 0x70000000,

    // Specify the start and size of the .opd section.
    DT_PPC64_OPD = 0x70000001,
    DT_PPC64_OPDSZ = 0x70000002,

    // Specify whether various optimisations are possible.
    DT_PPC64_OPT = 0x70000003,

    // The index of an STT_SPARC_REGISTER symbol within the DT_SYMTAB
    // symbol table.  One dynamic entry exists for every STT_SPARC_REGISTER
    // symbol in the symbol table.
    DT_SPARC_REGISTER = 0x70000001,

    // MIPS specific dynamic array tags.
    // 32 bit version number for runtime linker interface.
    DT_MIPS_RLD_VERSION = 0x70000001,
    // Time stamp.
    DT_MIPS_TIME_STAMP = 0x70000002,
    // Checksum of external strings and common sizes.
    DT_MIPS_ICHECKSUM = 0x70000003,
    // Index of version string in string table.
    DT_MIPS_IVERSION = 0x70000004,
    // 32 bits of flags.
    DT_MIPS_FLAGS = 0x70000005,
    // Base address of the segment.
    DT_MIPS_BASE_ADDRESS = 0x70000006,
    // ???
    DT_MIPS_MSYM = 0x70000007,
    // Address of .conflict section.
    DT_MIPS_CONFLICT = 0x70000008,
    // Address of .liblist section.
    DT_MIPS_LIBLIST = 0x70000009,
    // Number of local global offset table entries.
    DT_MIPS_LOCAL_GOTNO = 0x7000000a,
    // Number of entries in the .conflict section.
    DT_MIPS_CONFLICTNO = 0x7000000b,
    // Number of entries in the .liblist section.
    DT_MIPS_LIBLISTNO = 0x70000010,
    // Number of entries in the .dynsym section.
    DT_MIPS_SYMTABNO = 0x70000011,
    // Index of first external dynamic symbol not referenced locally.
    DT_MIPS_UNREFEXTNO = 0x70000012,
    // Index of first dynamic symbol in global offset table.
    DT_MIPS_GOTSYM = 0x70000013,
    // Number of page table entries in global offset table.
    DT_MIPS_HIPAGENO = 0x70000014,
    // Address of run time loader map, used for debugging.
    DT_MIPS_RLD_MAP = 0x70000016,
    // Delta C++ class definition.
    DT_MIPS_DELTA_CLASS = 0x70000017,
    // Number of entries in DT_MIPS_DELTA_CLASS.
    DT_MIPS_DELTA_CLASS_NO = 0x70000018,
    // Delta C++ class instances.
    DT_MIPS_DELTA_INSTANCE = 0x70000019,
    // Number of entries in DT_MIPS_DELTA_INSTANCE.
    DT_MIPS_DELTA_INSTANCE_NO = 0x7000001a,
    // Delta relocations.
    DT_MIPS_DELTA_RELOC = 0x7000001b,
    // Number of entries in DT_MIPS_DELTA_RELOC.
    DT_MIPS_DELTA_RELOC_NO = 0x7000001c,
    // Delta symbols that Delta relocations refer to.
    DT_MIPS_DELTA_SYM = 0x7000001d,
    // Number of entries in DT_MIPS_DELTA_SYM.
    DT_MIPS_DELTA_SYM_NO = 0x7000001e,
    // Delta symbols that hold class declarations.
    DT_MIPS_DELTA_CLASSSYM = 0x70000020,
    // Number of entries in DT_MIPS_DELTA_CLASSSYM.
    DT_MIPS_DELTA_CLASSSYM_NO = 0x70000021,
    // Flags indicating information about C++ flavor.
    DT_MIPS_CXX_FLAGS = 0x70000022,
    // Pixie information (???).
    DT_MIPS_PIXIE_INIT = 0x70000023,
    // Address of .MIPS.symlib
    DT_MIPS_SYMBOL_LIB = 0x70000024,
    // The GOT index of the first PTE for a segment
    DT_MIPS_LOCALPAGE_GOTIDX = 0x70000025,
    // The GOT index of the first PTE for a local symbol
    DT_MIPS_LOCAL_GOTIDX = 0x70000026,
    // The GOT index of the first PTE for a hidden symbol
    DT_MIPS_HIDDEN_GOTIDX = 0x70000027,
    // The GOT index of the first PTE for a protected symbol
    DT_MIPS_PROTECTED_GOTIDX = 0x70000028,
    // Address of `.MIPS.options'.
    DT_MIPS_OPTIONS = 0x70000029,
    // Address of `.interface'.
    DT_MIPS_INTERFACE = 0x7000002a,
    // ???
    DT_MIPS_DYNSTR_ALIGN = 0x7000002b,
    // Size of the .interface section.
    DT_MIPS_INTERFACE_SIZE = 0x7000002c,
    // Size of rld_text_resolve function stored in the GOT.
    DT_MIPS_RLD_TEXT_RESOLVE_ADDR = 0x7000002d,
    // Default suffix of DSO to be added by rld on dlopen() calls.
    DT_MIPS_PERF_SUFFIX = 0x7000002e,
    // Size of compact relocation section (O32).
    DT_MIPS_COMPACT_SIZE = 0x7000002f,
    // GP value for auxiliary GOTs.
    DT_MIPS_GP_VALUE = 0x70000030,
    // Address of auxiliary .dynamic.
    DT_MIPS_AUX_DYNAMIC = 0x70000031,
    // Address of the base of the PLTGOT.
    DT_MIPS_PLTGOT = 0x70000032,
    // Points to the base of a writable PLT.
    DT_MIPS_RWPLT = 0x70000034,
    // Relative offset of run time loader map, used for debugging.
    DT_MIPS_RLD_MAP_REL = 0x70000035,

    DT_AUXILIARY = 0x7ffffffd,
    DT_USED = 0x7ffffffe,
    DT_FILTER = 0x7fffffff
};

enum OpCodes
{
    OP_rtype = 0,
    OP_bltz = 1,
    OP_bgez = 1,
    OP_j = 2,
    OP_jal = 3,
    OP_beq = 4,
    OP_bne = 5,
    OP_blez = 6,
    OP_bgtz = 7,
    OP_addi = 8,
    OP_addiu = 9,
    OP_slti = 10,
    OP_sltiu = 11,
    OP_andi = 12,
    OP_ori = 13,
    OP_xori = 14,
    OP_lui = 15,
    OP_mfc0 = 16,
    OP_mtc0 = 16,
    OP_ftype = 17,
    OP_bclf = 17,
    OP_bclt = 17,
    OP_mul = 28,
    OP_lb = 32,
    OP_lh = 33,
    OP_lw = 35,
    OP_lbu = 36,
    OP_lhu = 37,
    OP_sb = 40,
    OP_sh = 41,
    OP_sw = 43,
    OP_lwcl = 49,
    OP_swcl = 56,
};

enum Registers
{
    R_zero = 0,
    R_at = 1,
    R_v0 = 2,
    R_v1 = 3,

    R_a0 = 4,
    R_a1 = 5,
    R_a2 = 6,
    R_a3 = 7,

    R_t0 = 8,
    R_t1 = 9,
    R_t2 = 10,
    R_t3 = 11,
    R_t4 = 12,
    R_t5 = 13,
    R_t6 = 14,
    R_t7 = 15,
    R_s0 = 16,
    R_s1 = 17,
    R_s2 = 18,
    R_s3 = 19,
    R_s4 = 20,
    R_s5 = 21,
    R_s6 = 22,
    R_s7 = 23,
    R_t8 = 24,
    R_t9 = 25,
    R_k0 = 26,
    R_k1 = 27,
    R_gp = 28,
    R_sp = 29,
    R_fp = 30,
    R_ra = 31
};

#endif // _mips_def_h_
