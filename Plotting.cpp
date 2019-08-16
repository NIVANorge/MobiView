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
		EIndexList[Idx]->WhenAction = THISBACK(RePlot);
		EIndexList[Idx]->MultiSelect();

		EIndexList[Idx]->AddColumn("(no name)");
	}
	
	Plot.SetFastViewX(true);
	Plot.SetSequentialXAll(true);
	Plot.SetMouseHandling(true, false);
	
	Plot.SetPlotAreaLeftMargin(70);
	Plot.SetGridDash("");
	Color Grey(180, 180, 180);
	Plot.SetGridColor(Grey);

	//Plot.Responsive(true, 1.2); //NOTE: This seems like it looks better, but has to be tested on more machines.


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




void PlotCtrl::PlotModeChange()
{
	
	//TODO: This is called twice on every selection of an input or equation. We should maybe find a way to guard
	//against doing all the work when the state has not changed. But that is tricky..
	
	
	int MajorMode = PlotMajorMode.GetData();
	
	ScatterInputs.Disable();
	YAxisMode.Disable();
	TimeIntervals.Disable();
	
	TimestepEdit.Hide();
	TimestepSlider.Hide();
	TimestepSlider.Disable();
	
	if(MajorMode == MajorMode_Regular || MajorMode == MajorMode_CompareBaseline)
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
		int TimeIntervalSelected = TimeIntervals.GetData();
		if(TimeIntervalSelected == 0)
		{
			Aggregation.Disable();
		}
		else
		{
			Aggregation.Enable();
			YAxisMode.Disable();
			TimestepEdit.Disable();
		}
	}
	else
	{
		Aggregation.Disable();
	}
	
	//TODO: This could be more fine-grained. We don't have to update the index list every time.
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		EIndexList[Idx]->Disable();
	}
	
	int RowCount = Parent->InputSelecter.GetCount();
	
	for(int Row = 0; Row < RowCount; ++Row)
	{
		if(Parent->InputSelecter.IsSelected(Row))
		{
			std::string Name = Parent->InputSelecter.Get(Row, 0).ToString().ToStd();
			
			uint64 IndexSetCount = Parent->ModelDll.GetInputIndexSetsCount(Parent->DataSet, Name.data());
			std::vector<char *> IndexSets(IndexSetCount);
			Parent->ModelDll.GetInputIndexSets(Parent->DataSet, Name.data(), IndexSets.data());
			
			for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
			{
				const char *IndexSet = IndexSets[Idx];
				size_t Id = Parent->IndexSetNameToId[IndexSet];
				EIndexList[Id]->Enable();
			}
		}
	}
	
	uint64 Timesteps = Parent->ModelDll.GetTimesteps(Parent->DataSet);
	if(Timesteps != 0)
	{
		int RowCount = Parent->EquationSelecter.GetCount();
		
		for(int Row = 0; Row < RowCount; ++Row)
		{
			if(Parent->EquationSelecter.IsSelected(Row))
			{
				std::string Name = Parent->EquationSelecter.Get(Row, 0).ToString().ToStd();
				
				uint64 IndexSetCount = Parent->ModelDll.GetResultIndexSetsCount(Parent->DataSet, Name.data()); //IMPORTANT! Returns 0 if the model has not been run at least once!!
				std::vector<char *> IndexSets(IndexSetCount);
				Parent->ModelDll.GetResultIndexSets(Parent->DataSet, Name.data(), IndexSets.data());
				
				for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
				{
					const char *IndexSet = IndexSets[Idx];
					size_t Id = Parent->IndexSetNameToId[IndexSet];
					EIndexList[Id]->Enable();
				}
			}
		}
	}
	
	
	//PromptOK("Got here!");
	
	RePlot();
}


int PlotCtrl::AddHistogram(String &Legend, String &Unit, double *Data, size_t Len)
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
		size_t NBins = 1 + (size_t)(3.322 * std::log((double)FiniteCount));   //NOTE: Sturge's rule.
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
		Plot.AddSeries(XValues.data(), YValues.data(), XValues.size()).Legend(Legend).PlotStyle<BarSeriesPlot>().BarWidth(0.5*Stride).NoMark().Fill(GraphColor).Stroke(1.0, BorderColor).Units("", Unit);
		
		return (int)NBins;
	}
	else
	{
		//TODO?
		return 0;
	}
}


void PlotCtrl::AddNormalApproximation(String &Legend, int SampleCount, double Min, double Max, double Mean, double StdDev)
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
	Plot.AddSeries(XValues.data(), YValues.data(), XValues.size()).Legend(Legend).MarkColor(GraphColor).Stroke(0.0, GraphColor).Dash("");
}


void PlotCtrl::AggregateData(Date &ReferenceDate, Date &StartDate, uint64 Timesteps, double *Data, int IntervalType, int AggregationType, std::vector<double> &XValues, std::vector<double> &YValues)
{
	double CurrentAggregate = 0.0;
	int    CurrentCount = 0;
	Date CurrentDate = StartDate;
	int CurrentTimestep = 0;
	
	while(CurrentTimestep < Timesteps)
	{
		double Val = Data[CurrentTimestep];
		if(std::isfinite(Val) && !IsNull(Val))
		{
			CurrentAggregate += Val;
			++CurrentCount;
		}
		
		++CurrentTimestep;
		Date NextDate = CurrentDate+1;
		
		bool PushAggregate = (CurrentTimestep == Timesteps-1) || (NextDate.year != CurrentDate.year);
		if(IntervalType == 1)
		{
			PushAggregate = PushAggregate || (NextDate.month != CurrentDate.month);
		}
		if(PushAggregate)
		{
			double YVal = CurrentAggregate;
			if(AggregationType == 0) YVal /= (double)CurrentCount; //Aggregation mode is 'mean', else we actually wanted the sum.
			
			if(!std::isfinite(YVal)) YVal = Null; //So that plots show it as empty instead of a line going to infinity.
			
			YValues.push_back(YVal);
			
			//TODO: Is it better to put it at the beginning of the year/month or the middle?
			Date Beginning = CurrentDate;
			Beginning.day = 0;
			if(IntervalType == 2) Beginning.month = 0;
			double XVal = (double)(Beginning - ReferenceDate);
			
			//NOTE put the X position roughly in the middle of the respective month or year. This
			//increases clarity in the plot.
			
			//Edit: No, it is not that good to do this actually...
			
			//if(IntervalType == 1) XVal += 15.5;
			//else if(IntervalType == 2) XVal += 182.5;
			
			XValues.push_back(XVal);
			
			CurrentAggregate = 0.0;
			CurrentCount = 0;
		}
		
		CurrentDate = NextDate;
	}
}


