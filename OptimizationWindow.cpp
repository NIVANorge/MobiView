#include "MobiView.h"

// NOTE: A little bad we have to re-include this here just to generate the Run icon.
#define IMAGECLASS IconImg4
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>



OptimizationWindow::OptimizationWindow()
{
	CtrlLayout(*this, "MobiView optimization setup CURRENTLY NOT FUNCTIONAL");
	
	Sizeable().Zoomable();
	
	PushRun.SetImage(IconImg4::Run());
	
	
	#define SET_SETTING(Handle, Name, Type)
	#define SET_RES_SETTING(Handle, Name, Type) SET_SETTING(Handle, Name, Type) \
		if(Type != -1) SelectStat.Add(Name);
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	#undef SET_RES_SETTING
	
	SelectStat.SetIndex(0);
	
	ParameterView.AddColumn(Id("__name"), "Name");
	ParameterView.AddColumn(Id("__indexes"), "Indexes");
	ParameterView.AddColumn(Id("__min"), "Min");
	ParameterView.AddColumn(Id("__max"), "Max");
	ParameterView.AddColumn(Id("__unit"), "Unit");
	
	
	PushAddParameter.WhenPush = THISBACK(AddParameterClicked);
	PushAddGroup.WhenPush = THISBACK(AddGroupClicked);
	PushRemoveParameter.WhenPush = THISBACK(RemoveParameterClicked);
	PushClearParameters.WhenPush = THISBACK(ClearAllClicked);
	PushRun.WhenPush = THISBACK(RunClicked);
}

void OptimizationWindow::AddSingleParameter(indexed_parameter &Parameter, int SourceRow)
{
	if(SourceRow == -1) return;
	
	int OverrideRow = -1;
	int Row = 0;
	for(indexed_parameter &Par2 : Parameters)
	{
		if(ParameterIsSubsetOf(Parameter, Par2)) return;
		
		if(ParameterIsSubsetOf(Par2, Parameter))
		{
			Par2 = Parameter;
			OverrideRow = Row;
		}	
		++Row;
	}
	
	String Indexes = MakeParameterIndexString(Parameter);
	
	if(OverrideRow < 0)
	{
		Parameters.push_back(Parameter);
		
		//TODO: Maybe these should be put on the indexed_parameter, but that is pretty superfluous
		//for the other purposes this struct is used for.
		double Min     = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__min"));
		double Max     = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__max"));
		String ParUnit = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__unit"));
		
		ParameterView.Add(Parameter.Name.data(), Indexes, Min, Max, ParUnit);
		
		EditMinCtrls.Create<EditDoubleNotNull>();
		EditMaxCtrls.Create<EditDoubleNotNull>();
		
		int Row = Parameters.size()-1;
		ParameterView.SetCtrl(Row, 2, EditMinCtrls[Row]);
		ParameterView.SetCtrl(Row, 3, EditMaxCtrls[Row]);
	}
	else
	{
		ParameterView.Set(OverrideRow, 1, Indexes);
	}
}

void OptimizationWindow::AddParameterClicked()
{
	int SelectedRow             = ParentWindow->FindSelectedParameterRow();
	indexed_parameter Parameter = ParentWindow->GetSelectedParameter();
	if(SelectedRow==-1 || !Parameter.Valid) return;  //TODO: Some kind of error message explaining how to use the feature?
	
	AddSingleParameter(Parameter, SelectedRow);
}

void OptimizationWindow::AddGroupClicked()
{
	int RowCount = ParentWindow->Params.ParameterView.GetCount();
	//PromptOK(Format("Row count %d", RowCount));
	for(int Row = 0; Row < RowCount; ++Row)
	{
		indexed_parameter Parameter = ParentWindow->GetParameterAtRow(Row);
		if(Parameter.Valid)
			AddSingleParameter(Parameter, Row);
	}
}

void OptimizationWindow::RemoveParameterClicked()
{
	int SelectedRow = -1;
	for(int Row = 0; Row < ParameterView.GetCount(); ++Row)
	{
		if(ParameterView.IsSel(Row))
		{
			SelectedRow = Row;
			break;
		}
	}
	
	if(SelectedRow == -1) return;
	
	ParameterView.Remove(SelectedRow);
	Parameters.erase(Parameters.begin()+SelectedRow);
	EditMinCtrls.Remove(SelectedRow);
	EditMaxCtrls.Remove(SelectedRow);
}

void OptimizationWindow::ClearAllClicked()
{
	ParameterView.Clear();
	Parameters.clear();
	EditMinCtrls.Clear();
	EditMaxCtrls.Clear();
}

void OptimizationWindow::RunClicked()
{
	ErrorLabel.SetText("");
	
	if(Parameters.empty())
	{
		ErrorLabel.SetText("At least one parameter must be added before running");
		return;
	}
	
	ErrorLabel.SetText("NOT IMPLEMENTED!");
	//TODO: implement!
}