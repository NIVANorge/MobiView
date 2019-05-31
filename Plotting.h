#ifndef _MobiView_Plotting_h_
#define _MobiView_Plotting_h_


#include <chrono>


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
	
	EquationSelecter.Enable();
	
	PlotModeChange(); //NOTE: Refresh the plot if necessary since the data can have changed after a new run.
	
	Log(String("Model was run.\nDuration: ") << Ms << " ms.");
}



void MobiView::PlotModeChange()
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
	
	int RowCount = InputSelecter.GetCount();
	
	for(int Row = 0; Row < RowCount; ++Row)
	{
		if(InputSelecter.IsSelected(Row))
		{
			std::string Name = InputSelecter.Get(Row, 0).ToString().ToStd();
			
			uint64 IndexSetCount = ModelDll.GetInputIndexSetsCount(DataSet, Name.data());
			std::vector<char *> IndexSets(IndexSetCount);
			ModelDll.GetInputIndexSets(DataSet, Name.data(), IndexSets.data());
			
			for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
			{
				const char *IndexSet = IndexSets[Idx];
				size_t Id = IndexSetNameToId[IndexSet];
				EIndexList[Id]->Enable();
			}
		}
	}
	
	uint64 Timesteps = ModelDll.GetTimesteps(DataSet);
	if(Timesteps != 0)
	{
		int RowCount = EquationSelecter.GetCount();
		
		for(int Row = 0; Row < RowCount; ++Row)
		{
			if(EquationSelecter.IsSelected(Row))
			{
				std::string Name = EquationSelecter.Get(Row, 0).ToString().ToStd();
				
				uint64 IndexSetCount = ModelDll.GetResultIndexSetsCount(DataSet, Name.data()); //IMPORTANT! Returns 0 if the model has not been run at least once!!
				std::vector<char *> IndexSets(IndexSetCount);
				ModelDll.GetResultIndexSets(DataSet, Name.data(), IndexSets.data());
				
				for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
				{
					const char *IndexSet = IndexSets[Idx];
					size_t Id = IndexSetNameToId[IndexSet];
					EIndexList[Id]->Enable();
				}
			}
		}
	}
	
	
	RePlot();
}


void MobiView::AddHistogram(String &Legend, int PlotIdx, double *Data, size_t Len)
{
	AggregateX.push_back({});
	AggregateY.push_back({});
	
	std::vector<double> &XValues = AggregateX[AggregateX.size()-1];
	std::vector<double> &YValues = AggregateY[AggregateY.size()-1];
	
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
		
		int ColorIdx = PlotIdx % PlotColors.size();
		Color &GraphColor = PlotColors[ColorIdx];
		double Darken = 0.4;
		Color BorderColor((int)(((double)GraphColor.GetR())*Darken), (int)(((double)GraphColor.GetG())*Darken), (int)(((double)GraphColor.GetB())*Darken));
		Plot.AddSeries(XValues.data(), YValues.data(), XValues.size()).Legend(Legend).PlotStyle<BarSeriesPlot>().BarWidth(0.5*Stride).NoMark().Fill(GraphColor).Stroke(1.0, BorderColor);
	}
	else
	{
		//TODO?
	}
}


void MobiView::AggregateData(Date &ReferenceDate, Date &StartDate, uint64 Timesteps, double *Data, int IntervalType, int AggregationType, std::vector<double> &XValues, std::vector<double> &YValues)
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
			YValues.push_back(YVal);
			
			//TODO: Is it better to put it at the beginning of the year/month or the middle?
			Date Beginning = CurrentDate;
			Beginning.day = 0;
			if(IntervalType == 2) Beginning.month = 0;
			double XVal = (double)(Beginning - ReferenceDate);
			
			//NOTE put the X position roughly in the middle of the respective month or year. This
			//increases clarity in the plot.
			if(IntervalType == 1) XVal += 15.5;
			else if(IntervalType == 2) XVal += 182.5;
			
			XValues.push_back(XVal);
			
			CurrentAggregate = 0.0;
			CurrentCount = 0;
		}
		
		CurrentDate = NextDate;
	}
}