void PlotCtrl::AddPlot(String &Legend, String &Unit, double *Data, size_t Len, bool Scatter, bool LogY, bool NormalY, Date &ReferenceDate, Date &StartDate, double MinY, double MaxY)
{
	int Timesteps = (int)Len;
	
	int IntervalType = TimeIntervals.GetData();
	int AggregationType = Aggregation.GetData();
	
	Color GraphColor = PlotColors.Next();
	
	//TODO: Do some initial trimming to get rid of NaNs at the head and tail of the data.
	ScatterDraw *Graph = nullptr;
	
	if(IntervalType == 0) //Daily values
	{
		//TODO: It may not always be that good of an idea to modify the
		//incoming data...
		if(NormalY)
		{
			for(size_t Idx = 0; Idx < Len; ++Idx)
			{
				if(MaxY > 0.0) Data[Idx] = Data[Idx] / MaxY;
				else Data[Idx] = 0.0;
			}
		}
		if(LogY)
		{
			for(size_t Idx = 0; Idx < Len; ++Idx)
			{
				Data[Idx] = std::log10(Data[Idx]);
			}
		}
		
		double Offset = (double)(StartDate - ReferenceDate);
		
		Graph = &Plot.AddSeries(Data, Timesteps, Offset, 1);
	}
	else //Monthly values or yearly values
	{
		//Would it help to reserve some size for these?
		std::vector<double> &XValues = PlotData.Allocate(0);
		std::vector<double> &YValues = PlotData.Allocate(0);
		
		AggregateData(ReferenceDate, StartDate, Timesteps, Data, IntervalType, AggregationType, XValues, YValues);
		
		Graph = &Plot.AddSeries(XValues.data(), YValues.data(), XValues.size());
	}
	
	if(Graph) //NOTE: Graph should never be nullptr, but in case we have a bug, let's not crash.
	{
		Graph->Legend(Legend);
		Graph->Units(Unit);
		if(Scatter)
		{
			Graph->MarkBorderColor(GraphColor).Stroke(0.0, GraphColor).Opacity(0.5);
			Graph->MarkStyle<CircleMarkPlot>();
			
			int Index = Plot.GetCount()-1;
			Plot.SetMarkColor(Index, Null); //NOTE: Calling Graph->MarkColor(Null) does not work as intended.
		}
		else
		{
			Graph->NoMark().Stroke(1.5, GraphColor).Dash("");
		}
	}
}

void PlotCtrl::AddQQPlot(String &ModUnit, String &ObsUnit, String &ModName, String &ObsName, timeseries_stats &ModeledStats, timeseries_stats &ObservedStats)
{
	std::vector<double> &XValues = PlotData.Allocate(NUM_PERCENTILES);
	std::vector<double> &YValues = PlotData.Allocate(NUM_PERCENTILES);
	
	QQLabels.Clear();
	
	for(size_t Idx = 0; Idx < NUM_PERCENTILES; ++Idx)
	{
		XValues[Idx] = ModeledStats.Percentiles[Idx];
		YValues[Idx] = ObservedStats.Percentiles[Idx];
		QQLabels << Format("%g", PERCENTILES[Idx]*100.0);
	}
	
	Color GraphColor = PlotColors.Next();
	Plot.AddSeries(XValues.data(), YValues.data(), XValues.size()).MarkColor(GraphColor).Stroke(0.0, GraphColor).Dash("").Units(ModUnit, ModUnit)
		.AddLabelSeries(QQLabels, 10, 0, StdFont().Height(15), ALIGN_CENTER);
		
	Plot.ShowLegend(false);
	
	Plot.SetLabels(ModName, ObsName); //TODO: This does not work!
}

void PlotCtrl::AddLine(String &Legend, double X0, double X1, double Y0, double Y1)
{
	std::vector<double> &XValues = PlotData.Allocate(2);
	std::vector<double> &YValues = PlotData.Allocate(2);
	
	XValues[0] = X0;
	XValues[1] = X1;
	YValues[0] = Y0;
	YValues[1] = Y1;
	
	Color GraphColor = PlotColors.Next();
	Plot.AddSeries(XValues.data(), YValues.data(), XValues.size()).NoMark().Legend(Legend).Stroke(1.5, GraphColor).Dash("6 3");
}

void PlotCtrl::AddTrendLine(String &Legend, size_t Timesteps, double XYCovar, double XVar, double YMean, double XMean, Date &ReferenceDate, Date &StartDate)
{
	double Offset = (double)(StartDate - ReferenceDate);
	
	double Beta = XYCovar / XVar;
	double Alpha = YMean - Beta*(XMean + Offset);
	
	double X0 = Offset;
	double X1 = Offset + (double)Timesteps;
	double Y0 = Alpha + X0*Beta;
	double Y1 = Alpha + X1*Beta;
	
	AddLine(Legend, X0, X1, Y0, Y1);
}

