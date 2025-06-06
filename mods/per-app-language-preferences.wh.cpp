// ==WindhawkMod==
// @id              per-app-language-preferences
// @name            Per-app Language Preferences
// @description     Override the preferred UI language for specific apps.
// @version         1.2
// @author          yezhiyi9670
// @github          https://github.com/yezhiyi9670
// @include         *
// @architecture    x86
// @architecture    x86-64
// @compilerOptions -lShlwapi
// ==/WindhawkMod==

// Source code is published under The GNU General Public License v3.0.

// ==WindhawkModReadme==
/*
# Per-app Language Preferences

In Windows 11, it is no longer possible to set display languages for specific apps. This mod allow you to override the preferred language for specific apps using the mod's settings.

**Currently, the approach only works for some apps, not all. Specifically, it does not seem to work for some UWP apps. See [#1510](https://github.com/ramensoftware/windhawk-mods/issues/1510).**

Instructions on the settings:

- The file path pattern is parsed using [PathMatchSpecExW](https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-pathmatchspecexw). It supports wildcard `*` `?`. It also supports multiple semicolon-separated patterns.  
  Note that the full path (e.g. `C:\Program Files\MyApp\my-app.exe`), rather than the base filename (e.g. `my-app.exe`), is checked against the pattern.  
  To check the actual executable file path of your app, use the task manager or observe log output from this mod.
- The language ID can be found in the [MS-LCID](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/70feba9f-294e-491e-b6eb-56532684c37f) specification. For example, the language ID for English (United States) is 1033.

Use cases:

- You want to use a certain app in another language without affecting your current language, but that app does not provide individual language settings.
- A certain app is not compatible with the encoding on your machine and displays corrupt characters, and you want to force it into using English.

Note that it changes the language only and does not affect the encoding used in legacy programs. To actually fix corrupt characters issues with legacy programs or games, try using [LocaleRemulator](https://github.com/InWILL/Locale_Remulator).

![Wacom panel with corrupt characters, "fixed" by forcing English](https://raw.githubusercontent.com/yezhiyi9670/akioi-cdn/refs/heads/master/demo-img/wacom-panel-forced-engligh.png)

*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
# Here you can define settings, in YAML format, that the mod users will be able
# to configure. Metadata values such as $name and $description are optional.
# Check out the documentation for more information:
# https://github.com/ramensoftware/windhawk/wiki/Creating-a-new-mod#settings

- programList:
  - - glob: "*\\example-executable-file-114514.exe"
      $name: Filename pattern
      $description: "The pattern corresponding to the program filename."
    - langId: -1
      $name: Language identifier
      $description: "The language ID to apply. Set -1 to use the system default value."
  $name: Overrides List
  $description: >-
    Assign a new language to these programs.
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <shlwapi.h>

#include <windhawk_utils.h>

// ===========================================================

int my_langid = -1;

using LangGetter_t = short(WINAPI*)();

LangGetter_t GetUserDefaultUILanguage_Original;
short WINAPI GetUserDefaultUILanguage_Hook() {
    if(my_langid == -1) {
        return GetUserDefaultUILanguage_Original();
    } else {
        return my_langid;
    }
}

LangGetter_t GetUserDefaultLangID_Original;
short WINAPI GetUserDefaultLangID_Hook() {
    if(my_langid == -1) {
        return GetUserDefaultLangID_Original();
    } else {
        return my_langid;
    }
}

LangGetter_t GetUserDefaultLCID_Original;
short WINAPI GetUserDefaultLCID_Hook() {
    if(my_langid == -1) {
        return GetUserDefaultLCID_Original();
    } else {
        return my_langid;
    }
}

void determine_my_langid() {
    wchar_t *filename_buf = new wchar_t[2048];
    GetModuleFileNameW(NULL, filename_buf, 2048);
    Wh_Log(L">Process file: %ls", filename_buf);

    int lang_id = -1;
    for(int index = 0; ; index++) {
        PCWSTR glob = Wh_GetStringSetting(L"programList[%d].glob", index);
        if(!*glob) {
            Wh_FreeStringSetting(glob);
            break;
        }
        if(S_OK == PathMatchSpecExW(filename_buf, glob, PMSF_MULTIPLE)) {
            lang_id = Wh_GetIntSetting(L"programList[%d].langId", index);
            Wh_FreeStringSetting(glob);
            break;
        }
        Wh_FreeStringSetting(glob);
    }
    delete[] filename_buf;
    my_langid = lang_id;
}

// ===========================================================

bool HookKernel32DllSymbols() {
    HMODULE kernelBaseModule = GetModuleHandle(L"kernelbase.dll");
    HMODULE kernel32Module = GetModuleHandle(L"kernel32.dll");

    auto setKernelFunctionHook = [kernelBaseModule, kernel32Module](
                                        PCSTR targetName, void* hookFunction,
                                        void** originalFunction) {
        void* targetFunction =
            (void*)GetProcAddress(kernelBaseModule, targetName);
        if (!targetFunction) {
            targetFunction =
                (void*)GetProcAddress(kernel32Module, targetName);
            if (!targetFunction) {
                return FALSE;
            }
        }

        return Wh_SetFunctionHook(targetFunction, hookFunction, originalFunction);
    };

    // kernel32.dll
    return (
        setKernelFunctionHook("GetUserDefaultUILanguage", (void*)GetUserDefaultUILanguage_Hook, (void**)&GetUserDefaultUILanguage_Original) &&
        setKernelFunctionHook("GetUserDefaultLangID", (void*)GetUserDefaultLangID_Hook, (void**)&GetUserDefaultLangID_Original) &&
        setKernelFunctionHook("GetUserDefaultLCID", (void*)GetUserDefaultLCID_Hook, (void**)&GetUserDefaultLCID_Original)
    );
}

BOOL ModInit() {
    if (!HookKernel32DllSymbols()) {
        Wh_Log(L">Hook failed");
        return FALSE;
    }

    return TRUE;
}

// ===========================================================

// The mod is being initialized, load settings, hook functions, and do other
// initialization stuff if required.
BOOL Wh_ModInit() {
    Wh_Log(L">");

    determine_my_langid();

    return ModInit();
}

void Wh_ModSettingsChanged() {
    Wh_Log(L">");

    determine_my_langid();
}

// The mod is being unloaded, free all allocated resources.
void Wh_ModUninit() {
    Wh_Log(L">");
}

void Wh_ModAfterInit() {
    Wh_Log(L">");
}

void Wh_ModBeforeUninit() {
    Wh_Log(L">");
}
