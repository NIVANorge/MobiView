#ifndef _MobiView_ParameterEditing_h_
#define _MobiView_ParameterEditing_h_

#include "MobiView.h"




ParameterCtrl::ParameterCtrl()
{
	CtrlLayout(*this);
};



void MobiView::RefreshParameterView()
{

	Params.ParameterView.Clear();
	Params.ParameterView.Reset();
	
	Vector<int> Selected = ParameterGroupSelecter.GetSel();
	if(Selected.size() == 0 || Selected[0] == 0) return;
	
	Value SelectedGroup = ParameterGroupSelecter.Get(Selected[0]);
	std::string SelectedGroupName = SelectedGroup.ToString().ToStd();
	
	if(SelectedGroupName.empty()) return;
	
	uint64 IndexSetCount = ModelDll.GetParameterGroupIndexSetsCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> IndexSetNames(IndexSetCount);
	ModelDll.GetParameterGroupIndexSets(DataSet, SelectedGroupName.data(), IndexSetNames.data());
	
	
	//PromptOK("OK!");
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		IndexList[Idx]->Disable();
		IndexLock[Idx]->Enable();
	}
	
	std::vector<std::string> Indexes_String(IndexSetCount);
	std::vector<char *> Indexes(IndexSetCount);
	
	for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
	{
		size_t Id = IndexSetNameToId[IndexSetNames[Idx]];
		IndexList[Id]->Enable();
		
		Indexes_String[Idx] = IndexList[Id]->Get().ToString().ToStd();
		Indexes[IndexSetCount - Idx - 1] = (char *)Indexes_String[Idx].data(); //NOTE: Have to reverse since GetParameterGroupIndexSets returns in reverse order. May want to fix that!
	}
	
	
	uint64 ParameterCount = ModelDll.GetAllParametersCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> ParameterNames(ParameterCount);
	std::vector<char *> ParameterTypes(ParameterCount);
	ModelDll.GetAllParameters(DataSet, ParameterNames.data(), ParameterTypes.data(), SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	if(IndexSetNames.size() >= 2 && (strcmp(IndexSetNames[0], IndexSetNames[1]) == 0))
	{
		//NOTE: The last (not first since this list is reversed, sigh..) two index set
		//dependencies are the same, and so we edit this as a row..
		Params.ParameterView.AddColumn("Name");
		
		size_t Id = IndexSetNameToId[IndexSetNames[0]];
		
		IndexLock[Id]->Disable(); //If we were to enable it, we would have to make it work properly in this editing mode.
		
		DropList &Idxs = *IndexList[Id];
		std::vector<std::string> IterateIndexes(Idxs.GetCount());
		for(int Row = 0; Row < Idxs.GetCount(); ++Row)
		{
			IterateIndexes[Row] = Idxs.GetValue(Row).ToString().ToStd();
		}
		
		for(int Row = 0; Row < IterateIndexes.size(); ++Row)
		{
			Params.ParameterView.AddColumn(String(IterateIndexes[Row]));
		}
		//TODO: Maybe also add min, max, unit, description..
		
		ParameterControls.Clear();
		CurrentParameterTypes.clear();
		
		for(int Idx = 0; Idx < ParameterCount; ++Idx)
		{
			const char *Name = ParameterNames[Idx];
			
			Params.ParameterView.Add(String(Name));
			
			CurrentParameterTypes.push_back(ParameterType_Double); //TODO: Right now we assume all matrix-like parameters are of type double.
			
			for(int Col = 0; Col < IterateIndexes.size(); ++Col)
			{
				Indexes[Indexes.size() - 1] = (char *)IterateIndexes[Col].data(); //NOTE: Casting away constness, but it is not dangerous here.
				Value ParVal = ModelDll.GetParameterDouble(DataSet, Name, Indexes.data(), Indexes.size());
				Params.ParameterView.Set(Idx, Col+1, ParVal);
				ParameterControls.Create<EditDoubleNotNull>();
				Params.ParameterView.SetCtrl(Idx, Col+1, ParameterControls.Top());
				ParameterControls.Top().WhenAction = [=]() { ParameterEditAccepted(Idx, Col, true); };
			}
		}
	}
	else
	{
		// Otherwise normal editing of just one value
		Params.ParameterView.AddColumn("Name").HeaderTab();
		Params.ParameterView.AddColumn("Value").HeaderTab();
		Params.ParameterView.AddColumn("Min").HeaderTab();
		Params.ParameterView.AddColumn("Max").HeaderTab();
		Params.ParameterView.AddColumn("Unit").HeaderTab();
		Params.ParameterView.AddColumn("Description").HeaderTab();
		
		Params.ParameterView.ColumnWidths("20 12 10 10 10 38");
		
		ParameterControls.Clear();
		CurrentParameterTypes.clear();
		for(size_t Idx = 0; Idx < ParameterCount; ++Idx)
		{
			const char *Name = ParameterNames[Idx];
			const char *Type = ParameterTypes[Idx];
			
			Value ParVal;
			Value ParMin;
			Value ParMax;
			Value ParUnit;
			Value ParDesc;
			const char *Unit = ModelDll.GetParameterUnit(DataSet, Name);
			if(Unit) ParUnit = Unit;
			const char *Description = ModelDll.GetParameterDescription(DataSet, Name);
			if(Description) ParDesc = Description;
			
			if(strcmp(Type, "double") == 0)
			{
				ParVal = ModelDll.GetParameterDouble(DataSet, Name, Indexes.data(), IndexSetCount);
				double Min, Max;
				ModelDll.GetParameterDoubleMinMax(DataSet, Name, &Min, &Max);
				ParMin = Min;
				ParMax = Max;
				
				ParameterControls.Create<EditDoubleNotNull>();
				CurrentParameterTypes.push_back(ParameterType_Double);
				
				if (CheckDllUserError()) return;
			}
			else if(strcmp(Type, "uint") == 0)
			{
				//TODO: Converting to int potentially loses precision. However Value has no uint64
				//subtype
				ParVal = (int64)ModelDll.GetParameterUInt(DataSet, Name, Indexes.data(), IndexSetCount);
				uint64 Min, Max;
				ModelDll.GetParameterUIntMinMax(DataSet, Name, &Min, &Max);
				int64 M = (int64)Max;
				if(M < 0) M = INT64_MAX; //Stupid stopgap. We should do this better and actually display uint64's.
				ParMin = (int64)Min;
				ParMax = M;
				
				ParameterControls.Create<EditInt64NotNull>();
				CurrentParameterTypes.push_back(ParameterType_UInt);
				
				if (CheckDllUserError()) return;
			}
			else if(strcmp(Type, "bool") == 0)
			{
				ParVal = ModelDll.GetParameterBool(DataSet, Name, Indexes.data(), IndexSetCount);
				if(CheckDllUserError()) return;
				
				ParameterControls.Create<Option>();
				CurrentParameterTypes.push_back(ParameterType_Bool);
			}
			else if(strcmp(Type, "time") == 0)
			{
				char TimeVal[256];
				ModelDll.GetParameterTime(DataSet, Name, Indexes.data(), IndexSetCount, TimeVal);
				Date D;
				StrToDate(D, TimeVal); //Error handling? But should not be necessary.
				ParVal = D;
				
				ParameterControls.Create<EditDateNotNull>();
				CurrentParameterTypes.push_back(ParameterType_Time);
			}
			Params.ParameterView.Add(String(Name), ParVal, ParMin, ParMax, ParUnit, ParDesc);
			Params.ParameterView.SetCtrl((int)Idx, 1, ParameterControls.Top());
			ParameterControls.Top().WhenAction = [=]() { ParameterEditAccepted((int)Idx, 0, false); };
		}
	}
	
}