void PlotCtrl::AddPlotRecursive(std::string &Name, int Mode, std::vector<char *> &IndexSets,
	std::vector<std::string> &CurrentIndexes, int Level, uint64 Timesteps,
	Date &ReferenceDate, Date &StartDate)
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
		
		//TODO: Better way to do this: ?
		std::vector<double> &Data = PlotData.Allocate(Timesteps);
		
		String Unit;
		String Provided = "";
		
		if(Mode == 0)
		{
			Parent->ModelDll.GetResultSeries(Parent->DataSet, Name.data(), IndexData, Indexes.size(), Data.data());
			Unit = Parent->ModelDll.GetResultUnit(Parent->DataSet, Name.data());
		}
		else if(Mode == 1)
		{
			Parent->ModelDll.GetInputSeries(Parent->DataSet, Name.data(), IndexData, Indexes.size(), Data.data(), false);
			Unit = Parent->ModelDll.GetInputUnit(Parent->DataSet, Name.data());
			if(!Parent->ModelDll.InputWasProvided(Parent->DataSet, Name.data(), IndexData, Indexes.size()))
			{
				Provided = " (timeseries not provided)";
			}
		}
		if(Parent->CheckDllUserError()) return;
		
		double *Dat = Data.data();
		size_t Len = Data.size();
		
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
		Parent->ComputeTimeseriesStats(Stats, Dat, Len, StartDate);
		Parent->DisplayTimeseriesStats(Stats, Legend, Unit);
		
		bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get() && Mode == 1);
		bool LogY = (YAxisMode.IsEnabled() && YAxisMode.GetData() == 2);
		bool NormY = (YAxisMode.IsEnabled() && YAxisMode.GetData() == 1);
		AddPlot(Legend, Unit, Dat, Len, Scatter, LogY, NormY, ReferenceDate, StartDate, Stats.Min, Stats.Max);
		
		NullifyNans(Dat, Len);
	}
	else
	{
		char *IndexSetName = IndexSets[Level];
		size_t Id = Parent->IndexSetNameToId[IndexSetName];
		
		int RowCount = EIndexList[Id]->GetCount();
		
		//PromptOK(Format("Name: %s, Id: %d, RowCount: %d", IndexSetName, (int)Id, RowCount));
		
		for(int Row = 0; Row < RowCount; ++Row)
		{
			//PromptOK(EIndexList[Id]->Get(Row, 0).ToString());
			
			if(EIndexList[Id]->IsSelected(Row))
			{
				CurrentIndexes[Level] = EIndexList[Id]->Get(Row, 0).ToString().ToStd();
				AddPlotRecursive(Name, Mode, IndexSets, CurrentIndexes, Level + 1, Timesteps, ReferenceDate, StartDate);
			}
		}
	}
}

