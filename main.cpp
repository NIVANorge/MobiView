#include "MobiView.h"
#include "DllInterface.h"

#include <vector>
#include <iomanip>



#define IMAGECLASS IconImg
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>


#define IMAGECLASS MainIconImg
#define IMAGEFILE <MobiView/MobiView.iml>
#include <Draw/iml.h>


void MobiView::Log(String Msg)
{
	auto T = std::time(nullptr);
	auto TM = *std::localtime(&T);
	std::stringstream Oss;
	Oss << std::put_time(&TM, "%H:%M:%S : ");
	
	String FormatMsg = "";
	FormatMsg << Oss.str().data();
	FormatMsg << Msg;
	FormatMsg << "\n\n";
	LogBox.Append(FormatMsg);
	LogBox.SetCursor(INT64_MAX);
}


void MobiView::HandleDllError()
{
	LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

	
	Log(String((char *)lpMsgBuf));

    LocalFree(lpMsgBuf);
}

bool MobiView::CheckDllUserError()
{
	if(hinstModelDll && ModelDll.EncounteredError)
	{
		char ErrMsgBuf[1024];
		int ErrCode = ModelDll.EncounteredError(ErrMsgBuf);
		
		if(ErrCode != 0)
		{
			Log(String(ErrMsgBuf));
			return true;
		}
	}
	return false;
}

void MobiView::SubBar(Bar &bar)
{
	bar.Add(IconImg::Open(), THISBACK(Load)).Tip("Load model, parameter and input files").Key(K_CTRL_O);
	bar.Add(IconImg::Save(), THISBACK(SaveParameters)).Tip("Save parameters").Key(K_CTRL_S);
	bar.Add(IconImg::SaveAs(), THISBACK(SaveParametersAs)).Tip("Save parameters as").Key(K_ALT_S);
	bar.Add(IconImg::Search(), THISBACK(OpenSearch)).Tip("Search parameters").Key(K_CTRL_F);
	bar.Add(IconImg::ViewReaches(), THISBACK(OpenVisualizeBranches)).Tip("Visualize reach branches").Key(K_CTRL_R);
	bar.Separator();
	bar.Add(IconImg::Run(), THISBACK(RunModel)).Tip("Run model").Key(K_F7);
	bar.Separator();
	bar.Add(IconImg::SaveBaseline(), THISBACK(SaveBaseline)).Tip("Save baseline").Key(K_CTRL_B);
	bar.Separator();
	bar.Add(IconImg::SaveCsv(), THISBACK(SaveToCsv)).Tip("Save results to .csv").Key(K_CTRL_E);
	
}