void MobiView::AddPlot(String &Legend, String &Unit, int PlotIdx, double *Data, size_t Len, bool Scatter, bool LogY, bool NormalY, Date &ReferenceDate, Date &StartDate, double MinY, double MaxY)
{
	//TODO: Also implement logarithmic Y axis and normalized Y axis.
	
	int Timesteps = (int)Len;
	
	int IntervalType = TimeIntervals.GetData();
	int AggregationType = Aggregation.GetData();
	
	int ColorIdx = PlotIdx % PlotColors.size();
	Color &GraphColor = PlotColors[ColorIdx];
	
	//TODO: Do some initial trimming to get rid of NaNs at the head and tail of the data.
	ScatterDraw *Graph = nullptr;
	
	if(IntervalType == 0) //Daily values
	{
		
		//TODO TODO TODO TODO: It may not always be that good of an idea to modify the
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
		AggregateX.push_back({});
		AggregateY.push_back({});
		
		//Would it help to reserve some size for these?
		std::vector<double> &XValues = AggregateX[AggregateX.size()-1];
		std::vector<double> &YValues = AggregateY[AggregateY.size()-1];
		
		AggregateData(ReferenceDate, StartDate, Timesteps, Data, IntervalType, AggregationType, XValues, YValues);
		
		Graph = &Plot.AddSeries(XValues.data(), YValues.data(), XValues.size());
	}
	
	if(Graph) //NOTE: Graph should never be nullptr, but in case we have a bug, let's not crash.
	{
		Graph->Legend(Legend);
		Graph->Units(Unit);
		if(Scatter)
		{
			//TODO: Styles 4-5 are whacked when fill color is white.
			Graph->MarkColor(Color(255,255,255)).MarkBorderColor(GraphColor).Stroke(0.0, GraphColor).Opacity(0.5);
		}
		else
		{
			Graph->NoMark().Stroke(1.5, GraphColor).Dash("");
		}
	}
}

void MobiView::AddTrendLine(String &Legend, int PlotIdx, size_t Timesteps, double XYCovar, double XVar, double YMean, double XMean, Date &ReferenceDate, Date &StartDate)
{
	
	//TODO: Duh, there is apparently an inbuilt way to do this instead... we could use that.
	
	double Offset = (double)(StartDate - ReferenceDate);
	
	double Beta = XYCovar / XVar;
	double Alpha = YMean - Beta*(XMean + Offset);
	
	AggregateX.push_back({});
	AggregateY.push_back({});
		
	std::vector<double> &XValues = AggregateX[AggregateX.size()-1];
	std::vector<double> &YValues = AggregateY[AggregateY.size()-1];
	
	XValues.resize(2);
	YValues.resize(2);
	
	XValues[0] = Offset;
	XValues[1] = Offset + (double)Timesteps;
	YValues[0] = Alpha + XValues[0]*Beta;
	YValues[1] = Alpha + XValues[1]*Beta;
	
	int ColorIdx = PlotIdx % PlotColors.size();
	Color &GraphColor = PlotColors[ColorIdx];
	Plot.AddSeries(XValues.data(), YValues.data(), XValues.size()).NoMark().Legend(Legend).Stroke(1.5, GraphColor).Dash("6 3");
}

