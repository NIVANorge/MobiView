#include "MobiView.h"

#include <vector>
#include <iomanip>



#define IMAGECLASS IconImg
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>


#define IMAGECLASS MainIconImg
#define IMAGEFILE <MobiView/MobiView.iml>
#include <Draw/iml.h>


void MobiView::Log(String Msg, bool Error)
{
	auto T = std::time(nullptr);
	auto TM = *std::localtime(&T);
	std::stringstream Oss;
	Oss << std::put_time(&TM, "%H:%M:%S : ");
	
	String FormatMsg = "";
	FormatMsg << Oss.str().data();
	FormatMsg << Msg;
	FormatMsg << "&&";
	FormatMsg.Replace("[", "`[");
	FormatMsg.Replace("]", "`]");
	FormatMsg.Replace("_", "`_");
	FormatMsg.Replace("<", "`<");
	FormatMsg.Replace(">", "`>");
	FormatMsg.Replace("-", "`-");
	FormatMsg.Replace("/", "\1/\1");
	FormatMsg.Replace("\\", "\1\\\1");
	FormatMsg.Replace("\n", "&");
	
	if(Error)
		FormatMsg = String("[@R ") + FormatMsg + "]";
	
	LogBox.Append(FormatMsg);
	LogBox.ScrollEnd();
}


void MobiView::HandleDllError()
{
	Log("There was an error when trying to load the model dll. This could be because the dll is compiled from a version of Mobius that is too old compared to the build of MobiView you are using or vice versa.");
	Log(ModelDll.GetDllError(), true);
}

bool MobiView::CheckDllUserError()
{
	bool Error = false;
	if(ModelDll.IsLoaded())
	{
		const uint64 MsgBufLen = 2048;
		char MsgBuf[MsgBufLen];
		
		bool Warning = false;
		String Warnstr;
		int Warnlen = ModelDll.EncounteredWarning(MsgBuf, MsgBufLen);
		while(Warnlen > 0)
		{
			Warnstr += MsgBuf;
			Warning = true;
			Warnlen = ModelDll.EncounteredWarning(MsgBuf, MsgBufLen);
		}
		if(Warning) Log(Warnstr);
		
		String Errstr;
		int Errlen = ModelDll.EncounteredError(MsgBuf, MsgBufLen);
		while(Errlen > 0)
		{
			Errstr += MsgBuf;
			Error = true;
			Errlen = ModelDll.EncounteredError(MsgBuf, MsgBufLen);
		}
		if(Error) Log(Errstr, true);
	}
	return Error;
}

void MobiView::SubBar(Bar &bar)
{
	bar.Add(IconImg::Open(), THISBACK(Load)).Tip("Load model, parameter and input files").Key(K_CTRL_O);
	bar.Add(IconImg::Save(), THISBACK(SaveParameters)).Tip("Save parameters").Key(K_CTRL_S);
	bar.Add(IconImg::SaveAs(), THISBACK(SaveParametersAs)).Tip("Save parameters as").Key(K_ALT_S);
	bar.Add(IconImg::Search(), THISBACK(OpenSearch)).Tip("Search parameters").Key(K_CTRL_F);
	bar.Add(IconImg::ViewReaches(), THISBACK(OpenChangeIndexes)).Tip("Edit indexes");
	bar.Separator();
	bar.Add(IconImg::Run(), THISBACK(RunModel)).Tip("Run model").Key(K_F7);
	bar.Add(IconImg::ViewMorePlots(), THISBACK(OpenAdditionalPlotView)).Tip("Open additional plot view");
	bar.Add(IconImg::SaveCsv(), THISBACK(SaveToCsv)).Tip("Save results to .csv").Key(K_CTRL_R);
	//bar.Add(IconImg::SaveCsv(), THISBACK(SaveInputsAsDat)).Tip("Save inputs to .dat");
	bar.Separator();
	//bar.Gap(60);
	bar.Add(IconImg::SaveBaseline(), THISBACK(SaveBaseline)).Tip("Save baseline");
	bar.Add(IconImg::RevertBaseline(), THISBACK(RevertBaseline)).Tip("Revert to baseline");
	bar.Add(IconImg::Perturb(), THISBACK(OpenSensitivityView)).Tip("Sensitivity perturbation");
	bar.Add(IconImg::Optimize(), THISBACK(OpenOptimizationView)).Tip("Optimization and MCMC setup");
	bar.Separator();
	bar.Add(IconImg::StatSettings(), THISBACK(OpenStatSettings)).Tip("Edit statistics settings");
	bar.Add(IconImg::BatchStructure(), THISBACK(OpenStructureView)).Tip("View model equation batch structure");
	bar.Add(IconImg::Info(), THISBACK(OpenModelInfoView)).Tip("View model information");
}