MobiView::MobiView()
{
	WhenClose = THISBACK(ClosingChecks);
	
	AddFrame(Tool);
	Tool.Set(THISBACK(SubBar));
	
	hinstModelDll = NULL;
	ModelDll = {};
	DataSet = nullptr;
	
	CtrlLayout(*this, "MobiView");
	Sizeable().Zoomable().Icon(MainIconImg::i4());
	
	EquationSelecter.NoGrid();
	EquationSelecter.Disable();
	EquationSelecter.AddColumn("Equation").HeaderTab();
	EquationSelecter.AddColumn("F.").HeaderTab();
	EquationSelecter.WhenAction = THISBACK(PlotModeChange);
	EquationSelecter.MultiSelect();
	EquationSelecter.ColumnWidths("85 15");
	
	ShowFavorites.WhenAction = THISBACK(UpdateEquationSelecter);
	
	LogBox.SetColor(TextCtrl::PAPER_READONLY, LogBox.GetColor(TextCtrl::PAPER_NORMAL));
	PlotInfo.SetColor(TextCtrl::PAPER_READONLY, PlotInfo.GetColor(TextCtrl::PAPER_NORMAL));
	
	InputSelecter.AddColumn("Input");
	InputSelecter.WhenAction = THISBACK(PlotModeChange);
	InputSelecter.MultiSelect();
	InputSelecter.NoGrid();
	
	//ParameterGroupSelecter.AddColumn("Parameter group");
	ParameterGroupSelecter.SetRoot(Null, String("Parameter groups"));
	
	ParameterView.AddColumn("Name").HeaderTab();
	ParameterView.AddColumn("Value").HeaderTab();
	ParameterView.AddColumn("Min").HeaderTab();
	ParameterView.AddColumn("Max").HeaderTab();
	ParameterView.AddColumn("Unit").HeaderTab();
	ParameterView.AddColumn("Description").HeaderTab();
	
	ParameterView.ColumnWidths("20 12 10 10 10 38");
	
	ParameterGroupSelecter.WhenSel = THISBACK(RefreshParameterView);
	//ParameterGroupSelecter.HorzGrid(false);
	//ParameterGroupSelecter.VertGrid(false);
	
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
	
	IndexLock[0]    = &IndexLock1;
	IndexLock[1]    = &IndexLock2;
	IndexLock[2]    = &IndexLock3;
	IndexLock[3]    = &IndexLock4;
	IndexLock[4]    = &IndexLock5;
	IndexLock[5]    = &IndexLock6;
	
	EIndexList[0]   = &EIndexList1;
	EIndexList[1]   = &EIndexList2;
	EIndexList[2]   = &EIndexList3;
	EIndexList[3]   = &EIndexList4;
	EIndexList[4]   = &EIndexList5;
	EIndexList[5]   = &EIndexList6;
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		IndexSetName[Idx]->Hide();
		IndexList[Idx]->Hide();
		IndexList[Idx]->Disable();
		IndexList[Idx]->WhenAction = THISBACK(RefreshParameterView);
		
		IndexLock[Idx]->Hide();
		//IndexLock[Idx]->WhenAction = THISBACK(RefreshParameterView);
		
		EIndexList[Idx]->Hide();
		EIndexList[Idx]->Disable();
		EIndexList[Idx]->WhenAction = THISBACK(RePlot);
		EIndexList[Idx]->MultiSelect();
		//EIndexList1.NoHeader().NoVertGrid().AutoHideSb().NoGrid();
		
		EIndexList[Idx]->AddColumn("(no name)");
	}
	
	SetDateFormat("%d-%02d-%02d");
	SetDateScan("ymd");
	
	
	Plot.SetFastViewX(true);
	Plot.SetSequentialXAll(true);
	Plot.SetMouseHandling(true, false);
	
	Plot.SetPlotAreaLeftMargin(50);
	Plot.SetGridDash("");
	Plot.SetGridColor(Color(180, 180, 180));
	
	PlotColors = {{0, 130, 200}, {230, 25, 75}, {60, 180, 75}, {245, 130, 48}, {145, 30, 180},
                  {70, 240, 240}, {240, 50, 230}, {210, 245, 60}, {250, 190, 190}, {0, 128, 128}, {230, 190, 255},
                  {170, 110, 40}, {128, 0, 0}, {170, 255, 195}, {128, 128, 0}, {255, 215, 180}, {0, 0, 128}, {255, 225, 25}};



	//Plot mode buttons and controls:
	TimestepSlider.Range(10); //To be overwritten later.
	TimestepSlider.SetData(0);
	TimestepSlider.Hide();
	TimestepEdit.Hide();
	TimestepSlider.WhenAction << THISBACK(TimestepSliderEvent);
	TimestepEdit.WhenAction << THISBACK(TimestepEditEvent);
	
	PlotMajorMode.SetData(0);
	PlotMajorMode.Disable();
	PlotMajorMode.WhenAction << THISBACK(PlotModeChange);
	
	TimeIntervals.SetData(0);
	TimeIntervals.Disable();
	TimeIntervals.WhenAction << THISBACK(PlotModeChange);
	
	Aggregation.SetData(0);
	Aggregation.Disable();
	Aggregation.WhenAction << THISBACK(PlotModeChange);
	
	ScatterInputs.Disable();
	ScatterInputs.WhenAction << THISBACK(PlotModeChange);
	
	YAxisMode.SetData(0);
	YAxisMode.Disable();
	YAxisMode.WhenAction << THISBACK(PlotModeChange);
	
	Plot.RemoveMouseBehavior(ScatterCtrl::ZOOM_WINDOW);
	Plot.AddMouseBehavior(true, false, false, true, false, 0, false, ScatterCtrl::SCROLL);
}


void MobiView::OpenSearch()
{
	if(!Search)
	{
		Search = new SearchWindow(this);
	}
}


