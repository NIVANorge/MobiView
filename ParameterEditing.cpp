#ifndef _MobiView_ParameterEditing_h_
#define _MobiView_ParameterEditing_h_

#include "MobiView.h"




ParameterCtrl::ParameterCtrl()
{
	CtrlLayout(*this);
};


void MobiView::ExpandIndexSetClicked(size_t IndexSet)
{
	bool Checked = IndexExpand[IndexSet]->Get();
	
	if(Checked)
	{
		IndexLock[IndexSet]->Disable();
		IndexList[IndexSet]->Disable();
		
		for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
		{
			if(IndexSet != Idx)
			{
				IndexExpand[Idx]->Set(0);
				IndexLock[Idx]->Enable();
				IndexList[Idx]->Enable();
			}
		}
	}
	else
	{
		IndexLock[IndexSet]->Enable();
		IndexList[IndexSet]->Enable();
	}
	
	RefreshParameterView();
}


void MobiView::RefreshParameterView(bool RefreshValuesOnly)
{
	if(!RefreshValuesOnly)
	{
		Params.ParameterView.Clear();
		Params.ParameterView.Reset();
	}
	
	int ExpandedSet = -1;
	int ExpandedSetLocal = -1;
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		if(IndexExpand[Idx]->Get()) ExpandedSet = Idx;
	}
	
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		IndexExpand[Idx]->Enable();
		IndexList[Idx]->Disable();
		if(ExpandedSet != Idx) IndexLock[Idx]->Enable();
	}
	
	
	
	Vector<int> Selected = ParameterGroupSelecter.GetSel();
	if(Selected.size() == 0 || Selected[0] == 0) return;
	
	Value SelectedGroup = ParameterGroupSelecter.Get(Selected[0]);
	std::string SelectedGroupName = SelectedGroup.ToString().ToStd();

	if(SelectedGroupName.empty()) return;
	
	if(!ModelDll.IsParameterGroupName(DataSet, SelectedGroupName.data())) return;
	
	uint64 IndexSetCount = ModelDll.GetParameterGroupIndexSetsCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> IndexSetNames(IndexSetCount);
	ModelDll.GetParameterGroupIndexSets(DataSet, SelectedGroupName.data(), IndexSetNames.data());
	
	
	
	std::vector<std::string> Indexes_String(IndexSetCount);
	std::vector<char *> Indexes(IndexSetCount);
	
	bool ExpandedSetIsActive = false;
	for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
	{
		size_t Id = IndexSetNameToId[IndexSetNames[Idx]];
		if(Id != ExpandedSet)  IndexList[Id]->Enable();
		else
		{
			ExpandedSetIsActive = true;
			ExpandedSetLocal = Idx;
		}
		
		Indexes_String[Idx] = IndexList[Id]->Get().ToString().ToStd();
		Indexes[Idx] = (char *)Indexes_String[Idx].data();
	}
	if(!ExpandedSetIsActive)
		ExpandedSet = -1;
	
	
	
	
	uint64 ParameterCount = ModelDll.GetAllParametersCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> ParameterNames(ParameterCount);
	std::vector<char *> ParameterTypes(ParameterCount);
	ModelDll.GetAllParameters(DataSet, ParameterNames.data(), ParameterTypes.data(), SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> ExpandedIndexSet;
	if(ExpandedSet >= 0)
	{
		char *ExpandedSetName = IndexSetNames[ExpandedSetLocal];
		
		uint64 ExpandedIndexesCount = ModelDll.GetIndexCount(DataSet, ExpandedSetName);
		ExpandedIndexSet.resize(ExpandedIndexesCount);
		ModelDll.GetIndexes(DataSet, ExpandedSetName, ExpandedIndexSet.data());
		
		if(CheckDllUserError()) return;
	}
	else
		ExpandedIndexSet.push_back((char *)"dummy");
	
	
	//NOTE: If the last two index sets are the same, display this as a row
	int SecondExpandedSetLocal = -1;
	if(IndexSetNames.size() >= 2 && (strcmp(IndexSetNames[IndexSetCount-1], IndexSetNames[IndexSetCount-2]) == 0))
		SecondExpandedSetLocal = IndexSetCount-1;
	
	if(ExpandedSetLocal == SecondExpandedSetLocal) ExpandedSetLocal--;  //NOTE: Ensure that we don't expand it twice in the same position.
	
	
	std::vector<char *> SecondExpandedIndexSet;
	if(SecondExpandedSetLocal >= 0)
	{
		char *ExpandedSetName = IndexSetNames[SecondExpandedSetLocal];
		
		uint64 ExpandedIndexesCount = ModelDll.GetIndexCount(DataSet, ExpandedSetName);
		SecondExpandedIndexSet.resize(ExpandedIndexesCount);
		ModelDll.GetIndexes(DataSet, ExpandedSetName, SecondExpandedIndexSet.data());
		
		if(CheckDllUserError()) return;
	}
	else
		SecondExpandedIndexSet.push_back((char *)"dummy");
	
	
	if(!RefreshValuesOnly)
	{
		Params.ParameterView.AddColumn(Id("__name"), "Name");
		
		if(ExpandedSetLocal >= 0)
		{
			char *ExpandedSetName = IndexSetNames[ExpandedSetLocal];
			Params.ParameterView.AddColumn(Id("__index"), ExpandedSetName);
		}
		
		if(SecondExpandedSetLocal < 0)
		{
			// Otherwise regular editing of just one value
			Params.ParameterView.AddColumn(Id("__value"), "Value");
			Params.ParameterView.AddColumn(Id("__min"), "Min");
			Params.ParameterView.AddColumn(Id("__max"), "Max");
			Params.ParameterView.AddColumn(Id("__unit"), "Unit");
			Params.ParameterView.AddColumn(Id("__description"), "Description");
			
			//TODO: Since these are user-configurable, it would be better to store the previous sizing of these and reuse that.
			if(ExpandedSet >= 0)
				Params.ParameterView.ColumnWidths("20 12 12 10 10 10 26");
			else
				Params.ParameterView.ColumnWidths("20 12 10 10 10 38");
		}
		else
		{
			for(char *IndexName : SecondExpandedIndexSet)
				Params.ParameterView.AddColumn(Id(IndexName), IndexName);
		}
	
		ParameterControls.Clear();
		CurrentParameterTypes.clear();
	}
		
	int Row = 0;
		
	for(size_t Idx = 0; Idx < ParameterCount; ++Idx)
	{
		const char *Name = ParameterNames[Idx];
		const char *Type = ParameterTypes[Idx];
		
		ValueMap RowData;
		RowData.Set("__name", Name);
		const char *Unit = ModelDll.GetParameterUnit(DataSet, Name);
		if(Unit) RowData.Set("__unit", Unit);
		const char *Description = ModelDll.GetParameterDescription(DataSet, Name);
		if(Description) RowData.Set("__description", Description);
		
		for(char *ExpandedIndex : ExpandedIndexSet)
		{
			if(!RefreshValuesOnly) Params.ParameterView.Add();
			
			RowData.Set("__index", ExpandedIndex);
			if(ExpandedSetLocal >= 0) Indexes[ExpandedSetLocal] = ExpandedIndex;
		
			for(char *SecondExpandedIndex : SecondExpandedIndexSet)
			{
				Id ValueColumn = "__value";
				if(SecondExpandedSetLocal >= 0)
				{
					ValueColumn = SecondExpandedIndex;
					Indexes[SecondExpandedSetLocal] = SecondExpandedIndex;
				}
				
				if(strcmp(Type, "double") == 0)
				{
					RowData.Set(ValueColumn, ModelDll.GetParameterDouble(DataSet, Name, Indexes.data(), IndexSetCount));
					double Min, Max;
					ModelDll.GetParameterDoubleMinMax(DataSet, Name, &Min, &Max);
					RowData.Set("__min", Min);
					RowData.Set("__max", Max);
					
					if(!RefreshValuesOnly) ParameterControls.Create<EditDoubleNotNull>();
					CurrentParameterTypes.push_back(ParameterType_Double);
					
					if (CheckDllUserError()) return;
				}
				else if(strcmp(Type, "uint") == 0)
				{
					//TODO: Converting to int potentially loses precision. However Value has no uint64
					//subtype
					RowData.Set(ValueColumn, (int64)ModelDll.GetParameterUInt(DataSet, Name, Indexes.data(), IndexSetCount));
					uint64 Min, Max;
					ModelDll.GetParameterUIntMinMax(DataSet, Name, &Min, &Max);
					int64 M = (int64)Max;
					if(M < 0) M = INT64_MAX; //Stupid stopgap. We should do this better and actually display uint64's.
					RowData.Set("__min", (int64)Min);
					RowData.Set("__max", M);
					
					if(!RefreshValuesOnly) ParameterControls.Create<EditInt64NotNull>();
					CurrentParameterTypes.push_back(ParameterType_UInt);
					
					if (CheckDllUserError()) return;
				}
				else if(strcmp(Type, "bool") == 0)
				{
					RowData.Set(ValueColumn, ModelDll.GetParameterBool(DataSet, Name, Indexes.data(), IndexSetCount));
					if(CheckDllUserError()) return;
					
					if(!RefreshValuesOnly) ParameterControls.Create<Option>();
					CurrentParameterTypes.push_back(ParameterType_Bool);
				}
				else if(strcmp(Type, "time") == 0)
				{
					char TimeVal[256];
					ModelDll.GetParameterTime(DataSet, Name, Indexes.data(), IndexSetCount, TimeVal);
	
					Time D;
					StrToTime(D, TimeVal); //Error handling? But should not be necessary.
					RowData.Set(ValueColumn, D);
					
					if(!RefreshValuesOnly) ParameterControls.Create<EditTimeNotNull>();
					CurrentParameterTypes.push_back(ParameterType_Time);
					if(CheckDllUserError()) return;
				}
				else if(strcmp(Type, "enum") == 0)
				{
					const char *Val = ModelDll.GetParameterEnum(DataSet, Name, Indexes.data(), IndexSetCount);
					String Val2 = Val;
					RowData.Set(ValueColumn, Val);
					
					if(!RefreshValuesOnly)
					{
						ParameterControls.Create<DropList>();
						
						DropList *EnumList = (DropList *)&ParameterControls.Top();
						uint64 EnumCount = ModelDll.GetEnumValuesCount(DataSet, Name);
						std::vector<const char *> EnumNames(EnumCount);
						ModelDll.GetEnumValues(DataSet, Name, (char **)EnumNames.data());
						
						for(const char *Name : EnumNames)
							EnumList->Add(Name);
					}
					
					CurrentParameterTypes.push_back(ParameterType_Enum);
					if(CheckDllUserError()) return;
				}
				
				if(!RefreshValuesOnly)
				{
					Params.ParameterView.SetCtrl(Row, Params.ParameterView.GetPos(ValueColumn), ParameterControls.Top());
					ParameterControls.Top().WhenAction = [=]() { ParameterEditAccepted(Row, ValueColumn, ExpandedSetLocal, SecondExpandedSetLocal); };
				}
			}
			
			Params.ParameterView.SetMap(Row, RowData);
			
			++Row;
		}
	}
}


