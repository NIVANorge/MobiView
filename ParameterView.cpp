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


int
MobiView::FindSelectedParameterRow()
{
	int SelectedRow = -1;
	for(int Row = 0; Row < Params.ParameterView.GetCount(); ++Row)
	{
		if(Params.ParameterView.IsSel(Row))
		{
			SelectedRow = Row;
			break;
		}
	}
	return SelectedRow;
}

bool
MobiView::GetSelectedParameterGroupIndexSets(std::vector<char *> &IndexSetsOut, String &GroupNameOut)
{
	Vector<int> Selected = ParameterGroupSelecter.GetSel();
	if(Selected.size() == 0) return false;
	
	if(!ModelDll.IsLoaded()) return false;
	
	//NOTE: Only one group can be selected at a time.
	GroupNameOut = ParameterGroupSelecter.Get(Selected[0]).ToString();
	std::string SelectedGroupName = GroupNameOut.ToStd();
	
	if(!ModelDll.IsParameterGroupName(DataSet, SelectedGroupName.data()))
		return false;
	
	uint64 IndexSetCount = ModelDll.GetParameterGroupIndexSetsCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return false;
	
	IndexSetsOut.resize(IndexSetCount);
	ModelDll.GetParameterGroupIndexSets(DataSet, SelectedGroupName.data(), IndexSetsOut.data());
	if (CheckDllUserError()) return false;
	
	return true;
}

indexed_parameter
MobiView::GetParameterAtRow(int Row)
{
	indexed_parameter Result = {};
	Result.Valid = false;
	
	if(Row == -1 || Row >= Params.ParameterView.GetCount())
		return Result;  //NOTE: In case the function is called incorrectly, don't hard crash.
	
	Result.Type = CurrentParameterTypes[Row];
	if(Result.Type == ParameterType_Bool) return Result;
	
	Result.Name = Params.ParameterView.Get(Row, Id("__name")).ToString().ToStd();
	
	//TODO: Is it necessary to look up the group here? Can't we just use the parameter name to
	//look up the index sets?
	
	String SelectedGroupName;
	std::vector<char *> IndexSetNames;
	bool Success = GetSelectedParameterGroupIndexSets(IndexSetNames, SelectedGroupName);
	if(!Success) return Result;
	
	int IdxIdx = 0;
	for(char * IndexSetName : IndexSetNames)
	{
		int IdxSetId = IndexSetNameToId[IndexSetName];

		parameter_index Idx = {};
		String IndexName;
		if(SecondExpandedSetLocal == IdxIdx)
		{
			//NOTE: This is a "little" hacky...
			for(int Col = 0; Col < Params.ParameterView.GetColumnCount(); ++Col)
			{
				LineEdit *Control = (LineEdit *)Params.ParameterView.GetCtrl(Row, Col);
				if(Control->HasFocus())
				{
					IndexName = Params.ParameterView.GetId(Col);
					break;
				}
			}
		}
		else if(ExpandedSetLocal == IdxIdx)
			IndexName = Params.ParameterView.Get(Row, Id("__index"));
		else
			IndexName = IndexList[IdxSetId]->Get();

		Idx.IndexSetName = std::string(IndexSetName);
		Idx.Name = IndexName.ToStd();
		Idx.Locked = IndexLock[IdxSetId]->IsEnabled() && ((bool)IndexLock[IdxSetId]->Get());
		Result.Indexes.push_back(Idx);
		
		++IdxIdx;
	}
	
	Result.Valid = true;
	
	return Result;
}


indexed_parameter
MobiView::GetSelectedParameter()
{
	//NOTE: This currently only works for non-bool parameters!
	
	indexed_parameter Result = {};
	Result.Valid = false;
	
	int Row = FindSelectedParameterRow();
	if(Row == -1) return Result;
	
	Result = GetParameterAtRow(Row);
	
	return Result;
}

