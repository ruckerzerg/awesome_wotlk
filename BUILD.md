# Build (Windows)

Requires Visual Studio 2026 Community with:
- MSVC build tools for x64/x86
- C++ ATL for x64/x86
- C++ CMake tools for Windows
- Windows 11 SDK

Open **x86 Native Tools Command Prompt for VS 2026** (must be x86, not x64), then:

```
cd /d C:\awesome_wotlk
cmake -S . -B build -A Win32 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output: `build\Release\AwesomeWotlkLib.dll`

Copy it over `AwesomeWotlkLib.dll` in your WoW folder, and copy `addons\AwesomeCVar\` over `<WoW>\Interface\AddOns\AwesomeCVar\`.
