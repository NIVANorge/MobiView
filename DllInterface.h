#ifndef _MobiView_DllInterface_h_
#define _MobiView_DllInterface_h_

typedef void * (__cdecl *SetupModel_t)(const char *Parfile, const char *Inputfile);
typedef void * (__cdecl *RunModel_t)(void *DataSetPtr);
typedef int    (__cdecl *EncounteredError_t)(char *Errmsgout);
typedef uint64 (__cdecl *GetTimesteps_t)(void *DataSetPtr);
typedef void   (__cdecl *GetStartDate_t)(void *DataSetPtr, char *DateOut);
typedef void   (__cdecl *GetResultSeries_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount, double *WriteTo);
typedef void   (__cdecl *SetParameterDouble_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount, double Value);
typedef void   (__cdecl *SetParameterUInt_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount, uint64 Value);
typedef void   (__cdecl *SetParameterBool_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount, bool Value);
typedef void   (__cdecl *SetParameterTime_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount, const char *Value);
typedef double (__cdecl *GetParameterDouble_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount);
typedef uint64 (__cdecl *GetParameterUInt_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount);
typedef bool   (__cdecl *GetParameterBool_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount);
typedef void   (__cdecl *GetParameterTime_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount, char *WriteTo);
typedef void   (__cdecl *GetParameterDoubleMinMax_t)(void *DataSetPtr, const char *Name, double *MinOut, double *MaxOut);
typedef void   (__cdecl *GetParameterUIntMinMax_t)(void *DataSetPtr, const char *Name, uint64 *MinOut, uint64 *MaxOut);
typedef const char * (__cdecl *GetParameterDescription_t)(void *DataSetPtr, const char *Name);
typedef const char * (__cdecl *GetParameterUnit_t)(void *DataSetPtr, const char *Name);
typedef void   (__cdecl *WriteParametersToFile_t)(void *DataSetPtr, const char *Filename);
typedef uint64 (__cdecl *GetIndexSetsCount_t)(void *DataSetPtr);
typedef void   (__cdecl *GetIndexSets_t)(void *DataSetPtr, char **NamesOut);
typedef uint64 (__cdecl *GetIndexCount_t)(void *DataSetPtr, const char *IndexSetName);
typedef void   (__cdecl *GetIndexes_t)(void *DataSetPtr, const char *IndexSetName, char **NamesOut);
typedef uint64 (__cdecl *GetParameterGroupIndexSetsCount_t)(void *DataSetPtr, const char *ParameterGroupName);
typedef void   (__cdecl *GetParameterGroupIndexSets_t)(void *DataSetPtr, const char *ParameterGroupName, char **NamesOut);
typedef uint64 (__cdecl *GetResultIndexSetsCount_t)(void *DataSetPtr, const char *ResultName);
typedef void   (__cdecl *GetResultIndexSets_t)(void *DataSetPtr, const char *ResultName, char **NamesOut);
typedef uint64 (__cdecl *GetAllParameterGroupsCount_t)(void *DataSetPtr, const char *ParentGroupName);
typedef void   (__cdecl *GetAllParameterGroups_t)(void *DataSetPtr, char **NamesOut, const char *ParentGroupName);
typedef uint64 (__cdecl *GetAllParametersCount_t)(void *DataSetPtr, const char *GroupName);
typedef void   (__cdecl *GetAllParameters_t)(void *DataSetPtr, char **NamesOut, char **TypesOut, const char *GroupName);
typedef uint64 (__cdecl *GetAllResultsCount_t)(void *DataSetPtr);
typedef void   (__cdecl *GetAllResults_t)(void *DataSetPtr, char **NamesOut, char **TypesOut);

struct model_dll_interface
{
	SetupModel_t         SetupModel;
	RunModel_t           RunModel;
	EncounteredError_t   EncounteredError;
	GetTimesteps_t       GetTimesteps;
	GetStartDate_t       GetStartDate;
	GetResultSeries_t    GetResultSeries;
	SetParameterDouble_t SetParameterDouble;
	SetParameterUInt_t   SetParameterUInt;
	SetParameterBool_t   SetParameterBool;
	SetParameterTime_t   SetParameterTime;
	GetParameterDouble_t GetParameterDouble;
	GetParameterUInt_t   GetParameterUInt;
	GetParameterBool_t   GetParameterBool;
	GetParameterTime_t   GetParameterTime;
	WriteParametersToFile_t WriteParametersToFile;
	GetParameterDoubleMinMax_t GetParameterDoubleMinMax;
	GetParameterUIntMinMax_t GetParameterUIntMinMax;
	GetParameterDescription_t GetParameterDescription;
	GetParameterUnit_t   GetParameterUnit;
	GetIndexSetsCount_t  GetIndexSetsCount;
	GetIndexSets_t       GetIndexSets;
	GetIndexCount_t      GetIndexCount;
	GetIndexes_t         GetIndexes;
	GetParameterGroupIndexSetsCount_t GetParameterGroupIndexSetsCount;
	GetParameterGroupIndexSets_t GetParameterGroupIndexSets;
	GetResultIndexSetsCount_t GetResultIndexSetsCount;
	GetResultIndexSets_t GetResultIndexSets;
	GetAllParameterGroupsCount_t GetAllParameterGroupsCount;
	GetAllParameterGroups_t GetAllParameterGroups;
	GetAllParametersCount_t GetAllParametersCount;
	GetAllParameters_t   GetAllParameters;
	GetAllResultsCount_t GetAllResultsCount;
	GetAllResults_t      GetAllResults;
};

