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
	LoadProcedure(SetParameterEnum);
	LoadProcedure(GetParameterDouble);
	LoadProcedure(GetParameterUInt);
	LoadProcedure(GetParameterBool);
	LoadProcedure(GetParameterTime);
	LoadProcedure(GetParameterEnum);
	LoadProcedure(GetEnumValuesCount);
	LoadProcedure(GetEnumValues);
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
	LoadProcedure(GetModuleDescription);
	LoadProcedure(GetAllParametersCount);
	LoadProcedure(GetAllParameters);
	LoadProcedure(GetAllResultsCount);
	LoadProcedure(GetAllResults);
	LoadProcedure(GetAllInputsCount);
	LoadProcedure(GetAllInputs);
	LoadProcedure(InputWasProvided);
	LoadProcedure(ResultWasComputed);
	LoadProcedure(GetBranchInputsCount);
	LoadProcedure(GetBranchInputs);
	LoadProcedure(PrintResultStructure);
	LoadProcedure(GetTimestepSize);
	LoadProcedure(CopyData);
	
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