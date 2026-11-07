# Build tutorial

## Requirements

- Windows 10 or 11, x64
- Visual Studio 2022 with:
  - Desktop development with C++
  - .NET desktop development
  - .NET Framework 4.7.2 targeting pack
  - Windows 10 or 11 SDK
- MSVC v143 toolset for `SkidBase`
- MSVC v145 toolset for `skidjector`. If you only have VS2022, retarget that project to v143 (see Common problems)

## Projects and what they produce

| Project | Type | Output | Location |
| --- | --- | --- | --- |
| SkidBase | Dynamic library (DLL) | skid.dll | SkidBase\bin\skid.dll |
| UI | Windows application (WinExe) | UI.exe | UI\bin\Release\UI.exe |
| skidjector | Application | skidjector.exe | skidjector\Build\Bin\ |

The UI is a Windows Forms application. It is an EXE, not a DLL. It is not built as a DLL and there is no configuration that makes it one.

## Build everything in Visual Studio

1. Open `SkidBase.sln`
2. Set the configuration dropdowns to `Release` and `x64`
3. `Build` then `Build Solution` (Ctrl+Shift+B)

That builds `skid.dll` and `UI.exe`.

`skidjector` is not part of the solution. Open `skidjector\skidjector.vcxproj` directly, set `Release` and `x64`, then build. The exe lands in `skidjector\Build\Bin\`.

## Build from the command line

```
msbuild SkidBase.sln /m /p:Configuration=Release /p:Platform=x64
msbuild skidjector\skidjector.vcxproj /m /p:Configuration=Release /p:Platform=x64
```

Build only the UI:

```
msbuild UI\UI.csproj /p:Configuration=Release /p:Platform=AnyCPU
```

Build only the DLL:

```
msbuild SkidBase\SkidBase.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Common problems

**Someone says the UI builds as a DLL.** It does not. `UI\UI.csproj` has `OutputType` set to `WinExe`. If you end up with `UI.dll`, you built the wrong project, or you are looking at the `SkidBase` output, which is a DLL by design (`ConfigurationType` is `DynamicLibrary`). Rebuild the `UI` project and check `UI\bin\Release\UI.exe`.

**The solution does not offer Any CPU.** Use the solution's `Release|x64` configuration. The UI is mapped to `Release|Any CPU` inside the solution, so it still builds as AnyCPU.

**skidjector fails with a toolset error.** Its platform toolset is v145, which needs a newer Visual Studio. Right click the project, `Retarget Projects`, pick v143, or change `PlatformToolset` to v143 in `skidjector\skidjector.vcxproj`.

**The UI cannot find the injector.** `UI.exe` looks for `skidjector.exe` in its own folder, then `injector.exe`. Keep them together.

**The injector cannot load the module.** `skidjector.exe` loads `skid.dll` from the folder it sits in. Keep them together.

## Final layout

Put these three files in one folder:

```
UI.exe
skidjector.exe
skid.dll
```

Run `UI.exe`, click `Inject`, then write a script and click `Execute`.

## Notes

- `Execute` sends the script to `127.0.0.1:9002`. That server only exists after `skid.dll` is injected and the game has fully loaded, so execute after injecting and after the game is in.
- Port 9002 must not be used by anything else.
- GitHub Actions builds the solution and uploads only `SkidBase/bin/skid.dll` as the `skid-dll` artifact.