void MobiView::RefreshParameterView(bool RefreshValuesOnly)
{
	if(!RefreshValuesOnly)
	{
		Params.ParameterView.Clear();
		Params.ParameterView.Reset();
		
		Params.ParameterView.NoVertGrid();
		
		ParameterControls.Clear();
		CurrentParameterTypes.clear();
	}
	
	int ExpandedSet = -1;
	ExpandedSetLocal = -1;
	
	SecondExpandedSetLocal = -1;
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
		if(IndexExpand[Idx]->Get()) ExpandedSet = Idx;
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		IndexExpand[Idx]->Enable();
		IndexList[Idx]->Disable();
		if(ExpandedSet != Idx) IndexLock[Idx]->Enable();
	}
	
	
	String SelectedGroupNameStr;
	std::vector<char *> IndexSetNames;
	bool Success = GetSelectedParameterGroupIndexSets(IndexSetNames, SelectedGroupNameStr);
	if(!Success) return;
	std::string SelectedGroupName = SelectedGroupNameStr.ToStd();
	
	std::vector<std::string> Indexes_String(IndexSetNames.size());
	std::vector<char *> Indexes(IndexSetNames.size());
	
	bool ExpandedSetIsActive = false;
	for(size_t Idx = 0; Idx < IndexSetNames.size(); ++Idx)
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
	if(IndexSetNames.size() >= 2 && (strcmp(IndexSetNames[IndexSetNames.size()-1], IndexSetNames[IndexSetNames.size()-2]) == 0))
		SecondExpandedSetLocal = IndexSetNames.size()-1;
	
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
		}
		else
		{
			for(char *IndexName : SecondExpandedIndexSet)
				Params.ParameterView.AddColumn(Id(IndexName), IndexName);
		}
		
		Params.ParameterView.AddColumn(Id("__min"), "Min");
		Params.ParameterView.AddColumn(Id("__max"), "Max");
		Params.ParameterView.AddColumn(Id("__unit"), "Unit");
		Params.ParameterView.AddColumn(Id("__description"), "Description");
		
		if(SecondExpandedSetLocal < 0)
		{
			//TODO: Since these are user-configurable, it would be better to store the previous sizing of these and reuse that.
			if(ExpandedSet >= 0)
				Params.ParameterView.ColumnWidths("20 12 12 10 10 10 26");
			else
				Params.ParameterView.ColumnWidths("20 12 10 10 10 38");
		}
		if(SecondExpandedSetLocal >= 0)
		{
			//NOTE Hide the min, max, unit fields. We still have to add them since we use the
			//info stored in them some other places.
			int TabBase = 1 + (int)(ExpandedSetLocal >= 0) + SecondExpandedIndexSet.size();
			Params.ParameterView.HeaderObject().HideTab(TabBase + 0);
			Params.ParameterView.HeaderObject().HideTab(TabBase + 1);
			Params.ParameterView.HeaderObject().HideTab(TabBase + 2);
			Params.ParameterView.HeaderObject().HideTab(TabBase + 3);
		}
	}
	
	indexed_parameter Parameter = {};
	if(!RefreshValuesOnly)
	{
		Parameter.Valid = true;
		Parameter.Indexes.resize(IndexSetNames.size());
		for(size_t Idx = 0; Idx < IndexSetNames.size(); ++Idx)
		{
			Parameter.Indexes[Idx].IndexSetName = IndexSetNames[Idx];
			Parameter.Indexes[Idx].Name = Indexes_String[Idx];
			//NOTE: We don't store info about it being locked here, since that has to be
			//overridden later anyway (the lock status can have changed since the table was
			//constructed.
			Parameter.Indexes[Idx].Locked = false;
		}
	}
		
	int Row = 0;
	int CtrlIdx = 0;
	
	Color RowColors[2] = {{255, 255, 255}, {240, 240, 255}};
	
	for(size_t Idx = 0; Idx < ParameterCount; ++Idx)
	{
		const char *Name = ParameterNames[Idx];
		const char *TypeName = ParameterTypes[Idx];
		
		ValueMap RowData;
		RowData.Set("__name", Name);
		const char *Unit = ModelDll.GetParameterUnit(DataSet, Name);
		if(Unit) RowData.Set("__unit", Unit);
		const char *Description = ModelDll.GetParameterDescription(DataSet, Name);
		if(Description) RowData.Set("__description", Description);
		
		int ExpandedIndexRow = 0;
		for(char *ExpandedIndex : ExpandedIndexSet)
		{
			parameter_type Type = ParseParameterType(TypeName);
			
			if(!RefreshValuesOnly)
			{
				Parameter.Name    = Name;
				Parameter.Type    = Type;
				
				Params.ParameterView.Add();
				
				if(ExpandedSetLocal >= 0)
					Parameter.Indexes[ExpandedSetLocal].Name = ExpandedIndex;
			}
			
			if(ExpandedSetLocal >= 0)
				Indexes[ExpandedSetLocal] = ExpandedIndex;
			
			RowData.Set("__index", ExpandedIndex);
			
			for(char *SecondExpandedIndex : SecondExpandedIndexSet)
			{
				Id ValueColumn = "__value";
				if(SecondExpandedSetLocal >= 0)
				{
					ValueColumn = SecondExpandedIndex;
					Indexes[SecondExpandedSetLocal] = SecondExpandedIndex;
					if(!RefreshValuesOnly)
						Parameter.Indexes[SecondExpandedSetLocal].Name = SecondExpandedIndex;
				}
				
				if(Type == ParameterType_Double)
				{
					RowData.Set(ValueColumn, ModelDll.GetParameterDouble(DataSet, Name, Indexes.data(), IndexSetNames.size()));
					double Min, Max;
					ModelDll.GetParameterDoubleMinMax(DataSet, Name, &Min, &Max);
					RowData.Set("__min", Min);
					RowData.Set("__max", Max);
					
					if(!RefreshValuesOnly)
					{
						ParameterControls.Create<EditDoubleNotNull>();
						
						ParameterControls[CtrlIdx].WhenAction = [Parameter, CtrlIdx, this]()
						{
							//NOTE: It is very important that we re-extract the value inside the
							//lambda of course!
							EditDoubleNotNull *ValueField = (EditDoubleNotNull *)&this->ParameterControls[CtrlIdx];
							double Val = *ValueField;
							ParameterEditAccepted(Parameter, this->DataSet, Val, true);
						};
					}
					CurrentParameterTypes.push_back(ParameterType_Double);
					
					if (CheckDllUserError()) return;
				}
				else if(Type == ParameterType_UInt)
				{
					//TODO: Converting to int potentially loses precision. However Value has no uint64
					//subtype
					RowData.Set(ValueColumn, (int64)ModelDll.GetParameterUInt(DataSet, Name, Indexes.data(), IndexSetNames.size()));
					uint64 Min, Max;
					ModelDll.GetParameterUIntMinMax(DataSet, Name, &Min, &Max);
					int64 M = (int64)Max;
					if(M < 0) M = INT64_MAX; //Stupid stopgap. We should do this better and actually display uint64's.
					RowData.Set("__min", (int64)Min);
					RowData.Set("__max", M);
					
					if(!RefreshValuesOnly)
					{
						ParameterControls.Create<EditInt64NotNullSpin>();
						EditInt64NotNullSpin *ctrl = (EditInt64NotNullSpin *)&ParameterControls[CtrlIdx];
						ctrl->Min(0);
						
						ParameterControls[CtrlIdx].WhenAction = [Parameter, CtrlIdx, this]()
						{
							EditInt64NotNullSpin *ValueField = (EditInt64NotNullSpin *)&this->ParameterControls[CtrlIdx];
							int64 Val = *ValueField;
							ParameterEditAccepted(Parameter, this->DataSet, Val, true);
						};
					}
					CurrentParameterTypes.push_back(ParameterType_UInt);
					
					if (CheckDllUserError()) return;
				}
				else if(Type == ParameterType_Bool)
				{
					RowData.Set(ValueColumn, ModelDll.GetParameterBool(DataSet, Name, Indexes.data(), IndexSetNames.size()));
					if(CheckDllUserError()) return;
					
					if(!RefreshValuesOnly)
					{
						ParameterControls.Create<Option>();
						
						ParameterControls[CtrlIdx].WhenAction = [Parameter, CtrlIdx, this]()
						{
							Option *ValueField = (Option *)&this->ParameterControls[CtrlIdx];
							bool Val = (bool)ValueField->Get();
							ParameterEditAccepted(Parameter, this->DataSet, Val, true);
						};
					}
					CurrentParameterTypes.push_back(ParameterType_Bool);
				}
				else if(Type == ParameterType_Time)
				{
					char TimeVal[256];
					ModelDll.GetParameterTime(DataSet, Name, Indexes.data(), IndexSetNames.size(), TimeVal);
					if(CheckDllUserError()) return;
					
					Time D;
					StrToTime(D, TimeVal); //Error handling? But should not be necessary.
					RowData.Set(ValueColumn, D);
					
					if(!RefreshValuesOnly)
					{
						ParameterControls.Create<EditTimeNotNull>();
						
						ParameterControls[CtrlIdx].WhenAction = [Parameter, CtrlIdx, this]()
						{
							EditTimeNotNull *ValueField = (EditTimeNotNull *)&this->ParameterControls[CtrlIdx];
							Time Val = ValueField->GetData();
							ParameterEditAccepted(Parameter, this->DataSet, Val, true);
						};
					}
					CurrentParameterTypes.push_back(ParameterType_Time);
					
				}
				else if(Type == ParameterType_Enum)
				{
					const char *Val = ModelDll.GetParameterEnum(DataSet, Name, Indexes.data(), IndexSetNames.size());
					if(CheckDllUserError()) return;
					RowData.Set(ValueColumn, String(Val));
					
					if(!RefreshValuesOnly)
					{
						ParameterControls.Create<DropList>();
						
						DropList *EnumList = (DropList *)&ParameterControls.Top();
						uint64 EnumCount = ModelDll.GetEnumValuesCount(DataSet, Name);
						std::vector<const char *> EnumNames(EnumCount);
						ModelDll.GetEnumValues(DataSet, Name, (char **)EnumNames.data());
						
						for(const char *Name : EnumNames)
							EnumList->Add(Name);
						
						ParameterControls[CtrlIdx].WhenAction = [Parameter, CtrlIdx, this]()
						{
							DropList *ValueField = (DropList *)&this->ParameterControls[CtrlIdx];
							String Val = ValueField->GetData();
							ParameterEditAccepted(Parameter, this->DataSet, Val, true);
						};
					}
					
					CurrentParameterTypes.push_back(ParameterType_Enum);
				}
				
				if(!RefreshValuesOnly)
					Params.ParameterView.SetCtrl(Row, Params.ParameterView.GetPos(ValueColumn), ParameterControls[CtrlIdx]);
				
				++CtrlIdx;
			}
			
			Params.ParameterView.SetMap(Row, RowData);
			
			if(!RefreshValuesOnly && ExpandedIndexRow > 0)   //NOTE: Don't display the same parameter name, unit, min, max, desc for each row.
			{
				//NOTE: It is annoying that we have to hard code the columns here....
				Params.ParameterView.SetDisplay(Row, 0, Params.NoDisplay);
				if(SecondExpandedSetLocal < 0)
					for(int Col = 3; Col <= 6; ++Col) Params.ParameterView.SetDisplay(Row, Col, Params.NoDisplay);
			}
			
			if(ExpandedSetLocal >= 0)
			{
				Color RowCol = RowColors[Idx % 2];
				Params.ParameterView.SetLineColor(Row, RowCol);
			}
			
			++Row;
			++ExpandedIndexRow;
		}
	}
}


