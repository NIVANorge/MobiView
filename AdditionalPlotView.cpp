#include "MobiView.h"


#define IMAGECLASS IconImg2
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>

MiniPlot::MiniPlot()
{
	CtrlLayout(*this);
}

AdditionalPlotView::AdditionalPlotView()
{
	CtrlLayout(*this, "Additional plot view");
	
	MinimizeBox().Sizeable().Zoomable();
	
	Add(VerticalSplitter.VSizePos(25, 0));
	
	VerticalSplitter.Vert();

	EditNumRows.NotNull();
	EditNumRows.SetData(2);
	EditNumRows.Max(MAX_ADDITIONAL_PLOTS);
	
	LinkAll.NotNull();
	LinkAll.Set(1);
	
	EditNumRows.WhenAction << [this](){ NumRowsChanged(true); };
	
	LinkAll.WhenAction = THISBACK(UpdateLinkStatus);
	
	AddFrame(Tool);
	Tool.Set(THISBACK(SubBar));
	
	for(int Row = 0; Row < MAX_ADDITIONAL_PLOTS; ++Row)
	{
		Plots[Row].CopyMain.WhenPush << [this, Row](){ CopyMainPlot(Row); };
		Plots[Row].PushMain.WhenPush << [this, Row](){
			this->ParentWindow->Plotter.SetMainPlotSetup(Plots[Row].Plot.PlotSetup);
		};
		
		
		Plots[Row].PlotInfo.SetEditable(false);
		//Plots[Row].PlotInfo.SetColor(TextCtrl::PAPER_READONLY, Plots[Row].PlotInfo.GetColor(TextCtrl::PAPER_NORMAL));
	}
	
	NumRowsChanged();
}

void AdditionalPlotView::SubBar(Bar &bar)
{
	bar.Add(IconImg2::Open(), THISBACK(LoadFromJson)).Tip("Load setup from file");
	bar.Add(IconImg2::Save(), THISBACK(WriteToJson)).Tip("Save setup to file");
}

void AdditionalPlotView::NumRowsChanged(bool Rebuild)
{
	int NRows = EditNumRows.GetData();
	
	if(IsNull(NRows) || NRows <= 0) return;
	
	VerticalSplitter.Clear();
	
	for(int Row = 0; Row < NRows; ++Row)
	{
		VerticalSplitter.Add(Plots[Row]);
	}
	
	if(Rebuild) BuildAll();  //TODO: Technically only have to rebuild any newly added ones
}

void AdditionalPlotView::UpdateLinkStatus()
{
	for(int Row = 0; Row < MAX_ADDITIONAL_PLOTS; ++Row)
		Plots[Row].Plot.Unlinked();
	
	if(LinkAll.Get())
	{
		int FirstLinkable = -1;
		for(int Row = 0; Row < MAX_ADDITIONAL_PLOTS; ++Row)
		{
			if(Plots[Row].Plot.GetMouseHandlingX())
			{
				if(FirstLinkable >= 0)
					Plots[Row].Plot.LinkedWith(Plots[FirstLinkable].Plot);
				else
					FirstLinkable = Row;
				
			}
		}
	}
}

void AdditionalPlotView::CopyMainPlot(int Which)
{
	Plots[Which].Plot.PlotSetup = ParentWindow->Plotter.MainPlot.PlotSetup;
	Plots[Which].Plot.BuildPlot(ParentWindow, nullptr, false, Plots[Which].PlotInfo);
	UpdateLinkStatus();
}

void AdditionalPlotView::BuildAll(bool CausedByReRun)
{
	int NRows = EditNumRows.GetData();
	for(int Row = 0; Row < NRows; ++Row)
	{
		Plots[Row].Plot.BuildPlot(ParentWindow, nullptr, false, Plots[Row].PlotInfo, CausedByReRun);
	}
	UpdateLinkStatus();
}

void AdditionalPlotView::ClearAll()
{
	for(int Row = 0; Row < MAX_ADDITIONAL_PLOTS; ++Row)
	{
		Plots[Row].Plot.ClearAll();
	}
}

void SerializePlotSetup(Json &SetupJson, plot_setup &Setup, MobiView *ParentWindow)
{
	//NOTE: This is not robust if we change the enums!
	
	SetupJson("MajorMode", (int)Setup.MajorMode);
	SetupJson("AggregationType", (int)Setup.AggregationType);
	SetupJson("AggregationPeriod", (int)Setup.AggregationPeriod);
	SetupJson("YAxisMode", (int)Setup.YAxisMode);
	SetupJson("ProfileTimestep", Setup.ProfileTimestep);
	SetupJson("ScatterInputs", Setup.ScatterInputs);
	
	JsonArray ResultArr;
	for(std::string &R : Setup.SelectedResults)
		ResultArr << R.data();
	SetupJson("SelectedResults", ResultArr);
	
	JsonArray InputArr;
	for(std::string &R : Setup.SelectedInputs)
		InputArr << R.data();
	SetupJson("SelectedInputs", InputArr);
	
	Json IndexMap;
	int Id = 0;
	for(std::vector<std::string> &Arr : Setup.SelectedIndexes)
	{
		String IndexSetName = ParentWindow->IndexSetName[Id]->GetText();
		JsonArray InnerArr;
		for(std::string &Index : Arr)
			InnerArr << Index.data();
		IndexMap(IndexSetName, InnerArr);
		
		++Id;
	}
	SetupJson("SelectedIndexes", IndexMap);
	
	JsonArray ActiveIndexSetArr;
	for(bool IsActive : Setup.IndexSetIsActive)
		ActiveIndexSetArr << IsActive;
	SetupJson("IndexSetIsActive", ActiveIndexSetArr);
}


