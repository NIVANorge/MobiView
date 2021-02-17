#include "MobiView.h"


#define IMAGECLASS IconImg5
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>


ChangeIndexesWindow::ChangeIndexesWindow()
{
	CtrlLayout(*this, "Edit index sets");

	Zoomable().Sizeable();

	FinishEdit.WhenAction = THISBACK(DoIndexUpdate);
	
	Branches.OtherParent = this;
	
	Add(Branches.HSizePos(5, 205).VSizePos(5, 205));
	
	SelectBranchedSet.WhenAction = THISBACK(SelectedBranchListUpdate);
	
	PushAddIndex.WhenAction = THISBACK(AddIndexPushed);
	PushAddIndex.SetImage(IconImg5::Add());
	
	PushDeleteIndex.WhenAction = THISBACK(DeleteSelectedPushed);
	PushDeleteIndex.SetImage(IconImg5::Remove());
	
	
	IndexSetName[0] = &IndexSetName1;
	IndexSetName[1] = &IndexSetName2;
	IndexSetName[2] = &IndexSetName3;
	IndexSetName[3] = &IndexSetName4;
	IndexSetName[4] = &IndexSetName5;
	IndexSetName[5] = &IndexSetName6;
	
	IndexList[0]    = &IndexList1;
	IndexList[1]    = &IndexList2;
	IndexList[2]    = &IndexList3;
	IndexList[3]    = &IndexList4;
	IndexList[4]    = &IndexList5;
	IndexList[5]    = &IndexList6;
	
	BranchList[0]    = &BranchList1;
	BranchList[1]    = &BranchList2;
	BranchList[2]    = &BranchList3;
	BranchList[3]    = &BranchList4;
	BranchList[4]    = &BranchList5;
	BranchList[5]    = &BranchList6;
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		IndexList[Idx]->SetFont(Courier(11));
	}
}

void ChangeIndexesWindow::RefreshData()
{
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		IndexSetName[Idx]->Hide();
		IndexList[Idx]->Hide();
		IndexList[Idx]->Disable();
		BranchList[Idx]->Hide();
		BranchList[Idx]->Disable();
	}
	
	void *DataSet = ParentWindow->DataSet;
	model_dll_interface &ModelDll = ParentWindow->ModelDll;
	
	uint64 IndexSetCount = ModelDll.GetIndexSetsCount(DataSet);
	std::vector<char *> IndexSets(IndexSetCount);
	std::vector<char *> IndexSetTypes(IndexSetCount);
	ModelDll.GetIndexSets(DataSet, IndexSets.data(), IndexSetTypes.data());
	if (ParentWindow->CheckDllUserError()) return;
	
	SelectBranchedSet.Clear();
	
	BranchControls.Clear();
	NameControls.Clear();
	
	size_t Placement = 0;
	size_t BranchedPlacement = 0;
	for(size_t IndexSet = 0; IndexSet < IndexSetCount; ++IndexSet)
	{
		const char *Name = IndexSets[IndexSet];
		
		if(strcmp(IndexSetTypes[IndexSet], "branched")==0)
		{
			SelectBranchedSet.Add(Name);
			
			//BranchedSetNameToId[Name] = BranchedPlacement;
			
			BranchList[BranchedPlacement]->Reset();
			BranchList[BranchedPlacement]->AddColumn("Id");
			BranchList[BranchedPlacement]->AddColumn("Name");
			BranchList[BranchedPlacement]->AddColumn("Input ids");
			BranchList[BranchedPlacement]->ColumnWidths("15 45 40");
			
			BranchList[BranchedPlacement]->Clear();
		
			uint64 IndexCount = ModelDll.GetIndexCount(DataSet, Name);
			if (ParentWindow->CheckDllUserError()) return;
			std::vector<char *> IndexNames(IndexCount);
			ModelDll.GetIndexes(DataSet, Name, IndexNames.data());
			if (ParentWindow->CheckDllUserError()) return;
			
			std::map<std::string, int> IndexNameToId;
			
			BranchControls.Create<Array<Ctrl>>();
			NameControls.Create<Array<Ctrl>>();
			
			for(int Idx = 0; Idx < IndexCount; ++Idx)
			{
				IndexNameToId[IndexNames[Idx]] = Idx;
				
				String BranchListStr;
				std::vector<char *> BranchNameList;
				uint64 BranchCount = ModelDll.GetBranchInputsCount(DataSet, Name, IndexNames[Idx]);
				BranchNameList.resize(BranchCount);
				ModelDll.GetBranchInputs(DataSet, Name, IndexNames[Idx], BranchNameList.data());
				int Count = 0;
				for(char *BranchName : BranchNameList)
				{
					if(Count > 0) BranchListStr += ",";
					BranchListStr += Format("%d", IndexNameToId[BranchName]);
					++Count;
				}
				
				BranchList[BranchedPlacement]->Add(Idx, IndexNames[Idx], BranchListStr);
				
				
				BranchControls.Top().Create<EditString>();
				BranchList[BranchedPlacement]->SetCtrl(Idx, 2, BranchControls.Top().Top());
				BranchControls.Top().Top().WhenAction = [this]() { this->Branches.Refresh(); };
				
				NameControls.Top().Create<EditString>();
				BranchList[BranchedPlacement]->SetCtrl(Idx, 1, NameControls.Top().Top());
				NameControls.Top().Top().WhenAction = [this]() { this->Branches.Refresh(); };
				
			}
			
			++BranchedPlacement;
		}
		else
		{
			IndexSetName[Placement]->SetText(Name);
			IndexSetName[Placement]->Show();

			uint64 IndexCount = ModelDll.GetIndexCount(DataSet, Name);
			if (ParentWindow->CheckDllUserError()) return;
			std::vector<char *> IndexNames(IndexCount);
			ModelDll.GetIndexes(DataSet, Name, IndexNames.data());
			if (ParentWindow->CheckDllUserError()) return;
			
			IndexList[Placement]->Clear();
			for(const char *IndexName : IndexNames)
			{
				IndexList[Placement]->Append(IndexName, CHARSET_UTF8);
				IndexList[Placement]->Append("\n", CHARSET_UTF8);
			}
		
			IndexList[Placement]->Show();
			IndexList[Placement]->Enable();
			
			++Placement;
		}
	}
	
	SelectBranchedSet.GoBegin();
	
	if(SelectBranchedSet.GetCount() == 0)
	{
		PushAddIndex.Disable();
		PushDeleteIndex.Disable();
	}
	else
	{
		PushAddIndex.Enable();
		PushDeleteIndex.Enable();
	}

	SelectedBranchListUpdate();
}