void MobiView::OpenVisualizeBranches()
{
	if(!hinstModelDll || !DataSet)
	{
		Log("Can't visualize branch network before a dataset is loaded in.");
		return;
	}
	
	if(!Visualize)
	{
		Visualize = new VisualizeBranches(this);
	}
}



void MobiView::UpdateEquationSelecter()
{
	//TODO: The top of the array is weird when row height for some rows is 0.
	//TODO: Unfavouriting your last selected series does not remove the plot.
	
	bool ShowFavOnly = ShowFavorites.GetData();
	
	for(int Row = 0; Row < EquationSelecter.GetCount(); ++Row)
	{
		if(!ShowFavOnly || EquationSelecter.GetCtrl(Row, 1)->GetData())
		{
			EquationSelecter.ShowLine(Row, true);
		}
		else
		{
			EquationSelecter.HideLine(Row);
			EquationSelecter.Select(Row, false, false); //TODO: Should probably think about whether we really want this actually..
		}
	}
	
	PlotModeChange(); //In order to replot in case the selection changed.
	
	StoreSettings();
}



void MobiView::StoreSettings()
{
	String SettingsFile = LoadFile(GetDataFile("settings.json"));
	Value ExistingSettings = ParseJSON(SettingsFile);
	
	ValueMap Eq = ExistingSettings["Favorite equations"];
	
	Json SettingsJson;
	
	SettingsJson
		("Model dll path", String(DllFile))
		("Parameter file path", String(CurrentParameterFile))
		("Input file path", String(InputFile));
	
	Json Favorites;
	
	String DllFileName = GetFileName(DllFile.data());
	
	for(const auto &K : Eq.GetKeys())
	{
		if(K != DllFileName)
		{
			Favorites(K.ToString(), Eq[K]);
		}
	}
	
	JsonArray FavForCurrent;
	for(int Row = 0; Row < EquationSelecter.GetCount(); ++Row)
	{
		if(EquationSelecter.Get(Row, 1)) //I.e. if it was favorited
		{
			FavForCurrent << EquationSelecter.Get(Row, 0);
		}
	}
	Favorites(DllFileName, FavForCurrent);
	
	SettingsJson("Favorite equations", Favorites);
	
	SaveFile("settings.json", SettingsJson.ToString());
	
}


