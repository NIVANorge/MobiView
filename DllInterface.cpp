#include "DllInterface.h"

void SetupModelDllInterface(model_dll_interface *Model, HINSTANCE hinstanceDll)
{
	Model->SetupModel =         (SetupModel_t)        GetProcAddress(hinstanceDll, "DllSetupModel");
	Model->EncounteredError =   (EncounteredError_t)  GetProcAddress(hinstanceDll, "DllEncounteredError");
	Model->RunModel =           (RunModel_t)          GetProcAddress(hinstanceDll, "DllRunModel");
	Model->CopyDataSet =        (CopyDataSet_t)       GetProcAddress(hinstanceDll, "DllCopyDataSet");
	Model->DeleteDataSet =      (DeleteDataSet_t)     GetProcAddress(hinstanceDll, "DllDeleteDataSet");
	Model->GetTimesteps =       (GetTimesteps_t)      GetProcAddress(hinstanceDll, "DllGetTimesteps");
	Model->GetStartDate =       (GetStartDate_t)      GetProcAddress(hinstanceDll, "DllGetStartDate");
	Model->GetInputTimesteps =  (GetInputTimesteps_t) GetProcAddress(hinstanceDll, "DllGetInputTimesteps");
	Model->GetInputStartDate =  (GetInputStartDate_t) GetProcAddress(hinstanceDll, "DllGetInputStartDate");
	Model->GetResultSeries =    (GetResultSeries_t)   GetProcAddress(hinstanceDll, "DllGetResultSeries");
	Model->GetInputSeries =     (GetInputSeries_t)    GetProcAddress(hinstanceDll, "DllGetInputSeries");
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
	Model->GetParameterUnit =   (GetParameterUnit_t)  GetProcAddress(hinstanceDll, "DllGetParameterUnit");
	Model->GetResultUnit =      (GetResultUnit_t)     GetProcAddress(hinstanceDll, "DllGetResultUnit");
	Model->GetInputUnit =       (GetInputUnit_t)      GetProcAddress(hinstanceDll, "DllGetInputUnit");
	Model->WriteParametersToFile = (WriteParametersToFile_t)GetProcAddress(hinstanceDll, "DllWriteParametersToFile");
	Model->GetIndexSetsCount =  (GetIndexSetsCount_t) GetProcAddress(hinstanceDll, "DllGetIndexSetsCount");
	Model->GetIndexSets =       (GetIndexSets_t)      GetProcAddress(hinstanceDll, "DllGetIndexSets");
	Model->GetIndexCount =      (GetIndexCount_t)     GetProcAddress(hinstanceDll, "DllGetIndexCount");
	Model->GetIndexes =         (GetIndexes_t)        GetProcAddress(hinstanceDll, "DllGetIndexes");
	Model->GetParameterGroupIndexSetsCount = (GetParameterGroupIndexSetsCount_t)GetProcAddress(hinstanceDll, "DllGetParameterGroupIndexSetsCount");
	Model->GetParameterGroupIndexSets = (GetParameterGroupIndexSets_t)GetProcAddress(hinstanceDll, "DllGetParameterGroupIndexSets");
	Model->GetResultIndexSetsCount = (GetResultIndexSetsCount_t)GetProcAddress(hinstanceDll, "DllGetResultIndexSetsCount");
	Model->GetResultIndexSets = (GetResultIndexSets_t)GetProcAddress(hinstanceDll, "DllGetResultIndexSets");
	Model->GetInputIndexSetsCount = (GetInputIndexSetsCount_t)GetProcAddress(hinstanceDll, "DllGetInputIndexSetsCount");
	Model->GetInputIndexSets = (GetInputIndexSets_t)GetProcAddress(hinstanceDll, "DllGetInputIndexSets");
	Model->GetAllParameterGroupsCount = (GetAllParameterGroupsCount_t)GetProcAddress(hinstanceDll, "DllGetAllParameterGroupsCount");
	Model->GetAllParameterGroups = (GetAllParameterGroups_t)GetProcAddress(hinstanceDll, "DllGetAllParameterGroups");
	Model->GetAllParametersCount = (GetAllParametersCount_t)GetProcAddress(hinstanceDll, "DllGetAllParametersCount");
	Model->GetAllParameters =   (GetAllParameters_t)GetProcAddress(hinstanceDll, "DllGetAllParameters");
	Model->GetAllResultsCount = (GetAllResultsCount_t)GetProcAddress(hinstanceDll, "DllGetAllResultsCount");
	Model->GetAllResults      = (GetAllResults_t)     GetProcAddress(hinstanceDll, "DllGetAllResults");
	Model->GetAllInputsCount  = (GetAllInputsCount_t) GetProcAddress(hinstanceDll, "DllGetAllInputsCount");
	Model->GetAllInputs       = (GetAllInputs_t)      GetProcAddress(hinstanceDll, "DllGetAllInputs");
	Model->InputWasProvided   = (InputWasProvided_t)  GetProcAddress(hinstanceDll, "DllInputWasProvided");
	Model->GetBranchInputsCount = (GetBranchInputsCount_t) GetProcAddress(hinstanceDll, "DllGetBranchInputsCount");
	Model->GetBranchInputs    = (GetBranchInputs_t)   GetProcAddress(hinstanceDll, "DllGetBranchInputs");
	
	//TODO: Handle errors if GetProcAddress fails (can happen if e.g. somebody uses an old dll version)!
	
}