bool HasIndex(model_dll_interface &ModelDll, void *DataSet, char *IndexSetName, char *IndexName)
{
	uint64 IndexesCount = ModelDll.GetIndexCount(DataSet, IndexSetName);
	std::vector<char *> Indexes(IndexesCount);
	ModelDll.GetIndexes(DataSet, IndexSetName, Indexes.data());
	for(char *Index : Indexes)
	{
		if(strcmp(IndexName, Index)==0) return true;
	}
	return false;
}

void RecursiveTransferParameterData(model_dll_interface &ModelDll, void *OldDataSet, void *NewDataSet, char *ParameterName, char *ParameterType, std::vector<char *> &IndexSetNames, std::vector<char *> &IndexNames, int Level)
{
	if(Level < IndexSetNames.size())
	{
		uint64 NewIndexesCount = ModelDll.GetIndexCount(NewDataSet, IndexSetNames[Level]);
		std::vector<char *> NewIndexes(NewIndexesCount);
		ModelDll.GetIndexes(NewDataSet, IndexSetNames[Level], NewIndexes.data());
		
		for(char *NewIndex : NewIndexes)
		{
			IndexNames[Level] = NewIndex;
			RecursiveTransferParameterData(ModelDll, OldDataSet, NewDataSet, ParameterName, ParameterType, IndexSetNames, IndexNames, Level+1);
		}
	}
	else
	{
		bool FoundMatchingOldIndexes = true;
		for(size_t Idx = 0; Idx < IndexSetNames.size(); ++Idx)
		{
			if(!HasIndex(ModelDll, OldDataSet, IndexSetNames[Idx], IndexNames[Idx]))
			{
				FoundMatchingOldIndexes = false;
				break;
			}
		}
		
		if(FoundMatchingOldIndexes)
		{
			if(strcmp(ParameterType, "double")==0)
			{
				double Value = ModelDll.GetParameterDouble(OldDataSet, ParameterName, IndexNames.data(), IndexNames.size());
				ModelDll.SetParameterDouble(NewDataSet, ParameterName, IndexNames.data(), IndexNames.size(), Value);
			}
			else if(strcmp(ParameterType, "uint")==0)
			{
				uint64 Value = ModelDll.GetParameterUInt(OldDataSet, ParameterName, IndexNames.data(), IndexNames.size());
				ModelDll.SetParameterUInt(NewDataSet, ParameterName, IndexNames.data(), IndexNames.size(), Value);
			}
			else if(strcmp(ParameterType, "bool")==0)
			{
				bool Value = ModelDll.GetParameterBool(OldDataSet, ParameterName, IndexNames.data(), IndexNames.size());
				ModelDll.SetParameterBool(NewDataSet, ParameterName, IndexNames.data(), IndexNames.size(), Value);
			}
			else if(strcmp(ParameterType, "time")==0)
			{
				char TimeVal[256];
				ModelDll.GetParameterTime(OldDataSet, ParameterName, IndexNames.data(), IndexNames.size(), TimeVal);
				ModelDll.SetParameterTime(NewDataSet, ParameterName, IndexNames.data(), IndexNames.size(), TimeVal);
			}
		}
	}
}