void MobiView::RecursiveUpdateParameter(std::vector<char *> &IndexSetNames, int Level, std::vector<std::string> &CurrentIndexes, int Row, Id ValueColumn, int ExpandedSetLocal, int SecondExpandedSetLocal)
{
	if(Level == IndexSetNames.size())
	{
		//Do the actual update.
		size_t IndexCount = CurrentIndexes.size();
		std::vector<char *> Indexes(IndexCount);
		for(size_t Idx = 0; Idx < IndexCount; ++Idx)
		{
			Indexes[Idx] = (char *)CurrentIndexes[Idx].data();
		}
		
		std::string Name = Params.ParameterView.Get(Row, Id("__name")).ToString().ToStd();
		parameter_type Type = CurrentParameterTypes[Row];
		
		int Col = Params.ParameterView.GetPos(ValueColumn);
		
		switch(Type)
		{
			case ParameterType_Double:
			{
				double V = Params.ParameterView.Get(Row, ValueColumn);
				if(!IsNull(V))
					ModelDll.SetParameterDouble(DataSet, Name.data(), Indexes.data(), Indexes.size(), V);
			} break;
			
			case ParameterType_UInt:
			{
				int64 V = Params.ParameterView.Get(Row, ValueColumn);
				if(!IsNull(V))
					ModelDll.SetParameterUInt(DataSet, Name.data(), Indexes.data(), Indexes.size(), (uint64)V);
			} break;
			
			case ParameterType_Bool:
			{
				Ctrl *ctrl = Params.ParameterView.GetCtrl(Row, Col);
				bool V = (bool)((Option*)ctrl)->Get();
				ModelDll.SetParameterBool(DataSet, Name.data(), Indexes.data(), Indexes.size(), V);
			} break;
			
			case ParameterType_Time:
			{
				EditTimeNotNull *ctrl = (EditTimeNotNull*)Params.ParameterView.GetCtrl(Row, Col);
				Time D = ctrl->GetData();
				std::string V = Format(D, true).ToStd();
				if(V.size() > 0)    // Seems like D.IsValid() and !IsNull(D)  don't work correctly, so we do this instead.
					ModelDll.SetParameterTime(DataSet, Name.data(), Indexes.data(), Indexes.size(), V.data());
			} break;
			
			case ParameterType_Enum:
			{
				DropList *ctrl = (DropList *)Params.ParameterView.GetCtrl(Row, Col);
				String V = ctrl->GetData();
				std::string V2 = V.ToStd();
				ModelDll.SetParameterEnum(DataSet, Name.data(), Indexes.data(), Indexes.size(), V2.data());
			} break;
		}
		CheckDllUserError();
	}
	else
	{
		const char *IndexSetName = IndexSetNames[Level];
		size_t Id = IndexSetNameToId[IndexSetName];
		
		if(IndexLock[Id]->IsEnabled() && IndexLock[Id]->Get())
		{
			size_t IndexCount = ModelDll.GetIndexCount(DataSet, IndexSetName);
			std::vector<char *> IndexNames(IndexCount);
			ModelDll.GetIndexes(DataSet, IndexSetName, IndexNames.data());
			for(size_t Idx = 0; Idx < IndexCount; ++Idx)
			{
				CurrentIndexes[Level] = IndexNames[Idx];
				RecursiveUpdateParameter(IndexSetNames, Level + 1, CurrentIndexes, Row, ValueColumn, ExpandedSetLocal, SecondExpandedSetLocal);
			}
		}
		else
		{
			if(SecondExpandedSetLocal >= 0 && SecondExpandedSetLocal == Level)
				CurrentIndexes[Level] = ValueColumn.ToString().ToStd();
			else if(ExpandedSetLocal >= 0 && ExpandedSetLocal == Level)
				CurrentIndexes[Level] = Params.ParameterView.Get(Row, "__index").ToString().ToStd();
			else
				CurrentIndexes[Level] = IndexList[Id]->Get().ToString().ToStd();
			
			RecursiveUpdateParameter(IndexSetNames, Level + 1, CurrentIndexes, Row, ValueColumn, ExpandedSetLocal, SecondExpandedSetLocal);
		}
	}
}


