#ifndef _MobiView_DllInterface_h_
#define _MobiView_DllInterface_h_

#include <stdint.h>
#include <string>
#include <windows.h>

//using namespace Upp;

#if !defined PLATFORM_WIN32 && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
	#define PLATFORM_WIN32 1
#endif


#ifndef PLATFORM_WIN32
	#define __cdecl
#endif

struct timestep_size
{
	int Type;         //NOTE: This is an int because it is an enum in the dll code.
	int32_t Magnitude;
};

struct dll_branch_index
{
	const char *IndexName;
	uint64_t BranchCount;
	char **BranchNames;
};


#define DLL_FUNCTION(RetType, Name, ...) typedef RetType (__cdecl *Name##_t)(__VA_ARGS__);
#include "DllFunctions.h"
#undef DLL_FUNCTION


struct model_dll_interface
{
	#define DLL_FUNCTION(RetType, Name, ...) Name##_t Name;
	#include "DllFunctions.h"
	#undef DLL_FUNCTION
	
#ifdef PLATFORM_WIN32
	HINSTANCE hinstModelDll;
#else
	void     *hinstModelDll;
#endif
	
	void UnLoad();
	bool Load(const char *DllName);
	
	bool IsLoaded() { return hinstModelDll != 0; }
	
	std::string GetDllError();
};


#endif
