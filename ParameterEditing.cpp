#ifndef _MobiView_ParameterEditing_h_
#define _MobiView_ParameterEditing_h_

#include "MobiView.h"




ParameterCtrl::ParameterCtrl()
{
	CtrlLayout(*this);
};

String MakeIndexString(const std::vector<char *> &Indexes)
{
	String Result = "";
	if(Indexes.size() > 0)
	{
		Result << "(";
		int Idx = 0;
		for(char *Index : Indexes)
		{
			Result << "\"" << Index << "\"";
			if(Idx != Indexes.size()-1) Result << " ";
			++Idx;
		}
		Result << ")";
	}
	return Result;
}

String MakeIndexString(const std::vector<std::string> &Indexes)
{
	String Result = "";
	if(Indexes.size() > 0)
	{
		Result << "(";
		int Idx = 0;
		for(const std::string &Index : Indexes)
		{
			Result << "\"" << Index.data() << "\"";
			if(Idx != Indexes.size()-1) Result << " ";
			++Idx;
		}
		Result << ")";
	}
	return Result;
}

String MakeParameterIndexString(const indexed_parameter &Parameter)
{
	String Result = "";
	if(Parameter.Indexes.size() > 0)
	{
		Result << "(";
		int Idx = 0;
		for(const parameter_index &Index : Parameter.Indexes)
		{
			if(Index.Locked)
				Result << "<locked \"" << Index.IndexSetName.data() << "\">";               //TODO: This is a bit unclear as to what index set is locked
			else
				Result << "\"" << Index.Name.data() << "\"";
			if(Idx != Parameter.Indexes.size()-1) Result << " ";
			++Idx;
		}
		Result << ")";
	}
	return Result;
}

bool ParameterIsSubsetOf(const indexed_parameter &Parameter, const indexed_parameter &CompareTo)
{
	if(Parameter.Name == CompareTo.Name)
	{
		for(int Idx = 0; Idx < Parameter.Indexes.size(); ++Idx)
		{
			if(CompareTo.Indexes[Idx].Locked) continue;
			if(Parameter.Indexes[Idx].Locked) return false;
			if(CompareTo.Indexes[Idx].Name != Parameter.Indexes[Idx].Name) return false;
		}
		return true;
	}
	return false;
}

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
	//NOTE: This is not in use yet!
	
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
	}
	
	int ExpandedSet = -1;
	ExpandedSetLocal = -1;
	
	SecondExpandedSetLocal = -1;
	
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
	
		ParameterControls.Clear();
		CurrentParameterTypes.clear();
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
			Parameter.Name    = Name;
			Parameter.Type    = ParseParameterType(Type);
			
			if(!RefreshValuesOnly) Params.ParameterView.Add();
			
			RowData.Set("__index", ExpandedIndex);
			if(ExpandedSetLocal >= 0)
			{
				Indexes[ExpandedSetLocal] = ExpandedIndex;
				Parameter.Indexes[ExpandedSetLocal].Name = ExpandedIndex;
			}
		
			for(char *SecondExpandedIndex : SecondExpandedIndexSet)
			{
				Id ValueColumn = "__value";
				if(SecondExpandedSetLocal >= 0)
				{
					ValueColumn = SecondExpandedIndex;
					Indexes[SecondExpandedSetLocal] = SecondExpandedIndex;
					Parameter.Indexes[SecondExpandedSetLocal].Name = SecondExpandedIndex;
				}
				
				if(Parameter.Type == ParameterType_Double)
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
				else if(Parameter.Type == ParameterType_UInt)
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
				else if(Parameter.Type == ParameterType_Bool)
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
				else if(Parameter.Type == ParameterType_Time)
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
				else if(Parameter.Type == ParameterType_Enum)
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
			
			++Row;
		}
	}
}


void
MobiView::RecursiveUpdateParameter(int Level, std::vector<std::string> &CurrentIndexes, const indexed_parameter &Parameter, void *DataSet, Value Val)
{
	if(IsNull(Val)) return;
	
	if(Level == Parameter.Indexes.size())
	{
		//Do the actual update.
		size_t IndexCount = CurrentIndexes.size();
		std::vector<char *> Indexes(IndexCount);
		for(size_t Idx = 0; Idx < IndexCount; ++Idx)
		{
			Indexes[Idx] = (char *)CurrentIndexes[Idx].data();
		}
		
		const std::string &Name = Parameter.Name;
		parameter_type Type = Parameter.Type;
		
		switch(Type)
		{
			case ParameterType_Double:
			{
				ModelDll.SetParameterDouble(DataSet, Name.data(), Indexes.data(), Indexes.size(), (double)Val);
			} break;
			
			case ParameterType_UInt:
			{
				ModelDll.SetParameterUInt(DataSet, Name.data(), Indexes.data(), Indexes.size(), (uint64)(int64)Val);
			} break;
			
			case ParameterType_Bool:
			{
				ModelDll.SetParameterBool(DataSet, Name.data(), Indexes.data(), Indexes.size(), (bool)Val);
			} break;
			
			case ParameterType_Time:
			{
				Time D = (Time)Val;
				
				std::string V = Format(D, true).ToStd();
				if(V.size() > 0)    // Seems like D.IsValid() and !IsNull(D)  don't work correctly, so we do this instead.
					ModelDll.SetParameterTime(DataSet, Name.data(), Indexes.data(), Indexes.size(), V.data());
			} break;
			
			case ParameterType_Enum:
			{
				std::string V2 = Val.ToString().ToStd();
				ModelDll.SetParameterEnum(DataSet, Name.data(), Indexes.data(), Indexes.size(), V2.data());
			} break;
		}
		CheckDllUserError();
	}
	else
	{
		const char *IndexSetName = Parameter.Indexes[Level].IndexSetName.data();
		
		if(Parameter.Indexes[Level].Locked)
		{
			size_t IndexCount = ModelDll.GetIndexCount(DataSet, IndexSetName);
			std::vector<char *> IndexNames(IndexCount);
			ModelDll.GetIndexes(DataSet, IndexSetName, IndexNames.data());
			for(size_t Idx = 0; Idx < IndexCount; ++Idx)
			{
				CurrentIndexes[Level] = IndexNames[Idx];
				RecursiveUpdateParameter(Level + 1, CurrentIndexes, Parameter, DataSet, Val);
			}
		}
		else
		{
			CurrentIndexes[Level] = Parameter.Indexes[Level].Name;
			
			RecursiveUpdateParameter(Level + 1, CurrentIndexes, Parameter, DataSet, Val);
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
	
	if(IsNull(Val)) return;
	
	std::vector<std::string> CurrentIndexes(Parameter.Indexes.size());
	
	//TODO: Implement UpdateLocks
	if(UpdateLocks)
	{
		indexed_parameter Par2 = Parameter; //Ugh, causes a lot of vector and string copies, but we have to have the incoming one being const :(
		
		for(parameter_index &Index : Par2.Indexes)
		{
			size_t Id = IndexSetNameToId[Index.IndexSetName];
			Index.Locked = IndexLock[Id]->IsEnabled() && (bool)IndexLock[Id]->Get();
		}
		
		RecursiveUpdateParameter(0, CurrentIndexes, Par2, DataSet, Val);
	}
	else
		RecursiveUpdateParameter(0, CurrentIndexes, Parameter, DataSet, Val);
	
	if(DataSet == this->DataSet) //NOTE: Some sensitivty and calibration routines can operate on a copy of the data set.
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
