using BuildTool;
using BuildTool.BuildSystem;
using ModuleSystem;

public class dinofst : BuildRules
{
	public override void Register(BuildRulesConfig Config)
	{
		BuildType = BuildOutputTypes.Executable;
        IgnoreAllWarnings = true;
		DependencyModules.Add("FW_Core");
        IncludePaths.Add("Source/ELFIO");
    }
}
