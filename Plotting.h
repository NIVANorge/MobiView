#ifndef _MobiView_Plotting_h_
#define _MobiView_Plotting_h_



void MobiView::RunModel()
{
	if(!hinstModelDll || !ModelDll.RunModel) return;
	
	ModelDll.RunModel(DataSet);
	
	EquationSelecter.Enable();
	
	PlotModeChange(); //NOTE: This is to refresh the plot if necessary.
}

void MobiView::PlotModeChange()
{
	int MajorMode = PlotMajorMode.GetData();
	
	ScatterInputs.Disable();
	LogarithmicY.Disable();
	NormalizeY.Disable();
	TimeIntervals.Disable();
	
	TimestepLabel.Hide();
	TimestepSlider.Hide();
	TimestepSlider.Disable();
	
	if(MajorMode == MajorMode_Regular || MajorMode == MajorMode_CompareBaseline)
	{
		ScatterInputs.Enable();
		LogarithmicY.Enable();
		NormalizeY.Enable();
		TimeIntervals.Enable();
	}
	else if(MajorMode == MajorMode_Residuals)
	{
		ScatterInputs.Enable();
		TimeIntervals.Enable();
	}
	//else if //IMPLEMENTME
	else if(MajorMode == MajorMode_Profile)
	{
		TimestepSlider.Show();
		TimestepLabel.Show();
	}
	else
	{
		
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
			NormalizeY.Disable();
		}
	}
	
	/*
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		EIndexList[Idx]->Enable();
	}
	
	uint64 Timesteps = ModelDll.GetTimesteps(DataSet);
	if(Timesteps != 0)
	{
		EquationSelecter.Enable();
	}
	InputSelecter.Enable();
	*/
	
	//TODO: This could be more fine-grained. We don't have to update the index list every time.
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		EIndexList[Idx]->Disable();
	}
	
	int RowCount = InputSelecter.GetCount();
	
	for(int Row = 0; Row < RowCount; ++Row)
	{
		if(InputSelecter.IsSelected(Row)) //Ugh. does not work properly when you deselect!
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
			if(EquationSelecter.IsSelected(Row)) //Ugh. does not work properly when you deselect!
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
		if(std::isfinite(D))
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
		
		for(size_t Idx = 0; Idx < Len; ++Idx)
		{
			double D = Data[Idx];
			if(std::isfinite(D))
			{
				size_t BinIndex = (size_t)((D - Min)/Stride);
				if(BinIndex == NBins) BinIndex = NBins-1;     //NOTE: Happens if D==Max
				YValues[BinIndex] += 1.0;
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
		//TODO
	}
}


void MobiView::AddPlot(String &Legend, int PlotIdx, double *Data, size_t Len, bool Scatter, bool LogY, bool NormalY, Date &ReferenceDate, Date &StartDate, double MinY, double MaxY)
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
		if(NormalY)
		{
			for(size_t Idx = 0; Idx < Len; ++Idx)
			{
				//TODO TODO TODO TODO: It may not always be that good of an idea to modify the
				//incoming data...
				if(MaxY > 0.0) Data[Idx] = Data[Idx] / MaxY;
				else Data[Idx] = 0.0;
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
		
		double CurrentAggregate = 0.0;
		int    CurrentCount = 0;
		Date CurrentDate = StartDate;
		int CurrentTimestep = 0;
		
		while(CurrentTimestep < Timesteps)
		{
			double Val = Data[CurrentTimestep];
			if(std::isfinite(Val))
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
				XValues.push_back(XVal);
				
				CurrentAggregate = 0.0;
				CurrentCount = 0;
			}
			
			CurrentDate = NextDate;
		}
		
		Graph = &Plot.AddSeries(XValues.data(), YValues.data(), XValues.size());
	}
	
	if(Graph) //NOTE: Graph should never be nullptr, but in case we have a bug, let's not crash.
	{
		Graph->Legend(Legend);
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

/*
void MobiView::AddNormalPlot(String &Legend, int PlotIdx, double MinX, double MaxX, double Mean, double Variance)
{
	size_t Resolution = 200;
	
	AggregateX.push_back({});
	AggregateY.push_back({});
		
	std::vector<double> &XValues = AggregateX[AggregateX.size()-1];
	std::vector<double> &YValues = AggregateY[AggregateY.size()-1];
	
	XValues.resize(Resolution);
	YValues.resize(Resolution);
	
	double Scale = 1.0 / std::sqrt(2.0*M_PI*Variance);
	
	for(size_t Idx = 0; Idx < Resolution; ++Idx)
	{
		XValues
	}
}
*/

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
		
		if(Mode == 0)
		{
			ModelDll.GetResultSeries(DataSet, Name.data(), IndexData, Indexes.size(), PlotData[PlotIdx].data());
		}
		else if(Mode == 1)
		{
			ModelDll.GetInputSeries(DataSet, Name.data(), IndexData, Indexes.size(), PlotData[PlotIdx].data(), false);
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
		DisplayTimeseriesStats(Stats, Legend);
		
		bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get() && Mode == 1);
		bool LogY = (LogarithmicY.IsEnabled() && LogarithmicY.Get());
		bool NormY = (NormalizeY.IsEnabled() && NormalizeY.Get());
		AddPlot(Legend, PlotIdx, Data, Len, Scatter, LogY, NormY, ReferenceDate, StartDate, Stats.Min, Stats.Max);
		
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
		if(EIndexList[IndexSet]->GetSelectCount() > 1)
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
			String Legend = "";
		
			Date StartDate = ResultStartDate;
			if(EquationSelecter.IsSelection() && ResultTimesteps != 0)
			{
				Data.resize(ResultTimesteps);
				
				GetSingleSelectedResultSeries(DataSet, Legend, Data.data());
			}
			else if(InputSelecter.IsSelection())
			{
				Data.resize(InputTimesteps);
				GetSingleSelectedInputSeries(DataSet, Legend, Data.data(), false);
				StartDate = InputStartDate;
			}
			
			AddHistogram(Legend, PlotIdx, Data.data(), Data.size());
			
			timeseries_stats Stats = {};
			ComputeTimeseriesStats(Stats, Data.data(), Data.size(), StartDate);
			DisplayTimeseriesStats(Stats, Legend);
			
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
			if(EIndexList[IndexSet]->GetSelectCount() > 1)
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
			
			if(EquationSelecter.GetSelectCount() != 0)
			{
				size_t IdxIdx = 0;
				int RowCount = EIndexList[ProfileIndexSet]->GetCount();
				
				//String LA = "Rows\n";
				for(int Row = 0; Row < RowCount; ++Row)
				{
					if(EIndexList[ProfileIndexSet]->IsSelected(Row))
					{
						PlotData[IdxIdx].resize(ResultTimesteps);
						GetSingleResultSeries(DataSet, PlotData[IdxIdx].data(), ProfileIndexSet, Row);
						
						ProfileLabels[IdxIdx] = EIndexList[ProfileIndexSet]->Get(Row, 0).ToString();
						//LA << Row << "\n";
						
						++IdxIdx;
					}
				}
				//Log(LA);
				
				ProfileLegend = EquationSelecter.Get(0).ToString();
				ProfileLegend << " profile";
				
				TimestepSlider.Range((int)ResultTimesteps - 1);
				
			}
			else if(InputSelecter.GetSelectCount() != 0)
			{
				
			}
			
			AggregateX.push_back({});
			AggregateY.push_back({});
			
			std::vector<double> &XValues = AggregateX[AggregateX.size()-1];
			std::vector<double> &YValues = AggregateY[AggregateY.size()-1];
			
			XValues.resize(IndexCount);
			YValues.resize(IndexCount);
			
			double YMax = DBL_MIN;
			for(size_t Idx = 0; Idx < PlotData.size(); ++Idx)
			{
				for(size_t Ts = 0; Ts < ResultTimesteps; ++Ts)
				{
					YMax = std::max(YMax, PlotData[Idx][Ts]);
				}
				
				XValues[Idx] = (double)Idx + 0.5;
			}
			
			Plot.SetXYMin(0.0, 0.0);
			Plot.SetRange((double)IndexCount, YMax);
			Plot.SetMajorUnits(1.0);
			Plot.SetMinUnits(0.5);
			//Plot.SetMajorUnitsNum(IndexCount)
			
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
			bool LogY = (LogarithmicY.IsEnabled() && LogarithmicY.Get());
			bool NormY = (NormalizeY.IsEnabled() && NormalizeY.Get());
			
			
			uint64 BaselineTimesteps = ModelDll.GetTimesteps(BaselineDataSet);
			ModelDll.GetStartDate(BaselineDataSet, DateStr);
			Date BaselineStartDate;
			StrToDate(BaselineStartDate, DateStr);
			
			PlotData.push_back({});
			std::vector<double> &Baseline = PlotData[PlotIdx];
			Baseline.resize(BaselineTimesteps);
			String BS;
			GetSingleSelectedResultSeries(BaselineDataSet, BS, Baseline.data());
			BS << " baseline";
			
			timeseries_stats Stats = {};
			ComputeTimeseriesStats(Stats, Baseline.data(), Baseline.size(), BaselineStartDate);
			DisplayTimeseriesStats(Stats, BS);
			
			AddPlot(BS, PlotIdx, Baseline.data(), Baseline.size(), false, LogY, NormY, InputStartDate, BaselineStartDate, Stats.Min, Stats.Max);
			++PlotIdx;
			
			
			
			PlotData.push_back({});
			std::vector<double> &Current = PlotData[PlotIdx];
			Current.resize(ResultTimesteps);
			String CurrentLegend;
			GetSingleSelectedResultSeries(DataSet, CurrentLegend, Current.data());
			
			timeseries_stats Stats2 = {};
			ComputeTimeseriesStats(Stats2, Current.data(), Current.size(), ResultStartDate);
			DisplayTimeseriesStats(Stats2, CurrentLegend);
			
			AddPlot(CurrentLegend, PlotIdx, Current.data(), Current.size(), false, LogY, NormY, InputStartDate, ResultStartDate, Stats2.Min, Stats2.Max);
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
				GetSingleSelectedInputSeries(DataSet, InputLegend, Obs.data(), false);
				
				timeseries_stats Stats3 = {};
				ComputeTimeseriesStats(Stats3, Obs.data(), Obs.size(), InputStartDate);
				DisplayTimeseriesStats(Stats3, InputLegend);
				
				AddPlot(InputLegend, PlotIdx, Obs.data(), Obs.size(), Scatter, LogY, NormY, InputStartDate, InputStartDate, Stats3.Min, Stats3.Max);
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
			
			GetSingleSelectedResultSeries(DataSet, ModeledLegend, ModeledSeries.data());
			GetSingleSelectedInputSeries(DataSet, ObservedLegend, ObservedSeries.data(), true);
			
			for(size_t Idx = 0; Idx < ResultTimesteps; ++Idx)
			{
				Residuals[Idx] = ObservedSeries[Idx] - ModeledSeries[Idx];
			}
			
			String Legend = String("Residuals of ") + ModeledLegend + " vs " + ObservedLegend;
			
			timeseries_stats ModeledStats = {};
			ComputeTimeseriesStats(ModeledStats, ModeledSeries.data(), ModeledSeries.size(), ResultStartDate);
			DisplayTimeseriesStats(ModeledStats, ModeledLegend);
			
			timeseries_stats ObservedStats = {};
			ComputeTimeseriesStats(ObservedStats, ObservedSeries.data(), ObservedSeries.size(), ResultStartDate);
			DisplayTimeseriesStats(ObservedStats, ObservedLegend);
			
			residual_stats ResidualStats = {};
			ComputeResidualStats(ResidualStats, Residuals.data(), ObservedStats.Variance, Residuals.size(), ResultStartDate);
			String GOF = "Goodness of fit: ";
			DisplayResidualStats(ResidualStats, GOF);
			
			
			if(PlotMajorMode == MajorMode_Residuals)
			{
				bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get());
				//NOTE: Using the input start date as reference date is just so that we agree with the date formatting below.
				AddPlot(Legend, PlotIdx, Residuals.data(), ResultTimesteps, Scatter, false, false, InputStartDate, ResultStartDate);
				
				++PlotIdx;
				
				double XMean, XVar, XYCovar;
				ComputeTrendStats(Residuals.data(), Residuals.size(), ResidualStats.MeanError, XMean, XVar, XYCovar);
				
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
			
			Plot.cbModifFormatX << [InputStartDate](String &s, int i, double d)
			{
				Date D2 = InputStartDate + (int)d;
				s = Format(D2);
			};
		}
	}
	
	Plot.Refresh();
}

