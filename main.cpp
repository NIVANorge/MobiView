#include "MobiView.h"
#include "DllInterface.h"

#include <vector>
#include <iostream> //TODO: Remove. Just used for debugging now and then

#define MAGNUSDEV

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

	ErrorBox.Set(String((char *)lpMsgBuf));

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
			ErrorBox.Set(String(ErrMsgBuf));
			return true;
		}
	}
	return false;
}

void MobiView::SubBar(Bar &bar)
{
	bar.Add(IconImg::Open(), THISBACK(Load)).Tip("Load model, parameter and input files").Key(K_CTRL_O);
	bar.Add(IconImg::Save(), THISBACK(SaveParameters)).Tip("Save parameters").Key(K_CTRL_S);
	bar.Separator();
	bar.Add(IconImg::Run(), THISBACK(RunModel)).Tip("Run model").Key(K_F7);
}

MobiView::MobiView()
{
	AddFrame(Tool);
	Tool.Set(THISBACK(SubBar));
	
	hinstModelDll = NULL;
	ModelDll = {};
	DataSet = nullptr;
	
	CtrlLayout(*this, "MobiView");
	Sizeable();
	
	EquationSelecter.AddColumn("Equation");
	EquationSelecter.MultiSelect();
	EquationSelecter.WhenSel = THISBACK(EquationOrInputSelected);
	
	ParameterGroupSelecter.AddColumn("Parameter group");
	
	//ParameterView.NoCursor();  //Don't highlight selected row
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
	
	for(size_t Idx = 0; Idx < 6; ++Idx)
	{
		IndexSetName[Idx]->Hide();
		IndexList[Idx]->Hide();
		IndexList[Idx]->Disable();
		IndexList[Idx]->WhenAction = THISBACK(RefreshParameterView);
		
		IndexLock[Idx]->Hide();
		IndexLock[Idx]->WhenAction = THISBACK(RefreshParameterView);
		
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
	
	
	PlotColors = {{0, 130, 200}, {230, 25, 75}, {60, 180, 75}, {245, 130, 48}, {145, 30, 180},
                  {70, 240, 240}, {240, 50, 230}, {210, 245, 60}, {250, 190, 190}, {0, 128, 128}, {230, 190, 255},
                  {170, 110, 40}, {128, 0, 0}, {170, 255, 195}, {128, 128, 0}, {255, 215, 180}, {0, 0, 128}, {255, 225, 25}};
}

void MobiView::RefreshParameterView()
{
	ParameterView.Clear();
	
	//if(ParameterGroupSelecter.GetSelectCount() <= 0) return;   //Why doesn't this work?
	
	Value SelectedGroup = ParameterGroupSelecter.Get(0);
	std::string SelectedGroupName = SelectedGroup.ToString().ToStd();
	
	uint64 IndexSetCount = ModelDll.GetParameterGroupIndexSetsCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> IndexSetNames(IndexSetCount);
	ModelDll.GetParameterGroupIndexSets(DataSet, SelectedGroupName.data(), IndexSetNames.data());
	
	for(size_t Idx = 0; Idx < 6; ++Idx)
	{
		IndexList[Idx]->Disable();
	}
	
	std::vector<std::string> Indexes_String(IndexSetCount);
	std::vector<char *> Indexes(IndexSetCount);
	
	for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
	{
		size_t Id = IndexSetNameToId[IndexSetNames[Idx]];
		IndexList[Id]->Enable();
		
		//Indexes_String[Idx] = IndexList[Id]->GetList().Get(0).ToString().ToStd();
		Indexes_String[Idx] = IndexList[Id]->Get().ToString().ToStd();
		Indexes[IndexSetCount - Idx - 1] = (char *)Indexes_String[Idx].data(); //NOTE: Have to reverse since GetParameterGroupIndexSets returns in reverse order. May want to fix that!
	}
	
	uint64 ParameterCount = ModelDll.GetAllParametersCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> ParameterNames(ParameterCount);
	std::vector<char *> ParameterTypes(ParameterCount);
	ModelDll.GetAllParameters(DataSet, ParameterNames.data(), ParameterTypes.data(), SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	ParameterControls.Clear();
	CurrentParameterTypes.clear();
	for(size_t Idx = 0; Idx < ParameterCount; ++Idx)
	{
		const char *Name = ParameterNames[Idx];
		const char *Type = ParameterTypes[Idx];
		
		Value ParVal;
		Value ParMin;
		Value ParMax;
		Value ParUnit;
		Value ParDesc;
		const char *Unit = ModelDll.GetParameterUnit(DataSet, Name);
		if(Unit) ParUnit = Unit;
		const char *Description = ModelDll.GetParameterDescription(DataSet, Name);
		if(Description) ParDesc = Description;
		
		if(strcmp(Type, "double") == 0)
		{
			ParVal = ModelDll.GetParameterDouble(DataSet, Name, Indexes.data(), IndexSetCount);
			double Min, Max;
			ModelDll.GetParameterDoubleMinMax(DataSet, Name, &Min, &Max);
			ParMin = Min;
			ParMax = Max;
			
			ParameterControls.Create<EditDoubleNotNull>();
			CurrentParameterTypes.push_back(ParameterType_Double);
			
			if (CheckDllUserError()) return;
		}
		else if(strcmp(Type, "uint") == 0)
		{
			//TODO: Converting to int potentially loses precision. However Value has no uint64
			//subtype
			ParVal = (int64)ModelDll.GetParameterUInt(DataSet, Name, Indexes.data(), IndexSetCount);
			uint64 Min, Max;
			ModelDll.GetParameterUIntMinMax(DataSet, Name, &Min, &Max);
			int64 M = (int64)Max;
			if(M < 0) M = INT64_MAX; //Stupid stopgap. We should do this better and actually display uint64's.
			ParMin = (int64)Min;
			ParMax = M;
			
			ParameterControls.Create<EditInt64NotNull>();
			CurrentParameterTypes.push_back(ParameterType_UInt);
			
			if (CheckDllUserError()) return;
		}
		else if(strcmp(Type, "bool") == 0)
		{
			ParVal = ModelDll.GetParameterBool(DataSet, Name, Indexes.data(), IndexSetCount);
			if(CheckDllUserError()) return;
			
			ParameterControls.Create<Option>();
			CurrentParameterTypes.push_back(ParameterType_Bool);
		}
		else if(strcmp(Type, "time") == 0)
		{
			char TimeVal[256];
			ModelDll.GetParameterTime(DataSet, Name, Indexes.data(), IndexSetCount, TimeVal);
			Date D;
			StrToDate(D, TimeVal); //Error handling? But should not be necessary.
			ParVal = D;
			
			ParameterControls.Create<EditDateNotNull>();
			CurrentParameterTypes.push_back(ParameterType_Time);
		}
		ParameterView.Add(String(Name), ParVal, ParMin, ParMax, ParUnit, ParDesc);
		ParameterView.SetCtrl((int)Idx, 1, ParameterControls.Top());
		ParameterControls.Top().WhenAction = [=]() { ParameterEditAccepted((int)Idx); };
	}
}


void MobiView::RecursiveUpdateParameter(std::vector<char *> &IndexSetNames, int Level, std::vector<std::string> &CurrentIndexes, int Row)
{
	if(Level == IndexSetNames.size())
	{
		//Do the actual update.
		size_t IndexCount = CurrentIndexes.size();
		std::vector<char *> Indexes(IndexCount);
		for(size_t Idx = 0; Idx < IndexCount; ++Idx)
		{
			Indexes[IndexCount - Idx - 1] = (char *)CurrentIndexes[Idx].data(); //NOTE: Have to do the reversing since the GetParameterGroupIndexSets returns them in "reverse order". We may want to fix that..
		}
		
		std::string Name = ParameterView.Get(Row, 0).ToString().ToStd();
		parameter_type Type = CurrentParameterTypes[Row];
		
		switch(Type)
		{
			case ParameterType_Double:
			{
				double V = ParameterView.Get(1);
				ModelDll.SetParameterDouble(DataSet, Name.data(), Indexes.data(), Indexes.size(), V);
			} break;
			
			case ParameterType_UInt:
			{
				int64 V = ParameterView.Get(1);
				ModelDll.SetParameterUInt(DataSet, Name.data(), Indexes.data(), Indexes.size(), (uint64)V);
			} break;
			
			case ParameterType_Bool:
			{
				Ctrl *ctrl = ParameterView.GetCtrl(Row, 1);
				bool V = (bool)((Option*)ctrl)->Get();
				ModelDll.SetParameterBool(DataSet, Name.data(), Indexes.data(), Indexes.size(), V);
			} break;
			
			case ParameterType_Time:
			{
				Ctrl *ctrl = ParameterView.GetCtrl(Row, 1);
				Date D = ((EditDateNotNull*)ctrl)->GetData();
				std::string V = Format(D).ToStd();
				ModelDll.SetParameterTime(DataSet, Name.data(), Indexes.data(), Indexes.size(), V.data());
			} break;
		}
		CheckDllUserError();
	}
	else
	{
		const char *IndexSetName = IndexSetNames[Level];
		size_t Id = IndexSetNameToId[IndexSetName];
		
		if(IndexLock[Id]->Get())
		{
			size_t IndexCount = ModelDll.GetIndexCount(DataSet, IndexSetName);
			std::vector<char *> IndexNames(IndexCount);
			ModelDll.GetIndexes(DataSet, IndexSetName, IndexNames.data());
			for(size_t Idx = 0; Idx < IndexCount; ++Idx)
			{
				CurrentIndexes[Level] = IndexNames[Idx];
				RecursiveUpdateParameter(IndexSetNames, Level + 1, CurrentIndexes, Row);
			}
		}
		else
		{
			CurrentIndexes[Level] = IndexList[Id]->Get().ToString().ToStd();
			RecursiveUpdateParameter(IndexSetNames, Level + 1, CurrentIndexes, Row);
		}
	}
}


void MobiView::ParameterEditAccepted(int Row)
{
	//TODO: High degree of copypaste from above. Factor this out.
	Value SelectedGroup = ParameterGroupSelecter.Get(0);
	std::string SelectedGroupName = SelectedGroup.ToString().ToStd();
	
	uint64 IndexSetCount = ModelDll.GetParameterGroupIndexSetsCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> IndexSetNames(IndexSetCount);
	ModelDll.GetParameterGroupIndexSets(DataSet, SelectedGroupName.data(), IndexSetNames.data());
	
	std::vector<std::string> CurrentIndexes(IndexSetCount);
	RecursiveUpdateParameter(IndexSetNames, 0, CurrentIndexes, Row);
}

void MobiView::SaveParameters()
{
	if(!hinstModelDll || !ModelDll.WriteParametersToFile || !CurrentParameterFile.size()) return;
	//TODO: Want both save and "save as"?
	//TODO: Mechanism for determining if there has actually been edits that need to be saved.
	//TODO: Maybe also a "do you really want to overwrite <filename>".
	ModelDll.WriteParametersToFile(DataSet, CurrentParameterFile.data());
	CheckDllUserError();
}

void MobiView::Load()
{
	std::string DllFile;
	std::string InputFile;
	
	FileSel DllSel;
	DllSel.Type("Model dll files", "*.dll");
#if defined(MAGNUSDEV)
	DllSel.PreSelect("C:/Users/Magnus/Documents/Mobius/PythonWrapper/PERSiST/persist.dll");
#endif
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

	SetupModelDllInterface(&ModelDll, hinstModelDll);

	FileSel InputSel;
	InputSel.Type("Input dat files", "*.dat");
#if defined(MAGNUSDEV)
	InputSel.PreSelect("C:/Users/Magnus/Documents/Mobius/Applications/Persist/persist_inputs_Tarland.dat");
#endif
	InputSel.ExecuteOpen();
	InputFile = InputSel.Get().ToStd();
	
	Success = InputFile.size() > 0;
	
	if(!Success) return;
	
	FileSel ParameterSel;
	ParameterSel.Type("Input dat files", "*.dat");
#if defined(MAGNUSDEV)
	ParameterSel.PreSelect("C:/Users/Magnus/Documents/Mobius/Applications/Persist/persist_params_Tarland.dat");
#endif
	ParameterSel.ExecuteOpen();
	CurrentParameterFile = ParameterSel.Get().ToStd();
	
	Success = CurrentParameterFile.size() > 0;
	
	if(!Success) return;
	
	DataSet = ModelDll.SetupModel(CurrentParameterFile.data(), InputFile.data());
	if (CheckDllUserError()) return;
	
	uint64 ResultCount = ModelDll.GetAllResultsCount(DataSet);
	if (CheckDllUserError()) return;
	std::vector<char *> ResultNames(ResultCount);
	std::vector<char *> ResultTypes(ResultCount);
	ModelDll.GetAllResults(DataSet, ResultNames.data(), ResultTypes.data());
	if (CheckDllUserError()) return;
	
	for(size_t Idx = 0; Idx < ResultCount; ++Idx)
	{
		if(strcmp(ResultTypes[Idx], "initialvalue") != 0)
		{
			EquationSelecter.Add(ResultNames[Idx]);
		}
	}
	
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
		IndexLock[IndexSet]->Show();
		
		EIndexList[IndexSet]->GoBegin();
		EIndexList[IndexSet]->Show();
		
		MaxIndexCount = MaxIndexCount > IndexCount ? MaxIndexCount : IndexCount;
	}
	
}

void MobiView::RunModel()
{
	if(!hinstModelDll || !ModelDll.RunModel) return;
	
	ModelDll.RunModel(DataSet);
	
	RePlot();
}

void MobiView::EquationOrInputSelected()
{
	uint64 Timesteps = ModelDll.GetTimesteps(DataSet);
	if(Timesteps == 0) return; //Oops, only for equations, not inputs..
	
	int RowCount = EquationSelecter.GetCount();
	
	int CursorRow = EquationSelecter.GetCursor();
	
	for(int Row = 0; Row < RowCount; ++Row)
	{
		if(EquationSelecter.IsSelected(Row))// || Row == CursorRow) //Ugh. does not work properly when you deselect!
		{
			std::string EquationName = EquationSelecter.Get(Row, 0).ToString().ToStd();
			
			uint64 IndexSetCount = ModelDll.GetResultIndexSetsCount(DataSet, EquationName.data()); //IMPORTANT! Returns 0 if the model has not been run at least once!!
			std::vector<char *> IndexSets(IndexSetCount);
			ModelDll.GetResultIndexSets(DataSet, EquationName.data(), IndexSets.data());
			
			for(size_t Idx = 0; Idx < 6; ++Idx)
			{
				EIndexList[Idx]->Disable();
			}
			
			for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
			{
				const char *IndexSet = IndexSets[Idx];
				size_t Id = IndexSetNameToId[IndexSet];
				EIndexList[Id]->Enable();
			}
		}
	}
	
	RePlot();
}


void MobiView::RePlot()
{
	if(!EquationSelecter.IsSelection()) return;
	
	//TODO: Instead of just returning, maybe print text in the plotting view saying that the
	//model has not been run.
	uint64 Timesteps = ModelDll.GetTimesteps(DataSet);
	if(Timesteps == 0) return; //Oops, only for equations, not inputs..
	
	Plot.RemoveAllSeries(); //TODO: We could see if we want to cache some series, but I doubt it will be necessary.
	PlotData.clear();
	
	int RowCount = EquationSelecter.GetCount();
	
	int CursorRow = EquationSelecter.GetCursor();
	
	int PlotIdx = 0;
	for(int Row = 0; Row < RowCount; ++Row)
	{
		if(EquationSelecter.IsSelected(Row) || Row == CursorRow)
		{
			std::string EquationName = EquationSelecter.Get(Row, 0).ToString().ToStd();
			
			uint64 IndexSetCount = ModelDll.GetResultIndexSetsCount(DataSet, EquationName.data());
			std::vector<char *> IndexSets(IndexSetCount);
			ModelDll.GetResultIndexSets(DataSet, EquationName.data(), IndexSets.data());
			
			std::vector<std::string> Indexes_String(IndexSetCount);
			std::vector<char *> Indexes(IndexSetCount);
			
			for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
			{
				const char *IndexSet = IndexSets[Idx];
				size_t Id = IndexSetNameToId[IndexSet];
				
				Indexes_String[Idx] = EIndexList[Id]->Get(0).ToString().ToStd();
				Indexes[Idx] = (char *)Indexes_String[Idx].data();
			}
			
			//TODO: Better way to do this: ?
			PlotData.push_back({});
			PlotData[PlotIdx].resize(Timesteps);
			
			ModelDll.GetResultSeries(DataSet, EquationName.data(), Indexes.data(), IndexSetCount, PlotData[PlotIdx].data());
			if(CheckDllUserError()) return;
			
			double *Data = PlotData[PlotIdx].data();
			int Len = (int)PlotData[PlotIdx].size();
			
			int ColorIdx = PlotIdx % PlotColors.size();
			Plot.AddSeries(Data, Len, 0, 1).Stroke(1.0, PlotColors[ColorIdx]).NoMark();//NOTE: For legend, need to cache the string.. .Legend(EquationName.data());
			++PlotIdx;
		}
	}
	Plot.ZoomToFit(true, true);
	
	char DateStr[256];
	ModelDll.GetStartDate(DataSet, DateStr);
	Date D;
	StrToDate(D, DateStr);
	
	Plot.cbModifFormatX.Clear();
	Plot.cbModifFormatX << [D](String &s, int i, double d)
	{
		Date D2 = D + (int)d;
		s = Format(D2);
	};
	
	Plot.Refresh();
}

GUI_APP_MAIN
{
	MobiView().Run();
}
