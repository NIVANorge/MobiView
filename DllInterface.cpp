#include "DllInterface.h"

#include <locale>
#include <codecvt>

#ifndef PLATFORM_WIN32
#include <dlfcn.h>
#endif

void model_dll_interface::UnLoad()
{
	if(hinstModelDll)
	{
#ifdef PLATFORM_WIN32
		FreeLibrary(hinstModelDll);
#else
		dlclose(hinstModelDll);
#endif
		hinstModelDll = 0;
	}
}


bool model_dll_interface::Load(const char *DllName)
{
	UnLoad();
	
#ifdef PLATFORM_WIN32
	std::u16string Filename16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(DllName);
	hinstModelDll = LoadLibraryW((wchar_t *)Filename16.data());
	#define LoadProc(Handle, Name) GetProcAddress(Handle, Name)
#else
	hinstModelDll = dlopen(DllName, RTLD_LAZY);
	#define LoadProc(Handle, Name) dlsym(Handle, Name)
#endif

	if(!hinstModelDll) return false;

#define DLL_FUNCTION(RetType, Name, ...) \
	Name = ( Name##_t )  LoadProc(hinstModelDll, "Dll"#Name ); \
	if(!Name) return false;
#include "DllFunctions.h"
#undef DLL_FUNCTION

	return true;
}

std::string model_dll_interface::GetDllError()
{
#ifdef PLATFORM_WIN32
	LPVOID lpMsgBuf;
	DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

	std::string Result((char *)lpMsgBuf);

    LocalFree(lpMsgBuf);
#else
	std::string Result(dlerror());
#endif
    return Result;
}