void MobiView::RecursiveUpdateParameter(std::vector<char *> &IndexSetNames, int Level, std::vector<std::string> &CurrentIndexes, int Row, int Col, bool EditingAsRow)
{
	if(Level == IndexSetNames.size())
	{
		//Do the actual update.
		size_t IndexCount = CurrentIndexes.size();
		std::vector<char *> Indexes(IndexCount);
		for(size_t Idx = 0; Idx < IndexCount; ++Idx)
		{
			Indexes[IndexCount - Idx - 1] = (char *)CurrentIndexes[Idx].data(); //NOTE: Have to do the reversing since the GetParameterGroupIndexSets returns them in "reverse order". We may want to fix that..
		}
		
		std::string Name = Params.ParameterView.Get(Row, 0).ToString().ToStd();
		parameter_type Type = CurrentParameterTypes[Row];
		
		switch(Type)
		{
			case ParameterType_Double:
			{
				double V = Params.ParameterView.Get(Row, Col + 1);
				if(!IsNull(V))
					ModelDll.SetParameterDouble(DataSet, Name.data(), Indexes.data(), Indexes.size(), V);
			} break;
			
			case ParameterType_UInt:
			{
				int64 V = Params.ParameterView.Get(Row, Col + 1);
				if(!IsNull(V))
					ModelDll.SetParameterUInt(DataSet, Name.data(), Indexes.data(), Indexes.size(), (uint64)V);
			} break;
			
			case ParameterType_Bool:
			{
				Ctrl *ctrl = Params.ParameterView.GetCtrl(Row, Col + 1);
				bool V = (bool)((Option*)ctrl)->Get();
				ModelDll.SetParameterBool(DataSet, Name.data(), Indexes.data(), Indexes.size(), V);
			} break;
			
			case ParameterType_Time:
			{
				EditDateNotNull* ctrl = (EditDateNotNull*)Params.ParameterView.GetCtrl(Row, Col + 1);
				Date D = ctrl->GetData();
				if(D.IsValid())
				{
					std::string V = Format(D).ToStd();
					ModelDll.SetParameterTime(DataSet, Name.data(), Indexes.data(), Indexes.size(), V.data());
				}
			} break;
		}
		CheckDllUserError();
	}
	else
	{
		const char *IndexSetName = IndexSetNames[Level];
		size_t Id = IndexSetNameToId[IndexSetName];
		
		if(EditingAsRow && (Level == 0))
		{
			//NOTE: This happens for instance for the Percolation Matrix of PERSiST where all
			//parameters of a row are displayed for editing at the same time. In that case the
			//last (first) index set is over that row, and is not governed by the other index
			//set control.
			CurrentIndexes[Level] = Params.ParameterView.HeaderTab(Col + 1).GetText().ToStd();
			RecursiveUpdateParameter(IndexSetNames, Level + 1, CurrentIndexes, Row, Col, EditingAsRow);
		}
		else if(IndexLock[Id]->IsEnabled() && IndexLock[Id]->Get())
		{
			size_t IndexCount = ModelDll.GetIndexCount(DataSet, IndexSetName);
			std::vector<char *> IndexNames(IndexCount);
			ModelDll.GetIndexes(DataSet, IndexSetName, IndexNames.data());
			for(size_t Idx = 0; Idx < IndexCount; ++Idx)
			{
				CurrentIndexes[Level] = IndexNames[Idx];
				RecursiveUpdateParameter(IndexSetNames, Level + 1, CurrentIndexes, Row, Col, EditingAsRow);
			}
		}
		else
		{
			CurrentIndexes[Level] = IndexList[Id]->Get().ToString().ToStd();
			RecursiveUpdateParameter(IndexSetNames, Level + 1, CurrentIndexes, Row, Col, EditingAsRow);
		}
	}
}


