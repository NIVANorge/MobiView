#ifndef _MobiView_DllInterface_h_
#define _MobiView_DllInterface_h_

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

typedef void * (__cdecl *SetupModel_t)(const char *Parfile, const char *Inputfile);
typedef void * (__cdecl *RunModel_t)(void *DataSetPtr);
typedef void * (__cdecl *CopyDataSet_t)(void *DataSetPtr, bool CopyResults);
typedef void * (__cdecl *DeleteDataSet_t)(void *DataSetPtr);
typedef void * (__cdecl *DeleteModelAndDataSet_t)(void *DataSetPtr);
typedef int    (__cdecl *EncounteredError_t)(char *Errmsgout);
typedef uint64 (__cdecl *GetTimesteps_t)(void *DataSetPtr);
typedef void   (__cdecl *GetStartDate_t)(void *DataSetPtr, char *DateOut);
typedef uint64 (__cdecl *GetInputTimesteps_t)(void *DataSetPtr);
typedef void   (__cdecl *GetInputStartDate_t)(void *DataSetPtr, char *DateOut);
typedef void   (__cdecl *GetResultSeries_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount, double *WriteTo);
typedef void   (__cdecl *GetInputSeries_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount, double *WriteTo, bool AlignWithResults);
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
typedef const char * (__cdecl *GetResultUnit_t)(void *DataSetPtr, const char *Name);
typedef const char * (__cdecl *GetInputUnit_t)(void *DataSetPtr, const char *Name);
typedef void   (__cdecl *WriteParametersToFile_t)(void *DataSetPtr, const char *Filename);
typedef uint64 (__cdecl *GetIndexSetsCount_t)(void *DataSetPtr);
typedef void   (__cdecl *GetIndexSets_t)(void *DataSetPtr, char **NamesOut);
typedef uint64 (__cdecl *GetIndexCount_t)(void *DataSetPtr, const char *IndexSetName);
typedef void   (__cdecl *GetIndexes_t)(void *DataSetPtr, const char *IndexSetName, char **NamesOut);
typedef uint64 (__cdecl *GetParameterGroupIndexSetsCount_t)(void *DataSetPtr, const char *ParameterGroupName);
typedef void   (__cdecl *GetParameterGroupIndexSets_t)(void *DataSetPtr, const char *ParameterGroupName, char **NamesOut);
typedef uint64 (__cdecl *GetResultIndexSetsCount_t)(void *DataSetPtr, const char *ResultName);
typedef void   (__cdecl *GetResultIndexSets_t)(void *DataSetPtr, const char *ResultName, char **NamesOut);
typedef uint64 (__cdecl *GetInputIndexSetsCount_t)(void *DataSetPtr, const char *ResultName);
typedef void   (__cdecl *GetInputIndexSets_t)(void *DataSetPtr, const char *ResultName, char **NamesOut);
typedef uint64 (__cdecl *GetAllParameterGroupsCount_t)(void *DataSetPtr, const char *ParentGroupName);
typedef void   (__cdecl *GetAllParameterGroups_t)(void *DataSetPtr, char **NamesOut, const char *ParentGroupName);
typedef uint64 (__cdecl *GetAllParametersCount_t)(void *DataSetPtr, const char *GroupName);
typedef void   (__cdecl *GetAllParameters_t)(void *DataSetPtr, char **NamesOut, char **TypesOut, const char *GroupName);
typedef uint64 (__cdecl *GetAllResultsCount_t)(void *DataSetPtr);
typedef void   (__cdecl *GetAllResults_t)(void *DataSetPtr, char **NamesOut, char **TypesOut);
typedef uint64 (__cdecl *GetAllInputsCount_t)(void *DataSetPtr);
typedef void   (__cdecl *GetAllInputs_t)(void *DataSetPtr, char **NamesOut, char **TypesOut);
typedef bool   (__cdecl *InputWasProvided_t)(void *DataSetPtr, const char *Name, char **IndexNames, uint64 IndexCount);
typedef uint64 (__cdecl *GetBranchInputsCount_t)(void *DataSetPtr, const char *IndexSetName, const char *IndexName);
typedef void   (__cdecl *GetBranchInputs_t)(void *DataSetPtr, const char *IndexSetName, const char *IndexName, char **BranchInputsOut);
typedef void   (__cdecl *PrintResultStructure_t)(void *DataSetPtr, char *Buf, uint64 BufLen);

struct model_dll_interface
{
	SetupModel_t         SetupModel;
	RunModel_t           RunModel;
	CopyDataSet_t        CopyDataSet;
	DeleteDataSet_t      DeleteDataSet;
	DeleteModelAndDataSet_t DeleteModelAndDataSet;
	EncounteredError_t   EncounteredError;
	GetTimesteps_t       GetTimesteps;
	GetStartDate_t       GetStartDate;
	GetInputTimesteps_t  GetInputTimesteps;
	GetInputStartDate_t  GetInputStartDate;
	GetResultSeries_t    GetResultSeries;
	GetInputSeries_t     GetInputSeries;
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
	GetResultUnit_t      GetResultUnit;
	GetInputUnit_t       GetInputUnit;
	GetIndexSetsCount_t  GetIndexSetsCount;
	GetIndexSets_t       GetIndexSets;
	GetIndexCount_t      GetIndexCount;
	GetIndexes_t         GetIndexes;
	GetParameterGroupIndexSetsCount_t GetParameterGroupIndexSetsCount;
	GetParameterGroupIndexSets_t GetParameterGroupIndexSets;
	GetResultIndexSetsCount_t GetResultIndexSetsCount;
	GetResultIndexSets_t GetResultIndexSets;
	GetInputIndexSetsCount_t GetInputIndexSetsCount;
	GetInputIndexSets_t GetInputIndexSets;
	GetAllParameterGroupsCount_t GetAllParameterGroupsCount;
	GetAllParameterGroups_t GetAllParameterGroups;
	GetAllParametersCount_t GetAllParametersCount;
	GetAllParameters_t   GetAllParameters;
	GetAllResultsCount_t GetAllResultsCount;
	GetAllResults_t      GetAllResults;
	GetAllInputsCount_t  GetAllInputsCount;
	GetAllInputs_t       GetAllInputs;
	InputWasProvided_t   InputWasProvided;
	GetBranchInputsCount_t GetBranchInputsCount;
	GetBranchInputs_t    GetBranchInputs;
	PrintResultStructure_t PrintResultStructure;
};

void SetupModelDllInterface(model_dll_interface *Model, HINSTANCE hinstanceDll);


#endif