void PlotCtrl::RePlot()
{
	if(!Parent->EquationSelecter.IsSelection() && !Parent->InputSelecter.IsSelection()) return;

	bool MultiIndex = false;
	for(size_t IndexSet = 0; IndexSet < MAX_INDEX_SETS; ++IndexSet)
	{
		if(EIndexList[IndexSet]->GetSelectCount() > 1 && EIndexList[IndexSet]->IsEnabled())
		{
			MultiIndex = true;
			break;
		}
	}

	Plot.RemoveAllSeries(); //TODO: We could see if we want to cache some series and not re-extract everything every time.
	PlotData.Clear();
	PlotColors.Reset();
	
	Plot.ShowLegend(true);
	
	Plot.SetLabelX(" ");
	Plot.SetLabelY(" ");
	
	Parent->PlotInfo.Clear();
	
	int MajorMode = PlotMajorMode.GetData();
	
	Plot.SetTitle(String(""));
	
	char DateStr[256];
	Parent->ModelDll.GetStartDate(Parent->DataSet, DateStr);
	Date ResultStartDate;
	StrToDate(ResultStartDate, DateStr);

	Parent->ModelDll.GetInputStartDate(Parent->DataSet, DateStr);
	StrToDate(InputStartDate, DateStr);
	
	uint64 InputTimesteps = Parent->ModelDll.GetInputTimesteps(Parent->DataSet);
	uint64 ResultTimesteps = Parent->ModelDll.GetTimesteps(Parent->DataSet);
	
	if(PlotMajorMode == MajorMode_Regular)
	{
		int RowCount = Parent->InputSelecter.GetCount();
		
		for(int Row = 0; Row < RowCount; ++Row)
		{
			if(Parent->InputSelecter.IsSelected(Row))
			{
				std::string Name = Parent->InputSelecter.Get(Row, 0).ToString().ToStd();
				
				uint64 IndexSetCount = Parent->ModelDll.GetInputIndexSetsCount(Parent->DataSet, Name.data());
				std::vector<char *> IndexSets(IndexSetCount);
				Parent->ModelDll.GetInputIndexSets(Parent->DataSet, Name.data(), IndexSets.data());
				if(Parent->CheckDllUserError()) return;
				
				std::vector<std::string> CurrentIndexes(IndexSets.size());
				AddPlotRecursive(Name, 1, IndexSets, CurrentIndexes, 0, InputTimesteps, InputStartDate, InputStartDate);
				
				if(Parent->CheckDllUserError()) return;
			}
		}
		
		if(ResultTimesteps != 0) //Timesteps  == 0 if the model has not been run yet. In this case it should not be possible to select the equation though
		{
			int RowCount = Parent->EquationSelecter.GetCount();
			
			for(int Row = 0; Row < RowCount; ++Row)
			{
				if(Parent->EquationSelecter.IsSelected(Row))
				{
					std::string Name = Parent->EquationSelecter.Get(Row, 0).ToString().ToStd();
					
					uint64 IndexSetCount = Parent->ModelDll.GetResultIndexSetsCount(Parent->DataSet, Name.data());
					std::vector<char *> IndexSets(IndexSetCount);
					Parent->ModelDll.GetResultIndexSets(Parent->DataSet, Name.data(), IndexSets.data());
					if(Parent->CheckDllUserError()) return;
					
					std::vector<std::string> CurrentIndexes(IndexSets.size());
					AddPlotRecursive(Name, 0, IndexSets, CurrentIndexes, 0, ResultTimesteps, InputStartDate, ResultStartDate);
					
					if(Parent->CheckDllUserError()) return;
				}
			}
		}
	}
	else if(PlotMajorMode == MajorMode_Histogram)
	{
		int TimeseriesCount = Parent->EquationSelecter.GetSelectCount() + Parent->InputSelecter.GetSelectCount();
		
		
		if(TimeseriesCount > 1 || MultiIndex)
		{
			//TODO: Setting the title is a lousy way to provide an error message....
			Plot.SetTitle(String("In histogram mode you can only have one timeseries selected, for one index combination"));
		}
		else
		{
			std::vector<double> &Data = PlotData.Allocate(0);
			String Legend;
			String Unit;
		
			Date StartDate = ResultStartDate;
			if(Parent->EquationSelecter.IsSelection() && ResultTimesteps != 0)
			{
				Data.resize(ResultTimesteps);
				
				Parent->GetSingleSelectedResultSeries(Parent->DataSet, Legend, Unit, Data.data());
			}
			else if(Parent->InputSelecter.IsSelection())
			{
				Data.resize(InputTimesteps);
				Parent->GetSingleSelectedInputSeries(Parent->DataSet, Legend, Unit, Data.data(), false);
				StartDate = InputStartDate;
			}
			
			AddHistogram(Legend, Unit, Data.data(), Data.size());
			
			timeseries_stats Stats = {};
			Parent->ComputeTimeseriesStats(Stats, Data.data(), Data.size(), StartDate);
			Parent->DisplayTimeseriesStats(Stats, Legend, Unit);
		}
	}
	else if(PlotMajorMode == MajorMode_Profile)
	{
		int TimeseriesCount = Parent->EquationSelecter.GetSelectCount() + Parent->InputSelecter.GetSelectCount();
		
		size_t ProfileIndexSet;
		int NumberOfIndexSetsWithMultipleIndexesSelected = 0;
		for(size_t IndexSet = 0; IndexSet < MAX_INDEX_SETS; ++IndexSet)
		{
			if(EIndexList[IndexSet]->GetSelectCount() > 1 && EIndexList[IndexSet]->IsEnabled())
			{
				NumberOfIndexSetsWithMultipleIndexesSelected++;
				ProfileIndexSet = IndexSet;
			}
		}
		
		if(TimeseriesCount != 1 || NumberOfIndexSetsWithMultipleIndexesSelected != 1)
		{
			//TODO: Setting the title is a lousy way to provide an error message....
			Plot.SetTitle(String("In profile mode you can only have one timeseries selected, and exactly one index set must have multiple indexes selected"));
		}
		else
		{
			size_t IndexCount = EIndexList[ProfileIndexSet]->GetSelectCount();
			
			ProfileLabels.resize(IndexCount);
			
			int Mode = Parent->EquationSelecter.GetSelectCount() != 0 ? 0 : 1; //I.e. Mode = 0 if it was an equation that was selected, otherwise Mode = 1 if it was an input that was selected
			
			Date CurrentStartDate;
			size_t Timesteps;
			if(Mode == 0)
			{
				Timesteps = ResultTimesteps;
				CurrentStartDate = ResultStartDate;
			}
			else
			{
				Timesteps = InputTimesteps;
				CurrentStartDate = InputStartDate;
			}
			
			size_t IdxIdx = 0;
			int RowCount = EIndexList[ProfileIndexSet]->GetCount();
			
			int IntervalType = TimeIntervals.GetData();
			int AggregationType = Aggregation.GetData();
			
			for(int Row = 0; Row < RowCount; ++Row)
			{
				if(EIndexList[ProfileIndexSet]->IsSelected(Row))
				{
					std::vector<double> &Data = PlotData.Allocate(Timesteps);
					
					if(Mode == 0)
					{
						Parent->GetSingleResultSeries(Parent->DataSet, Data.data(), ProfileIndexSet, Row);
					}
					else
					{
						Parent->GetSingleInputSeries(Parent->DataSet, Data.data(), ProfileIndexSet, Row);
					}
					
					NullifyNans(Data.data(), Data.size());
					
					ProfileLabels[IdxIdx] = EIndexList[ProfileIndexSet]->Get(Row, 0).ToString();
					
					if(IntervalType == 1 || IntervalType == 2) //Monthly or yearly aggregation
					{
						std::vector<double> XValues; //TODO: Ugh, it is stupid to have to declare this when it is not going to be used.
						std::vector<double> YValues;
						
						AggregateData(CurrentStartDate, CurrentStartDate, Timesteps, Data.data(), IntervalType, AggregationType, XValues, YValues);
						
						Data = YValues; //Note: vector copy
					}
					
					++IdxIdx;
				}
			}
			
			if(Mode == 0)
			{
				ProfileLegend = Parent->EquationSelecter.Get(0).ToString();
				ProfileUnit = Parent->ModelDll.GetResultUnit(Parent->DataSet, ProfileLegend);
			}
			else
			{
				ProfileLegend = Parent->InputSelecter.Get(0).ToString();
				ProfileUnit = Parent->ModelDll.GetInputUnit(Parent->DataSet, ProfileLegend);
			}
			ProfileLegend << " profile";
			
			size_t DataLength = PlotData.Data[0].size();
			
			TimestepSlider.Range((int)DataLength - 1);
			
			//TODO: This should be based on the current position of the slider instead unless
			//we reset the slider!
			ProfileDisplayDate = CurrentStartDate;
			if(IntervalType == 1 || IntervalType == 2)
			{
				ProfileDisplayDate.day = 1;
				if(IntervalType == 2) ProfileDisplayDate.month = 1;
			}
			TimestepEdit.SetData(ProfileDisplayDate);
			
			ProfileIndexesCount = IndexCount;
			//std::assert(IndexCount == PlotData.Data.size());
			
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
			
			Plot.SetXYMin(0.0, 0.0);
			Plot.SetRange((double)IndexCount, YMax);
			Plot.SetMajorUnits(1.0);
			Plot.SetMinUnits(0.5);
			
			Plot.cbModifFormatX.Clear();
			Plot.cbModifFormatX <<
			[IndexCount, this](String &s, int i, double d)
			{
				int Idx = (int)d;
				if(d >= 0 && d < IndexCount) s = this->ProfileLabels[Idx];
			};
			
			TimestepSlider.Enable();
			ReplotProfile();
		}
	}
	else if(PlotMajorMode == MajorMode_CompareBaseline)
	{
		//TODO: Limit to one selected result series. Maybe also allow plotting a observation
		//comparison.
		if(Parent->EquationSelecter.GetSelectCount() > 1 || MultiIndex)
		{
			Plot.SetTitle(String("In baseline comparison mode you can only have one result series selected, for one index combination"));
		}
		else
		{
			bool LogY = (YAxisMode.IsEnabled() && YAxisMode.GetData() == 2);
			bool NormY = (YAxisMode.IsEnabled() && YAxisMode.GetData() == 1);
			
			
			uint64 BaselineTimesteps = Parent->ModelDll.GetTimesteps(Parent->BaselineDataSet);
			Parent->ModelDll.GetStartDate(Parent->BaselineDataSet, DateStr);
			Date BaselineStartDate;
			StrToDate(BaselineStartDate, DateStr);
			
			std::vector<double> &Baseline = PlotData.Allocate(BaselineTimesteps);
			String BS;
			String Unit;
			Parent->GetSingleSelectedResultSeries(Parent->BaselineDataSet, BS, Unit, Baseline.data());
			NullifyNans(Baseline.data(), Baseline.size());
			BS << " baseline";
			
			timeseries_stats Stats = {};
			Parent->ComputeTimeseriesStats(Stats, Baseline.data(), Baseline.size(), BaselineStartDate);
			Parent->DisplayTimeseriesStats(Stats, BS, Unit);
			
			AddPlot(BS, Unit, Baseline.data(), Baseline.size(), false, LogY, NormY, InputStartDate, BaselineStartDate, Stats.Min, Stats.Max);

			
			
			std::vector<double> &Current = PlotData.Allocate(ResultTimesteps);
			String CurrentLegend;
			Parent->GetSingleSelectedResultSeries(Parent->DataSet, CurrentLegend, Unit, Current.data());
			NullifyNans(Current.data(), Current.size());
			
			timeseries_stats Stats2 = {};
			Parent->ComputeTimeseriesStats(Stats2, Current.data(), Current.size(), ResultStartDate);
			Parent->DisplayTimeseriesStats(Stats2, CurrentLegend, Unit);
			
			AddPlot(CurrentLegend, Unit, Current.data(), Current.size(), false, LogY, NormY, InputStartDate, ResultStartDate, Stats2.Min, Stats2.Max);

			
			
			//TODO: Should we compute any residual statistics here?
			
			if(Parent->InputSelecter.GetSelectCount() == 1)
			{
				//TODO: Should we allow displaying multiple input series here? Probably no
				//reason to
				
				bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get());
				
				std::vector<double> &Obs = PlotData.Allocate(InputTimesteps);
				String InputLegend;
				String Unit;
				Parent->GetSingleSelectedInputSeries(Parent->DataSet, InputLegend, Unit, Obs.data(), false);
				NullifyNans(Obs.data(), Obs.size());
				
				timeseries_stats Stats3 = {};
				Parent->ComputeTimeseriesStats(Stats3, Obs.data(), Obs.size(), InputStartDate);
				Parent->DisplayTimeseriesStats(Stats3, InputLegend, Unit);
				
				AddPlot(InputLegend, Unit, Obs.data(), Obs.size(), Scatter, LogY, NormY, InputStartDate, InputStartDate, Stats3.Min, Stats3.Max);
			}
		}
		
		
	}
	else if(PlotMajorMode == MajorMode_Residuals || PlotMajorMode == MajorMode_ResidualHistogram || PlotMajorMode == MajorMode_QQ)
	{
		
		if(Parent->EquationSelecter.GetSelectCount() != 1 || Parent->InputSelecter.GetSelectCount() != 1 || MultiIndex)
		{
			//TODO: Setting the title is a lousy way to provide an error message....
			Plot.SetTitle(String("In residual mode you must select exactly 1 result series and 1 input series, for one index combination only"));
		}
		else
		{
			std::vector<double> &Residuals = PlotData.Allocate(ResultTimesteps);
			std::vector<double> ModeledSeries(ResultTimesteps);
			std::vector<double> ObservedSeries(ResultTimesteps);
			String ModeledLegend;
			String ObservedLegend;
			
			//These should be the same, but just in case..
			String ModUnit;
			String ObsUnit;
			
			Parent->GetSingleSelectedResultSeries(Parent->DataSet, ModeledLegend, ModUnit, ModeledSeries.data());
			Parent->GetSingleSelectedInputSeries(Parent->DataSet, ObservedLegend, ObsUnit, ObservedSeries.data(), true);
			
			for(size_t Idx = 0; Idx < ResultTimesteps; ++Idx)
			{
				Residuals[Idx] = ObservedSeries[Idx] - ModeledSeries[Idx];
			}
			
			String Legend = String("Residuals of ") + ModeledLegend + " vs " + ObservedLegend;
			
			timeseries_stats ModeledStats = {};
			Parent->ComputeTimeseriesStats(ModeledStats, ModeledSeries.data(), ModeledSeries.size(), ResultStartDate);
			Parent->DisplayTimeseriesStats(ModeledStats, ModeledLegend, ModUnit);
			
			timeseries_stats ObservedStats = {};
			Parent->ComputeTimeseriesStats(ObservedStats, ObservedSeries.data(), ObservedSeries.size(), ResultStartDate);
			Parent->DisplayTimeseriesStats(ObservedStats, ObservedLegend, ObsUnit);
			
			residual_stats ResidualStats = {};
			Parent->ComputeResidualStats(ResidualStats, ObservedSeries.data(), ModeledSeries.data(), ObservedSeries.size(), ResultStartDate);
			String GOF = "Goodness of fit: ";
			Parent->DisplayResidualStats(ResidualStats, GOF);
			
			if(PlotMajorMode == MajorMode_Residuals)
			{
				bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get());
				
				double XMean, XVar, XYCovar;
				Parent->ComputeTrendStats(Residuals.data(), Residuals.size(), ResidualStats.MeanError, XMean, XVar, XYCovar);
				
				//NOTE: Using the input start date as reference date is just so that we agree with the date formatting below.
				AddPlot(Legend, ModUnit, Residuals.data(), ResultTimesteps, Scatter, false, false, InputStartDate, ResultStartDate);
				
				NullifyNans(Residuals.data(), Residuals.size());
				
				String TL = "Trend line";
				AddTrendLine(TL, Residuals.size(), XYCovar, XVar, ResidualStats.MeanError, XMean, InputStartDate, ResultStartDate);
				
			}
			else if(PlotMajorMode == MajorMode_ResidualHistogram)
			{
				int NBins = AddHistogram(Legend, ModUnit, Residuals.data(), Residuals.size());
				String NormLegend = "Normal distr.";
				
				timeseries_stats RS2;
				Parent->ComputeTimeseriesStats(RS2, Residuals.data(), Residuals.size(), ResultStartDate);
				
				AddNormalApproximation(NormLegend, NBins, RS2.Min, RS2.Max, RS2.Mean, RS2.StandardDeviation);
			}
			else if(PlotMajorMode == MajorMode_QQ)
			{
				AddQQPlot(ModUnit, ObsUnit, ModeledLegend, ObservedLegend, ModeledStats, ObservedStats);
				
				double Min = std::min(ModeledStats.Percentiles[0], ObservedStats.Percentiles[0]);
				double Max = std::max(ModeledStats.Percentiles[NUM_PERCENTILES-1], ObservedStats.Percentiles[NUM_PERCENTILES-1]);
				
				String Dum = "";
				AddLine(Dum, Min, Max, Min, Max);
			}
		}
	}
	
	Plot.SetGridLinesX.Clear();
	
	if(PlotMajorMode == MajorMode_Histogram || PlotMajorMode == MajorMode_ResidualHistogram)
	{
		Plot.cbModifFormatX.Clear();
		//NOTE: Histograms require completely different zooming.
		Plot.ZoomToFit(true, true);
		PlotWasAutoResized = false;
	}
	else if(PlotMajorMode == MajorMode_QQ)
	{
		Plot.cbModifFormatX.Clear();
		//NOTE: Histograms require completely different zooming.
		Plot.ZoomToFit(true, true);
		PlotWasAutoResized = false;
		
		double YRange = Plot.GetYRange();
		double YMin   = Plot.GetYMin();
		double XRange = Plot.GetXRange();
		double XMin   = Plot.GetXMin();
		
		//NOTE: Make it so that the extremal points are not on the border
		double ExtendY = YRange * 0.1;
		YRange += 2.0 * ExtendY;
		YMin -= ExtendY;
		double ExtendX = XRange * 0.1;
		XRange += 2.0 * ExtendX;
		XMin -= ExtendX;
		
		Plot.SetRange(XRange, YRange);
		Plot.SetXYMin(XMin, YMin);
	}
	else if(PlotMajorMode == MajorMode_Profile)
	{
		PlotWasAutoResized = false;
	}
	else
	{
		Plot.cbModifFormatX.Clear();
		Plot.ZoomToFit(false, true);
		
		Plot.SetGridLinesX = THISBACK(UpdateDateGridLinesX);
		
		double YRange = Plot.GetYRange();
		double YMin   = Plot.GetYMin();
		if(YMin > 0.0)
		{
			double NewRange = YRange + YMin;
			Plot.SetRange(Plot.GetXRange(), NewRange);
			Plot.SetXYMin(Plot.GetXMin(), 0.0);
		}
		
		if(!PlotWasAutoResized)
		{
			Plot.ZoomToFit(true, false);
			PlotWasAutoResized = true;
		}
		
		bool MonthlyOrYearly = false;
		if(TimeIntervals.IsEnabled())
		{
			int IntervalType = TimeIntervals.GetData();
			if(IntervalType == 1)
			{
				MonthlyOrYearly = true;
				
				Plot.cbModifFormatX << [this](String &s, int i, double d)
				{
					Date D2 = this->InputStartDate + (int)(d + 0.5); //NOTE: The +0.5 is to avoid flickering when panning
					s << Format("%d-%02d", D2.year, D2.month);
				};
			}
			else if(IntervalType == 2)
			{
				MonthlyOrYearly = true;
				
				Plot.cbModifFormatX << [this](String &s, int i, double d)
				{
					Date D2 = this->InputStartDate + (int)(d + 0.5); //NOTE: The +0.5 is to avoid flickering when panning
					s = Format("%d", D2.year);
				};
			}
		}
		
		if(!MonthlyOrYearly)
		{
			Plot.cbModifFormatX << [this](String &s, int i, double d)
			{
				Date D2 = this->InputStartDate + (int)(d + 0.5); //NOTE: The +0.5 is to avoid flickering when panning
				s = Format(D2);
			};
		}
	}
	
	if(Plot.GetShowLegend())
	{
		Plot.SetRange(Plot.GetXRange(), Plot.GetYRange() * 1.15);  //So that the legend does not obscure the plot (in most cases).
	}
	
	Plot.cbModifFormatY.Clear();
	if(YAxisMode.IsEnabled() && YAxisMode.GetData() == 2) //If we want a logarithmic Y axis
	{
		Plot.cbModifFormatY << [](String &s, int i, double d)
		{
			s = FormatDoubleExp(pow(10., d), 2);
		};
	}
	
	SetBetterGridLinePositions();
	
	Size PlotSize = Plot.GetSize();
	Plot.SetSaveSize(PlotSize); //TODO: This then breaks if somebody resizes the window in between....
	
	
	
	Plot.Refresh();
}


