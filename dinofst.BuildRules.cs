using WarBuild.Core;
using WarBuild.ModuleSystem;
using WarBuild.BuildSystem;

public class dinofst : BuildRules
{
	public override void Register(BuildRulesConfig Config)
	{
		BuildType = BuildOutputTypes.Executable;
        IgnoreAllWarnings = true;
		DependencyModules.Add("FW_Core");
        IncludePaths.Add("Source/ELFIO");

        // for xdelta
        Defines.AddRange(new string[]
        {
            "WIN32",
            "XD3_MAIN=0",
            "XD3_DEBUG=0",
            "XD3_USE_LARGEFILE64=1",
            "REGRESSION_TEST=0",
            "SECONDARY_DJW=1",
            "SECONDARY_FGK=1",
            "XD3_WIN32=1",
            "EXTERNAL_COMPRESSION=0",
            "SHELL_TESTS=0",
        });
    }
}
