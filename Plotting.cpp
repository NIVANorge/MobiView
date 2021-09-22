#ifndef _MobiView_Plotting_h_
#define _MobiView_Plotting_h_


#include "MobiView.h"

#include <chrono>


PlotCtrl::PlotCtrl(MobiView *Parent)
{
	this->Parent = Parent;
	CtrlLayout(*this);
	
	
	EIndexList[0]   = &EIndexList1;
	EIndexList[1]   = &EIndexList2;
	EIndexList[2]   = &EIndexList3;
	EIndexList[3]   = &EIndexList4;
	EIndexList[4]   = &EIndexList5;
	EIndexList[5]   = &EIndexList6;
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		EIndexList[Idx]->Hide();
		EIndexList[Idx]->Disable();
		EIndexList[Idx]->WhenSel << [this](){ RePlot(); };
		EIndexList[Idx]->MultiSelect();

		EIndexList[Idx]->AddColumn("(no name)");
	}
	

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
	
	TimeIntervals.SetData((int)Aggregation_None);
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

void PlotCtrl::BuildTimeIntervalsCtrl()
{
	TimeIntervals.Reset();
	TimeIntervals.Add((int)Aggregation_None, "No aggr.");
	TimeIntervals.SetData((int)Aggregation_None);
	
	if(!Parent->ModelDll.IsLoaded()) return;
	
	int64 Magnitude = Parent->TimestepSize.Magnitude;
	
	if(Parent->TimestepSize.Type == 0)  //Timestep magnitude measured in seconds.
	{
		if(Magnitude < 7*86400)
			TimeIntervals.Add((int)Aggregation_Weekly, "Weekly");
		if(Magnitude < 30*86400)
			TimeIntervals.Add((int)Aggregation_Monthly, "Monthly");
		if(Magnitude < 365*86400)
			TimeIntervals.Add((int)Aggregation_Yearly, "Yearly");
	}
	else                        //Timestep magnitude measured in months.
	{
		if(Magnitude < 12)
			TimeIntervals.Add((int)Aggregation_Yearly, "Yearly");
	}
}


MyPlot::MyPlot()
{
	
	//this->SetFastViewX(true); Can't be used with scatter plot data since it combines points.
	this->SetSequentialXAll(true);
	
	Size PlotReticleSize = GetTextSize("00000000", this->GetReticleFont());
	Size PlotUnitSize    = GetTextSize("[dummy]", this->GetLabelsFont());
	this->SetPlotAreaLeftMargin(PlotReticleSize.cx + PlotUnitSize.cy + 20);
	this->SetPlotAreaBottomMargin(PlotReticleSize.cy*2 + PlotUnitSize.cy + 20);
	
	this->SetGridDash("");
	Color Grey(180, 180, 180);
	this->SetGridColor(Grey);
	
	this->RemoveMouseBehavior(ScatterCtrl::ZOOM_WINDOW);
	this->AddMouseBehavior(true, false, false, true, false, 0, false, ScatterCtrl::SCROLL);
}


void PlotCtrl::GatherCurrentPlotSetup(plot_setup &C)
{
	C.SelectedResults.clear();
	C.SelectedInputs.clear();
	C.SelectedIndexes.clear();
	C.SelectedIndexes.resize(MAX_INDEX_SETS);
	C.IndexSetIsActive.clear();
	C.IndexSetIsActive.resize(MAX_INDEX_SETS);
	
	if(TimeIntervals.IsEnabled())
	{
		C.AggregationPeriod  = (aggregation_period)(int)TimeIntervals.GetData();
		C.AggregationType    = (aggregation_type)(int)Aggregation.GetData();
	}
	else
		C.AggregationPeriod = Aggregation_None;
	
	C.MajorMode          = (plot_major_mode)(int)PlotMajorMode.GetData();
	
	if(YAxisMode.IsEnabled())
		C.YAxisMode          = (y_axis_mode)(int)YAxisMode.GetData();
	else
		C.YAxisMode          = YAxis_Regular;
	
	if(ScatterInputs.IsEnabled())
		C.ScatterInputs = ScatterInputs.Get();
	else
		C.ScatterInputs = false;
	
	if(!Parent->ModelDll.IsLoaded() || !Parent->DataSet) return;
	
	int InputRowCount = Parent->InputSelecter.GetCount();
	for(int Row = 0; Row < InputRowCount; ++Row)
	{
		if(Parent->InputSelecter.IsSelected(Row) && !IsNull(Parent->InputSelecter.Get(Row, 1)))   //TODO: We should unify how we mark header rows with the EquationSelecter
		{
			std::string Name = Parent->InputSelecter.Get(Row, 0).ToString().ToStd();
			C.SelectedInputs.push_back(Name);
		}
	}
	uint64 Timesteps = Parent->ModelDll.GetTimesteps(Parent->DataSet);
	if(Timesteps > 0)    //NOTE: To avoid errors if inadvertently an equation is selected even if the model was not run yet
	{
		int ResultRowCount = Parent->EquationSelecter.GetCount();
		for(int Row = 0; Row < ResultRowCount; ++Row)
		{
			if(Parent->EquationSelecter.IsSelected(Row) && Parent->EquationSelecter.GetCtrl(Row, 1) != nullptr)
			{
				std::string Name = Parent->EquationSelecter.Get(Row, 0).ToString().ToStd();
				C.SelectedResults.push_back(Name);
			}
		}
	}
	
	C.SelectedIndexes.resize(MAX_INDEX_SETS);
	for(int IndexSet = 0; IndexSet < MAX_INDEX_SETS; ++IndexSet)
	{
		if(EIndexList[IndexSet]->IsVisible())
		{
			int RowCount = EIndexList[IndexSet]->GetCount();
			for(int Row = 0; Row < RowCount; ++Row)
			{
				if(EIndexList[IndexSet]->IsSelected(Row))
				{
					std::string Name = EIndexList[IndexSet]->Get(Row,0).ToString().ToStd();
					C.SelectedIndexes[IndexSet].push_back(Name);
				}
			}
		}
	}
	
	for(std::string &Name : C.SelectedInputs)
	{
		uint64 IndexSetCount = Parent->ModelDll.GetInputIndexSetsCount(Parent->DataSet, Name.data());
		std::vector<char *> IndexSets(IndexSetCount);
		Parent->ModelDll.GetInputIndexSets(Parent->DataSet, Name.data(), IndexSets.data());
		
		for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
		{
			const char *IndexSet = IndexSets[Idx];
			size_t Id = Parent->IndexSetNameToId[IndexSet];
			C.IndexSetIsActive[Id] = true;
		}
	}
	
	for(std::string &Name : C.SelectedResults)
	{
		uint64 IndexSetCount = Parent->ModelDll.GetResultIndexSetsCount(Parent->DataSet, Name.data());
		std::vector<char *> IndexSets(IndexSetCount);
		Parent->ModelDll.GetResultIndexSets(Parent->DataSet, Name.data(), IndexSets.data());
		
		for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
		{
			const char *IndexSet = IndexSets[Idx];
			size_t Id = Parent->IndexSetNameToId[IndexSet];
			C.IndexSetIsActive[Id] = true;
		}
	}
}


void PlotCtrl::SetMainPlotSetup(plot_setup &C)
{
	//MainPlot.PlotSetup = C;  //NOTE: Not necessary, since it will then be regathered
	//automatically.
	
	PlotMajorMode.SetData((int)C.MajorMode);
	ScatterInputs.SetData((int)C.ScatterInputs);
	YAxisMode.SetData((int)C.YAxisMode);
	TimeIntervals.SetData((int)C.AggregationPeriod);
	Aggregation.SetData((int)C.AggregationType);
	TimestepSlider.SetData((int)C.ProfileTimestep);
	
	for(int IndexSet = 0; IndexSet < MAX_INDEX_SETS; ++IndexSet)
	{
		EIndexList[IndexSet]->ClearSelection(false);
		int RowCount = EIndexList[IndexSet]->GetCount();
		for(int Row = 0; Row < RowCount; ++Row)
		{
			std::string Index = EIndexList[IndexSet]->Get(Row, 0).ToString().ToStd();
			if(std::find(C.SelectedIndexes[IndexSet].begin(), C.SelectedIndexes[IndexSet].end(), Index) != C.SelectedIndexes[IndexSet].end())
				EIndexList[IndexSet]->Select(Row);
		}
	}
	
	Parent->EquationSelecter.ClearSelection(false);
	int ERowCount = Parent->EquationSelecter.GetCount();
	for(int Row = 0; Row < ERowCount; ++Row)
	{
		std::string Equation = Parent->EquationSelecter.Get(Row, 0).ToString().ToStd();
		if(std::find(C.SelectedResults.begin(), C.SelectedResults.end(), Equation) != C.SelectedResults.end())
			Parent->EquationSelecter.Select(Row);
	}
	
	Parent->InputSelecter.ClearSelection(false);
	int IRowCount = Parent->InputSelecter.GetCount();
	for(int Row = 0; Row < IRowCount; ++Row)
	{
		std::string Input = Parent->InputSelecter.Get(Row, 0).ToString().ToStd();
		if(std::find(C.SelectedInputs.begin(), C.SelectedInputs.end(), Input) != C.SelectedInputs.end())
			Parent->InputSelecter.Select(Row);
	}
	
	
	PlotModeChange();
}


void PlotCtrl::PlotModeChange()
{
	
	if(Parent->DataSet==0 || !Parent->ModelDll.IsLoaded()) return;
	
	GatherCurrentPlotSetup(MainPlot.PlotSetup);
	
	plot_major_mode MajorMode = MainPlot.PlotSetup.MajorMode;
	
	ScatterInputs.Disable();
	YAxisMode.Disable();
	TimeIntervals.Disable();
	
	TimestepEdit.Hide();
	TimestepSlider.Hide();
	TimestepSlider.Disable();
	
	if(MajorMode == MajorMode_Regular || MajorMode == MajorMode_Stacked || MajorMode == MajorMode_StackedShare || MajorMode == MajorMode_CompareBaseline)
	{
		ScatterInputs.Enable();
		YAxisMode.Enable();
		TimeIntervals.Enable();
	}
	else if(MajorMode == MajorMode_Residuals)
	{
		ScatterInputs.Enable();
		TimeIntervals.Enable();
	}
	else if(MajorMode == MajorMode_Profile)
	{
		TimestepSlider.Show();
		TimestepEdit.Show();
		TimeIntervals.Enable();
	}
	
	if(TimeIntervals.IsEnabled())
	{
		aggregation_period Interval = MainPlot.PlotSetup.AggregationPeriod;
		if(Interval == Aggregation_None)
			Aggregation.Disable();
		else
		{
			Aggregation.Enable();
			YAxisMode.Disable();
			TimestepEdit.Disable();
		}
	}
	else
		Aggregation.Disable();
	
	uint64 Timesteps = Parent->ModelDll.GetInputTimesteps(Parent->DataSet);
	if(Timesteps != 0)
	{
		Time StartTime = Parent->CalibrationIntervalStart.GetData();
		Time EndTime   = Parent->CalibrationIntervalStart.GetData();
		
		if(IsNull(StartTime))
		{
			char TimeVal[256];
			Parent->ModelDll.GetInputStartDate(Parent->DataSet, TimeVal);
			StrToTime(StartTime, TimeVal);
			Parent->CalibrationIntervalStart.SetData(StartTime);
		}
		if(IsNull(EndTime))
		{
			EndTime = StartTime;
			if(Timesteps != 0)
				AdvanceTimesteps(EndTime, Timesteps - 1, Parent->TimestepSize);
			Parent->CalibrationIntervalEnd.SetData(EndTime);
		}
	}
	

	//NOTE: Enable / Disable index selectors that are relevant for the selected time series
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		EIndexList[Idx]->Enable(MainPlot.PlotSetup.IndexSetIsActive[Idx]);
	}
	
	RePlot();
}



