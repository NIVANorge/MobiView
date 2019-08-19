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
	bar.Add(IconImg::BatchStructure(), THISBACK(OpenStructureView)).Tip("View model equation batch structure");
	bar.Separator();
	bar.Add(IconImg::SaveBaseline(), THISBACK(SaveBaseline)).Tip("Save baseline").Key(K_CTRL_B);
	bar.Separator();
	bar.Add(IconImg::SaveCsv(), THISBACK(SaveToCsv)).Tip("Save results to .csv").Key(K_CTRL_E);
	
}

MobiView::MobiView() : Plotter(this)
{
	Title("MobiView").MinimizeBox().Sizeable().Zoomable().Icon(MainIconImg::i4());
	
	UpperHorizontal.Horz();
	UpperHorizontal.Add(ParameterGroupSelecter);
	UpperHorizontal.Add(Params);
	UpperHorizontal.Add(PlotInfo);
	UpperHorizontal.Add(LogBox);
	
	UpperHorizontal.SetPos(1500, 0).SetPos(7000, 1).SetPos(8500, 2);
	
	EquationSelecterRect.Add(EquationSelecter.HSizePos().VSizePos(0, 30));
	EquationSelecterRect.Add(ShowFavorites.SetLabel("Show favorites only").HSizePos(5).BottomPos(5));
	
	LowerHorizontal.Horz();
	LowerHorizontal.Add(Plotter);
	LowerHorizontal.Add(EquationSelecterRect);
	LowerHorizontal.Add(InputSelecter);
	
	LowerHorizontal.SetPos(7000, 0).SetPos(8500, 1);

	MainVertical.Vert(UpperHorizontal, LowerHorizontal);
	
	MainVertical.SetPos(3000, 0);

	Add(MainVertical);
	
	
	WhenClose = THISBACK(ClosingChecks);
	
	AddFrame(Tool);
	Tool.Set(THISBACK(SubBar));
	
	hinstModelDll = NULL;
	ModelDll = {};
	DataSet = nullptr;
	
	LogBox.SetColor(TextCtrl::PAPER_READONLY, LogBox.GetColor(TextCtrl::PAPER_NORMAL));
	PlotInfo.SetColor(TextCtrl::PAPER_READONLY, PlotInfo.GetColor(TextCtrl::PAPER_NORMAL));
	
	ParameterGroupSelecter.SetRoot(Null, String("Parameter groups"));
	
	Params.ParameterView.AddColumn("Name").HeaderTab();
	Params.ParameterView.AddColumn("Value").HeaderTab();
	Params.ParameterView.AddColumn("Min").HeaderTab();
	Params.ParameterView.AddColumn("Max").HeaderTab();
	Params.ParameterView.AddColumn("Unit").HeaderTab();
	Params.ParameterView.AddColumn("Description").HeaderTab();
	
	Params.ParameterView.ColumnWidths("20 12 10 10 10 38");
	
	ParameterGroupSelecter.WhenSel = THISBACK(RefreshParameterView);
	
	IndexSetName[0] = &Params.IndexSetName1;
	IndexSetName[1] = &Params.IndexSetName2;
	IndexSetName[2] = &Params.IndexSetName3;
	IndexSetName[3] = &Params.IndexSetName4;
	IndexSetName[4] = &Params.IndexSetName5;
	IndexSetName[5] = &Params.IndexSetName6;
	
	IndexList[0]    = &Params.IndexList1;
	IndexList[1]    = &Params.IndexList2;
	IndexList[2]    = &Params.IndexList3;
	IndexList[3]    = &Params.IndexList4;
	IndexList[4]    = &Params.IndexList5;
	IndexList[5]    = &Params.IndexList6;
	
	IndexLock[0]    = &Params.IndexLock1;
	IndexLock[1]    = &Params.IndexLock2;
	IndexLock[2]    = &Params.IndexLock3;
	IndexLock[3]    = &Params.IndexLock4;
	IndexLock[4]    = &Params.IndexLock5;
	IndexLock[5]    = &Params.IndexLock6;
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		IndexSetName[Idx]->Hide();
		IndexList[Idx]->Hide();
		IndexList[Idx]->Disable();
		IndexList[Idx]->WhenAction = THISBACK(RefreshParameterView);
		
		IndexLock[Idx]->Hide();
	}
	
	SetDateFormat("%d-%02d-%02d");
	SetDateScan("ymd");
	
	EquationSelecter.NoGrid();
	EquationSelecter.Disable();
	EquationSelecter.AddColumn("Equation").HeaderTab();
	EquationSelecter.AddColumn("F.").HeaderTab();
	EquationSelecter.AddColumn();
	EquationSelecter.WhenAction = THISBACK(PlotModeChange);
	EquationSelecter.MultiSelect();
	EquationSelecter.ColumnWidths("85 15 0");
	EquationSelecter.HeaderObject().HideTab(2);
	
	EquationSelecter.HeaderTab(0).WhenAction = [this]()
	{
		if(!this->EquationSelecterIsSorted)
			this->EquationSelecter.Sort(0);
		else
			this->EquationSelecter.Sort(2);
		this->EquationSelecterIsSorted = !this->EquationSelecterIsSorted;
	};
	
	InputSelecter.AddColumn("Input");
	InputSelecter.AddColumn();
	InputSelecter.WhenAction = THISBACK(PlotModeChange);
	InputSelecter.MultiSelect();
	InputSelecter.NoGrid();
	InputSelecter.HeaderObject().HideTab(1);
	
	InputSelecter.HeaderTab(0).WhenAction = [this]()
	{
		if(!this->InputSelecterIsSorted)
			this->InputSelecter.Sort(0);
		else
			this->InputSelecter.Sort(1);
		this->InputSelecterIsSorted = !this->InputSelecterIsSorted;
	};
	
	ShowFavorites.WhenAction = THISBACK(UpdateEquationSelecter);
	
	
	
	
	
	//Load in some settings
	String SettingsFile = LoadFile(GetDataFile("settings.json"));
	Value SettingsJson = ParseJSON(SettingsFile);
	
	//NOTE: We have to load in this info here already, otherwise it is lost when exiting the
	//application before loading in files.
	String PreviouslyLoadedDll         = SettingsJson["Model dll path"];
	String PreviouslyLoadedParameterFile = SettingsJson["Parameter file path"];
	String PreviouslyLoadedInputFile     = SettingsJson["Input file path"];
	DllFile       = PreviouslyLoadedDll.ToStd();
	ParameterFile = PreviouslyLoadedParameterFile.ToStd();
	InputFile     = PreviouslyLoadedInputFile.ToStd();
	
	Value WindowDim = SettingsJson["Window dimensions"];
	if(WindowDim.GetCount() == 2 && (int)WindowDim[0] > 0 && (int)WindowDim[1] > 0)
	{
		SetRect(0, 0, (int)WindowDim[0], (int)WindowDim[1]);
	}
	
	Value MainVertPos = SettingsJson["Main vertical splitter"];
	if((int)MainVertPos > 0)
	{
		MainVertical.SetPos((int)MainVertPos, 0);
	}
	
	Value UpperHorzPos = SettingsJson["Upper horizontal splitter"];
	if(UpperHorzPos.GetCount() == UpperHorizontal.GetCount())
	{
		for(int Idx = 0; Idx < UpperHorzPos.GetCount(); ++Idx)
		{
			UpperHorizontal.SetPos((int)UpperHorzPos[Idx], Idx);
		}
	}
	
	Value LowerHorzPos = SettingsJson["Lower horizontal splitter"];
	if(LowerHorzPos.GetCount() == LowerHorizontal.GetCount())
	{
		for(int Idx = 0; Idx < LowerHorzPos.GetCount(); ++Idx)
		{
			LowerHorizontal.SetPos((int)LowerHorzPos[Idx], Idx);
		}
	}
	
	if((bool)SettingsJson["Maximize"]) Maximize();
}