void MobiView::ReplotProfile()
{
	Plot.RemoveAllSeries();
	
	int Timestep = TimestepSlider.GetData();
	char Label[256];
	sprintf(Label, "Timestep: %d", Timestep);
	TimestepLabel.SetText(Label);
	
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


void MobiView::GetSingleSelectedResultSeries(void *DataSet, String &Legend, double *WriteTo)
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

void MobiView::GetSingleSelectedInputSeries(void *DataSet, String &Legend, double *WriteTo, bool AlignWithResults)
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


void MobiView::DisplayTimeseriesStats(timeseries_stats &Stats, String &Name)
{
	String Display = Name;
	
	Display << ":\n";
	Display << "min: " << FormatDouble(Stats.Min, 2) << "\n";
	Display << "max: " << FormatDouble(Stats.Max, 2) << "\n";
	Display << "sum: " << FormatDouble(Stats.Sum, 2) << "\n";
	Display << "mean: " << FormatDouble(Stats.Mean, 2) << "\n";
	Display << "std.dev.: " << FormatDouble(Stats.StandardDeviation, 2) << "\n";
	Display << "data points: " << Stats.DataPoints << "\n";
	Display << "\n";
	
	PlotInfo.Append(Display);
}

void MobiView::DisplayResidualStats(residual_stats &Stats, String &Name)
{
	String Display = Name;
	
	Display << "\n";
	Display << "Mean error (bias): "  << FormatDouble(Stats.MeanError, 2) << "\n";
	Display << "MAE: " << FormatDouble(Stats.MeanAbsoluteError, 2) << "\n";
	Display << "RMSE: " << FormatDouble(Stats.RootMeanSquareError, 2) << "\n";
	Display << "N-S: " << FormatDouble(Stats.NashSutcliffe, 2) << "\n";
	Display << "common data points: " << Stats.DataPoints << "\n";
	Display << "\n";
	
	PlotInfo.Append(Display);
}


void MobiView::ComputeTimeseriesStats(timeseries_stats &StatsOut, double *Data, size_t Len, Date &StartDate)
{
	double Min = DBL_MAX;
	double Max = DBL_MIN;
	
	double Sum = 0.0;
	size_t FiniteCount = 0;
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double Val = Data[Idx];
		if(std::isfinite(Val))
		{
			Min = std::min(Min, Val);
			Max = std::max(Max, Val);
			
			Sum += Val;
			
			++FiniteCount;
		}
	}
	
	double Mean = Sum / (double)FiniteCount;
	
	double Variance = 0.0;
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double Val = Data[Idx];
		if(std::isfinite(Val))
		{
			double Dev = Mean - Val;
			Variance += Dev*Dev;
		}
	}
	
	Variance /= (double)FiniteCount;
	
	StatsOut.Min = Min;
	StatsOut.Max = Max;
	StatsOut.Sum = Sum;
	StatsOut.Mean = Mean;
	StatsOut.Variance = Variance;
	StatsOut.StandardDeviation = std::sqrt(Variance);
	StatsOut.DataPoints = FiniteCount;
}