void PlotCtrl::RePlot(bool CausedByReRun)
{
	//NOTE: This is to allow for expansion with multiple plots later
	GatherCurrentPlotSetup(MainPlot.PlotSetup);
	MainPlot.BuildPlot(Parent, this, true, Parent->PlotInfo, CausedByReRun);
}


void MyPlot::BuildPlot(MobiView *Parent, PlotCtrl *Control, bool IsMainPlot, MyRichView &PlotInfo, bool CausedByReRun, int OverrideMode)
{
	//TODO: Ideally we would want to not have to have the MobiView * pointer here, but only the
	//specifics that we need. However, it is a little tricky to let go of that dependency.
	
	//TODO: We should really really remove the dependency on the Control pointer from here...
	
	
	ClearAll(false);
	
	if(Parent==0 || Parent->DataSet==0 || !Parent->ModelDll.IsLoaded())
	{
		this->SetTitle("No model is loaded.");
		return;
	}
	
	if(PlotSetup.SelectedResults.empty() && PlotSetup.SelectedInputs.empty())
	{
		this->SetTitle("No time series is selected for plotting.");
		return;
	}
	
	uint64 InputTimesteps = Parent->ModelDll.GetInputTimesteps(Parent->DataSet);
	uint64 ResultTimesteps = Parent->ModelDll.GetTimesteps(Parent->DataSet);
	
	//NOTE: We could also just clear the PlotSetup.SelectedResults in this case to allow
	//plotting any selected input series
	if(PlotSetup.SelectedResults.size() > 0 && ResultTimesteps == 0)
	{
		this->SetTitle("Unable to generate a plot of any result series since the model has not been run.");
		return;
	}
	
	//NOTE: This should theoretically never happen
	if(PlotSetup.SelectedIndexes.empty())
	{
		this->SetTitle("No indexes were selected.");
		return;
	}
	
	for(int IndexSet = 0; IndexSet < MAX_INDEX_SETS; ++IndexSet)
	{
		if(PlotSetup.SelectedIndexes[IndexSet].empty() && PlotSetup.IndexSetIsActive[IndexSet])
		{
			this->SetTitle("At least one index has to be selected for each of the active index sets.");
			return;
		}
	}
	
	
	bool MultiIndex = false;
	for(size_t IndexSet = 0; IndexSet < MAX_INDEX_SETS; ++IndexSet)
	{
		if(PlotSetup.SelectedIndexes[IndexSet].size() > 1 && PlotSetup.IndexSetIsActive[IndexSet])
		{
			MultiIndex = true;
			break;
		}
	}
	
	this->ShowLegend(true);
	
	PlotInfo.Clear();
	
	plot_major_mode PlotMajorMode = PlotSetup.MajorMode;
	if(OverrideMode >= 0) PlotMajorMode = (plot_major_mode)OverrideMode;
	
	char TimeStr[256];
	Parent->ModelDll.GetStartDate(Parent->DataSet, TimeStr);
	Time ResultStartTime;
	StrToTime(ResultStartTime, TimeStr);

	Parent->ModelDll.GetInputStartDate(Parent->DataSet, TimeStr);
	Time InputStartTime;
	StrToTime(InputStartTime, TimeStr);

	int NBinsHistogram = 0;
	
	int TimeseriesCount = PlotSetup.SelectedInputs.size() + PlotSetup.SelectedResults.size();

	if(TimeseriesCount == 0) return;
	
	
	
	
	
	bool ResidualsAvailable = false;
	bool GOFStatsDesired = (bool)Parent->GOFOnOption.GetData();
	
	Time GofStartTime;
	Time GofEndTime;
	int64 GofOffset;
	int64 GofTimesteps;
	String ModeledLegend;
	String ObservedLegend;
	String ModUnit, ObsUnit;
	
	Time ReferenceTime = InputStartTime;
	{
		int64 ReferenceTimesteps = InputTimesteps;
		if(PlotMajorMode == MajorMode_Residuals || PlotMajorMode == MajorMode_ResidualHistogram || PlotMajorMode == MajorMode_QQ)
		{
			ReferenceTime = ResultStartTime;
			ReferenceTimesteps = ResultTimesteps;
		}
		Parent->GetGofOffsets(ReferenceTime, ReferenceTimesteps, GofStartTime, GofEndTime, GofOffset, GofTimesteps);
	}
	
	int64 ResultGofOffset = GofOffset;
	int64 ResultGofTimesteps = GofTimesteps;
	if(ReferenceTime != ResultStartTime)
	{
		int64 Diff = TimestepsBetween(InputStartTime, ResultStartTime, Parent->TimestepSize);
		ResultGofOffset = GofOffset - Diff;
		ResultGofTimesteps = GofTimesteps - Diff;
		if(ResultGofOffset < 0) { ResultGofTimesteps += ResultGofOffset; ResultGofOffset = 0; }
		if(ResultGofTimesteps+ResultGofOffset > ResultTimesteps) ResultGofTimesteps = ResultTimesteps-ResultGofOffset;
		if(ResultGofTimesteps < 0) ResultGofTimesteps = 0;
	}
	
	//PromptOK(Format("resultgofoffset %d, resultgoftimesteps %d, gofoffset %d, goftimesteps %d", ResultGofOffset, ResultGofTimesteps, GofOffset, GofTimesteps));
	
	double *ModeledSeries = nullptr;
	double *ObservedSeries = nullptr;
	residual_stats ResidualStats = {};
		
	PlotInfo.Append(Format("Showing statistics for interval %s to %s&&", GofStartTime, GofEndTime));
	
	if(PlotMajorMode == MajorMode_Residuals || PlotMajorMode == MajorMode_ResidualHistogram || PlotMajorMode == MajorMode_QQ || GOFStatsDesired)
	{
		if(PlotSetup.SelectedResults.size() == 1 && PlotSetup.SelectedInputs.size() == 1 && !MultiIndex)
		{
			ResidualsAvailable = true;
			
			ModeledSeries = PlotData.Allocate(ResultTimesteps).data();
			ObservedSeries = PlotData.Allocate(ResultTimesteps).data();
			
			// These will be called again later if we are not in residual mode, but that is not
			// too bad.
			Parent->GetSingleSelectedResultSeries(PlotSetup, Parent->DataSet, PlotSetup.SelectedResults[0], ModeledLegend, ModUnit, ModeledSeries);
			Parent->GetSingleSelectedInputSeries(PlotSetup, Parent->DataSet, PlotSetup.SelectedInputs[0], ObservedLegend, ObsUnit, ObservedSeries, true);
			
			//NOTE: Use ResultGofOffset for obs series here since it is aligned with results in
			//this case.
			ComputeResidualStats(ResidualStats, ObservedSeries+ResultGofOffset, ModeledSeries+ResultGofOffset, ResultGofTimesteps);
		}
	}
	
	
	
	if(PlotMajorMode == MajorMode_Regular || PlotMajorMode == MajorMode_Stacked || PlotMajorMode == MajorMode_StackedShare)
	{
		double *InputXValues = PlotData.Allocate(InputTimesteps).data();
		ComputeXValues(InputStartTime, InputStartTime, InputTimesteps, Parent->TimestepSize, InputXValues);
		
		for(std::string &Name : PlotSetup.SelectedInputs)
		{
			uint64 IndexSetCount = Parent->ModelDll.GetInputIndexSetsCount(Parent->DataSet, Name.data());
			std::vector<char *> IndexSets(IndexSetCount);
			Parent->ModelDll.GetInputIndexSets(Parent->DataSet, Name.data(), IndexSets.data());
			if(Parent->CheckDllUserError()) return;
			
			std::vector<std::string> CurrentIndexes(IndexSets.size());
			AddPlotRecursive(Parent, PlotInfo, Name, IndexSets, CurrentIndexes, 0, true, InputTimesteps, InputStartTime, InputStartTime, InputXValues, PlotMajorMode, GofOffset, GofTimesteps);
			
			if(Parent->CheckDllUserError()) return;
		}
		
		double *ResultXValues = PlotData.Allocate(ResultTimesteps).data();
		ComputeXValues(InputStartTime, ResultStartTime, ResultTimesteps, Parent->TimestepSize, ResultXValues);
			
		for(std::string &Name : PlotSetup.SelectedResults)
		{
			uint64 IndexSetCount = Parent->ModelDll.GetResultIndexSetsCount(Parent->DataSet, Name.data());
			std::vector<char *> IndexSets(IndexSetCount);
			Parent->ModelDll.GetResultIndexSets(Parent->DataSet, Name.data(), IndexSets.data());
			if(Parent->CheckDllUserError()) return;
			
			std::vector<std::string> CurrentIndexes(IndexSets.size());
			AddPlotRecursive(Parent, PlotInfo, Name, IndexSets, CurrentIndexes, 0, false, ResultTimesteps, InputStartTime, ResultStartTime, ResultXValues, PlotMajorMode, ResultGofOffset, ResultGofTimesteps);
			
			if(Parent->CheckDllUserError()) return;
		}
		
		StackedPlotFixup(PlotMajorMode);
	}
	else if(PlotMajorMode == MajorMode_Histogram)
	{
		if(TimeseriesCount > 1 || MultiIndex)
		{
			//TODO: Setting the title is a lousy way to provide an error message....
			this->SetTitle("In histogram mode you can only have one timeseries selected, for one index combination");
		}
		else
		{
			std::vector<double> &Data = PlotData.Allocate(0);
			String Legend;
			String Unit;
			
			int64 Offset = GofOffset;
			int64 StatTimesteps = GofTimesteps;
			if(PlotSetup.SelectedResults.size() == 1)
			{
				Data.resize(ResultTimesteps);
				Parent->GetSingleSelectedResultSeries(PlotSetup, Parent->DataSet, PlotSetup.SelectedResults[0], Legend, Unit, Data.data());
				Offset = ResultGofOffset;
				StatTimesteps = ResultGofTimesteps;
			}
			else if(PlotSetup.SelectedInputs.size() == 1)
			{
				Data.resize(InputTimesteps);
				Parent->GetSingleSelectedInputSeries(PlotSetup, Parent->DataSet, PlotSetup.SelectedInputs[0], Legend, Unit, Data.data(), false);
			
			}
			
			NBinsHistogram = AddHistogram(Legend, Unit, Data.data(), Data.size());
			
			timeseries_stats Stats = {};
			ComputeTimeseriesStats(Stats, Data.data()+Offset, StatTimesteps, Parent->StatSettings, false);
			DisplayTimeseriesStats(Stats, Legend, Unit, Parent->StatSettings, PlotInfo);
		}
	}
	else if(PlotMajorMode == MajorMode_Profile || PlotMajorMode == MajorMode_Profile2D)
	{
		size_t ProfileIndexSet;
		size_t IndexCount;
		int NumberOfIndexSetsWithMultipleIndexesSelected = 0;
		for(size_t IndexSet = 0; IndexSet < MAX_INDEX_SETS; ++IndexSet)
		{
			size_t ThisIndexCount = PlotSetup.SelectedIndexes[IndexSet].size();
			if(ThisIndexCount > 1 && PlotSetup.IndexSetIsActive[IndexSet])
			{
				NumberOfIndexSetsWithMultipleIndexesSelected++;
				ProfileIndexSet = IndexSet;
				IndexCount = ThisIndexCount;
			}
		}
		
		if(TimeseriesCount != 1 || NumberOfIndexSetsWithMultipleIndexesSelected != 1)
		{
			//TODO: Setting the title is a lousy way to provide an error message....
			this->SetTitle("In profile mode you can only have one timeseries selected, and exactly one index set must have multiple indexes selected");
		}
		else
		{
			int Mode = PlotSetup.SelectedResults.size() != 0 ? 0 : 1; //I.e. Mode = 0 if it was an equation that was selected, otherwise Mode = 1 if it was an input that was selected
			
			Time CurrentStartTime;
			size_t Timesteps;
			if(Mode == 0)
			{
				Timesteps = ResultTimesteps;
				CurrentStartTime = ResultStartTime;
			}
			else
			{
				Timesteps = InputTimesteps;
				CurrentStartTime = InputStartTime;
			}
			
			aggregation_period IntervalType = PlotSetup.AggregationPeriod;
			aggregation_type AggregationType = PlotSetup.AggregationType;
			
			ProfileIndexesCount = IndexCount;
			
			if(Mode == 0)
			{
				ProfileLegend = PlotSetup.SelectedResults[0].data();
				ProfileUnit = Parent->ModelDll.GetResultUnit(Parent->DataSet, PlotSetup.SelectedResults[0].data());
			}
			else
			{
				ProfileLegend = PlotSetup.SelectedInputs[0].data();
				ProfileUnit = Parent->ModelDll.GetInputUnit(Parent->DataSet, PlotSetup.SelectedInputs[0].data());
			}
			ProfileLegend << " profile";
			
			ProfileLabels = PlotSetup.SelectedIndexes[ProfileIndexSet];  //TODO: This storage should be unnecessary, as it should be easy to recover this
			
			//TODO: We could use the same data storage in both cases, so that we don't have to
			//branch here!
			if(PlotMajorMode == MajorMode_Profile)
			{
				for(std::string &Row : PlotSetup.SelectedIndexes[ProfileIndexSet])
				{
					std::vector<double> &Data = PlotData.Allocate(Timesteps);
					
					if(Mode == 0)
						Parent->GetSingleResultSeries(PlotSetup, Parent->DataSet, PlotSetup.SelectedResults[0], Data.data(), ProfileIndexSet, Row);
					else
						Parent->GetSingleInputSeries(PlotSetup, Parent->DataSet, PlotSetup.SelectedInputs[0], Data.data(), ProfileIndexSet, Row);
					
					//NullifyNans(Data.data(), Data.size());
					
					if(IntervalType != Aggregation_None)
					{
						std::vector<double> XValues; //TODO: Ugh, it is stupid to have to declare this when it is not going to be used.
						std::vector<double> YValues;
						
						AggregateData(CurrentStartTime, CurrentStartTime, Timesteps, Data.data(), IntervalType, AggregationType, Parent->TimestepSize, XValues, YValues);
						
						Data = YValues; //Note: vector copy
					}
				}
				
				size_t DataLength = PlotData.Data[0].size();
			
				//TODO: This should be based on the current position of the slider instead unless
				//we reset the slider!
				if(IsMainPlot)
				{
					Control->ProfileDisplayTime = CurrentStartTime;
					if(IntervalType != Aggregation_None)
					{
						//TODO: Clean this up
						Control->ProfileDisplayTime.second = 0;
						Control->ProfileDisplayTime.minute = 0;
						Control->ProfileDisplayTime.hour   = 0;
						if(IntervalType == Aggregation_Yearly || IntervalType == Aggregation_Monthly) Control->ProfileDisplayTime.day    = 1;
						if(IntervalType == Aggregation_Yearly) Control->ProfileDisplayTime.month = 1;
					}
					Control->TimestepEdit.SetData(Control->ProfileDisplayTime);
					
					Control->TimestepSlider.Range((int)DataLength - 1);
					Control->TimestepSlider.Enable();
					
					PlotSetup.ProfileTimestep = Control->TimestepSlider.GetData();
				}

				std::vector<double> &XValues = PlotData.Allocate(IndexCount);
				std::vector<double> &YValues = PlotData.Allocate(IndexCount);
				
				double YMax = DBL_MIN;
				for(size_t Idx = 0; Idx < IndexCount; ++Idx)
				{
					for(size_t Ts = 0; Ts < DataLength; ++Ts)
					{
						double Value = PlotData.Data[Idx][Ts];
						if(std::isfinite(Value) && !IsNull(Value))
						{
							YMax = std::max(YMax, Value);
						}
					}
					
					XValues[Idx] = (double)Idx + 0.5;
				}
				
				this->SetXYMin(0.0, 0.0);
				this->SetRange((double)IndexCount, YMax);
				this->SetMajorUnits(1.0);
				this->SetMinUnits(0.5);
				
				this->cbModifFormatXGridUnits.Clear();
				this->cbModifFormatX.Clear();
				this->cbModifFormatX <<
				[IndexCount, this](String &s, int i, double d)
				{
					int Idx = (int)d;
					if(d >= 0 && d < IndexCount) s = ProfileLabels[Idx].data();
				};
				
				
				ReplotProfile();
			}
			else // MajorMode == MajorMode_Profile2D
			{
				
				SurfY.Reserve(ProfileIndexesCount + 1);
				SurfX.Reserve(Timesteps + 1);
				SurfZ.Reserve(ProfileIndexesCount*Timesteps);
				
				size_t IdxIdx = 0;
				for(std::string &Row : PlotSetup.SelectedIndexes[ProfileIndexSet])
				{
					SurfY << (double)IdxIdx;
					
					double *Data = (double *)malloc(sizeof(double)*Timesteps);
					
					if(Mode == 0)
						Parent->GetSingleResultSeries(PlotSetup, Parent->DataSet, PlotSetup.SelectedResults[0], Data, ProfileIndexSet, Row);
					else
						Parent->GetSingleInputSeries(PlotSetup, Parent->DataSet, PlotSetup.SelectedInputs[0], Data, ProfileIndexSet, Row);
					
					for(size_t Ts = 0; Ts < Timesteps; ++Ts) SurfZ << Data[Ts];
					
					++IdxIdx;
					
					free(Data);
				}
				SurfY << (double)IdxIdx;
				
				double *XValues = PlotData.Allocate(Timesteps+1).data();
				ComputeXValues(InputStartTime, CurrentStartTime, Timesteps+1, Parent->TimestepSize, XValues);
				
				for(size_t Ts = 0; Ts < Timesteps+1; ++Ts) SurfX << XValues[Ts];
				
				SurfData.Init(SurfZ, SurfX, SurfY, TableInterpolate::NO, true);

				this->AddSurf(SurfData).ZoomToFitZ();//.Legend(ProfileLegend);
				
				this->ZoomToFit(false, true); //Apparently we have to do this before SetMajorUnits, or things get weird.
				this->SetMajorUnits(Null, 1.0);

				this->cbModifFormatYGridUnits.Clear();
				this->cbModifFormatY.Clear();
				this->cbModifFormatY <<
				[IndexCount, this](String &s, int i, double d)
				{
					int Idx = (int)d;
					if(d >= 0 && d < IndexCount) s = ProfileLabels[Idx].data();
				};
			}
		}
	}
	else if(PlotMajorMode == MajorMode_CompareBaseline)
	{
		//TODO: Limit to one selected result series. Maybe also allow plotting a observation
		//comparison.
		if(PlotSetup.SelectedResults.size() > 1 || MultiIndex)
			this->SetTitle("In baseline comparison mode you can only have one result series selected, for one index combination");
		else if(!Parent->BaselineDataSet)
			this->SetTitle("The baseline comparison can only be displayed if the baseline has been saved (using a button in the toolbar)");
		else
		{
			uint64 BaselineTimesteps = Parent->ModelDll.GetTimesteps(Parent->BaselineDataSet);
			Parent->ModelDll.GetStartDate(Parent->BaselineDataSet, TimeStr);
			Time BaselineStartTime;
			StrToTime(BaselineStartTime, TimeStr);
			
			//TODO: Should we compute any residual statistics here?
			if(PlotSetup.SelectedInputs.size() == 1)
			{
				//TODO: Should we allow displaying multiple input series here? Probably no
				//reason to
				std::vector<double> &Obs = PlotData.Allocate(InputTimesteps);
				String InputLegend;
				String Unit;
				Parent->GetSingleSelectedInputSeries(PlotSetup, Parent->DataSet, PlotSetup.SelectedInputs[0], InputLegend, Unit, Obs.data(), false);
				
				double *InputXValues = PlotData.Allocate(InputTimesteps).data();
				ComputeXValues(InputStartTime, InputStartTime, InputTimesteps, Parent->TimestepSize, InputXValues);
				
				timeseries_stats Stats3 = {};
				ComputeTimeseriesStats(Stats3, Obs.data()+GofOffset, GofTimesteps, Parent->StatSettings, false);
				
				Color Col = AddPlot(InputLegend, Unit, InputXValues, Obs.data(), Obs.size(), true, InputStartTime, InputStartTime, Parent->TimestepSize, Stats3.Min, Stats3.Max);
				
				DisplayTimeseriesStats(Stats3, InputLegend, Unit, Parent->StatSettings, PlotInfo, Col);
			}
			
			std::vector<double> &Baseline = PlotData.Allocate(BaselineTimesteps);
			String BS;
			String Unit;
			Parent->GetSingleSelectedResultSeries(PlotSetup, Parent->BaselineDataSet, PlotSetup.SelectedResults[0], BS, Unit, Baseline.data());
			//NullifyNans(Baseline.data(), Baseline.size());
			BS << " baseline";
			
			double *BaselineXValues = PlotData.Allocate(BaselineTimesteps).data();
			ComputeXValues(InputStartTime, BaselineStartTime, BaselineTimesteps, Parent->TimestepSize, BaselineXValues);
			
			timeseries_stats Stats = {};
			ComputeTimeseriesStats(Stats, Baseline.data()+ResultGofOffset, ResultGofTimesteps, Parent->StatSettings, false);
			
			Color Col = AddPlot(BS, Unit, BaselineXValues, Baseline.data(), Baseline.size(), false, InputStartTime, BaselineStartTime, Parent->TimestepSize, Stats.Min, Stats.Max);
			DisplayTimeseriesStats(Stats, BS, Unit, Parent->StatSettings, PlotInfo, Col);
			
			
			std::vector<double> &Current = PlotData.Allocate(ResultTimesteps);
			String CurrentLegend;
			Parent->GetSingleSelectedResultSeries(PlotSetup, Parent->DataSet, PlotSetup.SelectedResults[0], CurrentLegend, Unit, Current.data());
			//NullifyNans(Current.data(), Current.size());
			
			double *ResultXValues = PlotData.Allocate(ResultTimesteps).data();
			ComputeXValues(InputStartTime, ResultStartTime, ResultTimesteps, Parent->TimestepSize, ResultXValues);
			
			timeseries_stats Stats2 = {};
			ComputeTimeseriesStats(Stats2, Current.data()+ResultGofOffset, ResultGofTimesteps, Parent->StatSettings, false);
		
			Col = AddPlot(CurrentLegend, Unit, ResultXValues, Current.data(), Current.size(), false, InputStartTime, ResultStartTime, Parent->TimestepSize, Stats2.Min, Stats2.Max);

			DisplayTimeseriesStats(Stats2, CurrentLegend, Unit, Parent->StatSettings, PlotInfo, Col);
			
		}
		
		
	}
	else if(PlotMajorMode == MajorMode_Residuals || PlotMajorMode == MajorMode_ResidualHistogram || PlotMajorMode == MajorMode_QQ)
	{
		if(!ResidualsAvailable)
		{
			//TODO: Setting the title is a lousy way to provide an error message....
			this->SetTitle("In residual mode you must select exactly 1 result series and 1 input series, for one index combination only");
		}
		else
		{
			std::vector<double> &Residuals = PlotData.Allocate(ResultTimesteps);
			
			for(size_t Idx = 0; Idx < ResultTimesteps; ++Idx)
				Residuals[Idx] = ObservedSeries[Idx] - ModeledSeries[Idx];
			
			String Legend = String("Residuals of ") + ModeledLegend + " vs " + ObservedLegend;
			
			timeseries_stats ObservedStats = {};
			ComputeTimeseriesStats(ObservedStats, ObservedSeries+GofOffset, GofTimesteps, Parent->StatSettings, false);
			DisplayTimeseriesStats(ObservedStats, ObservedLegend, ObsUnit, Parent->StatSettings, PlotInfo);
			
			timeseries_stats ModeledStats = {};
			ComputeTimeseriesStats(ModeledStats, ModeledSeries+GofOffset, GofTimesteps, Parent->StatSettings, false);
			DisplayTimeseriesStats(ModeledStats, ModeledLegend, ModUnit, Parent->StatSettings, PlotInfo);
			
			if(PlotMajorMode == MajorMode_Residuals)
			{
				double StartX = (double)(GofStartTime - InputStartTime);
				double EndX   = (double)(GofEndTime   - InputStartTime);
				
				if(ResidualStats.DataPoints > 0)
				{
					double *ResidualXValues = PlotData.Allocate(ResultTimesteps).data();
					ComputeXValues(InputStartTime, ResultStartTime, ResultTimesteps, Parent->TimestepSize, ResidualXValues);
					
					double XMean, XVar, XYCovar;
					ComputeTrendStats(ResidualXValues + GofOffset, Residuals.data() + GofOffset, GofTimesteps, ResidualStats.MeanError, XMean, XVar, XYCovar);
					
					//NOTE: Using the input start date as reference date is just so that we agree with the date formatting below.
					AddPlot(Legend, ModUnit, ResidualXValues, Residuals.data(), ResultTimesteps, true, InputStartTime, ResultStartTime, Parent->TimestepSize);
					
					String TL = "Trend line";
					AddTrendLine(TL, XYCovar, XVar, ResidualStats.MeanError, XMean, StartX, EndX);
				}
				
				AddLine(Null, StartX, StartX, ResidualStats.MinError, ResidualStats.MaxError, Red());
				AddLine(Null, EndX, EndX, ResidualStats.MinError, ResidualStats.MaxError, Red());
			}
			else if(PlotMajorMode == MajorMode_ResidualHistogram)
			{
				NBinsHistogram = AddHistogram(Legend, ModUnit, Residuals.data()+GofOffset, GofTimesteps);
				String NormLegend = "Normal distr.";
				
				timeseries_stats RS2;
				ComputeTimeseriesStats(RS2, Residuals.data()+GofOffset, GofTimesteps, Parent->StatSettings, false);
				
				AddNormalApproximation(NormLegend, NBinsHistogram, RS2.Min, RS2.Max, RS2.Mean, RS2.StandardDev);
			}
			else if(PlotMajorMode == MajorMode_QQ)
			{
				AddQQPlot(ModUnit, ObsUnit, ModeledLegend, ObservedLegend, ModeledStats, ObservedStats, Parent->StatSettings);
				
				size_t NumPercentiles = Parent->StatSettings.Percentiles.size();
				
				double Min = std::min(ModeledStats.Min, ObservedStats.Min);
				double Max = std::max(ModeledStats.Max, ObservedStats.Max);
				
				String Dum = "";
				AddLine(Dum, Min, Max, Min, Max);
			}
		}
	}
	
	if(GOFStatsDesired)
	{
		if(ResidualsAvailable)
		{
			String GOF = "Goodness of fit:";
			if(PlotMajorMode == MajorMode_CompareBaseline)
				GOF = "Goodness of fit (changes rel. to baseline):";
		
			bool DisplayChange = CausedByReRun && CachedStats.WasInitialized;  //Whether or not to show difference between current and cached GOF stats.
				
			DisplayResidualStats(ResidualStats, CachedStats, GOF, Parent->StatSettings, PlotInfo, DisplayChange);
			
			//NOTE: If in baseline mode, only cache the stats of the baseline.
			if(PlotMajorMode != MajorMode_CompareBaseline || Parent->BaselineWasJustSaved)
			{
				CachedStats = ResidualStats;
				CachedStats.WasInitialized = true;
			}
		}
		else
		{
			PlotInfo.Append("\nTo show goodness of fit, select a single result and input series.\n");
			PlotInfo.ScrollEnd();
		}
	}
	
	FormatAxes(PlotMajorMode, NBinsHistogram, InputStartTime, Parent->TimestepSize);
	
	this->Refresh();
}