void MobiView::ParameterEditAccepted(const indexed_parameter &Parameter, void *DataSet, Value Val, bool UpdateLocks)
{
	if(!Parameter.Valid)
	{
		Log(Format("Internal bug: Trying to set value of parameter that is flagged as invalid: %s", Parameter.Name.data()), true);
		return;
	}
	
	if(Parameter.Virtual)
	{
		Log(Format("Internal bug: Trying to set value of parameter that is flagged as virtual: %s", Parameter.Name.data()), true);
		return;
	}
	
	if(IsNull(Val)) return;
	
	
	if(UpdateLocks)
	{
		indexed_parameter Par2 = Parameter; //Ugh, causes a lot of vector and string copies, but we have to have the incoming one being const :(
		
		for(parameter_index &Index : Par2.Indexes)
		{
			size_t Id = IndexSetNameToId[Index.IndexSetName];
			Index.Locked = IndexLock[Id]->IsEnabled() && (bool)IndexLock[Id]->Get();
		}
		
		SetParameterValue(Par2, DataSet, Val, ModelDll);
	}
	else
		SetParameterValue(Parameter, DataSet, Val, ModelDll);
	
	CheckDllUserError();
	
	if(DataSet == this->DataSet)
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
		Log("Parameter saving may have been unsuccessful.");
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
			Log("Parameter saving may have been unsuccessful.");
		else
		{
			ParameterFile = NewFile;
			Log(String("Parameters saved to ") + ParameterFile.data());
			ParametersWereChangedSinceLastSave = false;
			
			StoreSettings(); //So that the current working file is now default when reloading MobiView.
		}
	}
}