void ChangeIndexesWindow::DoIndexUpdate()
{
	void *OldDataSet = ParentWindow->DataSet;
	model_dll_interface &ModelDll = ParentWindow->ModelDll;
	
	void *NewDataSet = ModelDll.SetupModelBlankIndexSets(ParentWindow->InputFile.data());
	ParentWindow->DataSet = NewDataSet;
	
	//Basic index sets
	for(size_t IndexSet = 0; IndexSet < MAX_INDEX_SETS; ++IndexSet)
	{
		String Name0 = IndexSetName[IndexSet]->GetText();
		std::string Name1 = Name0.ToStd();
		const char *Name = Name1.data();
		if(!IndexList[IndexSet]->IsEnabled()) break;
		
		int LineCount = IndexList[IndexSet]->GetLineCount();
		
		std::vector<std::string> Lines; //NOTE: Need intermediate storage in a std::string to assure persistence until end of SetIndexes call.
		Lines.reserve(LineCount);
		for(int Line = 0; Line < LineCount; ++Line)
		{
			std::string RawLine = IndexList[IndexSet]->GetUtf8Line(Line).ToStd();
			int EmptyCount = std::count_if(RawLine.begin(), RawLine.end(), [](unsigned char c){ return std::isspace(c); });
			if(RawLine.size() != 0 && EmptyCount != RawLine.size())
			{
				Lines.push_back(RawLine);
			}
		}
		
		std::vector<char *> Indexes;
		for(int Line = 0; Line < Lines.size(); ++Line)
		{
			Indexes.push_back((char *)Lines[Line].data());
		}
		ModelDll.SetIndexes(NewDataSet, Name, Indexes.size(), (char**)Indexes.data());
	}
	
	bool Error = false;
	//Branched index sets
	for(size_t IndexSet = 0; IndexSet < SelectBranchedSet.GetCount(); ++IndexSet)
	{
		String Name0 = SelectBranchedSet.GetValue(IndexSet);
		std::string Name1 = Name0.ToStd();
		const char *Name = Name1.data();
		
		ArrayCtrl *CurrentBranchList = BranchList[IndexSet];
		
		int ErrorLine = -1;
		
		int IndexCount = CurrentBranchList->GetCount();
		std::vector<std::string> IndexNames(IndexCount);  //Have to store temporary data here so that it is not free'd before it is passed on
		std::vector<dll_branch_index> IndexData(IndexCount);
		std::vector<std::vector<char *>> Branches(IndexCount);
		
		for(int Idx = 0; Idx < IndexCount; ++Idx)
		{
			String IndexName0 = CurrentBranchList->Get(Idx, 1);
			IndexNames[Idx] = IndexName0.ToStd();
			
			IndexData[Idx].IndexName = IndexNames[Idx].data();
			
			String BranchListStr = CurrentBranchList->Get(Idx,2);
			std::vector<int> Vals;
			bool Success = ParseIntList(BranchListStr, Vals, Idx);
			if(!Success) ErrorLine = Idx;
			
			for(int Val : Vals)
			{
				Branches[Idx].push_back((char *)IndexNames[Val].data());
			}
			IndexData[Idx].BranchCount = Branches[Idx].size();
			IndexData[Idx].BranchNames = Branches[Idx].data();
		}
		
		if(ErrorLine >= 0)
		{
			PromptOK(Format("Error in index set %s at line %d : The input ids has to be a comma-separated list of numbers smaller than the line id", Name, ErrorLine));
			Error = true;
			break;
		}
		
		ModelDll.SetBranchIndexes(NewDataSet, Name, IndexData.size(), IndexData.data());
	}
		
	if(ParentWindow->CheckDllUserError() || Error)
	{
		ParentWindow->DataSet = OldDataSet;
		ModelDll.DeleteModelAndDataSet(NewDataSet);
		//Close();     //We don't want to close? Instead allow the user to fix the error
		return;
	}

	uint64 GroupCount = ModelDll.GetAllParameterGroupsCount(NewDataSet, "__all!!__");
	std::vector<char *> GroupNames(GroupCount);
	ModelDll.GetAllParameterGroups(NewDataSet, GroupNames.data(), "__all!!__");
	
	//TODO: Could do error handling, but it should be unnecessary at this point.
	
	for(char *GroupName : GroupNames)
	{
		uint64 IndexSetCount = ModelDll.GetParameterGroupIndexSetsCount(NewDataSet, GroupName);
		std::vector<char *> IndexSetNames(IndexSetCount);
		ModelDll.GetParameterGroupIndexSets(NewDataSet, GroupName, IndexSetNames.data());
		
		uint64 ParameterCount = ModelDll.GetAllParametersCount(NewDataSet, GroupName);
		std::vector<char *> ParameterNames(ParameterCount);
		std::vector<char *> ParameterTypes(ParameterCount);
		ModelDll.GetAllParameters(NewDataSet, ParameterNames.data(), ParameterTypes.data(), GroupName);
		
		for(int Idx = 0; Idx < ParameterCount; ++Idx)
		{
			char *ParameterName = ParameterNames[Idx];
			char *ParameterType = ParameterTypes[Idx];
			std::vector<char *> IndexNames(IndexSetNames.size());
			RecursiveTransferParameterData(ModelDll, OldDataSet, NewDataSet, ParameterName, ParameterType, IndexSetNames, IndexNames, 0);
		}
	}
	
	ModelDll.ReadInputs(NewDataSet, ParentWindow->InputFile.data());
	
	ParentWindow->CleanInterface();
	
	if(ParentWindow->BaselineDataSet)
	{
		ModelDll.DeleteDataSet(ParentWindow->BaselineDataSet);
		ParentWindow->BaselineDataSet = nullptr;
	}
	ModelDll.DeleteModelAndDataSet(OldDataSet);

	ParentWindow->ParametersWereChangedSinceLastSave = true;
	
	ParentWindow->BuildInterface();
	
	Close();
}