void MyPlot::FormatAxes(plot_major_mode PlotMajorMode, int NBinsHistogram, Time InputStartTime, timestep_size TimestepSize)
{
	
	this->SetGridLinesX.Clear();
	
	if(this->GetCount() > 0 || (PlotMajorMode == MajorMode_Profile2D && SurfZ.size() > 0))
	{
		if(PlotMajorMode == MajorMode_Histogram || PlotMajorMode == MajorMode_ResidualHistogram)
		{
			this->cbModifFormatXGridUnits.Clear();
			this->cbModifFormatX.Clear();
			//NOTE: Histograms require different zooming.
			this->ZoomToFit(true, true);
			PlotWasAutoResized = false;
			
			double XRange = this->GetXRange();
			double XMin   = this->GetXMin();
			
			//NOTE: The auto-resize cuts out half of each outer bar, so we fix that
			double Stride = XRange / (double)(NBinsHistogram-1);
			XMin   -= 0.5*Stride;
			XRange += Stride;
			this->SetXYMin(XMin);
			this->SetRange(XRange);
			
			int LineSkip = NBinsHistogram / 30 + 1;
			
			this->SetGridLinesX << [NBinsHistogram, Stride, LineSkip](Vector<double> &LinesOut)
			{
				double At = 0.0;//this->GetXMin();
				for(int Idx = 0; Idx < NBinsHistogram; ++Idx)
				{
					LinesOut << At;
					At += Stride * (double)LineSkip;
				}
			};
		}
		else if(PlotMajorMode == MajorMode_QQ)
		{
			this->cbModifFormatXGridUnits.Clear();
			this->cbModifFormatX.Clear();
			//NOTE: Histograms require completely different zooming.
			this->ZoomToFit(true, true);
			PlotWasAutoResized = false;
			
			double YRange = this->GetYRange();
			double YMin   = this->GetYMin();
			double XRange = this->GetXRange();
			double XMin   = this->GetXMin();
			
			//NOTE: Make it so that the extremal points are not on the border
			double ExtendY = YRange * 0.1;
			YRange += 2.0 * ExtendY;
			YMin -= ExtendY;
			double ExtendX = XRange * 0.1;
			XRange += 2.0 * ExtendX;
			XMin -= ExtendX;
			
			this->SetRange(XRange, YRange);
			this->SetXYMin(XMin, YMin);
		}
		else if(PlotMajorMode == MajorMode_Profile)
		{
			PlotWasAutoResized = false;
		}
		else
		{
			this->cbModifFormatXGridUnits.Clear();
			this->cbModifFormatX.Clear();
			this->ZoomToFit(false, true);
			
			this->SetGridLinesX << [this, InputStartTime, TimestepSize](Vector<double> &V){UpdateDateGridLinesX(V, InputStartTime, TimestepSize);}; //TODO: Reset to THISBACK when appropriate
			
			double YRange = this->GetYRange();
			double YMin   = this->GetYMin();
			if(YMin > 0.0)
			{
				double NewRange = YRange + YMin;
				this->SetRange(this->GetXRange(), NewRange);
				this->SetXYMin(this->GetXMin(), 0.0);
			}
			
			if(!PlotWasAutoResized)
			{
				this->ZoomToFit(true, false);
				PlotWasAutoResized = true;
			}
			
			const char *MonthNames[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
			
			int DoStep = GetSmallestStepResolution(PlotSetup.AggregationPeriod, TimestepSize);
			
			//NOTE: Format to be displayed at grid lines
			if(DoStep == 0)
			{
				this->cbModifFormatXGridUnits << [this, MonthNames, InputStartTime](String &s, int i, double d)
				{
					double dd = d <= 0.0 ? d - 0.01 : d + 0.01;   //NOTE: For weird reasons we get floating point imprecition unless we do this
					Time D2 = InputStartTime + (int64)dd;
					s << Format("%02d:%02d:%02d", D2.hour, D2.minute, D2.second);
					if(D2.hour==0 && D2.minute==0 && D2.second==0)
					{
						s << Format("\n%d.", D2.day);
						if(D2.day == 1)
						{
							s << " " << MonthNames[D2.month-1];
							if(D2.month == 1) s << Format("\n%d", D2.year);
						}
					}
				};
			}
			else if(DoStep == 1 || DoStep == 2)
			{
				this->cbModifFormatXGridUnits << [this, MonthNames, InputStartTime](String &s, int i, double d)
				{
					Time D2 = InputStartTime + (int64)(d + 0.5); //NOTE: The +0.5 is to avoid flickering when panning
					s << Format("%02d:%02d", D2.hour, D2.minute);
					if(D2.hour==0 && D2.minute==0)
					{
						s << Format("\n%d.", D2.day);
						if(D2.day == 1)
						{
							s << " " << MonthNames[D2.month-1];
							if(D2.month == 1) s << Format("\n%d", D2.year);
						}
					}
				};
			}
			else if(DoStep == 3)
			{
				this->cbModifFormatXGridUnits << [this, MonthNames, InputStartTime](String &s, int i, double d)
				{
					Time D2 = InputStartTime + (int64)(d + 0.5); //NOTE: The +0.5 is to avoid flickering when panning
					s << Format("%d.", D2.day);
					if(D2.day == 1) s << " " << MonthNames[D2.month-1];
					if(D2.month == 1 && D2.day == 1) s << Format("\n%d", D2.year);
				};
			}
			else if(DoStep == 4)
			{
				this->cbModifFormatXGridUnits << [this, MonthNames, InputStartTime](String &s, int i, double d)
				{
					Time D2 = InputStartTime + (int64)(d + 0.5); //NOTE: The +0.5 is to avoid flickering when panning
					if(D2.month == 1) s << Format("%s\n%d", MonthNames[D2.month-1], D2.year);
					else              s << MonthNames[D2.month-1];
				};
			}
			else if(DoStep == 5)
			{
				this->cbModifFormatXGridUnits << [this, InputStartTime](String &s, int i, double d)
				{
					Time D2 = InputStartTime + (int64)(d + 0.5); //NOTE: The +0.5 is to avoid flickering when panning
					s = Format("%d", D2.year);
				};
			}
			
			//NOTE: Format to be displayed at popup or data table
			if(DoStep == 0 || DoStep == 1 || DoStep == 2)
			{
				this->cbModifFormatX << [this, InputStartTime](String &s, int i, double d)
				{
					double dd = d <= 0.0 ? d - 0.01 : d + 0.01;   //NOTE: For weird reasons we get floating point imprecition unless we do this
					Time D2 = InputStartTime + (int64)dd;
					s = Format(D2);
				};
			}
			else if(DoStep == 3)
			{
				this->cbModifFormatX << [this, InputStartTime](String &s, int i, double d)
				{
					Time D2 = InputStartTime + (int64)(d + 0.5);
					Date D3(D2.year, D2.month, D2.day);
					s = Format(D3);
				};
			}
			else if(DoStep == 4)
			{
				this->cbModifFormatX << [this, InputStartTime](String &s, int i, double d)
				{
					Time D2 = InputStartTime + (int64)(d + 0.5);
					s = Format("%d-%d", D2.year, D2.month);
				};
			}
			else if(DoStep == 5)
			{
				this->cbModifFormatX << [this, InputStartTime](String &s, int i, double d)
				{
					Time D2 = InputStartTime + (int64)(d + 0.5);
					s = Format("%d", D2.year);
				};
			}
		}
		
		if(this->GetShowLegend() && PlotMajorMode != MajorMode_Profile2D)
			this->SetRange(this->GetXRange(), this->GetYRange() * 1.15);  //So that the legend does not obscure the plot (in most cases).
		
		if(PlotMajorMode != MajorMode_Profile2D)
		{
			this->cbModifFormatY.Clear();
			if(this->PlotSetup.YAxisMode == YAxis_Logarithmic)
			{
				this->cbModifFormatY << [](String &s, int i, double d)
				{
					s = FormatDoubleExp(pow(10., d), 2);
				};
			}

			if(PlotMajorMode != MajorMode_QQ) SetBetterGridLinePositions(1);
		}

		if(PlotMajorMode == MajorMode_QQ) SetBetterGridLinePositions(0);
	}
	
	bool AllowScrollX = !(PlotMajorMode == MajorMode_Profile || PlotMajorMode == MajorMode_Histogram || PlotMajorMode == MajorMode_ResidualHistogram);
	this->SetMouseHandling(AllowScrollX, false);
}

int MyPlot::AddHistogram(String &Legend, String &Unit, double *Data, size_t Len)
{
	std::vector<double> &XValues = PlotData.Allocate(0);
	std::vector<double> &YValues = PlotData.Allocate(0);
	
	double Min = DBL_MAX;
	double Max = DBL_MIN;
	
	size_t FiniteCount = 0;
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double D = Data[Idx];
		if(std::isfinite(D) && !IsNull(D))
		{
			FiniteCount++;
			Min = std::min(D, Min);
			Max = std::max(D, Max);
		}
	}
		
	if(Max != Min && FiniteCount > 0) //TODO: epsilon distance instead? But that may not make sense in this case.
	{
		double Span = Max - Min;
		//size_t NBins = 1 + (size_t)std::ceil(std::log2((double)FiniteCount));   //NOTE: Sturges' rule.
		size_t NBins = 2*(size_t)(std::ceil(std::cbrt((double)FiniteCount)));      //NOTE: Rice's rule.
		
		double Stride = Span / (double) NBins;
		
		XValues.resize(NBins);
		YValues.resize(NBins); //This 0-initializes the values, right?
		
		for(size_t Idx = 0; Idx < NBins; ++Idx)
		{
			XValues[Idx] = Min + (0.5 + (double)Idx)*Stride;
		}
		
		double Scale = 1.0 / (double)FiniteCount;
		
		for(size_t Idx = 0; Idx < Len; ++Idx)
		{
			double D = Data[Idx];
			if(std::isfinite(D) && !IsNull(D))
			{
				size_t BinIndex = (size_t)((D - Min)/Stride);
				if(BinIndex == NBins) BinIndex = NBins-1;     //NOTE: Happens if D==Max
				YValues[BinIndex] += Scale;
			}
		}
		
		Color GraphColor = PlotColors.Next();
		double Darken = 0.4;
		Color BorderColor((int)(((double)GraphColor.GetR())*Darken), (int)(((double)GraphColor.GetG())*Darken), (int)(((double)GraphColor.GetB())*Darken));
		this->AddSeries(XValues.data(), YValues.data(), XValues.size()).Legend(Legend).PlotStyle<BarSeriesPlot>().BarWidth(0.5*Stride).NoMark().Fill(GraphColor).Stroke(1.0, BorderColor).Units("", Unit);
		
		return (int)NBins;
	}
	else
	{
		//TODO?
		return 0;
	}
}