void PlotCtrl::SetBetterGridLinePositions()
{
	//TODO: For Q-Q plot we actually want to do this same thing for the X axis.
	
	//double XMin = Plot.GetXMin();
	double YMin = Plot.GetYMin();
	//double XRange = Plot.GetXRange();
	double YRange = Plot.GetYRange();
	
	double LogYRange = std::log10(YRange);
	double IntPart;
	double FracPart = std::modf(LogYRange, &IntPart);
	if(YRange < 1.0) IntPart -= 1.0;
	double YOrderOfMagnitude = std::pow(10.0, IntPart);
	double YStretch = YRange / YOrderOfMagnitude;
	
	double Stride = YOrderOfMagnitude * 0.1;
	if(YStretch >= 2.0) Stride = YOrderOfMagnitude * 0.2;
	if(YStretch >= 2.5) Stride = YOrderOfMagnitude * 0.25;
	if(YStretch >= 5.0) Stride = YOrderOfMagnitude * 0.5;
	
	double Min = std::floor(YMin / Stride)*Stride;
	int Count = (int)std::ceil(YRange / Stride);
	
	//Plot.SetMinUnits(Null, Min);  //We would prefer to do this, but for some reason it works
	//poorly when there are negative values...
	Plot.SetXYMin(Null, Min);
	
	Plot.SetMajorUnits(Null, Stride);
	//Plot.SetMajorUnitsNum(Null, Count);
	
	
	//String Debug = "Range: "; Debug << YRange << " Stretch: " << YStretch << " Min: " << Min << " Stride: " << Stride << " Count: " << Count; PromptOK(Debug);
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


void PlotCtrl::UpdateDateGridLinesX(Vector<double> &LinesOut)
{
	//InputStartDate is X=0. X resolution is daily
	
	int DesiredGridLineNum = 10;  //TODO: Make it sensitive to plot pixel size
	
	double XMin = Plot.GetXMin();
	double XRange = Plot.GetXRange();
	
	Date FirstDate = InputStartDate + (int)XMin;
	Date LastDate = FirstDate + (int)XRange;
	
	int DoStep = 0;
	
	int IntervalType = TimeIntervals.GetData();
	if(!TimeIntervals.IsEnabled() || IntervalType == 0)
	{
		int DayRange = LastDate - FirstDate;
		int DayStep = DayRange / DesiredGridLineNum + 1;
		
		if(DayStep >= 20) DoStep = 1; //Monthly;
		else
		{
			DoStep = 0;
			
			if(DayStep == 3) DayStep = 2;
			else if(DayStep == 4) DayStep = 5;
			else if(DayStep == 6 || DayStep == 7) DayStep = 5;
			else if(DayStep == 8 || DayStep == 9) DayStep = 10;
			else if(DayStep > 2 && DayStep < 20) DayStep = 15;
			
			Date IterDate = FirstDate;
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
				double XVal = (double)((IterDate - InputStartDate)) - XMin;
				if(XVal > 0 && XVal < XRange)
					LinesOut << XVal;
			}
			
			return;
		}
	}
	
	if(DoStep == 1 || IntervalType == 1)
	{
		int MonthRange = (int)LastDate.month - (int)FirstDate.month + 12*(LastDate.year - FirstDate.year);
		int MonthStep = MonthRange / DesiredGridLineNum + 1;
		
		if(MonthStep >= 10) DoStep = 2; //Yearly
		else
		{
			DoStep = 1; //Monthly
			
			if(MonthStep == 4) MonthStep = 3;
			else if(MonthStep == 5 || (MonthStep > 6 && MonthStep <= 9)) MonthStep = 6;
			
			int FirstMonth = (int)FirstDate.month;
			int Surp = ((FirstMonth-1) % MonthStep);
			MonthRange += Surp;
			FirstMonth -= Surp; //NOTE: This should not result in FirstMonth being negative.
			Date IterDate(FirstDate.year, (byte)FirstMonth, 1);
			for(int Month = 0; Month <= MonthRange; Month += MonthStep)
			{
				IterDate.month += MonthStep;
				if(IterDate.month > 12)
				{
					IterDate.month -= 12;
					IterDate.year++;
				}
				double XVal = (double)((IterDate - InputStartDate)) - XMin;
				if(XVal > 0 && XVal < XRange)
					LinesOut << XVal;
			}
			return;
		}
		
	}
	
	if(DoStep == 2 || IntervalType == 2) //Yearly
	{
		int FirstYear = FirstDate.year;
		int LastYear = LastDate.year;
		
		int YearCount = LastYear - FirstYear;
		int YearStep = YearCount / DesiredGridLineNum + 1;
		YearStep = RoundStep(YearStep);
		
		FirstYear -= (FirstYear % YearStep);
		
		Date IterDate(FirstYear, 1, 1);
		for(int Year = FirstYear; Year <= LastYear; Year += YearStep)
		{
			IterDate.year = Year;
			double XVal = (double)((IterDate - InputStartDate)) - XMin;
			if(XVal > 0 && XVal < XRange)
				LinesOut << XVal;
		}
	}
}


