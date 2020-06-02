#include "MobiView.h"


ChangeIndexesWindow::ChangeIndexesWindow(MobiView *ParentWindow)
{
	CtrlLayout(*this, "Edit index sets");
	
	
	this->ParentWindow = ParentWindow;
	//Sizeable().Zoomable();
	
	WhenClose << [ParentWindow](){ delete ParentWindow->ChangeIndexes; ParentWindow->ChangeIndexes = nullptr; }; //TODO: Is this always a safe way of doing it?? No, apparently not!!
	
	FinishEdit.WhenAction << [this](){ this->DoIndexUpdate(); };
	
	
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
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		IndexSetName[Idx]->Hide();
		IndexList[Idx]->Hide();
		IndexList[Idx]->Disable();
	}
	
	void *DataSet = ParentWindow->DataSet;
	model_dll_interface &ModelDll = ParentWindow->ModelDll;
	
	uint64 IndexSetCount = ModelDll.GetIndexSetsCount(DataSet);
	std::vector<char *> IndexSets(IndexSetCount);
	std::vector<char *> IndexSetTypes(IndexSetCount);
	ModelDll.GetIndexSets(DataSet, IndexSets.data(), IndexSetTypes.data());
	if (ParentWindow->CheckDllUserError()) return;
	
	for(size_t IndexSet = 0; IndexSet < IndexSetCount; ++IndexSet)
	{
		const char *Name = IndexSets[IndexSet];
		
		IndexSetName[IndexSet]->SetText(Name);
		IndexSetName[IndexSet]->Show();
		
		//IndexSetNameToId[Name] = IndexSet;
		
		uint64 IndexCount = ModelDll.GetIndexCount(DataSet, Name);
		if (ParentWindow->CheckDllUserError()) return;
		std::vector<char *> IndexNames(IndexCount);
		ModelDll.GetIndexes(DataSet, Name, IndexNames.data());
		if (ParentWindow->CheckDllUserError()) return;
		
		for(const char *IndexName : IndexNames)
		{
			IndexList[IndexSet]->Append(IndexName, CHARSET_UTF8);
			IndexList[IndexSet]->Append("\n", CHARSET_UTF8);
		}
	
		IndexList[IndexSet]->Show();
		IndexList[IndexSet]->Enable();
	}
	
	
	
	Open();
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
	
	uint64 IndexSetCount = ModelDll.GetIndexSetsCount(NewDataSet);
	std::vector<char *> IndexSets(IndexSetCount);
	std::vector<char *> IndexSetTypes(IndexSetCount);
	ModelDll.GetIndexSets(NewDataSet, IndexSets.data(), IndexSetTypes.data());
	if (ParentWindow->CheckDllUserError()) return;
	
	for(size_t IndexSet = 0; IndexSet < IndexSetCount; ++IndexSet)
	{
		char *IndexSetName = IndexSets[IndexSet];
		
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
		
		if(strcmp(IndexSetTypes[IndexSet], "basic") == 0)
		{
			//Basic index set
			std::vector<char *> Indexes;
			for(int Line = 0; Line < Lines.size(); ++Line)
			{
				Indexes.push_back((char *)Lines[Line].data());
			}
			ModelDll.SetIndexes(NewDataSet, IndexSetName, Indexes.size(), (char**)Indexes.data());
		}
		else
		{
			//Branched index set
			std::vector<dll_branch_index> Indexes(Lines.size());
			std::vector<std::vector<char *>> Branches(Lines.size());
			for(int Line = 0; Line < Lines.size(); ++Line)
			{
				char *IndexName = (char *)Lines[Line].data();
				Indexes[Line].IndexName = IndexName;
				if(HasIndex(ModelDll, OldDataSet, IndexSetName, IndexName))
				{
					//Attempt to copy over old branch connectivity if applicable
					
					std::vector<char *> OldBranches;
					uint64 OldBranchCount = ModelDll.GetBranchInputsCount(OldDataSet, IndexSetName, IndexName);
					OldBranches.resize(OldBranchCount);
					ModelDll.GetBranchInputs(OldDataSet, IndexSetName, IndexName, OldBranches.data());
					
					for(char *OldBranch : OldBranches)
					{
						bool Found = false;
						for(int PrevLine = 0; PrevLine < Line; ++PrevLine)
						{
							if(strcmp(OldBranch, Indexes[PrevLine].IndexName) == 0)
							{
								Branches[Line].push_back(OldBranch);
								break;
							}
						}
					}

					Indexes[Line].BranchCount = Branches[Line].size();
					Indexes[Line].BranchNames = Branches[Line].data();
				}
				else
					Indexes[Line].BranchCount = 0;
				
			}
			ModelDll.SetBranchIndexes(NewDataSet, IndexSetName, Indexes.size(), Indexes.data());
		}
		
		if(ParentWindow->CheckDllUserError())
		{
			ParentWindow->DataSet = OldDataSet;
			ModelDll.DeleteModelAndDataSet(NewDataSet);
			Close();
			return;
		}
	}

	uint64 GroupCount = ModelDll.GetAllParameterGroupsCount(NewDataSet, "__all!!__");
	std::vector<char *> GroupNames(GroupCount);
	ModelDll.GetAllParameterGroups(NewDataSet, GroupNames.data(), "__all!!__");
	
	//TODO: Could do error handling, but it should not be unnecessary at this point.
	
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
	ParentWindow->ChangeIndexes = nullptr;
	delete this;
}