void MyPlot::AddNormalApproximation(String &Legend, int SampleCount, double Min, double Max, double Mean, double StdDev)
{
	std::vector<double> &XValues = PlotData.Allocate(SampleCount);
	std::vector<double> &YValues = PlotData.Allocate(SampleCount);
	
	double Stride = (Max - Min) / (double)SampleCount;
	
	for(int Point = 0; Point < SampleCount; ++Point)
	{
		double Low = Min + (double)Point * Stride;
		double High = Min + (double)(Point+1)*Stride;
		
		XValues[Point] = 0.5*(Low + High);
		YValues[Point] = 0.5 * (std::erf((High-Mean) / (std::sqrt(2.0)*StdDev)) - std::erf((Low-Mean) / (std::sqrt(2.0)*StdDev)));
	}
	
	Color GraphColor = PlotColors.Next();
	this->AddSeries(XValues.data(), YValues.data(), XValues.size()).Legend(Legend).MarkColor(GraphColor).Stroke(0.0, GraphColor).Dash("");
}

void AddDays(Date &date, int Days)
{
	//NOTE: This is here to be able to call +=days on a Time object (which would otherwise be
	//interpreted as adding seconds. Seems to be no way to call the super class operator here when
	//it is overloaded.
	date += Days;
}


void AggregateData(Time &ReferenceTime, Time &StartTime, uint64 Timesteps, double *Data, aggregation_period IntervalType, aggregation_type AggregationType, timestep_size TimestepSize, std::vector<double> &XValues, std::vector<double> &YValues)
{
	double CurrentAggregate;
	if(AggregationType == Aggregation_Mean || AggregationType == Aggregation_Sum)
		CurrentAggregate = 0.0;
	else if(AggregationType == Aggregation_Min)
		CurrentAggregate = DBL_MAX;
	else if(AggregationType == Aggregation_Max)
		CurrentAggregate = -DBL_MAX;
		
	int    CurrentCount = 0;
	Time CurrentTime = StartTime;
	
	for(int CurrentTimestep = 0; CurrentTimestep < Timesteps; ++CurrentTimestep)
	{
		double Val = Data[CurrentTimestep];
		if(std::isfinite(Val) && !IsNull(Val))
		{
			if(AggregationType == Aggregation_Mean || AggregationType == Aggregation_Sum)
				CurrentAggregate += Val;
			else if(AggregationType == Aggregation_Min)
				CurrentAggregate = std::min(Val, CurrentAggregate);
			else if(AggregationType == Aggregation_Max)
				CurrentAggregate = std::max(Val, CurrentAggregate);
			
			++CurrentCount;
		}
		
		int NextTimestep = CurrentTimestep+1;
		Time NextTime = CurrentTime;
		AdvanceTimesteps(NextTime, 1, TimestepSize);
		
		//TODO: Want more aggregation interval types than year or month for models with
		//non-daily resolutions
		bool PushAggregate = (NextTimestep == Timesteps);
		if(IntervalType == Aggregation_Yearly || IntervalType == Aggregation_Monthly)
			PushAggregate = PushAggregate || (NextTime.year != CurrentTime.year);
		if(IntervalType == Aggregation_Monthly)
			PushAggregate = PushAggregate || (NextTime.month != CurrentTime.month);
		if(IntervalType == Aggregation_Weekly)
			PushAggregate = PushAggregate || (DayOfWeek(NextTime) == 1);
		
		if(PushAggregate)
		{
			double YVal = CurrentAggregate;
			if(AggregationType == Aggregation_Mean) YVal /= (double)CurrentCount;
			
			if(!std::isfinite(YVal) || CurrentCount == 0) YVal = Null; //So that plots show it as empty instead of a line going to infinity.
			
			YValues.push_back(YVal);
			
			//TODO: Clean this up eventually:
			Time Beginning = CurrentTime;
			Beginning.second = 0;
			Beginning.minute = 0;
			Beginning.hour   = 0;
			if(IntervalType == Aggregation_Weekly)
			{
				int Dow = DayOfWeek(Beginning)-1;
				if(Dow==-1) Dow = 6;
				AddDays(Beginning, -Dow);
			}
			else
			{
				Beginning.day    = 1;
				if(IntervalType == Aggregation_Yearly) Beginning.month = 1;
			}
			double XVal = (double)(Beginning - ReferenceTime);
			
			XValues.push_back(XVal);
			
			if(AggregationType == Aggregation_Mean || AggregationType == Aggregation_Sum)
				CurrentAggregate = 0.0;
			else if(AggregationType == Aggregation_Min)
				CurrentAggregate = DBL_MAX;
			else if(AggregationType == Aggregation_Max)
				CurrentAggregate = -DBL_MAX;
			
			CurrentCount = 0;
		}
		
		CurrentTime = NextTime;
	}
}