void MobiView::ComputeResidualStats(residual_stats &StatsOut, double *Residuals, double VarObs, size_t Len, Date &StartDate)
{
	double Sum = 0.0;
	double SumAbs = 0.0;
	double SumSquare = 0.0;
	size_t FiniteCount = 0;
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double Val = Residuals[Idx];
		if(std::isfinite(Val))
		{
			Sum += Val;
			SumAbs += std::abs(Val);
			SumSquare += Val*Val;
			
			++FiniteCount;
		}
	}
	
	double MeanError = Sum / (double)FiniteCount;
	
	double MeanSquareError = SumSquare / (double)FiniteCount;
	
	double NS = 1.0 - MeanSquareError / VarObs;
	
	StatsOut.MeanError = MeanError;
	StatsOut.MeanAbsoluteError = SumAbs / (double)FiniteCount;
	StatsOut.RootMeanSquareError = std::sqrt(MeanSquareError);
	StatsOut.NashSutcliffe = NS;
	StatsOut.DataPoints = FiniteCount;
}

void MobiView::ComputeTrendStats(double *YData, size_t Len, double MeanY, double &XMeanOut, double &XVarOut, double &XYCovarOut)
{
	double SumX = 0.0;
	size_t FiniteCount = 0;
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double YVal = YData[Idx];
		if(std::isfinite(YVal))
		{
			SumX += (double)Idx;
			FiniteCount++;
		}
	}
	
	double MeanX = SumX / (double)FiniteCount;
	
	double CovarAcc = 0.0;
	double XVarAcc = 0.0;
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double Val = YData[Idx];
		if(std::isfinite(Val))
		{
			double DevX = ((double)Idx - MeanX);
			CovarAcc += (Val - MeanY)*DevX;
			XVarAcc += DevX*DevX;
		}
	}
	
	XMeanOut = MeanX;
	XVarOut = XVarAcc / (double)FiniteCount;
	XYCovarOut = CovarAcc / (double)FiniteCount;
}


#endif