void MobiView::PlotModeChange()
{
	Plotter.PlotModeChange();
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

void MobiView::OpenStructureView()
{
	if(!hinstModelDll || !DataSet)
	{
		Log("Can't visualize the model before a dataset is loaded in.");
		return;
	}
	
	if(!StructureView)
	{
		StructureView = new StructureViewWindow(this);
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
		("Parameter file path", String(ParameterFile))
		("Input file path", String(InputFile));
	
	Json Favorites;
	
	String DllFileName = GetFileName(DllFile.data());
	
	for(const auto &K : Eq.GetKeys())
	{
		if(K != DllFileName || !hinstModelDll)
		{
			Favorites(K.ToString(), Eq[K]);
		}
	}
	
	if(hinstModelDll) // If the dll file is not actually loaded, the favorites are not stored in the EquationSelecter, just keep what was there originally instead
	{
		JsonArray FavForCurrent;
		for(int Row = 0; Row < EquationSelecter.GetCount(); ++Row)
		{
			if(EquationSelecter.Get(Row, 1)) //I.e. if it was favorited
			{
				FavForCurrent << EquationSelecter.Get(Row, 0);
			}
		}
		Favorites(DllFileName, FavForCurrent);
	}
	
	SettingsJson("Favorite equations", Favorites);
	
	JsonArray WindowDim;
	WindowDim << GetSize().cx << GetSize().cy;
	SettingsJson("Window dimensions", WindowDim);
	
	SettingsJson("Main vertical splitter", MainVertical.GetPos(0));
	
	JsonArray UpperHorzPos;
	for(int Idx = 0; Idx < UpperHorizontal.GetCount(); ++Idx)
	{
		UpperHorzPos << UpperHorizontal.GetPos(Idx);
	}
	SettingsJson("Upper horizontal splitter", UpperHorzPos);
	
	JsonArray LowerHorzPos;
	for(int Idx = 0; Idx < LowerHorizontal.GetCount(); ++Idx)
	{
		LowerHorzPos << LowerHorizontal.GetPos(Idx);
	}
	SettingsJson("Lower horizontal splitter", LowerHorzPos);
	
	SettingsJson("Maximize", IsMaximized());
	
	SaveFile("settings.json", SettingsJson.ToString());
	
}


void MobiView::Load()
{
	if(hinstModelDll)  //NOTE: If a model was previously loaded, we have to do cleanup to prepare for a new Load().
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
		
		Params.ParameterView.Clear();
		
		for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
		{
			IndexList[Idx]->Clear();
			IndexList[Idx]->Hide();
			Plotter.EIndexList[Idx]->Clear();
			Plotter.EIndexList[Idx]->Hide();
			IndexLock[Idx]->Hide();
			IndexSetName[Idx]->Hide();
		}
		
		IndexSetNameToId.clear();
		
		Plotter.Plot.RemoveAllSeries();
		Plotter.PlotData.Clear();
		Plotter.PlotWasAutoResized = false;
		
		Plotter.PlotMajorMode.DisableCase(MajorMode_CompareBaseline);
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
	ParameterFile = ParameterSel.Get().ToStd();
	
	Success = ParameterFile.size() > 0;
	
	if(!Success) return;
	
	Log(String("Loading parameter file: ") + ParameterFile.data());
	
	DataSet = ModelDll.SetupModel(ParameterFile.data(), InputFile.data());
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
			
			EquationSelecter.Add(ResultNames[Idx], IsFavorite, Row);
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
		InputSelecter.Add(InputNames[Idx], (int)Idx);
	}

	Plotter.PlotMajorMode.Enable();
	Plotter.PlotMajorMode.DisableCase(MajorMode_CompareBaseline);
	
	
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
		
		Plotter.EIndexList[IndexSet]->HeaderTab(0).SetText(Name);
		for(size_t Idx = 0; Idx < IndexCount; ++Idx)
		{
			IndexList[IndexSet]->Add(IndexNames[Idx]);
			
			Plotter.EIndexList[IndexSet]->Add(IndexNames[Idx]);
		}
		IndexList[IndexSet]->GoBegin();
		IndexList[IndexSet]->Show();
		
		Plotter.EIndexList[IndexSet]->GoBegin();
		Plotter.EIndexList[IndexSet]->Show();
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

void MobiView::RunModel()
{
	if(!hinstModelDll || !ModelDll.RunModel)
	{
		Log("Model can only be run once a model has been loaded along with a parameter and input file!");
		return;
	}
	
	auto Begin = std::chrono::high_resolution_clock::now();
	ModelDll.RunModel(DataSet);
	auto End = std::chrono::high_resolution_clock::now();
	double Ms = std::chrono::duration_cast<std::chrono::milliseconds>(End - Begin).count();
	
	CheckDllUserError();
	
	EquationSelecter.Enable();
	
	PlotModeChange(); //NOTE: Refresh the plot if necessary since the data can have changed after a new run.
	
	Log(String("Model was run.\nDuration: ") << Ms << " ms.");
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
		Plotter.PlotMajorMode.EnableCase(MajorMode_CompareBaseline);
		
		Log("Baseline saved");
		
		Plotter.RePlot(); //In case we had selected baseline already, and now the baseline changed.
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
			StoreSettings();
			Close();
		}
	}
	else
	{
		StoreSettings();
		Close();
	}
}


GUI_APP_MAIN
{
	MobiView().Run();
}