ScatterDraw &MyPlot::MyAddSeries(double *XValues, double *YValues, size_t Len, bool IsInput, const Color Col, plot_major_mode MajorMode)
{
	//TODO: We should make this work for input series too, but that would require serious
	//refactoring.
	if(IsInput || (MajorMode != MajorMode_Stacked && MajorMode != MajorMode_StackedShare))
		return this->AddSeries(XValues, YValues, Len);
	
	if(!this->CachedStackY.empty() && this->CachedStackLen != Len)
	{
		PromptOK("Internal error when generating stacked plot: Length of result series not matching!");
		return *this;
	}
	
	this->CachedStackY.push_back(YValues);
	this->CachedStackLen = Len;

	return this->AddSeries(XValues, YValues, Len).Fill(Col);
}

void MyPlot::StackedPlotFixup(plot_major_mode MajorMode)
{
	if(this->CachedStackY.empty()) return;
	
	size_t Len = this->CachedStackLen;
	
	std::vector<double> Sums(Len);
	std::fill(Sums.begin(), Sums.end(), 0.0);
	
	double *Sum = Sums.data();
	
	//NOTE we have to do backwards iteration so that the ones that were added first are the
	//ones on top. Otherwise the ones in front will be higher and over-paint them.
	for(int Idx = this->CachedStackY.size()-1; Idx >= 0; --Idx) 
	{
		double *Ys = this->CachedStackY[Idx];
		for(size_t Ts = 0; Ts < Len; ++Ts)
		{
			if(std::isfinite(Ys[Ts]))
				Sums[Ts] += Ys[Ts];
			Ys[Ts] = Sums[Ts];
		}
	}
	if(MajorMode == MajorMode_StackedShare)
	{
		for(int Idx = this->CachedStackY.size()-1; Idx >= 0; --Idx) 
		{
			double *Ys = this->CachedStackY[Idx];
			for(size_t Ts = 0; Ts < Len; ++Ts)
			{
				Ys[Ts] = Ys[Ts] * 100.0 / Sums[Ts];
				if(!std::isfinite(Ys[Ts])) Ys[Ts] = 0.0;
			}
		}
	}
	
}


Color MyPlot::AddPlot(String &Legend, String &Unit, double *XIn, double *Data, size_t Len, bool IsInput, Time &ReferenceTime, Time &StartTime, timestep_size TimestepSize, double MinY, double MaxY, Color OverrideColor, plot_major_mode MajorMode)
{
	aggregation_period IntervalType  = PlotSetup.AggregationPeriod;
	aggregation_type AggregationType = PlotSetup.AggregationType;
	y_axis_mode YAxis                = PlotSetup.YAxisMode;
	
	Color GraphColor;
	if(IsNull(OverrideColor))
		GraphColor = PlotColors.Next();
	else
		GraphColor = OverrideColor;
	
	ScatterDraw *Graph = nullptr;
	
	if(IntervalType == Aggregation_None)
	{
		//TODO: It may not always be that good of an idea to modify the
		//incoming data... Though it does not cause a problem now..
		if(YAxis == YAxis_Normalized)
		{
			for(size_t Idx = 0; Idx < Len; ++Idx)
			{
				if(MaxY > 0.0) Data[Idx] = Data[Idx] / MaxY;
				else Data[Idx] = 0.0;
			}
		}
		else if(YAxis == YAxis_Logarithmic)
		{
			for(size_t Idx = 0; Idx < Len; ++Idx)
				Data[Idx] = std::log10(Data[Idx]);
		}
		Graph = &this->MyAddSeries(XIn, Data, Len, IsInput, GraphColor, MajorMode);
	}
	else //Weekly, monthly values or yearly values
	{
		//Would it help to reserve some size for these?
		std::vector<double> &XValues = PlotData.Allocate(0);
		std::vector<double> &YValues = PlotData.Allocate(0);
		
		AggregateData(ReferenceTime, StartTime, Len, Data, IntervalType, AggregationType, TimestepSize, XValues, YValues);
		
		Graph = &this->MyAddSeries(XValues.data(), YValues.data(), XValues.size(), IsInput, GraphColor, MajorMode);
	}
	
	if(Graph) //NOTE: Graph should never be nullptr, but in case we have a bug, let's not crash.
	{
		Graph->Legend(Legend);
		Graph->Units(Unit);
		if(IsInput && PlotSetup.ScatterInputs)
		{
			Graph->MarkBorderColor(GraphColor).Stroke(0.0, GraphColor).Opacity(0.5);
			Graph->MarkStyle<CircleMarkPlot>();
			
			int Index = this->GetCount()-1;
			this->SetMarkColor(Index, Null); //NOTE: Calling Graph->MarkColor(Null) does not make it transparent, so we have to do it like this.
		}
		else
			Graph->NoMark().Stroke(1.5, GraphColor).Dash("");
	}
	
	return GraphColor;
}