void ChangeIndexesWindow::SelectedBranchListUpdate()
{
	if(SelectBranchedSet.GetCount() > 0)
	{
		int Select = SelectBranchedSet.GetIndex();
		
		for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
		{
			BranchList[Idx]->Hide();
			BranchList[Idx]->Disable();
		}
		
		BranchList[Select]->Show();
		BranchList[Select]->Enable();
		
		Branches.Refresh();
	}
}

void ChangeIndexesWindow::AddIndexPushed()
{
	if(SelectBranchedSet.GetCount() > 0)
	{
		int Select = SelectBranchedSet.GetIndex();
		int Idx = BranchList[Select]->GetCount();
		
		String Name = Format("new %d", Idx);
		
		BranchList[Select]->Add(Idx, Name, "");
							
		BranchControls.Top().Create<EditString>();
		BranchList[Select]->SetCtrl(Idx, 2, BranchControls.Top().Top());
		BranchControls.Top().Top().WhenAction = [this]() { this->Branches.Refresh(); };
		
		NameControls.Top().Create<EditString>();
		BranchList[Select]->SetCtrl(Idx, 1, NameControls.Top().Top());
		NameControls.Top().Top().WhenAction = [this]() { this->Branches.Refresh(); };
	}
	
	Branches.Refresh();
}

void ChangeIndexesWindow::DeleteSelectedPushed()
{
	if(SelectBranchedSet.GetCount() > 0)
	{
		int Select = SelectBranchedSet.GetIndex();
		
		int SelectRow = -1;
		for(int Row = 0; Row < BranchList[Select]->GetCount(); ++Row)
		{
			if(BranchList[Select]->IsSel(Row))
			{
				SelectRow = Row;
				break;
			}
		}
		if(SelectRow >= 0)
		{
			//PromptOK(Format("row %d is selected", SelectRow));
			for(int Row = SelectRow+1; Row < BranchList[Select]->GetCount(); ++Row)
			{
				BranchList[Select]->Set(Row, 0, Row-1); //Decrease subsequent indexes
				std::vector<int> Vals;
				String OldValList = BranchList[Select]->Get(Row, 2);
				bool Success = ParseIntList(OldValList, Vals, Row);
				//Ignore if Success is false, since then we don't know what to do about the
				//value?
				std::vector<int> NewVals;
				for(int Val : Vals)
				{
					if(Val < SelectRow) NewVals.push_back(Val);
					else if(Val > SelectRow) NewVals.push_back(Val-1);
					// if it is equal to SelectRow, it is now going to be deleted, so don't
					// re-add it
				}
				String NewValList = "";
				for(int Idx = 0; Idx < NewVals.size(); ++Idx)
				{
					if(Idx > 0) NewValList += ",";
					NewValList += Format("%d", NewVals[Idx]);
				}
				BranchList[Select]->Set(Row, 2, NewValList);
			}
			
			BranchList[Select]->Remove(SelectRow);
		}
	}
	
	Branches.Refresh();
}


bool ChangeIndexesWindow::ParseIntList(String &ListStr, std::vector<int> &Result, int Row)
{
	bool Success = true;
	
	Vector<String> SplitList = Split(ListStr, ",");
	for(String &Str : SplitList)
	{
		int Val = StrInt(Str);
		if(!IsNull(Val) && Val >= 0 && Val < Row)
			Result.push_back(Val);
		else
			Success = false;
	}
	return Success;
}