void MobiView::AddPlotRecursive(std::string &Name, int Mode, std::vector<char *> &IndexSets,
	std::vector<std::string> &CurrentIndexes, int Level, int &PlotIdx, uint64 Timesteps,
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
		PlotData.push_back({});
		PlotData[PlotIdx].resize(Timesteps);
		
		String Unit;
		
		if(Mode == 0)
		{
			ModelDll.GetResultSeries(DataSet, Name.data(), IndexData, Indexes.size(), PlotData[PlotIdx].data());
			Unit = ModelDll.GetResultUnit(DataSet, Name.data());
		}
		else if(Mode == 1)
		{
			ModelDll.GetInputSeries(DataSet, Name.data(), IndexData, Indexes.size(), PlotData[PlotIdx].data(), false);
			Unit = ModelDll.GetInputUnit(DataSet, Name.data());
		}
		if(CheckDllUserError()) return;
		
		double *Data = PlotData[PlotIdx].data();
		size_t Len = PlotData[PlotIdx].size();
		
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
		
		timeseries_stats Stats = {};
		ComputeTimeseriesStats(Stats, Data, Len, StartDate);
		DisplayTimeseriesStats(Stats, Legend, Unit);
		
		bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get() && Mode == 1);
		bool LogY = (YAxisMode.IsEnabled() && YAxisMode.GetData() == 2);
		bool NormY = (YAxisMode.IsEnabled() && YAxisMode.GetData() == 1);
		AddPlot(Legend, Unit, PlotIdx, Data, Len, Scatter, LogY, NormY, ReferenceDate, StartDate, Stats.Min, Stats.Max);
		
		NullifyNans(Data, Len);
		
		++PlotIdx;
	}
	else
	{
		char *IndexSetName = IndexSets[Level];
		size_t Id = IndexSetNameToId[IndexSetName];
		
		int RowCount = EIndexList[Id]->GetCount();
		
		for(int Row = 0; Row < RowCount; ++Row)
		{
			if(EIndexList[Id]->IsSelected(Row))
			{
				//PromptOK(EIndexList[Id]->Get(Row, 0).ToString());
				CurrentIndexes[Level] = EIndexList[Id]->Get(Row, 0).ToString().ToStd();
				AddPlotRecursive(Name, Mode, IndexSets, CurrentIndexes, Level + 1, PlotIdx, Timesteps, ReferenceDate, StartDate);
			}
		}
	}
}