void MobiView::Load()
{
	if(hinstModelDll)
	{
		//PromptOK("Tried to do this");
		
		if(DataSet && ParametersWereChangedSinceLastSave)
		{
			//TODO: Query if we really want to reload before saving changes to parameters
		}
		
		if(BaselineDataSet)
		{
			ModelDll.DeleteDataSet(BaselineDataSet);
			BaselineDataSet = 0;
		}
		
		if(DataSet)
		{
			ModelDll.DeleteModelAndDataSet(DataSet);
			DataSet = 0;
		}
		
		FreeLibrary(hinstModelDll);
		hinstModelDll = 0;
		
		ParametersWereChangedSinceLastSave = false;
		
		EquationSelecter.Clear();
		EquationSelecter.Disable();
		EquationSelecterFavControls.Clear();
		InputSelecter.Clear();
		
		ParameterGroupSelecter.Clear();
		ParameterGroupSelecter.SetRoot(Null, String("Parameter groups"));
		
		ParameterView.Clear();
		
		for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
		{
			IndexList[Idx]->Clear();
			IndexList[Idx]->Hide();
			EIndexList[Idx]->Clear();
			EIndexList[Idx]->Hide();
			//EIndexList[Idx]->Reset();
			IndexLock[Idx]->Hide();
			IndexSetName[Idx]->Hide();
		}
		
		IndexSetNameToId.clear();
		
		Plot.RemoveAllSeries();
		PlotData.Clear();
		//AggregateX.clear();
		//AggregateY.clear();
	}
	
	
	//TODO: We should probably break up loading of model, parameter and input files into separate steps.
	
	String SettingsFile = LoadFile(GetDataFile("settings.json"));
	Value SettingsJson = ParseJSON(SettingsFile);
	
	String PreviouslyLoadedModel         = SettingsJson["Model dll path"];
	String PreviouslyLoadedParameterFile = SettingsJson["Parameter file path"];
	String PreviouslyLoadedInputFile     = SettingsJson["Input file path"];
	
	FileSel DllSel;
	DllSel.Type("Model dll files", "*.dll");
	if(!PreviouslyLoadedModel.IsEmpty() && FileExists(PreviouslyLoadedModel))
	{
		DllSel.PreSelect(PreviouslyLoadedModel);
	}
	DllSel.ExecuteOpen();
	DllFile = DllSel.Get().ToStd();
	
	bool ChangedDll = DllFile != PreviouslyLoadedModel;
	
	bool Success = DllFile.size() > 0;
	
	if(!Success) return;
	
	//TODO: If a dll is already loaded, unload it first.
	
	hinstModelDll = LoadLibraryA(DllFile.data());
	
	Success = hinstModelDll != NULL;
	
	if(!Success)
	{
		HandleDllError();
		return;
	}
	
	Log(String("Loading model dll: ") + DllFile.data());

	SetupModelDllInterface(&ModelDll, hinstModelDll);

	FileSel InputSel;
	InputSel.Type("Input dat files", "*.dat");
	if(!ChangedDll && !PreviouslyLoadedInputFile.IsEmpty() && FileExists(PreviouslyLoadedInputFile))
	{
		InputSel.PreSelect(PreviouslyLoadedInputFile);
	}
	else
	{
		InputSel.ActiveDir(GetFileFolder(DllFile.data()));
	}
	InputSel.ExecuteOpen();
	InputFile = InputSel.Get().ToStd();
	
	Success = InputFile.size() > 0;
	
	if(!Success) return;
	
	Log(String("Loading input file: ") + InputFile.data());
	
	FileSel ParameterSel;
	ParameterSel.Type("Parameter dat files", "*.dat");
	if(!ChangedDll && !PreviouslyLoadedParameterFile.IsEmpty() && FileExists(PreviouslyLoadedParameterFile))
	{
		ParameterSel.PreSelect(PreviouslyLoadedParameterFile);
	}
	else
	{
		ParameterSel.ActiveDir(GetFileFolder(InputFile.data()));
	}
	ParameterSel.ExecuteOpen();
	CurrentParameterFile = ParameterSel.Get().ToStd();
	
	Success = CurrentParameterFile.size() > 0;
	
	if(!Success) return;
	
	Log(String("Loading parameter file: ") + CurrentParameterFile.data());
	
	DataSet = ModelDll.SetupModel(CurrentParameterFile.data(), InputFile.data());
	if (CheckDllUserError()) return;
	
	
	
	
	String DllFileName = GetFileName(DllFile.data());
	auto &FavoriteList = SettingsJson["Favorite equations"][DllFileName];
	
	uint64 ResultCount = ModelDll.GetAllResultsCount(DataSet);
	if (CheckDllUserError()) return;
	std::vector<char *> ResultNames(ResultCount);
	std::vector<char *> ResultTypes(ResultCount);
	ModelDll.GetAllResults(DataSet, ResultNames.data(), ResultTypes.data());
	if (CheckDllUserError()) return;
	
	int Row = 0;
	for(size_t Idx = 0; Idx < ResultCount; ++Idx)
	{
		if(strcmp(ResultTypes[Idx], "initialvalue") != 0)
		{
			String Name = ResultNames[Idx];
			
			int IsFavorite = (std::find(FavoriteList.begin(), FavoriteList.end(), Name) != FavoriteList.end());
			
			EquationSelecter.Add(ResultNames[Idx], IsFavorite);
			EquationSelecterFavControls.Create<StarOption>();
			Ctrl &Favorite = EquationSelecterFavControls.Top();
			
			EquationSelecter.SetCtrl((int)Row, 1, Favorite);
			Favorite.WhenAction = THISBACK(UpdateEquationSelecter);
			++Row;
		}
	}
	
	
	uint64 InputCount = ModelDll.GetAllInputsCount(DataSet);
	if(CheckDllUserError()) return;
	std::vector<char *> InputNames(InputCount);
	std::vector<char *> InputTypes(InputCount);
	ModelDll.GetAllInputs(DataSet, InputNames.data(), InputTypes.data());
	if (CheckDllUserError()) return;
	
	for(size_t Idx = 0; Idx < InputCount; ++Idx)
	{
		InputSelecter.Add(InputNames[Idx]);
	}

	PlotMajorMode.Enable();
	PlotMajorMode.DisableCase(MajorMode_CompareBaseline);
	
	
	uint64 ParameterGroupCount = ModelDll.GetAllParameterGroupsCount(DataSet, nullptr);
	if (CheckDllUserError()) return;
	
	AddParameterGroupsRecursive(0, 0, ParameterGroupCount);
	
	ParameterGroupSelecter.OpenDeep(0, true);
	
	uint64 IndexSetCount = ModelDll.GetIndexSetsCount(DataSet);
	
	if(IndexSetCount > MAX_INDEX_SETS)
	{
		PromptOK(String("MobiView does not currently support models with more than ") + MAX_INDEX_SETS + " index sets. The model you tried to load has " + IndexSetCount + ".");
		return;
	}
	
	std::vector<char *> IndexSets(IndexSetCount);
	ModelDll.GetIndexSets(DataSet, IndexSets.data());
	if (CheckDllUserError()) return;
	
	//size_t MaxIndexCount = 0;
	for(size_t IndexSet = 0; IndexSet < IndexSetCount; ++IndexSet)
	{
		IndexLock[IndexSet]->Show();
		
		const char *Name = IndexSets[IndexSet];
		
		IndexSetName[IndexSet]->SetText(Name);
		IndexSetName[IndexSet]->Show();
		
		IndexSetNameToId[Name] = IndexSet;
		
		uint64 IndexCount = ModelDll.GetIndexCount(DataSet, Name);
		if (CheckDllUserError()) return;
		std::vector<char *> IndexNames(IndexCount);
		ModelDll.GetIndexes(DataSet, Name, IndexNames.data());
		if (CheckDllUserError()) return;
		
		
		//EIndexList[IndexSet]->AddColumn(Name);
		EIndexList[IndexSet]->HeaderTab(0).SetText(Name);
		for(size_t Idx = 0; Idx < IndexCount; ++Idx)
		{
			IndexList[IndexSet]->Add(IndexNames[Idx]);
			
			EIndexList[IndexSet]->Add(IndexNames[Idx]);
		}
		IndexList[IndexSet]->GoBegin();
		IndexList[IndexSet]->Show();
		
		EIndexList[IndexSet]->GoBegin();
		EIndexList[IndexSet]->Show();
		
		//MaxIndexCount = MaxIndexCount > IndexCount ? MaxIndexCount : IndexCount;
	}
	
	PlotModeChange();
	
	StoreSettings();
}