void MobiView::ParameterEditAccepted(int Row, int Col, bool EditingAsRow)
{
	//TODO: High degree of copypaste from above. Factor this out.
	Vector<int> Selected = ParameterGroupSelecter.GetSel();
	if(Selected.size() == 0) return;
	
	Value SelectedGroup = ParameterGroupSelecter.Get(Selected[0]);
	std::string SelectedGroupName = SelectedGroup.ToString().ToStd();
	
	uint64 IndexSetCount = ModelDll.GetParameterGroupIndexSetsCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> IndexSetNames(IndexSetCount);
	ModelDll.GetParameterGroupIndexSets(DataSet, SelectedGroupName.data(), IndexSetNames.data());
	if (CheckDllUserError()) return;
	
	std::vector<std::string> CurrentIndexes(IndexSetCount);
	RecursiveUpdateParameter(IndexSetNames, 0, CurrentIndexes, Row, Col, EditingAsRow);
	
	ParametersWereChangedSinceLastSave = true;
}


void MobiView::SaveParameters()
{
	if(!ModelDll.IsLoaded() || !ModelDll.WriteParametersToFile || !ParameterFile.size())
	{
		Log("Parameters can only be saved once a model and parameter file is loaded");
		return;
	}
	//TODO: Mechanism for determining if there has actually been edits that need to be saved.
	//TODO: Maybe also a "do you really want to overwrite <filename>".
	ModelDll.WriteParametersToFile(DataSet, ParameterFile.data());
	if(CheckDllUserError())
	{
		Log("Parameter saving may have been unsuccessful.");
	}
	else
	{
		Log(String("Parameters saved to ") + ParameterFile.data());
		ParametersWereChangedSinceLastSave = false;
	}
}

void MobiView::SaveParametersAs()
{
	if(!ModelDll.IsLoaded() || !ModelDll.WriteParametersToFile || !ParameterFile.size())
	{
		Log("Parameters can only be saved once a model and parameter file is loaded");
		return;
	}
	
	FileSel Sel;
	Sel.Type("Parameter dat files", "*.dat");
	String ParFile = ParameterFile.data();
	Sel.PreSelect(ParFile);
	Sel.ExecuteSaveAs("Save parameters as");
	
	std::string NewFile = Sel.Get().ToStd();
	if(NewFile.size())
	{
		ModelDll.WriteParametersToFile(DataSet, NewFile.data());
		if(CheckDllUserError())
		{
			Log("Parameter saving may have been unsuccessful.");
		}
		else
		{
			ParameterFile = NewFile;
			Log(String("Parameters saved to ") + ParameterFile.data());
			ParametersWereChangedSinceLastSave = false;
			
			StoreSettings(); //So that the current working file is now default when reloading MobiView.
		}
	}
}


#endif