void PlotCtrl::TimestepSliderEvent()
{
	int Timestep = TimestepSlider.GetData();
	Date NewDate = ProfileDisplayDate;
	
	// Recompute the displayed date in the "TimestepEdit" based on the new position of the
	// slider.
	int IntervalType = TimeIntervals.GetData();
	if(IntervalType == 0) // Daily values
	{
		NewDate += Timestep;
	}
	else if(IntervalType == 1) // Monthly values
	{
		int Month = ((NewDate.month + Timestep - 1) % 12) + 1;
		int YearAdd = (NewDate.month + Timestep - 1) / 12;
		NewDate.month = Month;
		NewDate.year += YearAdd;
	}
	else if(IntervalType == 2) // Yearly values
	{
		NewDate.year += Timestep;
	}
	TimestepEdit.SetData(NewDate);
	
	
	ReplotProfile();
}

void PlotCtrl::TimestepEditEvent()
{
	//NOTE: This can only happen if we are in daily mode since the edit is disabled otherwise.
	//If this changes, this code has to be rethought.
	
	Date CurrentDate = TimestepEdit.GetData();
	int Timestep = CurrentDate - ProfileDisplayDate;
	TimestepSlider.SetData(Timestep);
	ReplotProfile();
}


void PlotCtrl::ReplotProfile()
{
	Plot.RemoveAllSeries();
	
	int Timestep = TimestepSlider.GetData();
	
	std::vector<double> &XValues = PlotData.Data[ProfileIndexesCount];
	std::vector<double> &YValues = PlotData.Data[ProfileIndexesCount+1];
	
	for(size_t Idx = 0; Idx < ProfileIndexesCount; ++Idx)
	{
		YValues[Idx] = PlotData.Data[Idx][Timestep];
	}
	
	Color &GraphColor = PlotColors.PlotColors[0];
	double Darken = 0.4;
	Color BorderColor((int)(((double)GraphColor.GetR())*Darken), (int)(((double)GraphColor.GetG())*Darken), (int)(((double)GraphColor.GetB())*Darken));
	Plot.AddSeries(XValues.data(), YValues.data(), XValues.size()).Legend(ProfileLegend).PlotStyle<BarSeriesPlot>().BarWidth(0.5).NoMark().Fill(GraphColor).Stroke(1.0, BorderColor).Units(ProfileUnit);
	
	Plot.SetLabelX(" ");
	Plot.SetLabelY(" ");
	
	Plot.Refresh();
}

