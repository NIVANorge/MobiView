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
	
	SetupModel =         (SetupModel_t)        LoadProc(hinstModelDll, "DllSetupModel");
	EncounteredError =   (EncounteredError_t)  LoadProc(hinstModelDll, "DllEncounteredError");
	GetModelName =       (GetModelName_t)      LoadProc(hinstModelDll, "DllGetModelName");
	RunModel =           (RunModel_t)          LoadProc(hinstModelDll, "DllRunModel");
	CopyDataSet =        (CopyDataSet_t)       LoadProc(hinstModelDll, "DllCopyDataSet");
	DeleteDataSet =      (DeleteDataSet_t)     LoadProc(hinstModelDll, "DllDeleteDataSet");
	DeleteModelAndDataSet = (DeleteModelAndDataSet_t) LoadProc(hinstModelDll, "DllDeleteModelAndDataSet");
	GetTimesteps =       (GetTimesteps_t)      LoadProc(hinstModelDll, "DllGetTimesteps");
	GetStartDate =       (GetStartDate_t)      LoadProc(hinstModelDll, "DllGetStartDate");
	GetInputTimesteps =  (GetInputTimesteps_t) LoadProc(hinstModelDll, "DllGetInputTimesteps");
	GetInputStartDate =  (GetInputStartDate_t) LoadProc(hinstModelDll, "DllGetInputStartDate");
	GetResultSeries =    (GetResultSeries_t)   LoadProc(hinstModelDll, "DllGetResultSeries");
	GetInputSeries =     (GetInputSeries_t)    LoadProc(hinstModelDll, "DllGetInputSeries");
	SetParameterDouble = (SetParameterDouble_t)LoadProc(hinstModelDll, "DllSetParameterDouble");
	SetParameterUInt   = (SetParameterUInt_t)  LoadProc(hinstModelDll, "DllSetParameterUInt");
	SetParameterBool   = (SetParameterBool_t)  LoadProc(hinstModelDll, "DllSetParameterBool");
	SetParameterTime   = (SetParameterTime_t)  LoadProc(hinstModelDll, "DllSetParameterTime");
	GetParameterDouble = (GetParameterDouble_t)LoadProc(hinstModelDll, "DllGetParameterDouble");
	GetParameterUInt =   (GetParameterUInt_t)  LoadProc(hinstModelDll, "DllGetParameterUInt");
	GetParameterBool =   (GetParameterBool_t)  LoadProc(hinstModelDll, "DllGetParameterBool");
	GetParameterTime =   (GetParameterTime_t)  LoadProc(hinstModelDll, "DllGetParameterTime");
	GetParameterDoubleMinMax = (GetParameterDoubleMinMax_t)LoadProc(hinstModelDll, "DllGetParameterDoubleMinMax");
	GetParameterUIntMinMax = (GetParameterUIntMinMax_t)LoadProc(hinstModelDll, "DllGetParameterUIntMinMax");
	GetParameterDescription = (GetParameterDescription_t)LoadProc(hinstModelDll, "DllGetParameterDescription");
	GetParameterUnit =   (GetParameterUnit_t)  LoadProc(hinstModelDll, "DllGetParameterUnit");
	GetResultUnit =      (GetResultUnit_t)     LoadProc(hinstModelDll, "DllGetResultUnit");
	GetInputUnit =       (GetInputUnit_t)      LoadProc(hinstModelDll, "DllGetInputUnit");
	WriteParametersToFile = (WriteParametersToFile_t)LoadProc(hinstModelDll, "DllWriteParametersToFile");
	GetIndexSetsCount =  (GetIndexSetsCount_t) LoadProc(hinstModelDll, "DllGetIndexSetsCount");
	GetIndexSets =       (GetIndexSets_t)      LoadProc(hinstModelDll, "DllGetIndexSets");
	GetIndexCount =      (GetIndexCount_t)     LoadProc(hinstModelDll, "DllGetIndexCount");
	GetIndexes =         (GetIndexes_t)        LoadProc(hinstModelDll, "DllGetIndexes");
	IsParameterGroupName = (IsParameterGroupName_t)LoadProc(hinstModelDll, "DllIsParameterGroupName");
	GetParameterGroupIndexSetsCount = (GetParameterGroupIndexSetsCount_t)LoadProc(hinstModelDll, "DllGetParameterGroupIndexSetsCount");
	GetParameterGroupIndexSets = (GetParameterGroupIndexSets_t)LoadProc(hinstModelDll, "DllGetParameterGroupIndexSets");
	GetResultIndexSetsCount = (GetResultIndexSetsCount_t)LoadProc(hinstModelDll, "DllGetResultIndexSetsCount");
	GetResultIndexSets = (GetResultIndexSets_t)LoadProc(hinstModelDll, "DllGetResultIndexSets");
	GetInputIndexSetsCount = (GetInputIndexSetsCount_t)LoadProc(hinstModelDll, "DllGetInputIndexSetsCount");
	GetInputIndexSets = (GetInputIndexSets_t)LoadProc(hinstModelDll, "DllGetInputIndexSets");
	GetAllParameterGroupsCount = (GetAllParameterGroupsCount_t)LoadProc(hinstModelDll, "DllGetAllParameterGroupsCount");
	GetAllParameterGroups = (GetAllParameterGroups_t)LoadProc(hinstModelDll, "DllGetAllParameterGroups");
	GetAllModulesCount = (GetAllModulesCount_t)LoadProc(hinstModelDll, "DllGetAllModulesCount");
	GetAllModules      = (GetAllModules_t)LoadProc(hinstModelDll, "DllGetAllModules");
	GetAllParametersCount = (GetAllParametersCount_t)LoadProc(hinstModelDll, "DllGetAllParametersCount");
	GetAllParameters =   (GetAllParameters_t)LoadProc(hinstModelDll, "DllGetAllParameters");
	GetAllResultsCount = (GetAllResultsCount_t)LoadProc(hinstModelDll, "DllGetAllResultsCount");
	GetAllResults      = (GetAllResults_t)     LoadProc(hinstModelDll, "DllGetAllResults");
	GetAllInputsCount  = (GetAllInputsCount_t) LoadProc(hinstModelDll, "DllGetAllInputsCount");
	GetAllInputs       = (GetAllInputs_t)      LoadProc(hinstModelDll, "DllGetAllInputs");
	InputWasProvided   = (InputWasProvided_t)  LoadProc(hinstModelDll, "DllInputWasProvided");
	GetBranchInputsCount = (GetBranchInputsCount_t) LoadProc(hinstModelDll, "DllGetBranchInputsCount");
	GetBranchInputs    = (GetBranchInputs_t)   LoadProc(hinstModelDll, "DllGetBranchInputs");
	PrintResultStructure = (PrintResultStructure_t) LoadProc(hinstModelDll, "DllPrintResultStructure");
	
	//TODO: Handle errors if LoadProc fails (can happen if e.g. somebody uses an old dll version)!
	
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