void MobiView::RePlot()
{
	if(!EquationSelecter.IsSelection() && !InputSelecter.IsSelection()) return;
	

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
	PlotData.clear();
	AggregateX.clear();
	AggregateY.clear();
	
	PlotInfo.Clear();
	
	int MajorMode = PlotMajorMode.GetData();
	
	Plot.SetTitle(String(""));
	
	
	char DateStr[256];
	ModelDll.GetStartDate(DataSet, DateStr);
	Date ResultStartDate;
	StrToDate(ResultStartDate, DateStr);

	ModelDll.GetInputStartDate(DataSet, DateStr);
	Date InputStartDate;
	StrToDate(InputStartDate, DateStr);
	
	int PlotIdx = 0;
	
	uint64 InputTimesteps = ModelDll.GetInputTimesteps(DataSet);
	uint64 ResultTimesteps = ModelDll.GetTimesteps(DataSet);
	
	if(PlotMajorMode == MajorMode_Regular)
	{
		int RowCount = InputSelecter.GetCount();
		
		for(int Row = 0; Row < RowCount; ++Row)
		{
			if(InputSelecter.IsSelected(Row))
			{
				std::string Name = InputSelecter.Get(Row, 0).ToString().ToStd();
				
				uint64 IndexSetCount = ModelDll.GetInputIndexSetsCount(DataSet, Name.data());
				std::vector<char *> IndexSets(IndexSetCount);
				ModelDll.GetInputIndexSets(DataSet, Name.data(), IndexSets.data());
				if(CheckDllUserError()) return;
				
				std::vector<std::string> CurrentIndexes(IndexSets.size());
				AddPlotRecursive(Name, 1, IndexSets, CurrentIndexes, 0, PlotIdx, InputTimesteps, InputStartDate, InputStartDate);
			}
		}
		
		if(ResultTimesteps != 0) //Timesteps  == 0 if the model has not been run yet. TODO: Maybe print a warning that the model has not been run but EquationSelecter.IsSelection()?
		{
			int RowCount = EquationSelecter.GetCount();
			
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
					AddPlotRecursive(Name, 0, IndexSets, CurrentIndexes, 0, PlotIdx, ResultTimesteps, InputStartDate, ResultStartDate);
				}
			}
		}
	}
	else if(PlotMajorMode == MajorMode_Histogram)
	{
		//UUUGH. GetSelectCount is also wrong because of that bug where the latest selection is
		//not counted..
		
		int TimeseriesCount = EquationSelecter.GetSelectCount() + InputSelecter.GetSelectCount();
		
		
		if(TimeseriesCount > 1 || MultiIndex)
		{
			//TODO: Setting the title is a lousy way to provide an error message....
			Plot.SetTitle(String("In histogram mode you can only have one timeseries selected, for one index combination"));
		}
		else
		{
			PlotData.push_back({});
			std::vector<double> &Data = PlotData[PlotIdx];
			String Legend;
			String Unit;
		
			Date StartDate = ResultStartDate;
			if(EquationSelecter.IsSelection() && ResultTimesteps != 0)
			{
				Data.resize(ResultTimesteps);
				
				GetSingleSelectedResultSeries(DataSet, Legend, Unit, Data.data());
			}
			else if(InputSelecter.IsSelection())
			{
				Data.resize(InputTimesteps);
				GetSingleSelectedInputSeries(DataSet, Legend, Unit, Data.data(), false);
				StartDate = InputStartDate;
			}
			
			AddHistogram(Legend, PlotIdx, Data.data(), Data.size());
			
			timeseries_stats Stats = {};
			ComputeTimeseriesStats(Stats, Data.data(), Data.size(), StartDate);
			DisplayTimeseriesStats(Stats, Legend, Unit);
			
			++PlotIdx;
		}
	}
	else if(PlotMajorMode == MajorMode_Profile)
	{
		int TimeseriesCount = EquationSelecter.GetSelectCount() + InputSelecter.GetSelectCount();
		
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
			PlotData.resize(IndexCount);
			
			ProfileLabels.resize(IndexCount);
			
			int Mode = EquationSelecter.GetSelectCount() != 0 ? 0 : 1; //I.e. Mode = 0 if it was an equation that was selected, otherwise Mode = 1 if it was an input that was selected
			
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
					std::vector<double> RawData(Timesteps);
					
					if(Mode == 0)
					{
						GetSingleResultSeries(DataSet, RawData.data(), ProfileIndexSet, Row);
					}
					else
					{
						GetSingleInputSeries(DataSet, RawData.data(), ProfileIndexSet, Row);
					}
					
					NullifyNans(RawData.data(), RawData.size());
					
					ProfileLabels[IdxIdx] = EIndexList[ProfileIndexSet]->Get(Row, 0).ToString();
					
					if(IntervalType == 0)
					{
						PlotData[IdxIdx] = RawData; //NOTE: vector copy. We should maybe have avoided this.
					}
					if(IntervalType == 1 || IntervalType == 2) //Monthly or yearly aggregation
					{
						std::vector<double> XValues; //TODO: Ugh, it is stupid to have to declare this when it is not going to be used.
						
						AggregateData(CurrentStartDate, CurrentStartDate, Timesteps, RawData.data(), IntervalType, AggregationType, XValues, PlotData[IdxIdx]);
					}
					
					++IdxIdx;
				}
			}
			
			if(Mode == 0)
			{
				ProfileLegend = EquationSelecter.Get(0).ToString();
			}
			else
			{
				ProfileLegend = InputSelecter.Get(0).ToString();
			}
			ProfileLegend << " profile";
			
			size_t DataLength = PlotData[0].size();
			
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
			
			//TODO: This is a reuse of AggregateX and AggregateY outside the original
			//intention. Maybe refactor the whole plot data storage system?
			AggregateX.push_back({});
			AggregateY.push_back({});
			
			std::vector<double> &XValues = AggregateX[AggregateX.size()-1];
			std::vector<double> &YValues = AggregateY[AggregateY.size()-1];
			
			XValues.resize(IndexCount);
			YValues.resize(IndexCount);
			
			double YMax = DBL_MIN;
			for(size_t Idx = 0; Idx < PlotData.size(); ++Idx)
			{
				for(size_t Ts = 0; Ts < DataLength; ++Ts)
				{
					double Value = PlotData[Idx][Ts];
					if(std::isfinite(Value) && !IsNull(Value))
					{
						YMax = std::max(YMax, PlotData[Idx][Ts]);
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
			
			++PlotIdx;
		}
	}
	else if(PlotMajorMode == MajorMode_CompareBaseline)
	{
		//TODO: Limit to one selected result series. Maybe also allow plotting a observation
		//comparison.
		if(EquationSelecter.GetSelectCount() > 1 || MultiIndex)
		{
			Plot.SetTitle(String("In baseline comparison mode you can only have one result series selected, for one index combination"));
		}
		else
		{
			bool LogY = (YAxisMode.IsEnabled() && YAxisMode.GetData() == 2);
			bool NormY = (YAxisMode.IsEnabled() && YAxisMode.GetData() == 1);
			
			
			uint64 BaselineTimesteps = ModelDll.GetTimesteps(BaselineDataSet);
			ModelDll.GetStartDate(BaselineDataSet, DateStr);
			Date BaselineStartDate;
			StrToDate(BaselineStartDate, DateStr);
			
			PlotData.push_back({});
			std::vector<double> &Baseline = PlotData[PlotIdx];
			Baseline.resize(BaselineTimesteps);
			String BS;
			String Unit;
			GetSingleSelectedResultSeries(BaselineDataSet, BS, Unit, Baseline.data());
			NullifyNans(Baseline.data(), Baseline.size());
			BS << " baseline";
			
			timeseries_stats Stats = {};
			ComputeTimeseriesStats(Stats, Baseline.data(), Baseline.size(), BaselineStartDate);
			DisplayTimeseriesStats(Stats, BS, Unit);
			
			AddPlot(BS, Unit, PlotIdx, Baseline.data(), Baseline.size(), false, LogY, NormY, InputStartDate, BaselineStartDate, Stats.Min, Stats.Max);
			++PlotIdx;
			
			
			
			PlotData.push_back({});
			std::vector<double> &Current = PlotData[PlotIdx];
			Current.resize(ResultTimesteps);
			String CurrentLegend;
			GetSingleSelectedResultSeries(DataSet, CurrentLegend, Unit, Current.data());
			NullifyNans(Current.data(), Current.size());
			
			timeseries_stats Stats2 = {};
			ComputeTimeseriesStats(Stats2, Current.data(), Current.size(), ResultStartDate);
			DisplayTimeseriesStats(Stats2, CurrentLegend, Unit);
			
			AddPlot(CurrentLegend, Unit, PlotIdx, Current.data(), Current.size(), false, LogY, NormY, InputStartDate, ResultStartDate, Stats2.Min, Stats2.Max);
			++PlotIdx;
			
			
			//TODO: Should we compute any residual statistics here?
			
			if(InputSelecter.GetSelectCount() == 1)
			{
				//TODO: Should we allow displaying multiple input series here? Probably no
				//reason to
				
				bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get());
				
				PlotData.push_back({});
				std::vector<double> &Obs = PlotData[PlotIdx];
				Obs.resize(InputTimesteps);
				String InputLegend;
				String Unit;
				GetSingleSelectedInputSeries(DataSet, InputLegend, Unit, Obs.data(), false);
				NullifyNans(Obs.data(), Obs.size());
				
				timeseries_stats Stats3 = {};
				ComputeTimeseriesStats(Stats3, Obs.data(), Obs.size(), InputStartDate);
				DisplayTimeseriesStats(Stats3, InputLegend, Unit);
				
				AddPlot(InputLegend, Unit, PlotIdx, Obs.data(), Obs.size(), Scatter, LogY, NormY, InputStartDate, InputStartDate, Stats3.Min, Stats3.Max);
				++PlotIdx;
			}
		}
		
		
	}
	else if(PlotMajorMode == MajorMode_Residuals || PlotMajorMode == MajorMode_ResidualHistogram)
	{
		
		if(EquationSelecter.GetSelectCount() != 1 || InputSelecter.GetSelectCount() != 1 || MultiIndex)
		{
			//TODO: Setting the title is a lousy way to provide an error message....
			Plot.SetTitle(String("In residual mode you must select exactly 1 result series and 1 input series, for one index combination only"));
		}
		else
		{
			PlotData.push_back({});
			std::vector<double> &Residuals = PlotData[PlotIdx];
			Residuals.resize(ResultTimesteps);
			std::vector<double> ModeledSeries(ResultTimesteps);
			std::vector<double> ObservedSeries(ResultTimesteps);
			String ModeledLegend;
			String ObservedLegend;
			
			//These should be the same, but just in case..
			String ModUnit;
			String ObsUnit;
			
			GetSingleSelectedResultSeries(DataSet, ModeledLegend, ModUnit, ModeledSeries.data());
			GetSingleSelectedInputSeries(DataSet, ObservedLegend, ObsUnit, ObservedSeries.data(), true);
			
			for(size_t Idx = 0; Idx < ResultTimesteps; ++Idx)
			{
				Residuals[Idx] = ObservedSeries[Idx] - ModeledSeries[Idx];
			}
			
			String Legend = String("Residuals of ") + ModeledLegend + " vs " + ObservedLegend;
			
			timeseries_stats ModeledStats = {};
			ComputeTimeseriesStats(ModeledStats, ModeledSeries.data(), ModeledSeries.size(), ResultStartDate);
			DisplayTimeseriesStats(ModeledStats, ModeledLegend, ModUnit);
			
			timeseries_stats ObservedStats = {};
			ComputeTimeseriesStats(ObservedStats, ObservedSeries.data(), ObservedSeries.size(), ResultStartDate);
			DisplayTimeseriesStats(ObservedStats, ObservedLegend, ObsUnit);
			
			residual_stats ResidualStats = {};
			ComputeResidualStats(ResidualStats, Residuals.data(), ObservedStats.Variance, Residuals.size(), ResultStartDate);
			String GOF = "Goodness of fit: ";
			DisplayResidualStats(ResidualStats, GOF);
			
			if(PlotMajorMode == MajorMode_Residuals)
			{
				bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get());
				
				double XMean, XVar, XYCovar;
				ComputeTrendStats(Residuals.data(), Residuals.size(), ResidualStats.MeanError, XMean, XVar, XYCovar);
				
				//NOTE: Using the input start date as reference date is just so that we agree with the date formatting below.
				AddPlot(Legend, ModUnit, PlotIdx, Residuals.data(), ResultTimesteps, Scatter, false, false, InputStartDate, ResultStartDate);
				
				NullifyNans(Residuals.data(), Residuals.size());
				
				++PlotIdx;
				
				String TL = "Trend line";
				AddTrendLine(TL,PlotIdx, Residuals.size(), XYCovar, XVar, ResidualStats.MeanError, XMean, InputStartDate, ResultStartDate);
				
				++PlotIdx;
			}
			else //PlotMajorMode == MajorMode_ResidualHistogram
			{
				AddHistogram(Legend, PlotIdx, Residuals.data(), Residuals.size());
				++PlotIdx;
			}
		}
	}
	
	
	if(PlotIdx > 0)
	{
		if(PlotMajorMode == MajorMode_Histogram || PlotMajorMode == MajorMode_ResidualHistogram)
		{
			Plot.cbModifFormatX.Clear();
			//NOTE: Histograms require completely different zooming.
			Plot.ZoomToFit(true, true);
			PlotWasAutoResized = false;
		}
		else if(PlotMajorMode == MajorMode_Profile)
		{
			PlotWasAutoResized = false;
		}
		else
		{
			Plot.cbModifFormatX.Clear();
			Plot.ZoomToFit(false, true);
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
					
					Plot.cbModifFormatX << [InputStartDate](String &s, int i, double d)
					{
						Date D2 = InputStartDate + (int)d;
						s << Format("%d-%02d", D2.year, D2.month);
					};
				}
				else if(IntervalType == 2)
				{
					MonthlyOrYearly = true;
					
					Plot.cbModifFormatX << [InputStartDate](String &s, int i, double d)
					{
						Date D2 = InputStartDate + (int)d;
						s = Format("%d", D2.year);
					};
				}
			}
			
			if(!MonthlyOrYearly)
			{
				Plot.cbModifFormatX << [InputStartDate](String &s, int i, double d)
				{
					Date D2 = InputStartDate + (int)d;
					s = Format(D2);
				};
			}
		}
		
		Plot.cbModifFormatY.Clear();
		if(YAxisMode.IsEnabled() && YAxisMode.GetData() == 2) //If we want a logarithmic Y axis
		{
			Plot.cbModifFormatY << [](String &s, int i, double d)
			{
				s = FormatDoubleExp(pow(10., d), 2);
			};
		}
	}
	
	Plot.SetLabelX(" ");
	Plot.SetLabelY(" ");
	
	Size PlotSize = Plot.GetSize();
	Plot.SetSaveSize(PlotSize); //TODO: This then breaks if somebody resizes the window in between....
	
	Plot.Refresh();
}