void SetupModelDllInterface(model_dll_interface *Model, HINSTANCE hinstanceDll)
{
	Model->SetupModel =         (SetupModel_t)        GetProcAddress(hinstanceDll, "DllSetupModel");
	Model->EncounteredError =   (EncounteredError_t)  GetProcAddress(hinstanceDll, "DllEncounteredError");
	Model->RunModel =           (RunModel_t)          GetProcAddress(hinstanceDll, "DllRunModel");
	Model->GetTimesteps =       (GetTimesteps_t)      GetProcAddress(hinstanceDll, "DllGetTimesteps");
	Model->GetStartDate =       (GetStartDate_t)      GetProcAddress(hinstanceDll, "DllGetStartDate");
	Model->GetResultSeries =    (GetResultSeries_t)   GetProcAddress(hinstanceDll, "DllGetResultSeries");
	Model->SetParameterDouble = (SetParameterDouble_t)GetProcAddress(hinstanceDll, "DllSetParameterDouble");
	Model->SetParameterUInt   = (SetParameterUInt_t)  GetProcAddress(hinstanceDll, "DllSetParameterUInt");
	Model->SetParameterBool   = (SetParameterBool_t)  GetProcAddress(hinstanceDll, "DllSetParameterBool");
	Model->SetParameterTime   = (SetParameterTime_t)  GetProcAddress(hinstanceDll, "DllSetParameterTime");
	Model->GetParameterDouble = (GetParameterDouble_t)GetProcAddress(hinstanceDll, "DllGetParameterDouble");
	Model->GetParameterUInt =   (GetParameterUInt_t)  GetProcAddress(hinstanceDll, "DllGetParameterUInt");
	Model->GetParameterBool =   (GetParameterBool_t)  GetProcAddress(hinstanceDll, "DllGetParameterBool");
	Model->GetParameterTime =   (GetParameterTime_t)  GetProcAddress(hinstanceDll, "DllGetParameterTime");
	Model->GetParameterDoubleMinMax = (GetParameterDoubleMinMax_t)GetProcAddress(hinstanceDll, "DllGetParameterDoubleMinMax");
	Model->GetParameterUIntMinMax = (GetParameterUIntMinMax_t)GetProcAddress(hinstanceDll, "DllGetParameterUIntMinMax");
	Model->GetParameterDescription = (GetParameterDescription_t)GetProcAddress(hinstanceDll, "DllGetParameterDescription");
	Model->GetParameterUnit =   (GetParameterUnit_t)GetProcAddress(hinstanceDll, "DllGetParameterUnit");
	Model->WriteParametersToFile = (WriteParametersToFile_t)GetProcAddress(hinstanceDll, "DllWriteParametersToFile");
	Model->GetIndexSetsCount =  (GetIndexSetsCount_t) GetProcAddress(hinstanceDll, "DllGetIndexSetsCount");
	Model->GetIndexSets =       (GetIndexSets_t)      GetProcAddress(hinstanceDll, "DllGetIndexSets");
	Model->GetIndexCount =      (GetIndexCount_t)     GetProcAddress(hinstanceDll, "DllGetIndexCount");
	Model->GetIndexes =         (GetIndexes_t)        GetProcAddress(hinstanceDll, "DllGetIndexes");
	Model->GetParameterGroupIndexSetsCount = (GetParameterGroupIndexSetsCount_t)GetProcAddress(hinstanceDll, "DllGetParameterGroupIndexSetsCount");
	Model->GetParameterGroupIndexSets = (GetParameterGroupIndexSets_t)GetProcAddress(hinstanceDll, "DllGetParameterGroupIndexSets");
	Model->GetResultIndexSetsCount = (GetResultIndexSetsCount_t)GetProcAddress(hinstanceDll, "DllGetResultIndexSetsCount");
	Model->GetResultIndexSets = (GetResultIndexSets_t)GetProcAddress(hinstanceDll, "DllGetResultIndexSets");
	Model->GetAllParameterGroupsCount = (GetAllParameterGroupsCount_t)GetProcAddress(hinstanceDll, "DllGetAllParameterGroupsCount");
	Model->GetAllParameterGroups = (GetAllParameterGroups_t)GetProcAddress(hinstanceDll, "DllGetAllParameterGroups");
	Model->GetAllParametersCount = (GetAllParametersCount_t)GetProcAddress(hinstanceDll, "DllGetAllParametersCount");
	Model->GetAllParameters =   (GetAllParameters_t)GetProcAddress(hinstanceDll, "DllGetAllParameters");
	Model->GetAllResultsCount = (GetAllResultsCount_t)GetProcAddress(hinstanceDll, "DllGetAllResultsCount");
	Model->GetAllResults      = (GetAllResults_t)     GetProcAddress(hinstanceDll, "DllGetAllResults");
	
	
	//TODO: Handle errors if GetProcAddress fails!
	
}

#endif
