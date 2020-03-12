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
	
	Size PlotReticleSize = GetTextSize("00000", Plot.GetReticleFont());
	Size PlotUnitSize    = GetTextSize("[dummy]", Plot.GetLabelsFont());
	Plot.SetPlotAreaLeftMargin(PlotReticleSize.cx + PlotUnitSize.cy + 20);
	Plot.SetPlotAreaBottomMargin(PlotReticleSize.cy + PlotUnitSize.cy + 20);
	
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


void AdvanceTimesteps(Time &T, uint64 Timesteps, timestep_size TimestepSize)
{
	if(TimestepSize.Type == 0)  //Timestep magnitude measured in seconds.
	{
		T += ((int64)Timesteps)*((int64)TimestepSize.Magnitude);
	}
	else                        //Timestep magnitude measured in months.
	{
		T.month += (int)Timesteps*TimestepSize.Magnitude;
		while(T.month > 12)
		{
			T.month -= 12;
			T.year++;
		}
	}
}

int64 TimestepsBetween(Time &T1, Time &T2, timestep_size TimestepSize)
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

int PlotCtrl::GetSmallestStepResolution()
{
	//NOTE: To compute the unit that we try to use for spacing the grid lines. 0=seconds, 1=minutes, 2=hours, 3=days,
	//4=months, 5=years

	int IntervalType = TimeIntervals.GetData();
	if(!TimeIntervals.IsEnabled() || IntervalType == 0)       //IntervalType==0 signifies no aggregation
	{
		//NOTE: The plot does not display aggregated data, so the unit of the grid line step should be
		//determined by the model step size.
		if(Parent->TimestepSize.Type == 0)          //Timestep size is measured in seconds
		{
			if(Parent->TimestepSize.Magnitude < 60)         return 0;
			else if(Parent->TimestepSize.Magnitude < 3600)  return 1;
			else if(Parent->TimestepSize.Magnitude < 86400) return 2;
			else                                            return 3;
		}
		else                                        //Timestep size is measured in months
		{
			if(Parent->TimestepSize.Magnitude < 12) return 4;
			else                                    return 5;
		}
	}
	else if(IntervalType == 1) return 4;
	else if(IntervalType == 2) return 5;
	
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
			
			CurTime.month += TimestepSize.Magnitude;
			while(CurTime.month > 12)
			{
				CurTime.month -= 12;
				CurTime.year++;
			}
		}
	}
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
	
	Plot.SetMouseHandling(true, false);
	if(MajorMode == MajorMode_Profile || MajorMode == MajorMode_Histogram || MajorMode == MajorMode_ResidualHistogram)
	{
		Plot.SetMouseHandling(false, false);
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
	
	
	if(MajorMode == MajorMode_Residuals || MajorMode == MajorMode_ResidualHistogram || MajorMode == MajorMode_QQ)
	{
		Parent->PlotInfo.HSizePos().VSizePos(0, 25);
		Parent->CalibrationIntervalStart.Show();
		Parent->CalibrationIntervalEnd.Show();
		Parent->CalibrationIntervalLabel.Show();
		
		Time StartTime = Parent->CalibrationIntervalStart.GetData();
		Time EndTime   = Parent->CalibrationIntervalStart.GetData();
		
		if(IsNull(StartTime))
		{
			char TimeVal[256];
			Parent->ModelDll.GetParameterTime(Parent->DataSet, "Start date", nullptr, 0, TimeVal);
			StrToTime(StartTime, TimeVal);
			Parent->CalibrationIntervalStart.SetData(StartTime);
		}
		if(IsNull(EndTime))
		{
			EndTime = StartTime;
			uint64 Timesteps = Parent->ModelDll.GetParameterUInt(Parent->DataSet, "Timesteps", nullptr, 0);
			if(Timesteps != 0)
				AdvanceTimesteps(EndTime, Timesteps - 1, Parent->TimestepSize);
			Parent->CalibrationIntervalEnd.SetData(EndTime);
		}
	}
	else
	{
		Parent->PlotInfo.SizePos();
		Parent->CalibrationIntervalStart.Hide();
		Parent->CalibrationIntervalEnd.Hide();
		Parent->CalibrationIntervalLabel.Hide();
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
			if(Parent->EquationSelecter.IsSelected(Row) && Parent->EquationSelecter.GetCtrl(Row, 1) != nullptr)
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


void PlotCtrl::AggregateData(Time &ReferenceTime, Time &StartTime, uint64 Timesteps, double *Data, int IntervalType, int AggregationType, std::vector<double> &XValues, std::vector<double> &YValues)
{
	double CurrentAggregate = 0.0;
	int    CurrentCount = 0;
	Time CurrentTime = StartTime;
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
		Time NextTime = CurrentTime;
		AdvanceTimesteps(NextTime, 1, Parent->TimestepSize);
		
		//TODO: Want more aggregation interval types than year or month for models with
		//non-daily resolutions
		bool PushAggregate = (CurrentTimestep == Timesteps-1) || (NextTime.year != CurrentTime.year);
		if(IntervalType == 1)
		{
			PushAggregate = PushAggregate || (NextTime.month != CurrentTime.month);
		}
		if(PushAggregate)
		{
			double YVal = CurrentAggregate;
			if(AggregationType == 0) YVal /= (double)CurrentCount; //Aggregation type is 'mean', otherwise it is 'sum', so we don't have to modify it.
			
			if(!std::isfinite(YVal)) YVal = Null; //So that plots show it as empty instead of a line going to infinity.
			
			YValues.push_back(YVal);
			
			//TODO: Clean this up eventually:
			Time Beginning = CurrentTime;
			Beginning.second = 0;
			Beginning.minute = 0;
			Beginning.hour   = 0;
			Beginning.day    = 1;
			if(IntervalType == 2) Beginning.month = 1;
			double XVal = (double)(Beginning - ReferenceTime);
			
			XValues.push_back(XVal);
			
			CurrentAggregate = 0.0;
			CurrentCount = 0;
		}
		
		CurrentTime = NextTime;
	}
}


void PlotCtrl::AddPlot(String &Legend, String &Unit, double *XIn, double *Data, size_t Len, bool Scatter, bool LogY, bool NormalY, Time &ReferenceTime, Time &StartTime, double MinY, double MaxY)
{
	int Timesteps = (int)Len;
	
	int IntervalType = TimeIntervals.GetData();
	int AggregationType = Aggregation.GetData();
	
	Color GraphColor = PlotColors.Next();
	
	ScatterDraw *Graph = nullptr;
	
	if(IntervalType == 0) //Daily values
	{
		//TODO: It may not always be that good of an idea to modify the
		//incoming data... Though it does not cause a problem now..
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
		
		double Offset = (double)(StartTime - ReferenceTime);
		
		Graph = &Plot.AddSeries(XIn, Data, Len);
	}
	else //Monthly values or yearly values
	{
		//Would it help to reserve some size for these?
		std::vector<double> &XValues = PlotData.Allocate(0);
		std::vector<double> &YValues = PlotData.Allocate(0);
		
		AggregateData(ReferenceTime, StartTime, Timesteps, Data, IntervalType, AggregationType, XValues, YValues);
		
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
			Plot.SetMarkColor(Index, Null); //NOTE: Calling Graph->MarkColor(Null) does not make it transparent, so we have to do it like this.
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

void PlotCtrl::AddLine(const String &Legend, double X0, double X1, double Y0, double Y1, Color GraphColor)
{
	std::vector<double> &XValues = PlotData.Allocate(2);
	std::vector<double> &YValues = PlotData.Allocate(2);
	
	XValues[0] = X0;
	XValues[1] = X1;
	YValues[0] = Y0;
	YValues[1] = Y1;
	
	if(IsNull(GraphColor)) GraphColor = PlotColors.Next();
	auto& Graph = Plot.AddSeries(XValues.data(), YValues.data(), XValues.size()).NoMark().Stroke(1.5, GraphColor).Dash("6 3");
	if(!IsNull(Legend)) Graph.Legend(Legend);
	else Graph.ShowSeriesLegend(false);
}

void PlotCtrl::AddTrendLine(String &Legend, double XYCovar, double XVar, double YMean, double XMean, double StartX, double EndX)
{
	double Beta = XYCovar / XVar;
	double Alpha = YMean - Beta*XMean;
	
	double X0 = StartX;
	double X1 = EndX;
	double Y0 = Alpha + X0*Beta;
	double Y1 = Alpha + X1*Beta;
	
	AddLine(Legend, X0, X1, Y0, Y1);
}

void PlotCtrl::AddPlotRecursive(std::string &Name, int Mode, std::vector<char *> &IndexSets,
	std::vector<std::string> &CurrentIndexes, int Level, uint64 Timesteps,
	Time &ReferenceDate, Time &StartDate, double *XIn)
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
		Parent->ComputeTimeseriesStats(Stats, Dat, Len);
		Parent->DisplayTimeseriesStats(Stats, Legend, Unit);
		
		bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get() && Mode == 1);
		bool LogY = (YAxisMode.IsEnabled() && YAxisMode.GetData() == 2);
		bool NormY = (YAxisMode.IsEnabled() && YAxisMode.GetData() == 1);
		AddPlot(Legend, Unit, XIn, Dat, Len, Scatter, LogY, NormY, ReferenceDate, StartDate, Stats.Min, Stats.Max);
		
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
				AddPlotRecursive(Name, Mode, IndexSets, CurrentIndexes, Level + 1, Timesteps, ReferenceDate, StartDate, XIn);
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
	
	char TimeStr[256];
	Parent->ModelDll.GetStartDate(Parent->DataSet, TimeStr);
	Time ResultStartTime;
	StrToTime(ResultStartTime, TimeStr);

	Parent->ModelDll.GetInputStartDate(Parent->DataSet, TimeStr);
	StrToTime(InputStartTime, TimeStr);
	
	//PromptOK(Format(InputStartTime,true));
	
	uint64 InputTimesteps = Parent->ModelDll.GetInputTimesteps(Parent->DataSet);
	uint64 ResultTimesteps = Parent->ModelDll.GetTimesteps(Parent->DataSet);
	
	int NBinsHistogram = 0;
	
	
	if(PlotMajorMode == MajorMode_Regular)
	{
		double *InputXValues = PlotData.Allocate(InputTimesteps).data();
		ComputeXValues(InputStartTime, InputStartTime, InputTimesteps, Parent->TimestepSize, InputXValues);
		
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
				AddPlotRecursive(Name, 1, IndexSets, CurrentIndexes, 0, InputTimesteps, InputStartTime, InputStartTime, InputXValues);
				
				if(Parent->CheckDllUserError()) return;
			}
		}
		
		if(ResultTimesteps != 0) //Timesteps  == 0 if the model has not been run yet. In this case it should not be possible to select the equation though
		{
			double *ResultXValues = PlotData.Allocate(ResultTimesteps).data();
			ComputeXValues(InputStartTime, ResultStartTime, ResultTimesteps, Parent->TimestepSize, ResultXValues);
			
			int RowCount = Parent->EquationSelecter.GetCount();
			
			for(int Row = 0; Row < RowCount; ++Row)
			{
				if(Parent->EquationSelecter.IsSelected(Row) && Parent->EquationSelecter.GetCtrl(Row, 1) != nullptr)
				{
					std::string Name = Parent->EquationSelecter.Get(Row, 0).ToString().ToStd();
					
					uint64 IndexSetCount = Parent->ModelDll.GetResultIndexSetsCount(Parent->DataSet, Name.data());
					std::vector<char *> IndexSets(IndexSetCount);
					Parent->ModelDll.GetResultIndexSets(Parent->DataSet, Name.data(), IndexSets.data());
					if(Parent->CheckDllUserError()) return;
					
					std::vector<std::string> CurrentIndexes(IndexSets.size());
					AddPlotRecursive(Name, 0, IndexSets, CurrentIndexes, 0, ResultTimesteps, InputStartTime, ResultStartTime, ResultXValues);
					
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
		
			//Time StartTime = ResultStartTime;
			if(Parent->EquationSelecter.IsSelection() && ResultTimesteps != 0)
			{
				Data.resize(ResultTimesteps);
				
				Parent->GetSingleSelectedResultSeries(Parent->DataSet, Legend, Unit, Data.data());
			}
			else if(Parent->InputSelecter.IsSelection())
			{
				Data.resize(InputTimesteps);
				Parent->GetSingleSelectedInputSeries(Parent->DataSet, Legend, Unit, Data.data(), false);
				//StartTime = InputStartTime;
			}
			
			NBinsHistogram = AddHistogram(Legend, Unit, Data.data(), Data.size());
			
			timeseries_stats Stats = {};
			Parent->ComputeTimeseriesStats(Stats, Data.data(), Data.size());
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
						
						AggregateData(CurrentStartTime, CurrentStartTime, Timesteps, Data.data(), IntervalType, AggregationType, XValues, YValues);
						
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
			ProfileDisplayTime = CurrentStartTime;
			if(IntervalType == 1 || IntervalType == 2)
			{
				//TODO: Clean this up
				ProfileDisplayTime.second = 0;
				ProfileDisplayTime.minute = 0;
				ProfileDisplayTime.hour   = 0;
				ProfileDisplayTime.day    = 1;
				if(IntervalType == 2) ProfileDisplayTime.month = 1;
			}
			TimestepEdit.SetData(ProfileDisplayTime);
			
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
			
			Plot.cbModifFormatXGridUnits.Clear();
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
			Parent->ModelDll.GetStartDate(Parent->BaselineDataSet, TimeStr);
			Time BaselineStartTime;
			StrToTime(BaselineStartTime, TimeStr);
			
			std::vector<double> &Baseline = PlotData.Allocate(BaselineTimesteps);
			String BS;
			String Unit;
			Parent->GetSingleSelectedResultSeries(Parent->BaselineDataSet, BS, Unit, Baseline.data());
			NullifyNans(Baseline.data(), Baseline.size());
			BS << " baseline";
			
			double *BaselineXValues = PlotData.Allocate(BaselineTimesteps).data();
			ComputeXValues(InputStartTime, BaselineStartTime, BaselineTimesteps, Parent->TimestepSize, BaselineXValues);
			
			timeseries_stats Stats = {};
			Parent->ComputeTimeseriesStats(Stats, Baseline.data(), Baseline.size());
			Parent->DisplayTimeseriesStats(Stats, BS, Unit);
			
			AddPlot(BS, Unit, BaselineXValues, Baseline.data(), Baseline.size(), false, LogY, NormY, InputStartTime, BaselineStartTime, Stats.Min, Stats.Max);

			
			
			std::vector<double> &Current = PlotData.Allocate(ResultTimesteps);
			String CurrentLegend;
			Parent->GetSingleSelectedResultSeries(Parent->DataSet, CurrentLegend, Unit, Current.data());
			NullifyNans(Current.data(), Current.size());
			
			double *ResultXValues = PlotData.Allocate(ResultTimesteps).data();
			ComputeXValues(InputStartTime, ResultStartTime, ResultTimesteps, Parent->TimestepSize, ResultXValues);
			
			timeseries_stats Stats2 = {};
			Parent->ComputeTimeseriesStats(Stats2, Current.data(), Current.size());
			Parent->DisplayTimeseriesStats(Stats2, CurrentLegend, Unit);
			
			AddPlot(CurrentLegend, Unit, ResultXValues, Current.data(), Current.size(), false, LogY, NormY, InputStartTime, ResultStartTime, Stats2.Min, Stats2.Max);

			
			
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
				
				double *InputXValues = PlotData.Allocate(InputTimesteps).data();
				ComputeXValues(InputStartTime, InputStartTime, InputTimesteps, Parent->TimestepSize, InputXValues);
				
				timeseries_stats Stats3 = {};
				Parent->ComputeTimeseriesStats(Stats3, Obs.data(), Obs.size());
				Parent->DisplayTimeseriesStats(Stats3, InputLegend, Unit);
				
				AddPlot(InputLegend, Unit, InputXValues, Obs.data(), Obs.size(), Scatter, LogY, NormY, InputStartTime, InputStartTime, Stats3.Min, Stats3.Max);
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
			Time ResultEndTime = ResultStartTime;
			AdvanceTimesteps(ResultEndTime, ResultTimesteps-1, Parent->TimestepSize);
			
			Time GofStartTime = Parent->CalibrationIntervalStart.GetData();
			Time GofEndTime   = Parent->CalibrationIntervalEnd.GetData();
			
			//PromptOK(Format(GofStartDate));
			
			if(IsNull(GofStartTime) || !GofStartTime.IsValid()  || GofStartTime < ResultStartTime   || GofStartTime > ResultEndTime)                                GofStartTime = ResultStartTime;
			if(IsNull(GofEndTime)   || !GofEndTime.IsValid()    || GofEndTime   < ResultStartTime   || GofEndTime   > ResultEndTime || GofEndTime < GofStartTime)   GofEndTime   = ResultEndTime;
			
			//TODO! Make sure these become correct!!
			//TODO! Should probably ensure that GofStartTime matches an exact timestep.
			
			int64 GofTimesteps = TimestepsBetween(GofStartTime, GofEndTime, Parent->TimestepSize) + 1; //NOTE: if start time = end time, there is still one timestep.
			int64 GofOffset    = TimestepsBetween(ResultStartTime, GofStartTime, Parent->TimestepSize);
			
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
			Parent->ComputeTimeseriesStats(ModeledStats, ModeledSeries.data()+GofOffset, GofTimesteps);
			Parent->DisplayTimeseriesStats(ModeledStats, ModeledLegend, ModUnit);
			
			timeseries_stats ObservedStats = {};
			Parent->ComputeTimeseriesStats(ObservedStats, ObservedSeries.data()+GofOffset, GofTimesteps);
			Parent->DisplayTimeseriesStats(ObservedStats, ObservedLegend, ObsUnit);
			
			residual_stats ResidualStats = {};
			Parent->ComputeResidualStats(ResidualStats, ObservedSeries.data()+GofOffset, ModeledSeries.data()+GofOffset, GofTimesteps);
			String GOF = "Goodness of fit: ";
			Parent->DisplayResidualStats(ResidualStats, GOF);
			
			if(PlotMajorMode == MajorMode_Residuals)
			{
				double StartX = (double)(GofStartTime - InputStartTime);
				double EndX   = (double)(GofEndTime   - InputStartTime);
				
				if(ResidualStats.DataPoints > 0)
				{
					double *ResidualXValues = PlotData.Allocate(ResultTimesteps).data();
					ComputeXValues(InputStartTime, ResultStartTime, ResultTimesteps, Parent->TimestepSize, ResidualXValues);
					
					double XMean, XVar, XYCovar;
					Parent->ComputeTrendStats(ResidualXValues + GofOffset, Residuals.data() + GofOffset, GofTimesteps, ResidualStats.MeanError, XMean, XVar, XYCovar);
					
					bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get());
					//NOTE: Using the input start date as reference date is just so that we agree with the date formatting below.
					AddPlot(Legend, ModUnit, ResidualXValues, Residuals.data(), ResultTimesteps, Scatter, false, false, InputStartTime, ResultStartTime);
					
					NullifyNans(Residuals.data(), Residuals.size());
					
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
				Parent->ComputeTimeseriesStats(RS2, Residuals.data()+GofOffset, GofTimesteps);
				
				AddNormalApproximation(NormLegend, NBinsHistogram, RS2.Min, RS2.Max, RS2.Mean, RS2.StandardDeviation);
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
	
	if(Plot.GetCount() > 0)
	{
		if(PlotMajorMode == MajorMode_Histogram || PlotMajorMode == MajorMode_ResidualHistogram)
		{
			Plot.cbModifFormatXGridUnits.Clear();
			Plot.cbModifFormatX.Clear();
			//NOTE: Histograms require different zooming.
			Plot.ZoomToFit(true, true);
			PlotWasAutoResized = false;
			
			double XRange = Plot.GetXRange();
			double XMin   = Plot.GetXMin();
			
			//NOTE: The auto-resize cuts out half of each outer bar, so we fix that
			double Stride = XRange / (double)(NBinsHistogram-1);
			XMin   -= 0.5*Stride;
			XRange += Stride;
			Plot.SetXYMin(XMin);
			Plot.SetRange(XRange);
			
			int LineSkip = NBinsHistogram / 30 + 1;
			
			Plot.SetGridLinesX << [NBinsHistogram, Stride, LineSkip, this](Vector<double> &LinesOut)
			{
				double At = Plot.GetXMin();
				for(int Idx = 0; Idx < NBinsHistogram; ++Idx)
				{
					LinesOut << At;
					At += Stride * (double)LineSkip;
				}
			};
		}
		else if(PlotMajorMode == MajorMode_QQ)
		{
			Plot.cbModifFormatXGridUnits.Clear();
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
			Plot.cbModifFormatXGridUnits.Clear();
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
			
			const char *MonthNames[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
			
			int DoStep = GetSmallestStepResolution();
			
			//NOTE: Format to be displayed at grid lines
			if(DoStep == 0)
			{
				Plot.cbModifFormatXGridUnits << [this, MonthNames](String &s, int i, double d)
				{
					double dd = d <= 0.0 ? d - 0.01 : d + 0.01;   //NOTE: For weird reasons we get floating point imprecition unless we do this
					Time D2 = this->InputStartTime + (int64)dd;
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
				Plot.cbModifFormatXGridUnits << [this, MonthNames](String &s, int i, double d)
				{
					Time D2 = this->InputStartTime + (int64)(d + 0.5); //NOTE: The +0.5 is to avoid flickering when panning
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
				Plot.cbModifFormatXGridUnits << [this, MonthNames](String &s, int i, double d)
				{
					Time D2 = this->InputStartTime + (int64)(d + 0.5); //NOTE: The +0.5 is to avoid flickering when panning
					s << Format("%d.", D2.day);
					if(D2.day == 1) s << " " << MonthNames[D2.month-1];
					if(D2.month == 1 && D2.day == 1) s << Format("\n%d", D2.year);
				};
			}
			else if(DoStep == 4)
			{
				Plot.cbModifFormatXGridUnits << [this, MonthNames](String &s, int i, double d)
				{
					Time D2 = this->InputStartTime + (int64)(d + 0.5); //NOTE: The +0.5 is to avoid flickering when panning
					if(D2.month == 1) s << Format("%s\n%d", MonthNames[D2.month-1], D2.year);
					else              s << MonthNames[D2.month-1];
				};
			}
			else if(DoStep == 5)
			{
				Plot.cbModifFormatXGridUnits << [this](String &s, int i, double d)
				{
					Time D2 = this->InputStartTime + (int64)(d + 0.5); //NOTE: The +0.5 is to avoid flickering when panning
					s = Format("%d", D2.year);
				};
			}
			
			//NOTE: Format to be displayed at popup or data table
			if(DoStep == 0 || DoStep == 1 || DoStep == 2)
			{
				Plot.cbModifFormatX << [this](String &s, int i, double d)
				{
					double dd = d <= 0.0 ? d - 0.01 : d + 0.01;   //NOTE: For weird reasons we get floating point imprecition unless we do this
					Time D2 = this->InputStartTime + (int64)dd;
					s = Format(D2);
				};
			}
			else if(DoStep == 3)
			{
				Plot.cbModifFormatX << [this](String &s, int i, double d)
				{
					Time D2 = this->InputStartTime + (int64)(d + 0.5);
					Date D3(D2.year, D2.month, D2.day);
					s = Format(D3);
				};
			}
			else if(DoStep == 4)
			{
				Plot.cbModifFormatX << [this](String &s, int i, double d)
				{
					Time D2 = this->InputStartTime + (int64)(d + 0.5);
					s = Format("%d-%d", D2.year, D2.month);
				};
			}
			else if(DoStep == 5)
			{
				Plot.cbModifFormatX << [this](String &s, int i, double d)
				{
					Time D2 = this->InputStartTime + (int64)(d + 0.5);
					s = Format("%d", D2.year);
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
		
		
		SetBetterGridLinePositions(1);

		if(PlotMajorMode == MajorMode_QQ) SetBetterGridLinePositions(0);
		
		Size PlotSize = Plot.GetSize();
		Plot.SetSaveSize(PlotSize); //TODO: If somebody resizes the window without changing plot mode, this does not get called, and so plot save size will be incorrect....
	}
	
	Plot.Refresh();
}


void PlotCtrl::SetBetterGridLinePositions(int Dim)
{
	double Min;
	double Range;
	
	if(Dim == 0)
	{
		Min = Plot.GetXMin();
		Range = Plot.GetXRange();
	}
	else
	{
		Min = Plot.GetYMin();
		Range = Plot.GetYRange();
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
	
	//Plot.SetMinUnits(Null, Min);  //We would prefer to do this, but for some reason it works
	//poorly when there are negative values...
	if(Dim == 0)
	{
		Plot.SetXYMin(Min, Null);
		Plot.SetMajorUnits(Stride, Null);
	}
	else
	{
		Plot.SetXYMin(Null, Min);
		Plot.SetMajorUnits(Null, Stride);
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


void PlotCtrl::UpdateDateGridLinesX(Vector<double> &LinesOut)
{
	//InputStartTime  corresponds to  X=0. X resolution is always measured in seconds
	
	//TODO! Has to be improved for sub-day timestep resolutions!!
	
	
	int DesiredGridLineNum = 10;  //TODO: Make it sensitive to plot size
	
	double XMin = Plot.GetXMin();
	double XRange = Plot.GetXRange();
	
	Time FirstTime = InputStartTime + (int64)XMin;
	Time LastTime  = FirstTime + (int64)XRange;
	
	//NOTE: DoStep denotes the unit that we try to use for spacing the grid lines. 0=seconds, 1=minutes, 2=hours, 3=days,
	//4=months, 5=years
	int DoStep = GetSmallestStepResolution();
	
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
	int IntervalType = TimeIntervals.GetData();
	if(IntervalType == 0) // Daily values
	{
		AdvanceTimesteps(NewTime, Timestep, Parent->TimestepSize);
	}
	else if(IntervalType == 1) // Monthly values
	{
		int Month   = ((NewTime.month + Timestep - 1) % 12) + 1;
		int YearAdd = (NewTime.month + Timestep - 1) / 12;
		NewTime.month = Month;
		NewTime.year += YearAdd;
	}
	else if(IntervalType == 2) // Yearly values
	{
		NewTime.year += Timestep;
	}
	TimestepEdit.SetData(NewTime);
	
	
	ReplotProfile();
}

void PlotCtrl::TimestepEditEvent()
{
	//NOTE: This can only happen if we are in daily mode since the edit is disabled otherwise.
	//If this changes, this code has to be rethought.
	
	//TODO: Fix this again later!!!
	
	Time CurrentTime = TimestepEdit.GetData();
	int Timestep = TimestepsBetween(ProfileDisplayTime, CurrentTime, Parent->TimestepSize);
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
		Log(String("Unable to open file") + FileName);
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
	
	Log(String("Results exported to " + FileName));
}





#endif