void MobiView::ParameterEditAccepted(int Row, Id ValueColumn, int ExpandedSetLocal, int SecondExpandedSetLocal)
{
	//TODO: High degree of copypaste from above. Factor this out.
	Vector<int> Selected = ParameterGroupSelecter.GetSel();
	if(Selected.size() == 0) return;
	
	//NOTE: Only one group can be selected at a time.
	Value SelectedGroup = ParameterGroupSelecter.Get(Selected[0]);
	std::string SelectedGroupName = SelectedGroup.ToString().ToStd();
	
	uint64 IndexSetCount = ModelDll.GetParameterGroupIndexSetsCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> IndexSetNames(IndexSetCount);
	ModelDll.GetParameterGroupIndexSets(DataSet, SelectedGroupName.data(), IndexSetNames.data());
	if (CheckDllUserError()) return;
	
	std::vector<std::string> CurrentIndexes(IndexSetCount);
	RecursiveUpdateParameter(IndexSetNames, 0, CurrentIndexes, Row, ValueColumn, ExpandedSetLocal, SecondExpandedSetLocal);
	
	ParametersWereChangedSinceLastSave = true;
}


void MobiView::SaveParameters()
{
	if(!ModelDll.IsLoaded() || !ModelDll.WriteParametersToFile || !ParameterFile.size())
	{
		Log("Parameters can only be saved once a model and parameter file is loaded");
		return;
	}
	
	//TODO: Mechanism for determining if there has actually been edits that need to be saved?
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