void PlotCtrl::NullifyNans(double *Data, size_t Len)
{
	//NOTE: We do this because the ScatterDraw does not draw gaps in the line at NaNs, only at
	//Null.
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		if(std::isnan(Data[Idx])) Data[Idx] = Null;
	}
}






void MobiView::GetSingleResultSeries(void *DataSet, double *WriteTo, size_t SelectRowFor, int Row)
{
	std::string Name = EquationSelecter.Get(0).ToString().ToStd();
	
	uint64 IndexSetCount = ModelDll.GetResultIndexSetsCount(DataSet, Name.data());
	std::vector<char *> IndexSets(IndexSetCount);
	ModelDll.GetResultIndexSets(DataSet, Name.data(), IndexSets.data());
	if(CheckDllUserError()) return;
	
	std::vector<std::string> Indexes_String(IndexSets.size());
	std::vector<char *> Indexes(IndexSets.size());
	for(size_t Idx = 0; Idx < IndexSets.size(); ++Idx)
	{
		const char *IndexSet = IndexSets[Idx];
		size_t Id = IndexSetNameToId[IndexSet];
		
		if(Id == SelectRowFor)
		{
			Indexes_String[Idx] = Plotter.EIndexList[Id]->Get(Row, 0).ToString().ToStd();
		}
		else
		{
			Indexes_String[Idx] = Plotter.EIndexList[Id]->Get(0).ToString().ToStd();
		}
		Indexes[Idx] = (char *)Indexes_String[Idx].data();
	}
	
	ModelDll.GetResultSeries(DataSet, Name.data(), Indexes.data(), Indexes.size(), WriteTo);
	CheckDllUserError();
}


