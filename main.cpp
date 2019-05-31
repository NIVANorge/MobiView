#include "MobiView.h"
#include "DllInterface.h"

#include <vector>
#include <iomanip>

#include "ParameterEditing.h"
#include "Plotting.h"
#include "Statistics.h"

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
	bar.Separator();
	bar.Add(IconImg::Run(), THISBACK(RunModel)).Tip("Run model").Key(K_F7);
	bar.Separator();
	bar.Add(IconImg::SaveBaseline(), THISBACK(SaveBaseline)).Tip("Save baseline").Key(K_CTRL_B);
}

MobiView::MobiView()
{
	AddFrame(Tool);
	Tool.Set(THISBACK(SubBar));
	
	hinstModelDll = NULL;
	ModelDll = {};
	DataSet = nullptr;
	
	CtrlLayout(*this, "MobiView");
	Sizeable().Zoomable();
	
	//Plot.SizePos();
	
	EquationSelecter.VertGrid(false);
	EquationSelecter.HorzGrid(false);
	EquationSelecter.Disable();
	HeaderCtrl::Column& ENameCol = EquationSelecter.AddColumn("Equation").HeaderTab();
	HeaderCtrl::Column& EFavCol  = EquationSelecter.AddColumn("F.").HeaderTab();
	EquationSelecter.WhenAction = THISBACK(PlotModeChange);
	EquationSelecter.MultiSelect();
	
	ShowFavorites.WhenAction = THISBACK(UpdateEquationSelecter);
	
	ENameCol.SetRatio(0.85);
	EFavCol.SetRatio(0.15);
	
	//NOTE: For some reason the columns don't properly resize unless we do this to make it
	//update.
	EquationSelecter.HeaderObject().ShowTab(0, false);
	EquationSelecter.HeaderObject().ShowTab(0, true);
	
	LogBox.SetColor(TextCtrl::PAPER_READONLY, LogBox.GetColor(TextCtrl::PAPER_NORMAL));
	PlotInfo.SetColor(TextCtrl::PAPER_READONLY, PlotInfo.GetColor(TextCtrl::PAPER_NORMAL));
	
	InputSelecter.AddColumn("Input");
	InputSelecter.WhenAction = THISBACK(PlotModeChange);
	InputSelecter.MultiSelect();
	InputSelecter.HorzGrid(false);
	InputSelecter.VertGrid(false);
	
	ParameterGroupSelecter.AddColumn("Parameter group");
	
	HeaderCtrl::Column& NameCol  = ParameterView.AddColumn("Name").HeaderTab();
	HeaderCtrl::Column& ValueCol = ParameterView.AddColumn("Value").HeaderTab();
	HeaderCtrl::Column& MinCol   = ParameterView.AddColumn("Min").HeaderTab();
	HeaderCtrl::Column& MaxCol   = ParameterView.AddColumn("Max").HeaderTab();
	HeaderCtrl::Column& UnitCol  = ParameterView.AddColumn("Unit").HeaderTab();
	HeaderCtrl::Column& DescCol  = ParameterView.AddColumn("Description").HeaderTab();
	
	NameCol.SetRatio(0.2);
	ValueCol.SetRatio(0.12);
	MinCol.SetRatio(0.1);
	MaxCol.SetRatio(0.1);
	UnitCol.SetRatio(0.1);
	DescCol.SetRatio(0.38);
	
	//NOTE: For some reason the columns don't properly resize unless we do this to make it
	//update.
	ParameterView.HeaderObject().ShowTab(0, false);
	ParameterView.HeaderObject().ShowTab(0, true);
	
	//ParameterView.NoCursor();
	
	ParameterGroupSelecter.WhenSel = THISBACK(RefreshParameterView);
	
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
	}
	
	SetDateFormat("%d-%02d-%02d");
	SetDateScan("ymd");
	
	
	Plot.SetFastViewX(true);
	Plot.SetSequentialXAll(true);
	Plot.SetMouseHandling(true, false);
	
	Plot.SetPlotAreaLeftMargin(50);
	
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
			EquationSelecter.SetLineCy(Row, Null);
		}
		else
		{
			EquationSelecter.SetLineCy(Row, 0);
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
		Log("Sorry! Can't currently reload when you have already loaded. Complain to Magnus!");
		return;
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
	if(!PreviouslyLoadedInputFile.IsEmpty() && FileExists(PreviouslyLoadedInputFile))
	{
		InputSel.PreSelect(PreviouslyLoadedInputFile);
	}
	else
	{
		InputSel.PreSelect(GetFileFolder(DllFile.data()));
	}
	InputSel.ExecuteOpen();
	InputFile = InputSel.Get().ToStd();
	
	Success = InputFile.size() > 0;
	
	if(!Success) return;
	
	Log(String("Loading input file: ") + InputFile.data());
	
	FileSel ParameterSel;
	ParameterSel.Type("Parameter dat files", "*.dat");
	if(!PreviouslyLoadedParameterFile.IsEmpty() && FileExists(PreviouslyLoadedParameterFile))
	{
		ParameterSel.PreSelect(PreviouslyLoadedParameterFile);
	}
	else
	{
		ParameterSel.PreSelect(GetFileFolder(InputFile.data()));
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
	std::vector<char *> ParameterGroupNames(ParameterGroupCount);
	ModelDll.GetAllParameterGroups(DataSet, ParameterGroupNames.data(), nullptr);
	if (CheckDllUserError()) return;

	for(size_t Idx = 0; Idx < ParameterGroupCount; ++Idx)
	{
		ParameterGroupSelecter.Add(ParameterGroupNames[Idx]);
	}
	
	
	uint64 IndexSetCount = ModelDll.GetIndexSetsCount(DataSet);
	if (CheckDllUserError()) return;
	std::vector<char *> IndexSets(IndexSetCount);
	ModelDll.GetIndexSets(DataSet, IndexSets.data());
	if (CheckDllUserError()) return;
	
	size_t MaxIndexCount = 0;
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
		
		
		EIndexList[IndexSet]->AddColumn(Name);
		for(size_t Idx = 0; Idx < IndexCount; ++Idx)
		{
			IndexList[IndexSet]->Add(IndexNames[Idx]);
			
			EIndexList[IndexSet]->Add(IndexNames[Idx]);
		}
		IndexList[IndexSet]->GoBegin();
		IndexList[IndexSet]->Show();
		
		EIndexList[IndexSet]->GoBegin();
		EIndexList[IndexSet]->Show();
		
		MaxIndexCount = MaxIndexCount > IndexCount ? MaxIndexCount : IndexCount;
	}
	
	PlotModeChange();
	
	StoreSettings();
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


GUI_APP_MAIN
{
	MobiView().Run();
}