MobiView::MobiView() : Plotter(this)
{
	Title("MobiView").MinimizeBox().Sizeable().Zoomable().Icon(MainIconImg::i4());
	
	PlotInfoRect.Add(PlotInfo.HSizePos().VSizePos(0, 25));
	CalibrationIntervalLabel.SetText("Stat interval:");
	GOFOnOption.SetLabel("Show GOF");
	GOFOnOption.SetData(1); //TODO: Should this be in config?
	PlotInfoRect.Add(CalibrationIntervalLabel.LeftPos(0).BottomPos(5));
	PlotInfoRect.Add(CalibrationIntervalStart.LeftPos(70, 70).BottomPos(0));
	PlotInfoRect.Add(CalibrationIntervalEnd.LeftPos(145, 70).BottomPos(0));
	PlotInfoRect.Add(GOFOnOption.LeftPos(220).BottomPos(5));
	
	UpperHorizontal.Horz();
	UpperHorizontal.Add(ParameterGroupSelecter);
	UpperHorizontal.Add(Params);
	UpperHorizontal.Add(PlotInfoRect);
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
	
	ModelDll = {};
	DataSet = nullptr;
	
	LogBox.SetEditable(false);
	PlotInfo.SetEditable(false);
	
	
	ParameterGroupSelecter.SetRoot(Null, String("Parameter groups"));
	
	Params.ParameterView.AddColumn("Name");
	Params.ParameterView.AddColumn("Value");
	Params.ParameterView.AddColumn("Min");
	Params.ParameterView.AddColumn("Max");
	Params.ParameterView.AddColumn("Unit");
	Params.ParameterView.AddColumn("Description");
	
	Params.ParameterView.ColumnWidths("20 12 10 10 10 38");
	
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
	
	IndexExpand[0]    = &Params.IndexExpand1;
	IndexExpand[1]    = &Params.IndexExpand2;
	IndexExpand[2]    = &Params.IndexExpand3;
	IndexExpand[3]    = &Params.IndexExpand4;
	IndexExpand[4]    = &Params.IndexExpand5;
	IndexExpand[5]    = &Params.IndexExpand6;
	
	CurrentSelectedParameter.Valid = false;
	
	auto SensitivityWindowUpdate = [this]()
	{
		//NOTE: This HAS to be done before the update of the sensitivity window.
		//It is also used by the Optimization window. We have to do it here since
		//the parameters can be out of focus when in the other window!
		this->CurrentSelectedParameter = this->GetSelectedParameter();
		
		SensitivityWindow.Update();
	};
	
	//TODO: This is not sufficient. It is not updated when selection changes within an individual row!
	// What we want is something like WhenLeftClick, but that
	// doesn't work either! Maybe we have to set one on each individual control?
	Params.ParameterView.WhenSel << SensitivityWindowUpdate;
	
	ParameterGroupSelecter.WhenSel << [this](){ RefreshParameterView(false); };
	ParameterGroupSelecter.WhenSel << SensitivityWindowUpdate;
	
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		IndexSetName[Idx]->Hide();
		IndexList[Idx]->Hide();
		IndexList[Idx]->Disable();
		IndexList[Idx]->WhenAction << [this](){ RefreshParameterView(false); };
		
		IndexLock[Idx]->Hide();
		IndexLock[Idx]->WhenAction << SensitivityWindowUpdate;
		IndexExpand[Idx]->Hide();
		IndexExpand[Idx]->WhenAction << [this, Idx](){ ExpandIndexSetClicked(Idx); };
	}
	
	SetDateFormat("%d-%02d-%02d");
	SetDateScan("ymd");
	
	EquationSelecter.NoGrid();
	EquationSelecter.Disable();
	EquationSelecter.AddColumn("Equation");
	EquationSelecter.AddColumn("F.");
	EquationSelecter.AddColumn();
	EquationSelecter.WhenSel = THISBACK(PlotModeChange);
	EquationSelecter.MultiSelect();
	EquationSelecter.ColumnWidths("85 15 0");
	EquationSelecter.HeaderObject().HideTab(2);
	
	/*   //NOTE: This no longer makes sense the way we organize modules..
	EquationSelecter.HeaderTab(0).WhenAction = [this]()
	{
		if(!this->EquationSelecterIsSorted)
			this->EquationSelecter.Sort(0);
		else
			this->EquationSelecter.Sort(2);
		this->EquationSelecterIsSorted = !this->EquationSelecterIsSorted;
	};
	*/
	
	InputSelecter.AddColumn("Input");
	InputSelecter.AddColumn();
	InputSelecter.WhenSel = THISBACK(PlotModeChange);
	InputSelecter.MultiSelect();
	InputSelecter.NoGrid();
	InputSelecter.HeaderObject().HideTab(1);
	
	/*
	InputSelecter.HeaderTab(0).WhenAction = [this]()
	{
		if(!this->InputSelecterIsSorted)
			this->InputSelecter.Sort(0);
		else
			this->InputSelecter.Sort(1);
		this->InputSelecterIsSorted = !this->InputSelecterIsSorted;
	};
	*/
	
	ShowFavorites.WhenAction = THISBACK(UpdateEquationSelecter);
	
	CalibrationIntervalStart.WhenAction = THISBACK(PlotRebuild);
	CalibrationIntervalEnd.WhenAction   = THISBACK(PlotRebuild);
	GOFOnOption.WhenAction              = THISBACK(PlotRebuild);
	
	
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
		SetRect(0, 0, (int)WindowDim[0], (int)WindowDim[1]);
	
	Value MainVertPos = SettingsJson["Main vertical splitter"];
	if((int)MainVertPos > 0)
		MainVertical.SetPos((int)MainVertPos, 0);
	
	Value UpperHorzPos = SettingsJson["Upper horizontal splitter"];
	if(UpperHorzPos.GetCount() == UpperHorizontal.GetCount())
	{
		for(int Idx = 0; Idx < UpperHorzPos.GetCount(); ++Idx)
			UpperHorizontal.SetPos((int)UpperHorzPos[Idx], Idx);
	}
	
	Value LowerHorzPos = SettingsJson["Lower horizontal splitter"];
	if(LowerHorzPos.GetCount() == LowerHorizontal.GetCount())
	{
		for(int Idx = 0; Idx < LowerHorzPos.GetCount(); ++Idx)
			LowerHorizontal.SetPos((int)LowerHorzPos[Idx], Idx);
	}
	
	if((bool)SettingsJson["Maximize"]) Maximize();
	
	
	ValueMap StatsJson = SettingsJson["Statistics"];
	#define SET_SETTING(Handle, Name, Type) \
		Value Handle##Json = StatsJson[#Handle]; \
		if(!IsNull(Handle##Json)) \
		{ \
			StatSettings.Display##Handle = (int)Handle##Json; \
		}
	#define SET_RES_SETTING(Handle, Name, Type) SET_SETTING(Handle, Name, Type)
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	#undef SET_RES_SETTING
	
	ValueArray Percentiles = StatsJson["Percentiles"];
	if(!IsNull(Percentiles) && Percentiles.GetCount() >= 1)
	{
		StatSettings.Percentiles.clear();
		for(Value Perc : Percentiles)
		{
			double Val = Perc;  //TODO: Should check that it is a valid value between 0 and 100. Would only be a problem if somebody edited it by hand though
			StatSettings.Percentiles.push_back(Val);
		}
	}
	
	Value PrecisionJson = StatsJson["Precision"];
	if(!IsNull(PrecisionJson))
		StatSettings.Precision = (int)PrecisionJson;
	Value BFIJson = StatsJson["BFIParam"];
	if(!IsNull(BFIJson))
		StatSettings.EckhardtFilterParam = (double)BFIJson;
	Value ShowInitial = StatsJson["ShowInitial"];
	if(!IsNull(ShowInitial))
		StatSettings.ShowInitialValue = (bool)ShowInitial;

	Value IdxEditWindowDim = SettingsJson["Index set editor window dimensions"];
	if(IdxEditWindowDim.GetCount() == 2 && (int)IdxEditWindowDim[0] > 0 && (int)IdxEditWindowDim[1] > 0)
		ChangeIndexes.SetRect(0, 0, (int)IdxEditWindowDim[0], (int)IdxEditWindowDim[1]);
	
	Value AdditionalPlotViewDim = SettingsJson["Additional plot view dimensions"];
	if(AdditionalPlotViewDim.GetCount() == 2 && (int)AdditionalPlotViewDim[0] > 0 && (int)AdditionalPlotViewDim[1] > 0)
		OtherPlots.SetRect(0, 0, (int)AdditionalPlotViewDim[0], (int)AdditionalPlotViewDim[1]);
	
	
	// NOTE: Just to make it initially set the message that no model is loaded
	Plotter.MainPlot.BuildPlot(this, nullptr, true, PlotInfo, true);
	
	
	Search.ParentWindow = this;
	StructureView.ParentWindow = this;
	ChangeIndexes.ParentWindow = this;
	ChangeIndexes.Branches.ParentWindow = this;
	EditStatSettings.ParentWindow = this;
	OtherPlots.ParentWindow = this;
	ModelInfo.ParentWindow = this;
	SensitivityWindow.ParentWindow = this;
	OptimizationWin.ParentWindow = this;
	MCMCResultWin.ParentWindow = this;
}


void MobiView::PlotModeChange()
{
	//NOTE: This is when changing any of the controls that affect the main plot.
	Plotter.PlotModeChange();
}

void MobiView::PlotRebuild()
{
	Plotter.RePlot(true);
	if(OtherPlots.IsOpen())
		OtherPlots.BuildAll(true);
	BaselineWasJustSaved = false;
}


void MobiView::OpenSearch()
{
	if(!ModelDll.IsLoaded() || !DataSet)
	{
		Log("Can't search parameters before a dataset is loaded.", true);
		return;
	}
	
	if(!Search.IsOpen())
		Search.Open();
}

void MobiView::OpenStatSettings()
{
	if(!EditStatSettings.IsOpen())
	{
		EditStatSettings.LoadData();
		EditStatSettings.Open();
	}
}


void MobiView::OpenAdditionalPlotView()
{
	if(!OtherPlots.IsOpen())
	{
		OtherPlots.Open();
		OtherPlots.BuildAll();
	}
}

void MobiView::OpenStructureView()
{
	if(!ModelDll.IsLoaded() || !DataSet)
	{
		Log("Can't visualize the model structure before a dataset is loaded.", true);
		return;
	}
	
	if(!StructureView.IsOpen())
	{
		StructureView.RefreshText();
		StructureView.Open();
	}
}

void MobiView::OpenModelInfoView()
{
	if(!ModelInfo.IsOpen())
	{
		ModelInfo.RefreshText();
		ModelInfo.Open();
	}
}

void MobiView::OpenChangeIndexes()
{
	if(!ModelDll.IsLoaded() || !DataSet)
	{
		Log("Can't edit indexes before a dataset is loaded.", true);
		return;
	}
	
	if(!ChangeIndexes.IsOpen())
	{
		ChangeIndexes.RefreshData();
		ChangeIndexes.Open();
	}
}

void MobiView::OpenSensitivityView()
{
	if(!ModelDll.IsLoaded() || !DataSet)
	{
		Log("Can't do a perturbation run before a dataset is loaded.", true);
		return;
	}
	
	if(!SensitivityWindow.IsOpen())
		SensitivityWindow.Open();
}

void MobiView::OpenOptimizationView()
{
	if(!ModelDll.IsLoaded() || !DataSet)
	{
		Log("Can't do an optimization setup before a dataset is loaded.", true);
		return;
	}
	
	if(!OptimizationWin.IsOpen())
		OptimizationWin.Open();
}



void MobiView::UpdateEquationSelecter()
{
	//TODO: Unfavouriting your last selected series does not remove the plot.
	
	bool ShowFavOnly = ShowFavorites.GetData();
	
	for(int Row = 0; Row < EquationSelecter.GetCount(); ++Row)
	{
		Ctrl *Star =  EquationSelecter.GetCtrl(Row, 1);
		if(!ShowFavOnly || (Star && Star->GetData()))
			EquationSelecter.ShowLine(Row, true);
		else
		{
			EquationSelecter.HideLine(Row);
			EquationSelecter.Select(Row, false, false); //TODO: Should probably think about whether we really want this actually..
		}
	}
	
	PlotModeChange(); //In order to replot in case the selection changed.
	
	StoreSettings(true);
}

void MobiView::StoreSettings(bool OverwriteFavorites)
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
		if(K != DllFileName || !OverwriteFavorites || !ModelDll.IsLoaded())
			Favorites(K.ToString(), Eq[K]);
	}
	
	if(OverwriteFavorites && ModelDll.IsLoaded()) // If the dll file is not actually loaded, the favorites are not stored in the EquationSelecter, just keep what was there originally instead
	{
		JsonArray FavForCurrent;
		for(int Row = 0; Row < EquationSelecter.GetCount(); ++Row)
		{
			if(EquationSelecter.Get(Row, 1)) //I.e. if it was favorited
				FavForCurrent << EquationSelecter.Get(Row, 0);
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
		UpperHorzPos << UpperHorizontal.GetPos(Idx);
	SettingsJson("Upper horizontal splitter", UpperHorzPos);
	
	JsonArray LowerHorzPos;
	for(int Idx = 0; Idx < LowerHorizontal.GetCount(); ++Idx)
		LowerHorzPos << LowerHorizontal.GetPos(Idx);
	SettingsJson("Lower horizontal splitter", LowerHorzPos);
	
	SettingsJson("Maximize", IsMaximized());
	
	JsonArray IdxEditWindowDim;
	IdxEditWindowDim << ChangeIndexes.GetSize().cx << ChangeIndexes.GetSize().cy;
	SettingsJson("Index set editor window dimensions", IdxEditWindowDim);
	
	JsonArray AdditionalPlotViewDim;
	AdditionalPlotViewDim << OtherPlots.GetSize().cx << OtherPlots.GetSize().cy;
	SettingsJson("Additional plot view dimensions", AdditionalPlotViewDim);
	
	
	Json Statistics;
	
	#define SET_SETTING(Handle, Name, Type) \
		Statistics(#Handle, StatSettings.Display##Handle);
	#define SET_RES_SETTING(Handle, Name, Type) SET_SETTING(Handle, Name, Type)
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	#undef SET_RES_SETTING
	
	JsonArray Percentiles;
	for(double Percentile : StatSettings.Percentiles) Percentiles << Percentile;
	Statistics("Percentiles", Percentiles);
	
	Statistics("Precision", StatSettings.Precision);
	Statistics("BFIParam", StatSettings.EckhardtFilterParam);
	Statistics("ShowInitial", StatSettings.ShowInitialValue);
	
	SettingsJson("Statistics", Statistics);
	
	SaveFile("settings.json", SettingsJson.ToString());
	
}


void MobiView::CleanInterface()
{
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
		IndexExpand[Idx]->Hide();
		IndexSetName[Idx]->Hide();
	}
	
	IndexSetNameToId.clear();
	
	Plotter.MainPlot.ClearAll();
	OtherPlots.ClearAll();
	
	PlotInfo.Clear();
	
	OptimizationWin.ClearAll();
}


void MobiView::BuildInterface()
{
	TimestepSize = ModelDll.GetTimestepSize(DataSet);
	
	Plotter.BuildTimeIntervalsCtrl();
	
	String SettingsFile = LoadFile(GetDataFile("settings.json"));
	Value SettingsJson = ParseJSON(SettingsFile);
	
	String DllFileName = GetFileName(DllFile.data());
	auto &FavoriteList = SettingsJson["Favorite equations"][DllFileName];
	
	uint64 ModuleCount = ModelDll.GetAllModulesCount(DataSet);
	std::vector<char *> ModuleNames(ModuleCount);
	std::vector<char *> ModuleVersions(ModuleCount);
	ModelDll.GetAllModules(DataSet, ModuleNames.data(), ModuleVersions.data());
	if (CheckDllUserError()) return;
	

	//Module-less equations //TODO: Clean up code repeat from below..
	uint64 ResultCount = ModelDll.GetAllResultsCount(DataSet, nullptr);
	std::vector<char *> ResultNames(ResultCount);
	std::vector<char *> ResultTypes(ResultCount);
	ModelDll.GetAllResults(DataSet, ResultNames.data(), ResultTypes.data(), nullptr);
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
	
	for(size_t ModuleIdx = 0; ModuleIdx < ModuleCount; ++ModuleIdx)
	{
		uint64 ResultCount = ModelDll.GetAllResultsCount(DataSet, ModuleNames[ModuleIdx]);
		std::vector<char *> ResultNames(ResultCount);
		std::vector<char *> ResultTypes(ResultCount);
		ModelDll.GetAllResults(DataSet, ResultNames.data(), ResultTypes.data(), ModuleNames[ModuleIdx]);
		if (CheckDllUserError()) return;
		
		if(ResultCount > 0)
		{
			EquationSelecter.Add(Format("%s (V%s)", ModuleNames[ModuleIdx], ModuleVersions[ModuleIdx]), Null, Row);
			EquationSelecter.SetLineColor(Row, Color(214, 234, 248));
			EquationSelecter.DisableLine(Row);
			
			++Row;
			for(size_t Idx = 0; Idx < ResultCount; ++Idx)
			{
				if(strcmp(ResultTypes[Idx], "initialvalue") != 0)
				{
					String Name = ResultNames[Idx];
					
					int IsFavorite = (std::find(FavoriteList.begin(), FavoriteList.end(), Name) != FavoriteList.end());
					
					EquationSelecter.Add(Name, IsFavorite, Row);
					EquationSelecterFavControls.Create<StarOption>();
					Ctrl &Favorite = EquationSelecterFavControls.Top();
					
					EquationSelecter.SetCtrl((int)Row, 1, Favorite);
					Favorite.WhenAction = THISBACK(UpdateEquationSelecter);
					++Row;
				}
			}
		}
	}
	

	uint64 InputCount = ModelDll.GetAllInputsCount(DataSet);
	if(CheckDllUserError()) return;
	std::vector<char *> InputNames(InputCount);
	std::vector<char *> InputTypes(InputCount);
	ModelDll.GetAllInputs(DataSet, InputNames.data(), InputTypes.data());
	if (CheckDllUserError()) return;
	
	InputSelecter.Add("Model inputs", Null);
	InputSelecter.SetLineColor(0, Color(214, 234, 248));
	InputSelecter.DisableLine(0);
	
	Row = 1;
	bool AdditionalSectionStarted = false;
	for(size_t Idx = 0; Idx < InputCount; ++Idx)
	{
		if(strcmp(InputTypes[Idx], "additional")==0  && !AdditionalSectionStarted)
		{
			InputSelecter.Add("Additional time series", Null);
			InputSelecter.SetLineColor(Row, Color(214, 234, 248));
			InputSelecter.DisableLine(Row);
			AdditionalSectionStarted = true;
			++Row;
		}
		InputSelecter.Add(InputNames[Idx], Row);
		++Row;
	}

	Plotter.PlotMajorMode.Enable();
	
	ParameterGroupSelecter.Set(0, ModelDll.GetModelName(DataSet));

	uint64 TopGroupCount = ModelDll.GetAllParameterGroupsCount(DataSet, nullptr);
	std::vector<char *> TopGroupNames(TopGroupCount);
	ModelDll.GetAllParameterGroups(DataSet, TopGroupNames.data(), nullptr);
	if (CheckDllUserError()) return;
	
	for(int Idx = 0; Idx < TopGroupCount; ++Idx)
		ParameterGroupSelecter.Add(0, Null, TopGroupNames[Idx], false);
	
	for(int Idx = 0; Idx < ModuleCount; ++Idx)
	{
		uint64 GroupCount = ModelDll.GetAllParameterGroupsCount(DataSet, ModuleNames[Idx]);
		std::vector<char *> GroupNames(GroupCount);
		ModelDll.GetAllParameterGroups(DataSet, GroupNames.data(), ModuleNames[Idx]);
		if (CheckDllUserError()) return;
		
		//If a module has no parameter groups, don't bother to show it in the parameter group view at all.
		if(GroupCount > 0)
		{
			int ModuleTreeId = ParameterGroupSelecter.Add(0, Null, Format("%s (V%s)", ModuleNames[Idx], ModuleVersions[Idx]), true);
			
			for(int GroupIdx = 0; GroupIdx < GroupCount; ++GroupIdx)
				ParameterGroupSelecter.Add(ModuleTreeId, Null, GroupNames[GroupIdx], false);
		}
	}

	ParameterGroupSelecter.OpenDeep(0, true);
	
	uint64 IndexSetCount = ModelDll.GetIndexSetsCount(DataSet);
	
	if(IndexSetCount > MAX_INDEX_SETS)
	{
		PromptOK(Format("MobiView does not currently support models with more than %d index sets. The model you tried to load has %d index sets.", MAX_INDEX_SETS, (int)IndexSetCount));
		return;
	}
	
	std::vector<char *> IndexSets(IndexSetCount);
	std::vector<char *> IndexSetTypes(IndexSetCount);
	ModelDll.GetIndexSets(DataSet, IndexSets.data(), IndexSetTypes.data());
	if (CheckDllUserError()) return;
	
	for(size_t IndexSet = 0; IndexSet < IndexSetCount; ++IndexSet)
	{
		IndexLock[IndexSet]->Show();
		IndexExpand[IndexSet]->Show();
		
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
}


void MobiView::Load()
{
	if(ParametersWereChangedSinceLastSave)
	{
		int Cl = PromptYesNo("Parameters have been edited since the last save. If you load a new dataset now, you will lose them. Continue anyway?");
		if(!Cl) return;
	}
	
	if(ModelDll.IsLoaded())  //NOTE: If a model was previously loaded, we have to do cleanup to prepare for a new Load().
	{
		if(BaselineDataSet)
		{
			ModelDll.DeleteDataSet(BaselineDataSet);
			BaselineDataSet = nullptr;
		}
		
		if(DataSet)
		{
			ModelDll.DeleteModelAndDataSet(DataSet);
			DataSet = nullptr;
		}
		
		ParametersWereChangedSinceLastSave = false;
		CleanInterface();
		
		CalibrationIntervalStart.SetData(Null);
		CalibrationIntervalEnd.SetData(Null);
		
		ModelDll.UnLoad();
	}
	
	
	//TODO: We should probably break up loading of model, parameter and input files into separate steps.
	
	String SettingsFile = LoadFile(GetDataFile("settings.json"));
	Value SettingsJson = ParseJSON(SettingsFile);
	
	String PreviouslyLoadedModel         = SettingsJson["Model dll path"];
	String PreviouslyLoadedParameterFile = SettingsJson["Parameter file path"];
	String PreviouslyLoadedInputFile     = SettingsJson["Input file path"];
	
	FileSel DllSel;
#ifdef PLATFORM_WIN32
	DllSel.Type("Model dll files", "*.dll");
#else
	DllSel.Type("Model shared object files", "*.so");
#endif
	if(!PreviouslyLoadedModel.IsEmpty() && FileExists(PreviouslyLoadedModel))
		DllSel.PreSelect(PreviouslyLoadedModel);
	
	DllSel.ExecuteOpen();
	DllFile = DllSel.Get().ToStd();
	
	bool ChangedDll = DllFile != PreviouslyLoadedModel.ToStd();
	
	bool Success = DllFile.size() > 0;
	
	if(!Success) return;
	
	Log(Format("Loading model dll: %s", DllFile.data()));
	
	Success = ModelDll.Load(DllFile.data());
	
	if(!Success)
	{
		HandleDllError();
		ModelDll.UnLoad();
		return;
	}

	FileSel InputSel;
#ifdef PLATFORM_WIN32
	InputSel.Type("Input .dat or spreadsheet files", "*.dat *.xls *.xlsx");
#else
	InputSel.Type("Input .dat files", "*.dat");
#endif

	if(!ChangedDll && !PreviouslyLoadedInputFile.IsEmpty() && FileExists(PreviouslyLoadedInputFile))
		InputSel.PreSelect(PreviouslyLoadedInputFile);
	else
		InputSel.ActiveDir(GetFileFolder(DllFile.data()));
	InputSel.ExecuteOpen();
	InputFile = InputSel.Get().ToStd();
	
	Success = InputFile.size() > 0;
	
	bool ChangedInputPath = GetFileDirectory(InputFile.data()) != GetFileDirectory(PreviouslyLoadedInputFile);
	
	if(!Success)
	{
		Log("Received empty input file name.", true);
		ModelDll.UnLoad();
		return;
	}
	
	Log(Format("Selecting input file: %s", InputFile.data()));
	
	FileSel ParameterSel;
	ParameterSel.Type("Parameter dat files", "*.dat");
	if(!ChangedInputPath && !ChangedDll && !PreviouslyLoadedParameterFile.IsEmpty() && FileExists(PreviouslyLoadedParameterFile))
		ParameterSel.PreSelect(PreviouslyLoadedParameterFile);
	else
		ParameterSel.ActiveDir(GetFileFolder(InputFile.data()));

	ParameterSel.ExecuteOpen();
	ParameterFile = ParameterSel.Get().ToStd();
	
	Success = ParameterFile.size() > 0;
	
	if(!Success)
	{
		Log("Received empty parameter file name.", true);
		ModelDll.UnLoad();
		return;
	}
	
	Log(Format("Selecting parameter file: %s", ParameterFile.data()));

	Success = false;

	String InputExt = GetFileExt(InputFile.data());
	

	//PromptOK("Bing!");
	
	DataSet = ModelDll.SetupModel(ParameterFile.data(), InputFile.data());
	Success = !CheckDllUserError();
	
	if(!Success)
	{
		ModelDll.UnLoad();
		StoreSettings(false); // So that it still remembers what files you selected for your next attempt at loading.
		return;
	}
	
	BuildInterface();
	
	StoreSettings(false);
}



void MobiView::RunModel()
{
	if(!ModelDll.IsLoaded() || !ModelDll.RunModel)
	{
		Log("A model can only be run once a model has been loaded along with a parameter and input file!", true);
		return;
	}
	
	auto Begin = std::chrono::high_resolution_clock::now();
	ModelDll.RunModel(DataSet, -1);
	auto End = std::chrono::high_resolution_clock::now();
	double Ms = std::chrono::duration_cast<std::chrono::milliseconds>(End - Begin).count();
	
	bool Error = CheckDllUserError();
	
	EquationSelecter.Enable();
	
	PlotRebuild(); //NOTE: Refresh the plot if necessary since the data can have changed after a new run.
	
	RefreshParameterView(true); //NOTE: In case there are computed parameters that are displayed, we need to refresh their values in the view
	
	if(!Error)
		Log(Format("Model was run.\nDuration: %g ms.", Ms ));
}

void MobiView::SaveBaseline()
{
	if(ModelDll.IsLoaded() && DataSet && ModelDll.GetTimesteps(DataSet) != 0)
	{
		//TODO: Ask if we really want to overwrite existing baseline?
		if(BaselineDataSet)
			ModelDll.DeleteDataSet(BaselineDataSet);
		
		BaselineDataSet = ModelDll.CopyDataSet(DataSet, true, true);
		
		Log("Baseline saved");
		
		BaselineWasJustSaved = true;
		
		PlotRebuild(); //In case we had selected baseline already, and now the baseline changed.
	}
	else
		Log("You can only save a baseline after the model has been run once", true);
}

void MobiView::RevertBaseline()
{
	if(ModelDll.IsLoaded() && DataSet && BaselineDataSet)
	{
		int Cl = 1;
		if(ParametersWereChangedSinceLastSave)
			Cl = PromptYesNo("Parameters have been edited since the last save. If you revert to the baseline parameters now you will lose these changes. Do you still want to revert to the baseline?");
		
		if(!Cl) return;
		
		ParametersWereChangedSinceLastSave = true;   //NOTE: Correctness of this may be dubious in some cases.
		
		ModelDll.CopyData(BaselineDataSet, DataSet, true, false, true); //NOTE: Copy parameter and result data. Input data is assumed to be unchanged.
		
		Log("Reverted to previously saved baseline");
		
		PlotRebuild();
		RefreshParameterView(true);
	}
	else
		Log("You can only revert to a baseline if you have one saved", true);
}

void MobiView::SaveInputsAsDat()
{
	if(!ModelDll.IsLoaded() || !DataSet)
	{
		Log("You can only export inputs if there is a Model and DataSet loaded", true);
		return;
	}
	
	FileSel InputSel;
	InputSel.Type("Input .dat files", "*.dat");

	if(InputFile!="" && FileExists(InputFile.data()))
		InputSel.ActiveDir(GetFileFolder(InputFile.data()));
	else
		InputSel.ActiveDir(GetFileFolder(DllFile.data()));
	InputSel.ExecuteSaveAs();
	std::string SaveFile = InputSel.Get().ToStd();
	
	if(SaveFile != "")
	{
		ModelDll.WriteInputsToFile(DataSet, SaveFile.data());
		if(CheckDllUserError()) return;
		Log(Format("Saving input data to %s", SaveFile.data()));
	}
}



void MobiView::ClosingChecks()
{
	int Cl = 1;
	if(ParametersWereChangedSinceLastSave)
		Cl = PromptYesNo("Parameters have been edited since the last save. If you exit now you will loose any changes. Do you still want to exit MobiView?");
	
	if(!Cl) return;
	
	StoreSettings();
	Close();
}


GUI_APP_MAIN
{
	MobiView().Run();
}