void MyPlot::AddPlotRecursive(MobiView *Parent, MyRichView &PlotInfo, std::string &Name, std::vector<char *> &IndexSets,
	std::vector<std::string> &CurrentIndexes, int Level, bool IsInput, uint64 Timesteps,
	Time &ReferenceDate, Time &StartDate, double *XIn, plot_major_mode MajorMode, int64 GofOffset, int64 GofTimesteps)
{
	//TODO: We would ideally decouple this from having a MobiView * pointer, but there is so
	//much that depends on that right now.
	
	if(Level == IndexSets.size())
	{
		std::vector<char *> Indexes(CurrentIndexes.size());
		for(size_t Idx = 0; Idx < CurrentIndexes.size(); ++Idx)
			Indexes[Idx] = (char *)CurrentIndexes[Idx].data();
		
		char **IndexData = Indexes.data();
		if(CurrentIndexes.size() == 0) IndexData = nullptr;
		
		//TODO: This is wasteful if the result series was not computed...
		std::vector<double> &Data = PlotData.Allocate(Timesteps);
		
		String Unit;
		String Provided = "";
		
		if(!IsInput)
		{
			if(!Parent->ModelDll.ResultWasComputed(Parent->DataSet, Name.data(), IndexData, Indexes.size()))
				Provided = " (time series not computed)"; //TODO: Should we just omit displaying it in this case?
			Parent->ModelDll.GetResultSeries(Parent->DataSet, Name.data(), IndexData, Indexes.size(), Data.data());
			Unit = Parent->ModelDll.GetResultUnit(Parent->DataSet, Name.data());
		}
		else
		{
			if(!Parent->ModelDll.InputWasProvided(Parent->DataSet, Name.data(), IndexData, Indexes.size()))
				Provided = " (time series not provided)"; //TODO: Should we just omit displaying it in this case?
			Parent->ModelDll.GetInputSeries(Parent->DataSet, Name.data(), IndexData, Indexes.size(), Data.data(), false);
			Unit = Parent->ModelDll.GetInputUnit(Parent->DataSet, Name.data());
		}
		if(Parent->CheckDllUserError()) return;
		
		double *Dat = Data.data();
		size_t Len = Data.size();  //This will always be == Timesteps though?
		
		String Legend = Name.data();
		if(CurrentIndexes.size() > 0)
		{
			Legend << " (";
			for(size_t Idx = 0; Idx < CurrentIndexes.size(); ++Idx)
			{
				Legend << CurrentIndexes[Idx].data();
				if(Idx < CurrentIndexes.size()-1) Legend << ", ";
			}
			Legend << ")";
		}
		Legend << Provided;
		
		timeseries_stats Stats = {};
		ComputeTimeseriesStats(Stats, Dat+GofOffset, GofTimesteps, Parent->StatSettings, false);
		
		Color Col = AddPlot(Legend, Unit, XIn, Dat, Len, IsInput, ReferenceDate, StartDate, Parent->TimestepSize, Stats.Min, Stats.Max, Null, MajorMode);
		
		DisplayTimeseriesStats(Stats, Legend, Unit, Parent->StatSettings, PlotInfo, Col);
	}
	else
	{
		char *IndexSetName = IndexSets[Level];
		size_t Id = Parent->IndexSetNameToId[IndexSetName];
	
		for(const std::string &IndexName : PlotSetup.SelectedIndexes[Id])
		{
			CurrentIndexes[Level] = IndexName;
			AddPlotRecursive(Parent, PlotInfo, Name, IndexSets, CurrentIndexes, Level + 1, IsInput, Timesteps, ReferenceDate, StartDate, XIn, MajorMode, GofOffset, GofTimesteps);
		}
	}
}




void MyPlot::AddQQPlot(String &ModUnit, String &ObsUnit, String &ModName, String &ObsName, timeseries_stats &ModeledStats, timeseries_stats &ObservedStats, StatisticsSettings &StatSettings)
{
	size_t NumPercentiles = StatSettings.Percentiles.size();
	
	std::vector<double> &XValues = PlotData.Allocate(NumPercentiles);
	std::vector<double> &YValues = PlotData.Allocate(NumPercentiles);
	
	QQLabels.Clear();
	
	for(size_t Idx = 0; Idx < NumPercentiles; ++Idx)
	{
		XValues[Idx] = ModeledStats.Percentiles[Idx];
		YValues[Idx] = ObservedStats.Percentiles[Idx];
		QQLabels << Format("%g", StatSettings.Percentiles[Idx]);
	}
	
	Color GraphColor = PlotColors.Next();
	this->AddSeries(XValues.data(), YValues.data(), XValues.size()).MarkColor(GraphColor).Stroke(0.0, GraphColor).Dash("").Units(ModUnit, ModUnit)
		.AddLabelSeries(QQLabels, 10, 0, StdFont().Height(15), ALIGN_CENTER);
		
	this->ShowLegend(false);
	
	this->SetLabels(ModName, ObsName); //TODO: This does not work!
}

void MyPlot::AddLine(const String &Legend, double X0, double X1, double Y0, double Y1, Color GraphColor)
{
	std::vector<double> &XValues = PlotData.Allocate(2);
	std::vector<double> &YValues = PlotData.Allocate(2);
	
	XValues[0] = X0;
	XValues[1] = X1;
	YValues[0] = Y0;
	YValues[1] = Y1;
	
	if(IsNull(GraphColor)) GraphColor = PlotColors.Next();
	auto& Graph = this->AddSeries(XValues.data(), YValues.data(), XValues.size()).NoMark().Stroke(1.5, GraphColor).Dash("6 3");
	if(!IsNull(Legend)) Graph.Legend(Legend);
	else Graph.ShowSeriesLegend(false);
}

void MyPlot::AddTrendLine(String &Legend, double XYCovar, double XVar, double YMean, double XMean, double StartX, double EndX)
{
	double Beta = XYCovar / XVar;
	double Alpha = YMean - Beta*XMean;
	
	double X0 = StartX;
	double X1 = EndX;
	double Y0 = Alpha + X0*Beta;
	double Y1 = Alpha + X1*Beta;
	
	AddLine(Legend, X0, X1, Y0, Y1);
}


void MyPlot::SetBetterGridLinePositions(int Dim)
{
	double Min;
	double Range;
	
	if(Dim == 0)
	{
		Min = this->GetXMin();
		Range = this->GetXRange();
	}
	else
	{
		Min = this->GetYMin();
		Range = this->GetYRange();
	}
	
	double LogRange = std::log10(Range);
	double IntPart;
	double FracPart = std::modf(LogRange, &IntPart);
	if(Range < 1.0) IntPart -= 1.0;
	double OrderOfMagnitude = std::pow(10.0, IntPart);
	double Stretch = Range / OrderOfMagnitude;
	
	double Stride = OrderOfMagnitude * 0.1;
	if(Stretch >= 2.0) Stride = OrderOfMagnitude * 0.2;
	if(Stretch >= 2.5) Stride = OrderOfMagnitude * 0.25;
	if(Stretch >= 5.0) Stride = OrderOfMagnitude * 0.5;
	
	Min = std::floor(Min / Stride)*Stride;
	int Count = (int)std::ceil(Range / Stride);
	
	//MainPlot.SetMinUnits(Null, Min);  //We would prefer to do this, but for some reason it works
	//poorly when there are negative values...
	if(Dim == 0)
	{
		this->SetXYMin(Min, Null);
		this->SetMajorUnits(Stride, Null);
	}
	else
	{
		this->SetXYMin(Null, Min);
		this->SetMajorUnits(Null, Stride);
	}
}


int RoundStep(int Step)
{
	int Log10Step = (int)std::log10((double)Step);
	
	int OrderOfMagnitude = 1;
	for(int Idx = 0; Idx < Log10Step; ++Idx) OrderOfMagnitude*=10;   //NOTE: Not efficient, but will not be a problem in practice
	int Stretch = Step / OrderOfMagnitude;
	if(Stretch == 1) return OrderOfMagnitude;
	if(Stretch == 2) return OrderOfMagnitude*2;
	if(Stretch <= 5) return OrderOfMagnitude*5;
	return OrderOfMagnitude*10;
}

int RoundStep60(int Step)
{
	if(Step == 3) Step = 2;
	else if(Step == 4) Step = 5;
	else if(Step == 6 || Step == 7) Step = 5;
	else if(Step >= 8 && Step <= 12) Step = 10;
	else if(Step >= 13 && Step <= 17) Step = 15;
	else if(Step >= 18 && Step <= 26) Step = 20;
	else if(Step >= 27 && Step <= 40) Step = 30;
		
	return Step;
}

int RoundStep24(int Step)
{
	if(Step == 3) Step = 2;
	if(Step == 5) Step = 4;
	if(Step == 7 || Step == 8) Step = 6;
	if(Step >= 9 && Step <= 14) Step = 12;
	
	return Step;
}