void MobiView::AddParameterGroupsRecursive(int ParentId, const char *ParentName, int Count)
{
	std::vector<char *> Names(Count);
	ModelDll.GetAllParameterGroups(DataSet, Names.data(), ParentName);
	if(CheckDllUserError()) return;
	
	for(int Idx = 0; Idx < Count; ++Idx)
	{
		uint64 ChildCount = ModelDll.GetAllParameterGroupsCount(DataSet, Names[Idx]);
		int GroupId = ParameterGroupSelecter.Add(ParentId, Null, Names[Idx], ChildCount!=0);
		
		if(ChildCount)
		{
			AddParameterGroupsRecursive(GroupId, Names[Idx], ChildCount);
		}
	}
}


void MobiView::SaveBaseline()
{
	if(hinstModelDll && DataSet && ModelDll.GetTimesteps && ModelDll.GetTimesteps(DataSet) != 0)
	{
		//TODO: Ask if we really want to overwrite existing baseline?
		if(BaselineDataSet)
		{
			ModelDll.DeleteDataSet(BaselineDataSet);
		}
		
		BaselineDataSet = ModelDll.CopyDataSet(DataSet, true);
		PlotMajorMode.EnableCase(MajorMode_CompareBaseline);
		
		Log("Baseline saved");
		
		RePlot(); //In case we had selected baseline already, and now the baseline changed.
	}
	else
	{
		Log("You can only save a baseline after the model has been run once");
	}
}


void MobiView::ClosingChecks()
{
	if(ParametersWereChangedSinceLastSave)
	{
		int Cl = PromptYesNo("Parameters have been edited since last save. Do you still want to exit MobiView?");
		if(Cl)
		{
			Close();
		}
	}
	else
	{
		Close();
	}
}


GUI_APP_MAIN
{
	MobiView().Run();
}