void MobiView::GetSingleInputSeries(void *DataSet, double *WriteTo, size_t SelectRowFor, int Row)
{
	std::string Name = InputSelecter.Get(0).ToString().ToStd();
	
	uint64 IndexSetCount = ModelDll.GetInputIndexSetsCount(DataSet, Name.data());
	std::vector<char *> IndexSets(IndexSetCount);
	ModelDll.GetInputIndexSets(DataSet, Name.data(), IndexSets.data());
	if(CheckDllUserError()) return;
	
	std::vector<std::string> Indexes_String(IndexSets.size());
	std::vector<char *> Indexes(IndexSets.size());
	for(size_t Idx = 0; Idx < IndexSets.size(); ++Idx)
	{
		const char *IndexSet = IndexSets[Idx];
		size_t Id = IndexSetNameToId[IndexSet];
		
		if(Id == SelectRowFor)
		{
			Indexes_String[Idx] = Plotter.EIndexList[Id]->Get(Row, 0).ToString().ToStd();
		}
		else
		{
			Indexes_String[Idx] = Plotter.EIndexList[Id]->Get(0).ToString().ToStd();
		}
		Indexes[Idx] = (char *)Indexes_String[Idx].data();
	}
	
	ModelDll.GetInputSeries(DataSet, Name.data(), Indexes.data(), Indexes.size(), WriteTo, false);
	CheckDllUserError();
}


void MobiView::GetSingleSelectedResultSeries(void *DataSet, String &Legend, String &Unit, double *WriteTo)
{
	std::string Name = EquationSelecter.Get(0).ToString().ToStd();
				
	uint64 IndexSetCount = ModelDll.GetResultIndexSetsCount(DataSet, Name.data());
	std::vector<char *> IndexSets(IndexSetCount);
	ModelDll.GetResultIndexSets(DataSet, Name.data(), IndexSets.data());
	Unit = ModelDll.GetResultUnit(DataSet, Name.data());
	if(CheckDllUserError()) return;
	
	std::vector<std::string> Indexes_String(IndexSets.size());
	std::vector<char *> Indexes(IndexSets.size());
	for(size_t Idx = 0; Idx < IndexSets.size(); ++Idx)
	{
		const char *IndexSet = IndexSets[Idx];
		size_t Id = IndexSetNameToId[IndexSet];
		
		Indexes_String[Idx] = Plotter.EIndexList[Id]->Get(0).ToString().ToStd();
		Indexes[Idx] = (char *)Indexes_String[Idx].data();
	}
	
	ModelDll.GetResultSeries(DataSet, Name.data(), Indexes.data(), Indexes.size(), WriteTo);
	CheckDllUserError();
	
	Legend << Name.data();
	if(Indexes.size() > 0)
	{
		Legend << " (";
		for(size_t Idx = 0; Idx < Indexes.size(); ++Idx)
		{
			Legend << Indexes[Idx];
			if(Idx < Indexes.size()-1) Legend << ", ";
		}
		Legend << ")";
	}
}

void MobiView::GetSingleSelectedInputSeries(void *DataSet, String &Legend, String &Unit, double *WriteTo, bool AlignWithResults)
{
	std::string Name = InputSelecter.Get(0).ToString().ToStd();
				
	uint64 IndexSetCount = ModelDll.GetInputIndexSetsCount(DataSet, Name.data());
	std::vector<char *> IndexSets(IndexSetCount);
	ModelDll.GetInputIndexSets(DataSet, Name.data(), IndexSets.data());
	Unit = ModelDll.GetInputUnit(DataSet, Name.data());
	if(CheckDllUserError()) return;
	
	std::vector<std::string> Indexes_String(IndexSets.size());
	std::vector<char *> Indexes(IndexSets.size());
	for(size_t Idx = 0; Idx < IndexSets.size(); ++Idx)
	{
		const char *IndexSet = IndexSets[Idx];
		size_t Id = IndexSetNameToId[IndexSet];
		
		Indexes_String[Idx] = Plotter.EIndexList[Id]->Get(0).ToString().ToStd();
		Indexes[Idx] = (char *)Indexes_String[Idx].data();
	}
	
	ModelDll.GetInputSeries(DataSet, Name.data(), Indexes.data(), Indexes.size(), WriteTo, AlignWithResults);
	CheckDllUserError();
	
	Legend << Name.data();
	if(Indexes.size() > 0)
	{
		Legend << " (";
		for(size_t Idx = 0; Idx < Indexes.size(); ++Idx)
		{
			Legend << Indexes[Idx];
			if(Idx < Indexes.size()-1) Legend << ", ";
		}
		Legend << ")";
	}
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
		if(EquationSelecter.IsSelected(Row))
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
	//TODO: Remember the last folder..
	Sel.ExecuteSaveAs();
	std::string FileName = Sel.Get().ToStd();
	if(FileName.empty())
	{
		return;
	}
	
	
	
	std::ofstream File;
	File.open(FileName.data());
	
	if(!File.is_open())
	{
		Log(String("Unable to open file") + FileName);
		return;
	}
	
	for(int Idx = 0; Idx < Names.size(); ++Idx)
	{
		File << Names[Idx];
		if(Idx != Names.size()-1) File << ";";
	}
	File << "\n";
	
	for(uint64 Timestep = 0; Timestep < Timesteps; ++Timestep)
	{
		for(int Idx = 0; Idx < Data.size(); ++Idx)
		{
			File << Data[Idx][Timestep];
			if(Idx != Data.size()-1) File << ";";
		}
		File << "\n";
	}
	
	File.close();
	
	Log(String("Results exported to " + FileName));
}





#endif