void MyPlot::UpdateDateGridLinesX(Vector<double> &LinesOut, Time InputStartTime, timestep_size TimestepSize)
{
	//InputStartTime  corresponds to  X=0. X resolution is always measured in seconds
	
	//TODO! Has to be improved for sub-day timestep resolutions!!
	
	
	int DesiredGridLineNum = 10;  //TODO: Make it sensitive to plot size
	
	double XMin = this->GetXMin();
	double XRange = this->GetXRange();
	
	Time FirstTime = InputStartTime + (int64)XMin;
	Time LastTime  = FirstTime + (int64)XRange;
	
	//NOTE: DoStep denotes the unit that we try to use for spacing the grid lines. 0=seconds, 1=minutes, 2=hours, 3=days,
	//4=months, 5=years
	int DoStep = GetSmallestStepResolution(PlotSetup.AggregationPeriod, TimestepSize);

	if(DoStep==0)    //Seconds
	{
		int64 SecondRange = (LastTime - FirstTime);
		int64 SecondStep = SecondRange / DesiredGridLineNum + 1;
		
		SecondStep = RoundStep60(SecondStep);
		
		if(SecondStep > 30) DoStep = 1;    //The plot is too wide to do secondly resolution, try minutely instead
		else
		{
			Time IterTime = FirstTime;
			IterTime -= (IterTime.second % SecondStep);
			for(; IterTime <= LastTime; IterTime += SecondStep)
			{
				double XVal = (double)((IterTime - InputStartTime)) - XMin;
				if(XVal > 0 && XVal < XRange)
					LinesOut << XVal;
			}
			
			return;
		}
	}
	
	if(DoStep == 1)
	{
		int64 MinuteRange = (LastTime - FirstTime) / 60;
		int64 MinuteStep = MinuteRange / DesiredGridLineNum + 1;
		
		MinuteStep = RoundStep60(MinuteStep);
		
		if(MinuteStep > 30) DoStep = 2;    //The plot is too wide to do minutely resolution, try hourly instead
		else
		{
			Time IterTime = FirstTime;
			IterTime.second = 0;
			IterTime -= 60*(IterTime.minute % MinuteStep);
			for(; IterTime <= LastTime; IterTime += 60*MinuteStep)
			{
				double XVal = (double)((IterTime - InputStartTime)) - XMin;
				if(XVal > 0 && XVal < XRange)
					LinesOut << XVal;
			}
			
			return;
		}
	}
	
	if(DoStep == 2)
	{
		int64 HourRange = (LastTime - FirstTime) / 3600;
		int64 HourStep = HourRange / DesiredGridLineNum + 1;
		
		HourStep = RoundStep24(HourStep);
		
		if(HourStep > 12) DoStep = 3;    //The plot is too wide to do hourly resolution, try daily instead
		else
		{
			Time IterTime = FirstTime;
			IterTime.second = 0;
			IterTime.minute = 0;
			IterTime -= 3600*(IterTime.hour % HourStep);
			for(; IterTime <= LastTime; IterTime += 3600*HourStep)
			{
				double XVal = (double)((IterTime - InputStartTime)) - XMin;
				if(XVal > 0 && XVal < XRange)
					LinesOut << XVal;
			}
			
			return;
		}
	}
	
	Time FirstDate(FirstTime.year, FirstTime.month, FirstTime.day);
	Time LastDate(LastTime.year, LastTime.month, LastTime.day);
	
	if(DoStep==3)    //Daily
	{
		int DayRange = (LastTime - FirstTime) / 86400 + 1;
		int DayStep = DayRange / DesiredGridLineNum + 1;
		
		if(DayStep >= 20) DoStep = 4; //The plot is too wide to do daily resolution, try monthly instead;
		else
		{
			if(DayStep == 3) DayStep = 2;
			else if(DayStep == 4) DayStep = 5;
			else if(DayStep == 6 || DayStep == 7) DayStep = 5;
			else if(DayStep == 8 || DayStep == 9) DayStep = 10;
			else if(DayStep > 2 && DayStep < 20) DayStep = 15;
			
			Time IterDate = FirstDate;
			IterDate.day -= (((int)IterDate.day-1) % DayStep);
			while(IterDate.Compare(LastDate) != 1)
			{
				int DaysOfMonth = GetDaysOfMonth(IterDate.month, IterDate.year);
				IterDate.day += DayStep;
				bool Advance = false;
				if(IterDate.day > DaysOfMonth) Advance = true;
				else if((DayStep != 1 && (DaysOfMonth - (int)IterDate.day < DayStep/2))) Advance = true; //This is kind of arbitrary.
				
				if(Advance)
				{
					IterDate.day = 1;
					IterDate.month++;
					if(IterDate.month > 12)
					{
						IterDate.month = 1;
						IterDate.year++;
					}
				}
				double XVal = (double)((IterDate - InputStartTime)) - XMin;
				if(XVal > 0 && XVal < XRange)
					LinesOut << XVal;
			}
			
			return;
		}
	}
	
	if(DoStep == 4) //Monthly
	{
		int MonthRange = (int)LastDate.month - (int)FirstDate.month + 12*(LastDate.year - FirstDate.year);
		int MonthStep = MonthRange / DesiredGridLineNum + 1;
		
		if(MonthStep >= 10) DoStep = 5; //The plot is too wide to do monthly resolution, try yearly instead
		else
		{
			if(MonthStep == 4) MonthStep = 3;
			else if(MonthStep == 5 || (MonthStep > 6 && MonthStep <= 9)) MonthStep = 6;
			
			int FirstMonth = (int)FirstDate.month;
			int Surp = ((FirstMonth-1) % MonthStep);
			MonthRange += Surp;
			FirstMonth -= Surp; //NOTE: This should not result in FirstMonth being negative.
			Time IterDate(FirstDate.year, (byte)FirstMonth, 1);
			for(int Month = 0; Month <= MonthRange; Month += MonthStep)
			{
				IterDate.month += MonthStep;
				if(IterDate.month > 12)
				{
					IterDate.month -= 12;
					IterDate.year++;
				}
				double XVal = (double)((IterDate - InputStartTime)) - XMin;
				if(XVal > 0 && XVal < XRange)
					LinesOut << XVal;
			}
			return;
		}
	}
	
	if(DoStep == 5) //Yearly
	{
		int FirstYear = FirstDate.year;
		int LastYear = LastDate.year;

		int YearCount = LastYear - FirstYear;
		int YearStep = YearCount / DesiredGridLineNum + 1;
		YearStep = RoundStep(YearStep);
		
		FirstYear -= (FirstYear % YearStep);
		
		Time IterDate(FirstYear, 1, 1);
		for(int Year = FirstYear; Year <= LastYear; Year += YearStep)
		{
			IterDate.year = Year;
			double XVal = (double)((IterDate - InputStartTime)) - XMin;
			if(XVal > 0 && XVal < XRange)
				LinesOut << XVal;
		}
	}
}


void PlotCtrl::TimestepSliderEvent()
{
	int Timestep = TimestepSlider.GetData();
	Time NewTime = ProfileDisplayTime;
	
	// Recompute the displayed date in the "TimestepEdit" based on the new position of the
	// slider.
	aggregation_period IntervalType = MainPlot.PlotSetup.AggregationPeriod;
	if(IntervalType == Aggregation_None)
		AdvanceTimesteps(NewTime, Timestep, Parent->TimestepSize);
	else if(IntervalType == Aggregation_Weekly)
	{
		AdvanceTimesteps(NewTime, 7*Timestep, Parent->TimestepSize); //TODO: Not properly tested. Should maybe "normalize" it to be on a monday.
	}
	else if(IntervalType == Aggregation_Monthly)
	{
		int Month   = ((NewTime.month + Timestep - 1) % 12) + 1;
		int YearAdd = (NewTime.month + Timestep - 1) / 12;
		NewTime.month = Month;
		NewTime.year += YearAdd;
	}
	else if(IntervalType == Aggregation_Yearly)
		NewTime.year += Timestep;
	
	TimestepEdit.SetData(NewTime);
	
	MainPlot.PlotSetup.ProfileTimestep = Timestep;
	MainPlot.ReplotProfile();
}

void PlotCtrl::TimestepEditEvent()
{
	//NOTE: This can only happen if we are in non-aggregated mode since the edit is disabled otherwise.
	//If this changes, this code has to be rethought.
	
	Time CurrentTime = TimestepEdit.GetData();
	int Timestep = TimestepsBetween(ProfileDisplayTime, CurrentTime, Parent->TimestepSize);
	TimestepSlider.SetData(Timestep);
	
	MainPlot.PlotSetup.ProfileTimestep = Timestep;
	MainPlot.ReplotProfile();
}


void MyPlot::ReplotProfile()
{
	this->RemoveAllSeries();
	
	std::vector<double> &XValues = PlotData.Data[ProfileIndexesCount];
	std::vector<double> &YValues = PlotData.Data[ProfileIndexesCount+1];
	
	for(size_t Idx = 0; Idx < ProfileIndexesCount; ++Idx)
	{
		YValues[Idx] = PlotData.Data[Idx][PlotSetup.ProfileTimestep];
	}
	
	Color &GraphColor = PlotColors.PlotColors[0];
	double Darken = 0.4;
	Color BorderColor((int)(((double)GraphColor.GetR())*Darken), (int)(((double)GraphColor.GetG())*Darken), (int)(((double)GraphColor.GetB())*Darken));
	this->AddSeries(XValues.data(), YValues.data(), XValues.size()).Legend(ProfileLegend).PlotStyle<BarSeriesPlot>().BarWidth(0.5).NoMark().Fill(GraphColor).Stroke(1.0, BorderColor).Units(ProfileUnit);
	
	this->SetLabelX(" ");
	this->SetLabelY(" ");
	
	this->Refresh();
}


void MyPlot::ClearAll(bool FullClear)
{
	this->RemoveAllSeries();
	PlotColors.Reset();
	PlotData.Clear();
	
	this->RemoveSurf();
	SurfX.Clear();
	SurfY.Clear();
	SurfZ.Clear();
	
	this->SetTitle(String(""));
	this->SetLabelX(" ");
	this->SetLabelY(" ");
	
	this->CachedStackY.clear();
	
	if(FullClear) PlotWasAutoResized = false;
}


void AdvanceTimesteps(Time &T, uint64 Timesteps, timestep_size TimestepSize)
{
	if(TimestepSize.Type == 0)  //Timestep magnitude measured in seconds.
	{
		T += ((int64)Timesteps)*((int64)TimestepSize.Magnitude);
	}
	else                        //Timestep magnitude measured in months.
	{
		int Month = (int)T.month + (int)Timesteps*TimestepSize.Magnitude;  //NOTE: Can't use T.month directly here since it is 8-bit and will overflow.
		while(Month > 12)
		{
			Month -= 12;
			T.year++;
		}
		T.month = Month;
	}
}

int64 TimestepsBetween(const Time &T1, const Time &T2, timestep_size TimestepSize)
{
	//NOTE: This could be quite tricky to get correct unless we can guarantee that T1 hits a
	//timestep exactly. So that should be ensured by the caller!
	
	if(TimestepSize.Type == 0)   //Timestep magnitude measured in seconds.
	{
		return (T2 - T1) / TimestepSize.Magnitude;
	}
	else                         //Timestep magnitude measured in months.
	{
		return ((T2.year - T1.year)*12 + T2.month - T1.month) / TimestepSize.Magnitude;
	}
}

int GetSmallestStepResolution(aggregation_period IntervalType, timestep_size TimestepSize)
{
	//NOTE: To compute the unit that we try to use for spacing the grid lines. 0=seconds, 1=minutes, 2=hours, 3=days,
	//4=months, 5=years

	if(IntervalType == Aggregation_None)
	{
		//NOTE: The plot does not display aggregated data, so the unit of the grid line step should be
		//determined by the model step size.
		if(TimestepSize.Type == 0)          //Timestep size is measured in seconds
		{
			if(TimestepSize.Magnitude < 60)         return 0;
			else if(TimestepSize.Magnitude < 3600)  return 1;
			else if(TimestepSize.Magnitude < 86400) return 2;
			else                                    return 3;
		}
		else                                        //Timestep size is measured in months
		{
			if(TimestepSize.Magnitude < 12) return 4;
			else                            return 5;
		}
	}
	else if(IntervalType == Aggregation_Weekly)  return 3;
	else if(IntervalType == Aggregation_Monthly) return 4;
	else if(IntervalType == Aggregation_Yearly)  return 5;
	
	return 0;
}

void ComputeXValues(Time &ReferenceTime, Time &StartTime, uint64 Timesteps, timestep_size TimestepSize, double *WriteX)
{
	if(TimestepSize.Type == 0)        //Timestep magnitude measured in seconds.
	{
		for(size_t Idx = 0; Idx < Timesteps; ++Idx)
		{
			WriteX[Idx] = (double)(StartTime - ReferenceTime) + (double)Idx*(double)TimestepSize.Magnitude;
		}
	}
	else                              //Timestep magnitude measured in months.
	{
		Time CurTime = StartTime;
		for(size_t Idx = 0; Idx < Timesteps; ++Idx)
		{
			WriteX[Idx] = (double)(CurTime - ReferenceTime);
			
			int Month = (int)CurTime.month + TimestepSize.Magnitude;
			//CurTime.month += TimestepSize.Magnitude;
			while(Month > 12)
			{
				Month -= 12;
				CurTime.year++;
			}
			CurTime.month = Month;
		}
	}
}

