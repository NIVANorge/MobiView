#include "MobiView.h"


SearchWindow::SearchWindow(MobiView *ParentWindow)
{
	this->ParentWindow = ParentWindow;
	
	CtrlLayout(*this, "MobiView search");
	
	Sizeable();
	
	Open();
	
	WhenClose << [ParentWindow](){ delete ParentWindow->Search; ParentWindow->Search = nullptr; }; //TODO: Is this always a safe way of doing it?? No, apparently not!!
	
	SearchField.WhenAction = THISBACK(Find);
	
	ResultField.AddColumn("Name");
	ResultField.AddColumn("Group");
	
	ResultField.WhenSel = THISBACK(SelectItem);
}

void SearchWindow::Find()
{
	void *DataSet = ParentWindow->DataSet;
	model_dll_interface &ModelDll = ParentWindow->ModelDll;
	
	if(ParentWindow->hinstModelDll && DataSet)
	{
		ResultField.Clear();
		
		String Match = SearchField.GetData();
		std::string MatchText = Match.ToStd();
		std::transform(MatchText.begin(), MatchText.end(), MatchText.begin(), ::tolower);
		
		uint64 GroupCount = ModelDll.GetAllParameterGroupsCount(DataSet, nullptr);
		std::vector<char *> GroupNames(GroupCount);
		ModelDll.GetAllParameterGroups(DataSet, GroupNames.data(), nullptr);
		
		for(int Idx = 0; Idx < GroupCount; ++Idx)
		{
			char *GroupName = GroupNames[Idx];
			uint64 ParCount = ModelDll.GetAllParametersCount(DataSet, GroupName);
			std::vector<char *> ParNames(ParCount);
			std::vector<char *> ParTypes(ParCount);
			ModelDll.GetAllParameters(DataSet, ParNames.data(), ParTypes.data(), GroupName);
			
			for(int Par = 0; Par < ParCount; ++Par)
			{
				//TODO: Better matching algorithm?
				std::string ParName = ParNames[Par];
				std::transform(ParName.begin(), ParName.end(), ParName.begin(), ::tolower);
				
				size_t Pos = ParName.find(MatchText);
				
				if(Pos != std::string::npos)
				{
					ResultField.Add(ParNames[Par], GroupName);
				}
			}
		}
		
		
		ParentWindow->CheckDllUserError();
	}
}

void SearchWindow::SelectItem()
{
	String SelectedGroupName = ResultField.Get(1);
	
	TreeCtrl &GroupSelect = ParentWindow->ParameterGroupSelecter;
	
	uint64 Count = ParentWindow->ModelDll.GetAllParameterGroupsCount(ParentWindow->DataSet, 0);
	
	for(int Row = 0; Row < Count; ++Row)
	{
		String CurName = GroupSelect.Get(Row);
		if(CurName == SelectedGroupName)
		{
			//GroupSelect.ClearSelection(true);
			GroupSelect.SetFocus();
			GroupSelect.SetCursor(Row);
			break;
		}
	}
}