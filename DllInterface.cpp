#include "DllInterface.h"

#ifndef PLATFORM_WIN32
#include <dlfcn.h>
#endif

bool model_dll_interface::Load(const char *DllName)
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
	
#ifdef PLATFORM_WIN32
	hinstModelDll = LoadLibraryA(DllName);
	#define LoadProc(Handle, Name) GetProcAddress(Handle, Name)
#else
	hinstModelDll = dlopen(DllName, RTLD_LAZY);
	#define LoadProc(Handle, Name) dlsym(Handle, Name)
#endif

	if(!hinstModelDll) return false;

#define LoadProcedure(Name) \
	Name = ( Name##_t )  LoadProc(hinstModelDll, "Dll"#Name ); \
	if(!Name) return false;
	
	
	
	LoadProcedure(SetupModel);
	LoadProcedure(SetupModelBlankIndexSets);
	LoadProcedure(ReadInputs);
	LoadProcedure(SetIndexes);
	LoadProcedure(SetBranchIndexes);
	LoadProcedure(EncounteredError);
	LoadProcedure(EncounteredWarning);
	LoadProcedure(GetModelName);
	LoadProcedure(RunModel);
	LoadProcedure(CopyDataSet);
	LoadProcedure(DeleteDataSet);
	LoadProcedure(DeleteModelAndDataSet);
	LoadProcedure(GetTimesteps);
	LoadProcedure(GetStartDate);
	LoadProcedure(GetInputTimesteps);
	LoadProcedure(GetInputStartDate);
	LoadProcedure(GetResultSeries);
	LoadProcedure(GetInputSeries);
	LoadProcedure(SetParameterDouble);
	LoadProcedure(SetParameterUInt);
	LoadProcedure(SetParameterBool);
	LoadProcedure(SetParameterTime);
	LoadProcedure(GetParameterDouble);
	LoadProcedure(GetParameterUInt);
	LoadProcedure(GetParameterBool);
	LoadProcedure(GetParameterTime);
	LoadProcedure(GetParameterDoubleMinMax);
	LoadProcedure(GetParameterUIntMinMax);
	LoadProcedure(GetParameterDescription);
	LoadProcedure(GetParameterUnit);
	LoadProcedure(GetResultUnit);
	LoadProcedure(GetInputUnit);
	LoadProcedure(WriteParametersToFile);
	LoadProcedure(GetIndexSetsCount);
	LoadProcedure(GetIndexSets);
	LoadProcedure(GetIndexCount);
	LoadProcedure(GetIndexes);
	LoadProcedure(IsParameterGroupName);
	LoadProcedure(GetParameterGroupIndexSetsCount);
	LoadProcedure(GetParameterGroupIndexSets);
	LoadProcedure(GetResultIndexSetsCount);
	LoadProcedure(GetResultIndexSets);
	LoadProcedure(GetInputIndexSetsCount);
	LoadProcedure(GetInputIndexSets);
	LoadProcedure(GetAllParameterGroupsCount);
	LoadProcedure(GetAllParameterGroups);
	LoadProcedure(GetAllModulesCount);
	LoadProcedure(GetAllModules);
	LoadProcedure(GetAllParametersCount);
	LoadProcedure(GetAllParameters);
	LoadProcedure(GetAllResultsCount);
	LoadProcedure(GetAllResults);
	LoadProcedure(GetAllInputsCount);
	LoadProcedure(GetAllInputs);
	LoadProcedure(InputWasProvided);
	LoadProcedure(GetBranchInputsCount);
	LoadProcedure(GetBranchInputs);
	LoadProcedure(PrintResultStructure);
	LoadProcedure(GetTimestepSize);
	
	return true;
}

std::string model_dll_interface::GetDllError()
{
#ifdef PLATFORM_WIN32
	LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
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