void AdditionalPlotView::WriteToJson()
{
	FileSel Sel;

	Sel.Type("Plot setups", "*.json");
	
	if(!ParentWindow->ParameterFile.empty())
		Sel.ActiveDir(GetFileFolder(ParentWindow->ParameterFile.data()));
	else
		Sel.ActiveDir(GetCurrentDirectory());
	
	Sel.ExecuteSaveAs();
	String Filename = Sel.Get();
	
	if(Filename.GetLength() == 0) return;
	
	if(GetFileName(Filename) == "settings.json")
	{
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store plot setups.");
		return;
	}
	
	Json MainFile;
	
	int NRows = EditNumRows.GetData();
	
	JsonArray Arr;
	
	for(int Row = 0; Row < NRows; ++Row)
	{
		Json RowSettings;
		SerializePlotSetup(RowSettings, Plots[Row].Plot.PlotSetup, ParentWindow);
		Arr << RowSettings;
	}
	
	MainFile("Setups", Arr);
	
	bool Lock = LinkAll.Get();
	
	MainFile("LinkAll", Lock);
	
	SaveFile(Filename, MainFile.ToString());
}


void DeserializePlotSetup(ValueMap &SetupJson, plot_setup &Setup, MobiView *ParentWindow)
{
	if(IsNull(SetupJson)) return;
	
	Value MajorMode = SetupJson["MajorMode"];
	if(!IsNull(MajorMode)) Setup.MajorMode = (plot_major_mode)(int)MajorMode;
	
	Value AggregationType = SetupJson["AggregationType"];
	if(!IsNull(AggregationType)) Setup.AggregationType = (aggregation_type)(int)AggregationType;
	
	Value AggregationPeriod = SetupJson["AggregationPeriod"];
	if(!IsNull(AggregationPeriod)) Setup.AggregationPeriod = (aggregation_period)(int)AggregationPeriod;
	
	Value YAxisMode = SetupJson["YAxisMode"];
	if(!IsNull(YAxisMode)) Setup.YAxisMode = (y_axis_mode)(int)YAxisMode;
	
	Value ScatterInputs = SetupJson["ScatterInputs"];
	if(!IsNull(ScatterInputs)) Setup.ScatterInputs = (bool)ScatterInputs;
	
	Value ProfileTimestep = SetupJson["ProfileTimestep"];
	if(!IsNull(ProfileTimestep)) Setup.ProfileTimestep = (int)ProfileTimestep;
	
	Setup.SelectedResults.clear();
	ValueArray SelectedResults = SetupJson["SelectedResults"];
	if(!IsNull(SelectedResults))
	{
		for(String Result : SelectedResults)
			Setup.SelectedResults.push_back(Result.ToStd());
	}
	
	Setup.SelectedInputs.clear();
	ValueArray SelectedInputs = SetupJson["SelectedInputs"];
	if(!IsNull(SelectedInputs))
	{
		for(String Input : SelectedInputs)
			Setup.SelectedInputs.push_back(Input.ToStd());
	}
	
	ValueMap SelectedIndexes = SetupJson["SelectedIndexes"];
	if(!IsNull(SelectedIndexes) && SelectedIndexes.GetCount() == MAX_INDEX_SETS)
	{
		Setup.SelectedIndexes.resize(MAX_INDEX_SETS);
		for(int IndexSet = 0; IndexSet < SelectedIndexes.GetCount(); ++IndexSet)
		{
			String IndexSetName = ParentWindow->IndexSetName[IndexSet]->GetText();
			ValueArray Indexes = SelectedIndexes[IndexSetName];
			if(!IsNull(Indexes))
			{
				Setup.SelectedIndexes[IndexSet].clear();
				for(String Index : Indexes)
				{
					Setup.SelectedIndexes[IndexSet].push_back(Index.ToStd());
				}
			}
		}
	}
	
	Setup.IndexSetIsActive.clear();
	ValueArray IndexSetIsActive = SetupJson["IndexSetIsActive"];
	if(!IsNull(IndexSetIsActive))
	{
		for(bool IsActive : IndexSetIsActive)
			Setup.IndexSetIsActive.push_back(IsActive);
	}
}

void AdditionalPlotView::LoadFromJson()
{
	FileSel Sel;
	
	Sel.Type("Plot setups", "*.json");
	
	if(!ParentWindow->ParameterFile.empty())
		Sel.ActiveDir(GetFileFolder(ParentWindow->ParameterFile.data()));
	else
		Sel.ActiveDir(GetCurrentDirectory());
	
	Sel.ExecuteOpen();
	String Filename = Sel.Get();
	
	if(!FileExists(Filename)) return;
	
	if(GetFileName(Filename) == "settings.json")
	{
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store plot setups.");
		return;
	}
	
	String SetupFile = LoadFile(Filename);
	
	Value SetupJson  = ParseJSON(SetupFile);
	
	Value Lock = SetupJson["LinkAll"];
	if(!IsNull(Lock)) LinkAll.Set((int)(bool)Lock);
	
	
	ValueArray Setups = SetupJson["Setups"];
	if(!IsNull(Setups))
	{
		int Count = Setups.GetCount();
		
		EditNumRows.SetData(Count);
		NumRowsChanged(false);
		
		for(int Row = 0; Row < Count; ++Row)
		{
			ValueMap Setup = Setups[Row];
			
			DeserializePlotSetup(Setup, Plots[Row].Plot.PlotSetup, ParentWindow);
		}
		
	}
	
	BuildAll();
}