void MobiView::GetGofOffsets(const Time &ReferenceTime, uint64 ReferenceTimesteps, Time &BeginOut, Time &EndOut, int64 &GofOffsetOut, int64 &GofTimestepsOut)
{
	Time ReferenceEndTime = ReferenceTime;
	AdvanceTimesteps(ReferenceEndTime, ReferenceTimesteps-1, TimestepSize);
	
	BeginOut = CalibrationIntervalStart.GetData();
	EndOut   = CalibrationIntervalEnd.GetData();
	
	
	if(IsNull(BeginOut) || !BeginOut.IsValid()  || BeginOut < ReferenceTime   || BeginOut > ReferenceEndTime)
		BeginOut = ReferenceTime;
	if(IsNull(EndOut)   || !EndOut.IsValid()    || EndOut   < ReferenceTime   || EndOut   > ReferenceEndTime || EndOut < BeginOut)
		EndOut   = ReferenceEndTime;
	
	//TODO! Should probably ensure that BeginOut matches an exact timestep.
	
	GofTimestepsOut = TimestepsBetween(BeginOut, EndOut, TimestepSize) + 1; //NOTE: if start time = end time, there is still one timestep.
	GofOffsetOut    = TimestepsBetween(ReferenceTime, BeginOut, TimestepSize);
}


void MobiView::GetSingleResultSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, double *WriteTo, size_t SelectRowFor, std::string &Row)
{
	uint64 IndexSetCount = ModelDll.GetResultIndexSetsCount(DataSet, Name.data());
	std::vector<char *> IndexSets(IndexSetCount);
	ModelDll.GetResultIndexSets(DataSet, Name.data(), IndexSets.data());
	if(CheckDllUserError()) return;
	
	std::vector<char *> Indexes(IndexSets.size());
	for(size_t Idx = 0; Idx < IndexSets.size(); ++Idx)
	{
		const char *IndexSet = IndexSets[Idx];
		size_t Id = IndexSetNameToId[IndexSet];
		
		if(Id == SelectRowFor)
		{
			Indexes[Idx] = (char *)Row.data();
		}
		else
		{
			Indexes[Idx] = (char *)PlotSetup.SelectedIndexes[Id][0].data();
		}
	}
	
	ModelDll.GetResultSeries(DataSet, Name.data(), Indexes.data(), Indexes.size(), WriteTo);
	CheckDllUserError();
}


void MobiView::GetSingleInputSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, double *WriteTo, size_t SelectRowFor, std::string &Row)
{
	uint64 IndexSetCount = ModelDll.GetInputIndexSetsCount(DataSet, Name.data());
	std::vector<char *> IndexSets(IndexSetCount);
	ModelDll.GetInputIndexSets(DataSet, Name.data(), IndexSets.data());
	if(CheckDllUserError()) return;
	
	std::vector<char *> Indexes(IndexSets.size());
	for(size_t Idx = 0; Idx < IndexSets.size(); ++Idx)
	{
		const char *IndexSet = IndexSets[Idx];
		size_t Id = IndexSetNameToId[IndexSet];
		
		if(Id == SelectRowFor)
		{
			Indexes[Idx] = (char *)Row.data();
		}
		else
		{
			Indexes[Idx] = (char *)PlotSetup.SelectedIndexes[Id][0].data();
		}
	}
	
	ModelDll.GetInputSeries(DataSet, Name.data(), Indexes.data(), Indexes.size(), WriteTo, false);
	CheckDllUserError();
}

bool MobiView::GetIndexSetsForSeries(void *DataSet, std::string &Name, int Type, std::vector<char *> &IndexSetsOut)
{
	uint64 IndexSetCount;
	if(Type == 0)
		IndexSetCount = ModelDll.GetResultIndexSetsCount(DataSet, Name.data());
	else
		IndexSetCount = ModelDll.GetInputIndexSetsCount(DataSet, Name.data());
	
	IndexSetsOut.resize(IndexSetCount);
	if(Type == 0)
		ModelDll.GetResultIndexSets(DataSet, Name.data(), IndexSetsOut.data());
	else
		ModelDll.GetInputIndexSets(DataSet, Name.data(), IndexSetsOut.data());
	if(CheckDllUserError()) return false;
	
	return true;
}

bool MobiView::GetSelectedIndexesForSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, int Type, std::vector<char *> &IndexesOut)
{
	std::vector<char *> IndexSets;
	
	bool Success = GetIndexSetsForSeries(DataSet, Name, Type, IndexSets);
	
	if(!Success) return false;
	
	IndexesOut.resize(IndexSets.size());
	for(size_t Idx = 0; Idx < IndexSets.size(); ++Idx)
	{
		size_t Id = IndexSetNameToId[IndexSets[Idx]];
		IndexesOut[Idx] = (char *)PlotSetup.SelectedIndexes[Id][0].data();
	}
	
	if(CheckDllUserError()) return false;
	
	return true;
}

void MobiView::GetSingleSelectedResultSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, String &Legend, String &Unit, double *WriteTo)
{
	std::vector<char *> Indexes;
	
	bool Success = GetSelectedIndexesForSeries(PlotSetup, DataSet, Name, 0, Indexes);
	if(!Success) return;
	
	ModelDll.GetResultSeries(DataSet, Name.data(), Indexes.data(), Indexes.size(), WriteTo);
	if(CheckDllUserError()) return;
	
	Unit = ModelDll.GetResultUnit(DataSet, Name.data());
	
	Legend << Name.data();
	Legend << MakeIndexString(Indexes);
}

void MobiView::GetSingleSelectedInputSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, String &Legend, String &Unit, double *WriteTo, bool AlignWithResults)
{
	std::vector<char *> Indexes;
	
	bool Success = GetSelectedIndexesForSeries(PlotSetup, DataSet, Name, 1, Indexes);
	if(!Success) return;
	
	ModelDll.GetInputSeries(DataSet, Name.data(), Indexes.data(), Indexes.size(), WriteTo, AlignWithResults);
	if(CheckDllUserError()) return;CheckDllUserError();
	
	Unit = ModelDll.GetInputUnit(DataSet, Name.data());
	
	Legend << Name.data();
	Legend << MakeIndexString(Indexes);
}


//TODO: Unify this with AddPlotRecursive!!
void MobiView::GetResultDataRecursive(std::string &Name, std::vector<char *> &IndexSets,
	std::vector<std::string> &CurrentIndexes, int Level, uint64 Timesteps, std::vector<std::vector<double>> &PushTo, std::vector<std::string> &PushNamesTo)
{
	if(Level == IndexSets.size())
	{
		std::vector<char *> Indexes(CurrentIndexes.size());
		for(size_t Idx = 0; Idx < CurrentIndexes.size(); ++Idx)
		{
			Indexes[Idx] = (char *)CurrentIndexes[Idx].data();
		}
		char **IndexData = Indexes.data();
		if(CurrentIndexes.size() == 0) IndexData = nullptr;
		
		PushTo.push_back({});
		std::vector<double> &WriteTo = PushTo[PushTo.size()-1];
		WriteTo.resize(Timesteps);
		
		ModelDll.GetResultSeries(DataSet, Name.data(), IndexData, Indexes.size(), WriteTo.data());
		
		PushNamesTo.push_back({});
		std::string &FullName = PushNamesTo[PushNamesTo.size()-1];
		
		FullName = Name;
		if(CurrentIndexes.size() > 0)
		{
			FullName += " (";
			for(size_t Idx = 0; Idx < CurrentIndexes.size(); ++Idx)
			{
				FullName += CurrentIndexes[Idx].data();
				if(Idx < CurrentIndexes.size()-1) FullName += ", ";
			}
			FullName += ")";
		}
		
		FullName += " [";
		FullName += ModelDll.GetResultUnit(DataSet, Name.data());
		FullName += "]";

		if(CheckDllUserError()) return;
	}
	else
	{
		char *IndexSetName = IndexSets[Level];
		size_t Id = IndexSetNameToId[IndexSetName];
		
		int RowCount = Plotter.EIndexList[Id]->GetCount();
		
		for(int Row = 0; Row < RowCount; ++Row)
		{
			if(Plotter.EIndexList[Id]->IsSelected(Row))
			{
				CurrentIndexes[Level] = Plotter.EIndexList[Id]->Get(Row, 0).ToString().ToStd();
				GetResultDataRecursive(Name, IndexSets, CurrentIndexes, Level+1, Timesteps, PushTo, PushNamesTo);
			}
		}
	}
}


#include <fstream>


void MobiView::SaveToCsv()
{
	if(!DataSet || (ModelDll.GetTimesteps(DataSet) == 0))
	{
		Log("Results can only be saved once the model has been loaded and run once.");
		return;
	}
	
	std::vector<std::string> Names;
	std::vector<std::vector<double>> Data;
	
	int RowCount = EquationSelecter.GetCount();
	
	uint64 Timesteps = ModelDll.GetTimesteps(DataSet);
	CheckDllUserError();
	
	int Idx = 0;
	for(int Row = 0; Row < RowCount; ++Row)
	{
		if(EquationSelecter.IsSelected(Row) && EquationSelecter.GetCtrl(Row, 1) != nullptr)
		{
			std::string Name = EquationSelecter.Get(Row, 0).ToString().ToStd();
			
			uint64 IndexSetCount = ModelDll.GetResultIndexSetsCount(DataSet, Name.data());
			std::vector<char *> IndexSets(IndexSetCount);
			ModelDll.GetResultIndexSets(DataSet, Name.data(), IndexSets.data());
			if(CheckDllUserError()) return;
			
			std::vector<std::string> CurrentIndexes(IndexSets.size());

			GetResultDataRecursive(Name, IndexSets,	CurrentIndexes, 0, Timesteps, Data, Names);
			
			++Idx;
		}
	}
	
	if(Names.empty())
	{
		Log("You must select some equations to export the result series of.");
		return;
	}
	
	FileSel Sel;
	Sel.Type("Csv files", "*.csv");
	//TODO: Remember the last folder you saved to so that the file selector does not default to C:/ ..
	Sel.ExecuteSaveAs();
	std::string FileName = Sel.Get().ToStd();
	if(FileName.empty())
	{
		return;
	}
	
	char DateStr[256];
	ModelDll.GetStartDate(DataSet, DateStr);
	Time ResultTime;
	StrToTime(ResultTime, DateStr);
	
	
	std::ofstream File;
	File.open(FileName.data());
	
	if(!File.is_open())
	{
		Log(Format("Unable to open file %s", FileName.data()));
		return;
	}
	
	File << "Date\t";
	for(int Idx = 0; Idx < Names.size(); ++Idx)
	{
		File << Names[Idx];
		if(Idx != Names.size()-1) File << "\t";
	}
	File << "\n";
	
	//TODO: Whether or not to print the timestamp could also be a config option.
	bool PrintTime = false;
	if(TimestepSize.Type == 0 && (TimestepSize.Magnitude % 86400 != 0 || ResultTime.second != 0 || ResultTime.minute != 0 || ResultTime.hour != 0) ) PrintTime = true;
	
	for(uint64 Timestep = 0; Timestep < Timesteps; ++Timestep)
	{
		if(PrintTime)
			File << Format(ResultTime).ToStd() << "\t";
		else
		{
			Date ResultDate(ResultTime.year, ResultTime.month, ResultTime.day);
			File << Format(ResultDate).ToStd() << "\t";
		}
		
		for(int Idx = 0; Idx < Data.size(); ++Idx)
		{
			File << Data[Idx][Timestep];
			if(Idx != Data.size()-1) File << "\t";
		}
		File << "\n";
		
		AdvanceTimesteps(ResultTime, 1, TimestepSize);
	}
	
	File.close();
	
	Log(Format("Results exported to %s", FileName.data()));
}





#endif
