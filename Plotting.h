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
	
	if(MajorMode == 0) //Regular timeseries plot
	{
		ScatterInputs.Enable();
		LogarithmicY.Enable();
		NormalizeY.Enable();
		TimeIntervals.Enable();
	}
	else if(MajorMode == 4)
	{
		ScatterInputs.Enable();
		TimeIntervals.Enable();
	}
	//else if //IMPLEMENTME
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
		}
	}
	
	//TODO: This could be more fine-grained. We don't have to update the index list every time.
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		EIndexList[Idx]->Disable();
	}
	
	int RowCount = InputSelecter.GetCount();
	int CursorRow = InputSelecter.GetCursor();
	
	for(int Row = 0; Row < RowCount; ++Row)
	{
		if(InputSelecter.IsSelected(Row) || Row == CursorRow) //Ugh. does not work properly when you deselect!
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
		int CursorRow = EquationSelecter.GetCursor();
		
		for(int Row = 0; Row < RowCount; ++Row)
		{
			if(EquationSelecter.IsSelected(Row) || Row == CursorRow) //Ugh. does not work properly when you deselect!
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
		Plot.AddSeries(XValues.data(), YValues.data(), XValues.size()).Legend(Legend).PlotStyle<StaggeredSeriesPlot>().NoMark().Fill(GraphColor).Stroke(1.0, GraphColor);
	}
	else
	{
		//TODO
	}
}


void MobiView::AddPlot(String &Legend, int PlotIdx, double *Data, int Timesteps, bool Scatter, bool LogY, bool NormalY, Date &ReferenceDate, Date &StartDate)
{
	//TODO: Also implement logarithmic Y axis and normalized Y axis.
	
	
	int IntervalType = TimeIntervals.GetData();
	int AggregationType = Aggregation.GetData();
	
	int ColorIdx = PlotIdx % PlotColors.size();
	Color &GraphColor = PlotColors[ColorIdx];
	
	//TODO: Do some initial trimming to get rid of NaNs at the head and tail of the data.
	ScatterDraw *Graph = nullptr;
	
	if(IntervalType == 0) //Daily values
	{
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
			Graph->NoMark().Stroke(1.5, GraphColor);
		}
	}
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
		int Len = (int)PlotData[PlotIdx].size();

		
		String Legend = Name.data();
		if(CurrentIndexes.size() > 0)
		{
			Legend << " {";
			for(size_t Idx = 0; Idx < CurrentIndexes.size(); ++Idx)
			{
				Legend << CurrentIndexes[Idx].data();
				if(Idx < CurrentIndexes.size()-1) Legend << ", ";
			}
			Legend << "}";
		}
		
		bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get() && Mode == 1);
		bool LogY = false; //TODO
		bool NormalY = false; //TODO
		AddPlot(Legend, PlotIdx, Data, Len, Scatter, LogY, NormalY, ReferenceDate, StartDate);
		
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
	
	Plot.RemoveAllSeries(); //TODO: We could see if we want to cache some series and not re-extract everything every time.
	PlotData.clear();
	AggregateX.clear();
	AggregateY.clear();
	
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
	
	if(PlotMajorMode == 0) //Regular timeseries plot
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
	else if(PlotMajorMode == 1) //Histogram
	{
		//UUUGH. GetSelectCount is also wrong because of that bug where the latest selection is
		//not counted..
		
		int TimeseriesCount = EquationSelecter.GetSelectCount() + InputSelecter.GetSelectCount();
		bool MultiIndex = false;
		for(size_t IndexSet = 0; IndexSet < MAX_INDEX_SETS; ++IndexSet)
		{
			if(EIndexList[IndexSet]->GetSelectCount() > 1)
			{
				MultiIndex = true;
				break;
			}
		}
		
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
		
			if(EquationSelecter.IsSelection() && ResultTimesteps != 0)
			{
				Data.resize(ResultTimesteps);
				
				GetSingleSelectedResultSeries(DataSet, Legend, Data.data());
			}
			else if(InputSelecter.IsSelection())
			{
				Data.resize(InputTimesteps);
				GetSingleSelectedInputSeries(DataSet, Legend, Data.data(), false);
			}
			
			AddHistogram(Legend, PlotIdx, Data.data(), Data.size());
			
			++PlotIdx;
		}
	}
	else if(PlotMajorMode == 2) //Profile
	{
		//IMPLEMENTME
	}
	else if(PlotMajorMode == 3) //Compare baseline
	{
		//IMPLEMENTME
	}
	else if(PlotMajorMode == 4 || PlotMajorMode == 5) //Residual or Residual histogram
	{
		bool MultiIndex = false;
		for(size_t IndexSet = 0; IndexSet < MAX_INDEX_SETS; ++IndexSet)
		{
			if(EIndexList[IndexSet]->GetSelectCount() > 1)
			{
				MultiIndex = true;
				break;
			}
		}
		
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
			
			if(PlotMajorMode == 4)
			{
				bool Scatter = (ScatterInputs.IsEnabled() && ScatterInputs.Get());
				//NOTE: Using the input start date as reference date is just so that we agree with the date formatting below.
				AddPlot(Legend, PlotIdx, Residuals.data(), ResultTimesteps, Scatter, false, false, InputStartDate, ResultStartDate);
				
				//TODO: Add in a trend too!
			}
			else //PlotMajorMode == 5
			{
				AddHistogram(Legend, PlotIdx, Residuals.data(), Residuals.size());
			}
			
			++PlotIdx;
		}
	}
	
	if(PlotIdx > 0)
	{
		Plot.cbModifFormatX.Clear();
		
		if(PlotMajorMode == 1 || PlotMajorMode == 5)
		{
			//NOTE: Histograms require completely different zooming.
			Plot.ZoomToFit(true, true);
			PlotWasAutoResized = false;
		}
		else
		{
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
		Legend << " {";
		for(size_t Idx = 0; Idx < Indexes.size(); ++Idx)
		{
			Legend << Indexes[Idx];
			if(Idx < Indexes.size()-1) Legend << ", ";
		}
		Legend << "}";
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
		Legend << " {";
		for(size_t Idx = 0; Idx < Indexes.size(); ++Idx)
		{
			Legend << Indexes[Idx];
			if(Idx < Indexes.size()-1) Legend << ", ";
		}
		Legend << "}";
	}
}

#endif