void MobiView::TimestepSliderEvent()
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

void MobiView::TimestepEditEvent()
{
	//NOTE: This can only happen if we are in daily mode since the edit is disabled otherwise.
	//If this changes, this code has to be rethought.
	
	Date CurrentDate = TimestepEdit.GetData();
	int Timestep = CurrentDate - ProfileDisplayDate;
	TimestepSlider.SetData(Timestep);
	ReplotProfile();
}


void MobiView::ReplotProfile()
{
	Plot.RemoveAllSeries();
	
	int Timestep = TimestepSlider.GetData();
	
	std::vector<double> &XValues = AggregateX[0];
	std::vector<double> &YValues = AggregateY[0];
	
	for(size_t Idx = 0; Idx < PlotData.size(); ++Idx)
	{
		YValues[Idx] = PlotData[Idx][Timestep];
	}
	
	Color &GraphColor = PlotColors[0];
	double Darken = 0.4;
	Color BorderColor((int)(((double)GraphColor.GetR())*Darken), (int)(((double)GraphColor.GetG())*Darken), (int)(((double)GraphColor.GetB())*Darken));
	Plot.AddSeries(XValues.data(), YValues.data(), XValues.size()).Legend(ProfileLegend).PlotStyle<BarSeriesPlot>().BarWidth(0.5).NoMark().Fill(GraphColor).Stroke(1.0, BorderColor);
	
	Plot.SetLabelX(" ");
	Plot.SetLabelY(" ");
	
	Plot.Refresh();
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
			Indexes_String[Idx] = EIndexList[Id]->Get(Row, 0).ToString().ToStd();
		}
		else
		{
			Indexes_String[Idx] = EIndexList[Id]->Get(0).ToString().ToStd();
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
			Indexes_String[Idx] = EIndexList[Id]->Get(Row, 0).ToString().ToStd();
		}
		else
		{
			Indexes_String[Idx] = EIndexList[Id]->Get(0).ToString().ToStd();
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
		
		Indexes_String[Idx] = EIndexList[Id]->Get(0).ToString().ToStd();
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
		
		Indexes_String[Idx] = EIndexList[Id]->Get(0).ToString().ToStd();
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


void MobiView::NullifyNans(double *Data, size_t Len)
{
	//NOTE: We do this because the ScatterDraw does not draw gaps in the line at NaNs, only at
	//Null.
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		if(std::isnan(Data[Idx])) Data[Idx] = Null;
	}
}


#endif
