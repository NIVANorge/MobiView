#include "MobiView.h"


SearchWindow::SearchWindow()
{

	CtrlLayout(*this, "MobiView search");
	
	Sizeable();

	SearchField.WhenAction = THISBACK(Find);
	
	ResultField.AddColumn("Name");
	ResultField.AddColumn("Group");
	
	ResultField.WhenSel = THISBACK(SelectItem);
}

void SearchWindow::Find()
{
	void *DataSet = ParentWindow->DataSet;
	model_dll_interface &ModelDll = ParentWindow->ModelDll;
	
	if(ModelDll.IsLoaded() && DataSet)
	{
		ResultField.Clear();
		
		String Match = SearchField.GetData();
		std::string MatchText = Match.ToStd();
		std::transform(MatchText.begin(), MatchText.end(), MatchText.begin(), ::tolower);  //TODO: trim leading whitespace
		
		uint64 GroupCount = ModelDll.GetAllParameterGroupsCount(DataSet, "__all!!__");
		std::vector<char *> GroupNames(GroupCount);
		ModelDll.GetAllParameterGroups(DataSet, GroupNames.data(), "__all!!__");
		
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
	String SelectedParameterName = ResultField.Get(0);
	String SelectedGroupName = ResultField.Get(1);
	
	TreeCtrl &GroupSelect = ParentWindow->ParameterGroupSelecter;
	
	uint64 Count = GroupSelect.GetLineCount(); //TODO: This is not ideal since it may not work if somebody collapses a branch!!
	
	for(int Row = 1; Row < Count; ++Row)  //NOTE: Start at 1 since Row 0 is the name of the model. Causes problems when model name is the same as one of the groups
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
	
	ArrayCtrl &ParameterSelect = ParentWindow->Params.ParameterView;
	
	Count = ParameterSelect.GetCount();
	
	for(int Row = 0; Row < Count; ++Row)
	{
		String CurName = ParameterSelect.Get(Row, 0);
		if(CurName == SelectedParameterName)
		{
			ParameterSelect.SetFocus();
			ParameterSelect.SetCursor(Row);
			break;
		}
	}
	
}