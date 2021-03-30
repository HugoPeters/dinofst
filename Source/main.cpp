#include "C_CoreModule.h"
#include "C_CommandLine.h"
#include "ROMFST.h"
#include "DLLCompiler.h"

struct CommandArgs
{
    bool Parse()
    {
        bool needsRomPath = false;
        bool needsOutPath = false;
        bool needsInPath = false;
        bool needsDefsPath = false;

        C_CommandLine* cl = C_CommandLine::GetInstance();

        if (cl->HasSwitch("dump_bin"))
        {
            mMode = MODE_DUMP_BIN;
            needsOutPath = true;
            needsRomPath = true;
        }
        else if (cl->HasSwitch("dump_files"))
        {
            mMode = MODE_DUMP_FILES;
            needsOutPath = true;
            needsRomPath = true;
        }
        else if (cl->HasSwitch("extract_files"))
        {
            mMode = MODE_EXTRACT_FILES;
            needsOutPath = true;
            needsRomPath = true;
        }
        else if (cl->HasSwitch("compile_bin"))
        {
            mMode = MODE_COMPILE_BIN;
            needsInPath = true;
            needsOutPath = true;
        }
        else if (cl->HasSwitch("compile_files"))
        {
            mMode = MODE_COMPILE_FILES;
            needsInPath = true;
            needsOutPath = true;
        }
        else if (cl->HasSwitch("compile_rom"))
        {
            mMode = MODE_COMPILE_ROM;
            needsInPath = true;
            needsRomPath = true;
            needsOutPath = true;
        }
        else if (cl->HasSwitch("elf2dll"))
        {
            mMode = MODE_ELF2DLL;
            needsInPath = true;
            needsOutPath = true;
            needsDefsPath = true;
        }
        else
        {
            WAR_LOG_ERROR(CAT_GENERAL, "No (valid) mode specified");
            return false;
        }

        if (needsRomPath)
        {
            if (!cl->GetValue("rom", mRomPath))
            {
                WAR_LOG_ERROR(CAT_GENERAL, "No -rom path specified");
                return false;
            }
        }

        if (needsOutPath)
        {
            if (!cl->GetValue("o", mOutPath))
            {
                WAR_LOG_ERROR(CAT_GENERAL, "No -o out path specified");
                return false;
            }
        }

        if (needsInPath)
        {
            if (!cl->GetValue("i", mInPath))
            {
                WAR_LOG_ERROR(CAT_GENERAL, "No -i in path specified");
                return false;
            }
        }

        // optional
        const bool hasDefs = cl->GetValue("defs", mDefsPath);

        if (needsDefsPath)
        {
            if (!hasDefs)
            {
                WAR_LOG_ERROR(CAT_GENERAL, "No -defs defs file specified");
                return false;
            }
        }

        return true;
    }

    enum Mode
    {
        // ROM -> fst.bin
        MODE_DUMP_BIN,

        // ROM -> DLLS.bin, MODELS.bin, etc.
        MODE_DUMP_FILES,

        // ROM -> split DLLS, GLOBALMAP.json, etc.
        MODE_EXTRACT_FILES,

        // split files -> fst.bin
        MODE_COMPILE_BIN,

        // split files -> DLLS.bin, MODELS.bin, etc.
        MODE_COMPILE_FILES,

        // split files -> ROM
        MODE_COMPILE_ROM,

        // make a .dll from a .elf
        MODE_ELF2DLL,
    };


    Mode mMode = MODE_DUMP_BIN;
    string mRomPath;
    string mOutPath;
    string mInPath;
    string mDefsPath;
};

// minimal runtime
int main(int argc, char** argv)
{
    C_ModuleLoadParams params;
    params.mArgC = argc;
    params.mArgV = argv;
    params.mCmdOptions.Add("q");
    params.mCmdOptions.Add("nopaths");

    C_ModuleManager::GetInstance()->RegisterModule<C_CorePlatformModule>();
    C_ModuleManager::GetInstance()->SetLoadParams(params);
    C_ModuleManager::GetInstance()->AutoLoadModules();
    WAR_CHECK(C_CorePlatformModule::IsLoadedStatic());

    CommandArgs args;

    if (!args.Parse())
    {
        WAR_LOG_INFO(CAT_GENERAL, "Printing usage\n\n");

        string help;
        help.append("usage: dpfst [mode] [options]\n");
        help.append("Modes:\n");
        help.append("-dump_bin: extract raw fst.bin from rom. options:\n");
        help.append("  -rom <path>: the path to the rom\n");
        help.append("  -o <path>: the output path to your fst.bin\n");
        help.append("-dump_files: extract raw files from rom to given directory. options:\n");
        help.append("  -rom <path>: the path to the rom\n");
        help.append("  -o <dir path>: the output directory\n");
        help.append("-extract_files: extract files into intermediate formats to given directory. options:\n");
        help.append("  -rom <path>: the path to the rom\n");
        help.append("  -o <dir path>: the output directory\n");
        help.append("\n");
        help.append("-compile_rom: compile new FST and build new rom. options:\n");
        help.append("  -i <dir path>: the input dir to the extracted fst\n");
        help.append("  -rom <path>: the path to the base rom\n");
        help.append("  -o <path>: the path to the output rom\n");
        help.append("\n");
        help.append("-elf2dll: converts a .ELF into a DP compatible .DLL. options:\n");
        help.append("  -i <path>: the input .elf\n");
        help.append("  -o <path>: the output .dll\n");
        help.append("  -defs <path>: path to DLLSIMPORTTAB.def (get it by extracting the FST), which is used for resolving symbols\n");

        printf(help.c_str());
        return -1;
    }

    switch (args.mMode)
    {
        case CommandArgs::MODE_DUMP_BIN:
        {
            ROMFST::DumpBin(args.mRomPath.c_str(), args.mOutPath.c_str());
            break;
        }

        case CommandArgs::MODE_DUMP_FILES:
        {
            ROMFST::DumpFiles(args.mRomPath.c_str(), args.mOutPath.c_str());
            break;
        }

        case CommandArgs::MODE_EXTRACT_FILES:
        {
            ROMFST::ExtractFiles(args.mRomPath.c_str(), args.mOutPath.c_str(), args.mDefsPath.c_str());
            break;
        }

        case CommandArgs::MODE_COMPILE_FILES:
        {
            ROMFST::CompileFiles(args.mInPath.c_str(), args.mOutPath.c_str());
            break;
        }

        case CommandArgs::MODE_COMPILE_ROM:
        {
            ROMFST::CompileROM(args.mRomPath.c_str(), args.mInPath.c_str(), args.mOutPath.c_str());
            break;
        }

        case CommandArgs::MODE_ELF2DLL:
        {
            DLLCompiler::ConvertELFtoDLL(args.mInPath.c_str(), args.mOutPath.c_str(), args.mDefsPath.c_str());
            break;
        }
    }

    return 0;
}