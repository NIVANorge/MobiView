#include "MobiView.h"


#define IMAGECLASS IconImg6
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>

#include <fstream>

#include <thread>

#include "Acor.h"

/*

Possible TODO:

- Responsivity of halt button (or rather reintroduce one that works).
- More error structures (e.g. some more advanced hydrological ones).
- Fix duration timer (sometimes negative number).
- Make result format more robust (it can have encoding problems it seems, so doesn't allow
editing by hand).
- In Projection window, show multiple confidence intervals (if desired).
*/


MCMCProjectionCtrl::MCMCProjectionCtrl()
{
	CtrlLayout(*this);
}


MCMCResultWindow::MCMCResultWindow()
{
	CtrlLayout(*this);
	
	SetRect(0, 0, 1200, 900);
	Title("MobiView MCMC results").Sizeable().Zoomable();
	
	ViewResultSummary.Add(ResultSummary.HSizePos().VSizePos(0, 30));
	ViewResultSummary.Add(PushWriteMAP.SetLabel("MAP to main").LeftPos(0, 100).BottomPos(5, 25));
	ViewResultSummary.Add(PushWriteMedian.SetLabel("Median to main").LeftPos(105, 100).BottomPos(5, 25));
	PushWriteMAP.WhenPush = THISBACK(MAPToMainPushed);
	PushWriteMedian.WhenPush = THISBACK(MedianToMainPushed);
	
	
	ViewProjections.PushGenerate.WhenPush = THISBACK(GenerateProjectionsPushed);
	ViewProjections.EditSamples.Min(1);
	ViewProjections.EditSamples.SetData(1000);
	ViewProjections.GenerateProgress.Hide();

	ViewProjections.Confidence.MinMax(0.0, 100.0).SetData(95.0);
	ViewProjections.PlotSelectTab.Add(ProjectionPlotScroll.SizePos(), "Confidence interval");
	ViewProjections.PlotSelectTab.Add(ResidPlotScroll.SizePos(), "Median residuals");
	ViewProjections.PlotSelectTab.Add(AutoCorrPlotScroll.SizePos(), "Median residual autocorr.");
	

	ChoosePlotsTab.Add(ChainPlotScroller.SizePos(), "Chain plots");
	ChoosePlotsTab.Add(TrianglePlotScroller.SizePos(), "Triangle plot");
	ChoosePlotsTab.Add(ViewResultSummary.SizePos(), "Parameter distribution summary");
	ChoosePlotsTab.Add(ViewProjections.SizePos(), "Result projections");
	
	ChoosePlotsTab.WhenSet << [&](){ RefreshPlots(); };
	
	BurninSlider.WhenAction = THISBACK(BurninSliderEvent);
	BurninEdit.WhenAction   = THISBACK(BurninEditEvent);
	
	BurninSlider.SetData(0);
	BurninSlider.MinMax(0,1);
	
	BurninEdit.SetData(0);
	BurninEdit.Min(0);
	
	BurninEdit.Disable();
	BurninSlider.Disable();
	
	
	AddFrame(Tool);
	Tool.Set(THISBACK(SubBar));
	
	//PushHalt.WhenPush << [&](){ HaltWasPushed = true; };
	//PushHalt.SetImage(IconImg6::Remove());
}

void MCMCResultWindow::SubBar(Bar &bar)
{
	auto Load = [&](){ bool Success = LoadResults(); if(!Success) PromptOK("There was an error in the file format"); }; //TODO: Handle error?
	
	bar.Add(IconImg6::Open(), Load).Tip("Load data from file");
	bar.Add(IconImg6::Save(), THISBACK(SaveResults)).Tip("Save data to file");
}

void MCMCResultWindow::ClearPlots()
{
	for(ScatterCtrl &Plot : ChainPlots)
	{
		Plot.RemoveAllSeries();
		Plot.Remove();
	}
	
	ChainPlots.Clear();
	
	for(ScatterCtrl &Plot : TrianglePlots)
	{
		Plot.RemoveSurf();
		Plot.Remove();
	}
	
	TrianglePlots.Clear();
	TrianglePlotDS.Clear();
	TrianglePlotData.clear();
	
	for(ScatterCtrl &Plot : Histograms)
	{
		Plot.RemoveAllSeries();
		Plot.Remove();
	}
	
	Histograms.Clear();
	HistogramData.clear();
	
	ResultSummary.Clear();

	AcorTimes.clear();
	
	//Debatable whether this should clear target plots.
}

int FlattenData(mcmc_data *Data, int &CurStep, int Burnin, std::vector<std::vector<double>> &FlattenedOut, bool Sort=true)
{
	if(CurStep < 0) CurStep = Data->NSteps - 1;
	int NumSteps = CurStep - Burnin;
	if(NumSteps <= 0) return 0;
	
	FlattenedOut.resize(Data->NPars);
			
	int NumValues = NumSteps * Data->NWalkers;
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		FlattenedOut[Par].resize(NumValues);
		
		for(int Walker = 0; Walker < Data->NWalkers; ++Walker)
		{
			for(int Step = Burnin; Step < CurStep; ++Step)
				FlattenedOut[Par][Walker*NumSteps + Step-Burnin] = (*Data)(Walker, Par, Step);
		}
		
		if(Sort)
			std::sort(FlattenedOut[Par].begin(), FlattenedOut[Par].end());
	}
			
	return NumValues;
}

void MCMCResultWindow::RefreshPlots(int CurStep)
{
	if(!Data) return;
	
	int ShowPlot = ChoosePlotsTab.Get();
	
	bool LastStep = CurStep == -1 || (Data && CurStep==Data->NSteps-1);
	
	if(ShowPlot == 0)
	{
		// Chain plots
		for(ScatterCtrl &Plot : ChainPlots) Plot.Refresh();
	}
	else if(ShowPlot == 1)
	{
		// Triangle plots
		int BurninVal = (int)Burnin[0];
		
		std::vector<std::vector<double>> ParValues;
		int NumValues = FlattenData(Data, CurStep, BurninVal, ParValues, true);
		
		if(NumValues > 0)
		{
			double LowerQuant = 0.025;
			double UpperQuant = 0.975;
			
			int PlotIdx = 0;
			if(Data->NPars > 1)
			{
				for(int Par1 = 0; Par1 < Data->NPars; ++Par1)
				{
					std::vector<double> &Par1Data = ParValues[Par1];
					
					double Minx = QuantileOfSorted(Par1Data.data(), Par1Data.size(), LowerQuant);
					double Maxx = QuantileOfSorted(Par1Data.data(), Par1Data.size(), UpperQuant);
						
					double MedianX = MedianOfSorted(Par1Data.data(), Par1Data.size());
					
					double StrideX = (Maxx - Minx)/(double)(DistrResolution);
					
					histogram_data &Dat  = HistogramData[Par1];
					ScatterCtrl    &Plot = Histograms[Par1];
					
					double Scale = 1.0;
					
					for(int Idx = 0; Idx < DistrResolution+1; ++Idx)
					{
						Dat.DistrX[Idx] = Minx + StrideX*(double)Idx;
						Dat.DistrY[Idx] = 0.0;
					}
					
					for(int Walker = 0; Walker < Data->NWalkers; ++Walker)
					{
						for(int Step = BurninVal; Step <= CurStep; ++Step)
						{
							double ValX = (*Data)(Walker, Par1, Step);
							
							if(ValX < Minx || ValX > Maxx)
								continue;
							
							int XX = std::min((int)std::floor((ValX - Minx)/StrideX), DistrResolution-1);
							XX = std::max(0, XX);
							
							Dat.DistrY[XX] += Scale;
						}
					}
					
					Plot.ZoomToFit(true, true);
					Plot.SetMinUnits(0.0, 0.0);
					Plot.SetMajorUnits(Maxx-Minx, 1.0);
					Plot.SetMajorUnitsNum(Par1==Data->NPars-1 ? 1: 0, 0);
					Plot.BarWidth(0.5*StrideX);
					Plot.SetTitle(Format("%s = %.2f", FreeSyms[Par1], MedianX));
					Plot.Refresh();
					
					
					for(int Par2 = Par1+1; Par2 < Data->NPars; ++Par2)
					{
						triangle_plot_data &Dat  = TrianglePlotData[PlotIdx];
						TableDataCArray    &Ds   = TrianglePlotDS[PlotIdx];
						ScatterCtrl        &Plot = TrianglePlots[PlotIdx];
						
						
						std::vector<double> &Par2Data = ParValues[Par2];
						
						double Miny = QuantileOfSorted(Par2Data.data(), Par2Data.size(), LowerQuant);
						double Maxy = QuantileOfSorted(Par2Data.data(), Par2Data.size(), UpperQuant);
						
						double StrideY = (Maxy - Miny)/(double)(DistrResolution);
						
						//TODO : could be optimized by just doing the steps since last update.
						for(int Idx = 0; Idx < Dat.DistrZ.size(); ++Idx) Dat.DistrZ[Idx] = 0.0;
					
						//double Scale = 1.0 / (double)(Data->NWalkers * CurStep);
						double Scale = 1.0; // Since we ZoomToFitZ, it shouldn't matter what the scale of Z is.
						
						for(int Idx = 0; Idx < DistrResolution+1; ++Idx)
						{
							Dat.DistrX[Idx] = Minx + StrideX*(double)Idx;
							Dat.DistrY[Idx] = Miny + StrideY*(double)Idx;
						}
						
						for(int Walker = 0; Walker < Data->NWalkers; ++Walker)
						{
							for(int Step = BurninVal; Step <= CurStep; ++Step)
							{
								double ValX = (*Data)(Walker, Par1, Step);
								double ValY = (*Data)(Walker, Par2, Step);
								
								if(ValX < Minx || ValX > Maxx || ValY < Miny || ValY > Maxy)
									continue;
								
								int XX = std::min((int)std::floor((ValX - Minx)/StrideX), DistrResolution-1);
								int YY = std::min((int)std::floor((ValY - Miny)/StrideY), DistrResolution-1);
								XX = std::max(0, XX);
								YY = std::max(0, YY);
								
								Dat.DistrZ[XX + YY*DistrResolution] += Scale;
							}
						}
						
						Plot.SetXYMin(Minx, Miny);
						Plot.SetRange(Maxx-Minx, Maxy-Miny);
						Plot.SetMinUnits(0.0, 0.0);
						Plot.SetMajorUnits(Maxx-Minx, Maxy-Miny);
						Plot.SetMajorUnitsNum(Par2==Data->NPars-1 ? 1: 0, Par1==0 ? 1 : 0); //Annoyingly we have to reset this.
						
						Plot.ZoomToFitZ();
						Plot.Refresh();
						
						++PlotIdx;
					}
				}
			}
		}
	}
	else if(ShowPlot == 2)
	{
		RefreshResultSummary(CurStep);
	}
	
	
}

void MCMCResultWindow::ResizeChainPlots()
{
	int PlotCount = ChainPlots.size();
	
	int WinWidth  = GetRect().GetWidth()-50; //Allow for the scrollbar.
	
	int NumCols = 4;
	int NumRows = PlotCount / 4 + (int)(PlotCount % 4 != 0);
	
	int PlotWidth = WinWidth / NumCols;
	int PlotHeight = 160;

	for(int PlotIdx = 0; PlotIdx < PlotCount; ++PlotIdx)
	{
		int XX = PlotIdx % NumCols;
		int YY = PlotIdx / NumCols;
		
		ChainPlots[PlotIdx].LeftPos(XX*PlotWidth, PlotWidth);
		ChainPlots[PlotIdx].TopPos(YY*PlotHeight, PlotHeight);
	}
	
	int TotX = WinWidth;
	int TotY = PlotHeight*NumRows + 50;
	ChainPlotScroller.ClearPane();
	Size TriSize(TotX, TotY);
	ChainPlotScroller.AddPane(ViewChainPlots.LeftPos(0, TriSize.cx).TopPos(0, TriSize.cy), TriSize);
}

void MCMCResultWindow::BeginNewPlots(mcmc_data *Data, double *MinBound, double *MaxBound, const Array<String> &FreeSyms, int RunType)
{
	//HaltWasPushed = false;
	if(RunType == 1)  //NOTE: If we extend an earlier run, we keep the Burnin that was set.
	{
		BurninSlider.SetData(0);
		BurninEdit.SetData(0);
		Burnin[0] = 0; Burnin[1] = 0;
	}
	
	this->FreeSyms.clear();
	for(const String &Str : FreeSyms)
		this->FreeSyms << Str;
	
	this->MinBound.resize(Data->NPars);
	this->MaxBound.resize(Data->NPars);
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		this->MaxBound[Par] = MaxBound[Par];
		this->MinBound[Par] = MinBound[Par];
	}
	
	BurninSlider.Enable();
	BurninEdit.Enable();
	
	BurninSlider.MinMax(0, (int)Data->NSteps);
	BurninEdit.Max(Data->NSteps);
	
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		ChainPlots.Create<ScatterCtrl>();
		ViewChainPlots.Add(ChainPlots.Top());
	}
	
	Color GraphColor(0, 0, 0);
	Color BurninColor(255, 0, 0);
	
	BurninPlotY.resize(2*Data->NPars);
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		ScatterCtrl &Plot = ChainPlots[Par];
		Plot.SetFastViewX(true);
		Plot.SetSequentialXAll(true);
		
		Plot.SetMode(ScatterDraw::MD_ANTIALIASED);
		Plot.SetPlotAreaLeftMargin(35).SetPlotAreaBottomMargin(15).SetPlotAreaTopMargin(4).SetTitle(FreeSyms[Par]); //NOTE: Seems like title height is auto added to top margin.
		Plot.SetMouseHandling(true, false);
		Plot.SetTitleFont(Plot.GetTitleFont().Height(14));
		Plot.SetReticleFont(Plot.GetReticleFont().Height(8));
		
		for(int Walker = 0; Walker < Data->NWalkers; ++Walker)
			Plot.AddSeries(&(*Data)(Walker, Par, 0), Data->NSteps, 0.0, 1.0).ShowLegend(false).NoMark().Stroke(1.0, GraphColor).Dash("").Opacity(0.4);
		
		BurninPlotY[2*Par]     = MinBound[Par];
		BurninPlotY[2*Par + 1] = MaxBound[Par];
		
		Plot.AddSeries((double*)&Burnin[0], BurninPlotY.data()+2*Par, (int)2).ShowLegend(false).NoMark().Stroke(1.0, BurninColor).Dash("");
		
		Plot.SetXYMin(0.0, MinBound[Par]);
		Plot.SetRange((double)Data->NSteps, MaxBound[Par]-MinBound[Par]);
		
		if(Par > 0)
			Plot.LinkedWith(ChainPlots[0]);
	}
	
	ResizeChainPlots();
	
	
	this->Data = Data;
	
	if(Data->NPars > 1)
	{
		
		int PlotIdx = 0;
		int NumPlots = (Data->NPars*(Data->NPars - 1)) / 2;
		TrianglePlotData.resize(NumPlots);
		TrianglePlotDS.InsertN(0, NumPlots);
		TrianglePlots.InsertN(0, NumPlots);
		
		HistogramData.resize(Data->NPars);
		Histograms.InsertN(0, Data->NPars);
		
		int Dim            = 55; //Size of the plot area excluding margins
		int SmallmarginY   = 5;
		int SmallmarginX   = 15;
		int Largemargin    = 40;
		int DimX = 0;
		int DimY = 0;
		int SumDimX = 0;
		int SumDimY = 0;
		
		int TotX = Data->NPars*Dim + Largemargin + (2*Data->NPars - 1)*SmallmarginX + 50;
		int TotY = Data->NPars*Dim + Largemargin + (2*Data->NPars - 1)*SmallmarginY + 50;
		TrianglePlotScroller.ClearPane();
		Size TriSize(TotX, TotY);
		TrianglePlotScroller.AddPane(ViewTrianglePlots.LeftPos(0, TriSize.cx).TopPos(0, TriSize.cy), TriSize);
		
		
		for(int Par1 = 0; Par1 < Data->NPars; ++Par1)
		{
			//Add the histogram on the top of the column
			
			SumDimY = (Dim + 2*SmallmarginY)*Par1;
			
			histogram_data &Dat  = HistogramData[Par1];
			ScatterCtrl    &Plot = Histograms[Par1];
			
			double Minx = MinBound[Par1];
			double Maxx = MaxBound[Par1];
			
			double StrideX = (Maxx - Minx)/(double)(DistrResolution);
			
			Dat.DistrX.resize(DistrResolution+1);
			Dat.DistrY.resize(DistrResolution+1);
			
			for(int Idx = 0; Idx < DistrResolution+1; ++Idx)
			{
				Dat.DistrX[Idx] = Minx + StrideX*(double)Idx;
				Dat.DistrY[Idx] = 0.0;
			}
			
			Plot.AddSeries(Dat.DistrX.data(), Dat.DistrY.data(), DistrResolution+1).PlotStyle<BarSeriesPlot>()
				.BarWidth(0.5*StrideX).NoMark().Stroke(1.0, GraphColor).ShowLegend(false);
				
			Plot.SetXYMin(Minx, 0.0);
			Plot.SetRange(Maxx-Minx, 1.0);
			
			Plot.SetMinUnits(0.0, 0.0).SetMajorUnits(Maxx-Minx, 1.0);
			
			int hLeft          = SmallmarginX;
			int hRight         = SmallmarginX;
			int vTop           = SmallmarginY;   //NOTE title height is not included in this number, and is added on top of it.
			int vBottom        = SmallmarginY;
			if(Par1 == 0)
				hLeft = Largemargin;
			
			if(Par1 == Data->NPars-1)
			{
				vBottom = Largemargin;
				Plot.SetMajorUnitsNum(1, 0);
			}
			else
				Plot.SetMajorUnitsNum(0, 0);
			
			Plot.SetTitle(FreeSyms[Par1]);
			Plot.SetTitleFont(Plot.GetTitleFont().Bold(false).Height(8));
			Plot.SetReticleFont(Plot.GetReticleFont().Bold(false).Height(8));
			
			Plot.SetPlotAreaMargin(hLeft, hRight, vTop, vBottom);
			
			DimX = Dim+hLeft+hRight;
			DimY = Dim+vTop+vBottom + 10;  //NOTE: Extra +10 is because of added title, but this is hacky... Check how to compute exact height of title?
			
			ViewTrianglePlots.Add(Plot.LeftPos(SumDimX, DimX).TopPos(SumDimY, DimY));
			Plot.SetMouseHandling(false, false);
			
			SumDimY += DimY;
			
			// Add the 2d joint distributions
			
			for(int Par2 = Par1+1; Par2 < Data->NPars; ++Par2)
			{
				triangle_plot_data &Dat  = TrianglePlotData[PlotIdx];
				TableDataCArray    &Ds   = TrianglePlotDS[PlotIdx];
				ScatterCtrl        &Plot = TrianglePlots[PlotIdx];
				
				double Miny = MinBound[Par2];
				double Maxy = MaxBound[Par2];
				
				double StrideY = (Maxy - Miny)/(double)(DistrResolution);
				
				//TODO: Reuse the X axis from the histogram data instead!.
				Dat.DistrX.resize(DistrResolution+1);
				Dat.DistrY.resize(DistrResolution+1);
				for(int Idx = 0; Idx < DistrResolution+1; ++Idx)
				{
					Dat.DistrX[Idx] = Minx + StrideX*(double)Idx;
					Dat.DistrY[Idx] = Miny + StrideY*(double)Idx;
				}
				
				int Zsize = DistrResolution*DistrResolution;
				
				Dat.DistrZ.resize(Zsize);
				for(int Idx = 0; Idx < Zsize; ++Idx) Dat.DistrZ[Idx] = 0;
			
				Ds.Init(Dat.DistrZ.data(), Dat.DistrZ.size(), Dat.DistrX.data(), Dat.DistrX.size(), Dat.DistrY.data(), Dat.DistrY.size(), TableInterpolate::NO, true);
				
				Plot.AddSurf(Ds);
				Plot.SetXYMin(Minx, Miny).SetRange(Maxx-Minx, Maxy-Miny);
				Plot.ShowRainbowPalette(false);
				
				String Par1Str = Null;
				String Par2Str = Null;
				
				int MajorUnitsNumX = 0;
				int MajorUnitsNumY = 0;
				
				int hLeft          = SmallmarginX;
				int hRight         = SmallmarginX;
				int vTop           = SmallmarginY;
				int vBottom        = SmallmarginY;
				
				if(Par1 == 0)
				{
					Par2Str = FreeSyms[Par2];
					MajorUnitsNumY = 1;
					hLeft          = Largemargin;
				}
				
				if(Par2 == Data->NPars-1)
				{
					Par1Str = FreeSyms[Par1];
					MajorUnitsNumX = 1;
					vBottom        = Largemargin;
				}
				
				Plot.SetMinUnits(0.0, 0.0).SetMajorUnits(Maxx-Minx, Maxy-Miny);
				Plot.SetLabels(Par1Str, Par2Str);
				Plot.SetLabelsFont(Plot.GetLabelsFont().Bold(false).Height(8));
				Plot.SetMajorUnitsNum(MajorUnitsNumX, MajorUnitsNumY);
				
				Plot.SetPlotAreaMargin(hLeft, hRight, vTop, vBottom);
			
				Color GridColor(255, 255, 255);
				Plot.SetGridColor(GridColor);
				
				DimX = Dim+hLeft+hRight;
				DimY = Dim+vTop+vBottom;
				
				ViewTrianglePlots.Add(Plot.LeftPos(SumDimX, DimX).TopPos(SumDimY, DimY));
				Plot.SetMouseHandling(false, false);
				Plot.SurfRainbow(WHITE_BLACK);
				Plot.SetReticleFont(Plot.GetReticleFont().Bold(false).Height(8));
			
				SumDimY += DimY;
				
				++PlotIdx;
			}
			SumDimX += DimX;
		}
	}

}

void MCMCResultWindow::RefreshResultSummary(int CurStep)
{
	ResultSummary.Clear();
	
	if(!Data) return;
	
	int BurninVal = (int)Burnin[0];
	
	if(CurStep < 0) CurStep = Data->NSteps - 1;
		
	std::vector<std::vector<double>> ParValues;
	int NumValues = FlattenData(Data, CurStep, BurninVal, ParValues, false);
	
	if(NumValues <= 0) return;
	
	int BestW = -1;
	int BestS = -1;
	Data->GetMAPIndex(BurninVal, CurStep, BestW, BestS);
	
	
	int Precision = 3;//ParentWindow->StatSettings.Precision;
	
	std::vector<double> Means(Data->NPars);
	std::vector<double> StdDevs(Data->NPars);
	Array<String> Syms;
	
	double Acceptance = 100.0*(double)Data->NAccepted / (double)(Data->NWalkers*Data->NSteps);
	ResultSummary.Append(Format("Acceptance rate: %.2f%%&&", Acceptance));
	
	String Table = "{{1:1:1:1:1:1";
	for(double Perc : ParentWindow->StatSettings.Percentiles) Table << ":1";
	Table << " [* Name]:: [* Int.acor.T.]:: [* Mean]:: [* Median]:: [* MAP]:: [* StdDev]";
	for(double Perc : ParentWindow->StatSettings.Percentiles) Table << ":: " << FormatDouble(Perc, 1) << "%";
	
	bool ComputeAcor = false;
	if(CurStep == Data->NSteps-1 && AcorTimes.empty())
	{
		ComputeAcor = true;
		AcorTimes.resize(Data->NPars);
		AcorBelowTolerance.resize(Data->NPars);
	}
	
	//NOTE: These are parameters to the autocorrelation time computation
	int C=5, Tol=10; //TODO: these should probably be configurable..
	
	bool AnyBelowTolerance = false;
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		timeseries_stats Stats;
		ComputeTimeseriesStats(Stats, ParValues[Par].data(), ParValues[Par].size(), ParentWindow->StatSettings, false);
		
		String Sym = FreeSyms[Par];
		Sym.Replace("_", "`_");
		Syms << Sym;
		
		Means[Par] = Stats.Mean;
		StdDevs[Par] = Stats.StandardDev;
		
		
		
		bool BelowTolerance;
		double Acor;
		if(ComputeAcor)
		{
			Acor = IntegratedTime(Data, Par, 5, 10, &BelowTolerance);
			AcorTimes[Par] = Acor;
			AcorBelowTolerance[Par] = BelowTolerance;
		}
		else
		{
			Acor = AcorTimes[Par];
			BelowTolerance = AcorBelowTolerance[Par];
		}
		if(BelowTolerance) AnyBelowTolerance = true;
		
		
		String AcorStr = FormatDouble(Acor, Precision, FD_REL);
		if(BelowTolerance) AcorStr = Format("[@R `*%s]", AcorStr);
		
		Table
			<< ":: " << Sym
			<< ":: " << AcorStr
			<< ":: " << FormatDouble(Stats.Mean, Precision, FD_REL)
			<< ":: " << FormatDouble(Stats.Median, Precision, FD_REL)
			<< ":: " << (BestW>=0 && BestS>=0 ? FormatDouble((*Data)(BestW, Par, BestS), Precision, FD_REL) : "N/A")
			<< ":: " << FormatDouble(Stats.StandardDev, Precision, FD_REL);
		for(double Perc : Stats.Percentiles) Table << ":: " << FormatDouble(Perc, Precision, FD_REL);
	}
	Table << "}}&";
	if(AnyBelowTolerance) Table << Format("[@R `*The chain is shorter than %d times the integrated autocorrelation time for this parameter. Consider running for more steps.]&", Tol);
	Table << "[* Table 1:] Statistics about the posterior distribution of the parameters. MAP = most accurate prediction. Percents are percentiles of the distribution. Int.acor.T.=(estimated) integrated autocorrelation time.&&";

	ResultSummary.Append(Table);

	Table = "{{1:";
	for(int Par = 0; Par < Data->NPars-2; ++Par) Table << "1:";
	Table << "1 ";
	
	for(int Par = 0; Par < Data->NPars-1; ++Par)
		Table << ":: " << Syms[Par];
		
	for(int Par1 = 1; Par1 < Data->NPars; ++Par1)
	{
		Table << ":: " << Syms[Par1];
		for(int Par2 = 0; Par2 < Data->NPars-1; ++Par2)
		{
			Table << "::";
			if(Par2 >= Par1){ Table << "@W "; continue; }
			
			double Corr = 0.0;
			for(int Step = 0; Step < ParValues[0].size(); ++Step)
			{
				double Val1 = ParValues[Par1][Step];
				double Val2 = ParValues[Par2][Step];
				Corr += (Val1 - Means[Par1])*(Val2 - Means[Par2]);
			}
			Corr /= (StdDevs[Par1]*StdDevs[Par2]*(double)NumValues);
			
			int R = 0, B = 0;
			if(Corr > 0) R = (int)(255.0*Corr/2.0);
			if(Corr < 0) B = (int)(-255.0*Corr/2.0);
			Table << Format("@(%d.%d.%d) %.2f", 255-B, 255-R-B, 255-R, Corr);
		}
	}
	Table << "}}&[* Table 2:] Pearson correlation coefficients between the parameters in the posterior distribution.";

	ResultSummary.Append(Table);
}

void MCMCResultWindow::MAPToMainPushed()
{
	//TODO: Should check that parameter setup in optim. window still correspond to this
	//parameter data!
	
	if(!Data) return;
	
	int BestW = -1;
	int BestS = -1;
	Data->GetMAPIndex((int)Burnin[0], Data->NSteps-1, BestW, BestS);
	
	if(BestW < 0 || BestS < 0) return;
	
	std::vector<double> Pars(Data->NPars);
	for(int Par = 0; Par < Data->NPars; ++Par)
		Pars[Par] = (*Data)(BestW, Par, BestS);
	
	ParentWindow->OptimizationWin.SetParameterValues(ParentWindow->DataSet, Pars.data(), Pars.size(), Parameters);
	ParentWindow->Log("Wrote MCMC MAP parameters to main dataset.");
	ParentWindow->RunModel();
}

void MCMCResultWindow::MedianToMainPushed()
{
	//TODO: Should check that parameter setup in optim. window still correspond to this
	//parameter data!
	//TODO: This is redoing a lot of unnecessary work, but it may not be a problem..
	
	if(!Data) return;
	
	std::vector<std::vector<double>> ParValues;
	int CurStep = Data->NSteps-1;
	FlattenData(Data, CurStep, (int)Burnin[0], ParValues, false);
	
	std::vector<double> Pars(Data->NPars);
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		timeseries_stats Stats;
		ComputeTimeseriesStats(Stats, ParValues[Par].data(), ParValues[Par].size(), ParentWindow->StatSettings, false);
		
		Pars[Par] = Stats.Median;
	}
	
	ParentWindow->OptimizationWin.SetParameterValues(ParentWindow->DataSet, Pars.data(), Pars.size(), Parameters);
	ParentWindow->Log("Wrote MCMC median parameters to main dataset");
	ParentWindow->RunModel();
}


void MCMCResultWindow::BurninEditEvent()
{
	int Val = BurninEdit.GetData();
	
	if(Data)
		Val = std::min(Val, (int)Data->NSteps);
	
	Burnin[0] = (double)Val;
	Burnin[1] = (double)Val;
	
	BurninSlider.SetData(Val);
	
	if(Data)
		RefreshPlots();
}

void MCMCResultWindow::BurninSliderEvent()
{
	int Val = BurninSlider.GetData();
	
	if(Data)
		Val = std::min(Val, (int)Data->NSteps);
	
	Burnin[0] = (double)Val;
	Burnin[1] = (double)Val;
	
	BurninEdit.SetData(Val);
	
	if(Data)
		RefreshPlots();
}

void MCMCResultWindow::GenerateProjectionsPushed()
{
	if(!Data) return;
	
	int NSamples   = ViewProjections.EditSamples.GetData();
	double Conf = ViewProjections.Confidence.GetData();
	double MinConf = 0.5*(100.0-Conf);
	double MaxConf = 100.0-MinConf;
	bool ParametricOnly = ViewProjections.OptionUncertainty.GetData();
	
	for(MyPlot &Plot : ProjectionPlots)
	{
		Plot.ClearAll(false);
		Plot.Remove();
	}
	ProjectionPlots.Clear();
	ProjectionPlots.InsertN(0, Targets.size());
	
	for(MyPlot &Plot : ResidPlots)
	{
		Plot.ClearAll(false);
		Plot.Remove();
	}
	ResidPlots.Clear();
	ResidPlots.InsertN(0, Targets.size());
	
	for(MyPlot &Plot : ResidHistograms)
	{
		Plot.ClearAll(false);
		Plot.Remove();
	}
	ResidHistograms.Clear();
	ResidHistograms.InsertN(0, Targets.size());
	
	for(MyPlot &Plot : AutoCorrPlots)
	{
		Plot.ClearAll(false);
		Plot.Remove();
	}
	AutoCorrPlots.Clear();
	AutoCorrPlots.InsertN(0, Targets.size());
	
	
	ProjectionPlotScroll.ClearPane();
	ResidPlotScroll.ClearPane();
	AutoCorrPlotScroll.ClearPane();
	
	int HSize = ProjectionPlotScroll.GetRect().Width() - 30;  // -30 is to make space for scrollbar
	
	int PlotHeight = 400;
	int AccumY = 0;
	
	int TargetIdx = 0;
	for(optimization_target &Target : Targets)
	{
		int HSizeSmall = (int)(0.60*(double)HSize);
		
		ProjectionPlotPane.Add(ProjectionPlots[TargetIdx].LeftPos(0, HSize).TopPos(AccumY, PlotHeight));
		ResidPlotPane.Add     (ResidPlots[TargetIdx].LeftPos(0, HSizeSmall).TopPos(AccumY, PlotHeight));
		ResidPlotPane.Add     (ResidHistograms[TargetIdx].LeftPos(HSizeSmall, HSize-HSizeSmall).TopPos(AccumY, PlotHeight));
		AutoCorrPlotPane.Add  (AutoCorrPlots[TargetIdx].LeftPos(0, HSize).TopPos(AccumY, PlotHeight));
		AccumY += PlotHeight;
		++TargetIdx;
	}
	Size PaneSz(HSize, AccumY);
	ProjectionPlotScroll.AddPane(ProjectionPlotPane.LeftPos(0, HSize).TopPos(0, AccumY), PaneSz);
	ResidPlotScroll.AddPane(ResidPlotPane.LeftPos(0, HSize).TopPos(0, AccumY), PaneSz);
	AutoCorrPlotScroll.AddPane(AutoCorrPlotPane.LeftPos(0, HSize).TopPos(0, AccumY), PaneSz);
	
	ViewProjections.GenerateProgress.Show();
	ViewProjections.GenerateProgress.Set(0, NSamples);
	
	int BurninVal = (int)Burnin[0];
	
	std::vector<std::vector<double>> ParValues;
	int CurStep = -1;
	int NumValues = FlattenData(Data, CurStep, BurninVal, ParValues, false);
	
	
	
	void *DataSet = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false);
	
	ParentWindow->ModelDll.RunModel(DataSet); //NOTE: One initial run so that everything is set up.
	
	char TimeStr[256];
	uint64 ResultTimesteps = ParentWindow->ModelDll.GetTimesteps(DataSet);
	ParentWindow->ModelDll.GetStartDate(DataSet, TimeStr);
	Time ResultStartTime;
	StrToTime(ResultStartTime, TimeStr);
	
	std::vector<std::vector<std::vector<double>>> DataBlock;
	DataBlock.resize(Targets.size());
	for(int Target = 0; Target < Targets.size(); ++Target)
	{
		DataBlock[Target].resize(NSamples+1);   //NOTE: the final +1 is to make space for a run with the median parameter set
		for(int Sample = 0; Sample < NSamples+1; ++Sample)
			DataBlock[Target][Sample].resize(ResultTimesteps);
	}

	
	
	std::vector<plot_setup> PlotSetups;
	ParentWindow->OptimizationWin.TargetsToPlotSetups(Targets, PlotSetups);
	
#define MCMC_SAMPLE_PARALELLIZE


#ifdef MCMC_SAMPLE_PARALELLIZE
	
	Array<AsyncWork<void>> Workers;
	auto NThreads = std::thread::hardware_concurrency();
	int NWorkers = std::max(32, (int)NThreads);
	Workers.InsertN(0, NWorkers);
	auto *DataBlockPtr = &DataBlock;
	
	std::vector<std::mt19937_64> Generators(NWorkers);
	std::vector<void *> DataSets(NWorkers);
	DataSets[0] = DataSet;
	for(int Worker = 1; Worker < NWorkers; ++Worker)
		DataSets[Worker] = ParentWindow->ModelDll.CopyDataSet(DataSet, false);
	
	for(int SuperSample = 0; SuperSample < NSamples/NWorkers+1; ++SuperSample)
	{
		for(int Worker = 0; Worker < NWorkers; ++Worker)
		{
			int Sample = SuperSample*NWorkers + Worker;
			if(Sample >= NSamples) break;
			
			int NPars = Data->NPars;
			Workers[Worker].Do([=, &Generators, &PlotSetups, &ParValues, &DataSets]()
			{
				std::vector<double> Pars(NPars);
				
				std::uniform_int_distribution<int> Dist(0, NumValues);
				int Draw = Dist(Generators[Worker]);
				for(int Par = 0; Par < NPars; ++Par) Pars[Par] = ParValues[Par][Draw];
				
				ParentWindow->OptimizationWin.SetParameterValues(DataSets[Worker], Pars.data(), Pars.size(), Parameters);
			
				ParentWindow->ModelDll.RunModel(DataSets[Worker]);
				
				for(int TargetIdx = 0; TargetIdx < Targets.size(); ++TargetIdx)
				{
					double *ResultYValues = (*DataBlockPtr)[TargetIdx][Sample].data();
					String Legend;
					String Unit;
					ParentWindow->GetSingleSelectedResultSeries(PlotSetups[TargetIdx], DataSets[Worker], PlotSetups[TargetIdx].SelectedResults[0], Legend, Unit, ResultYValues);
					if(!ParametricOnly)
					{
						optimization_target &Target = Targets[TargetIdx];
						std::vector<double> ErrParam(Target.ErrParNum.size());
						for(int Idx = 0; Idx < ErrParam.size(); ++Idx) ErrParam[Idx] = Pars[Target.ErrParNum[Idx]];
						AddRandomError(ResultYValues, ResultTimesteps, ErrParam, Target.ErrStruct, Generators[Worker]);
					}
				}
			});
		}
		for(auto &Worker : Workers) Worker.Get();
		
		ViewProjections.GenerateProgress.Set(std::min((SuperSample+1)*NWorkers-1, NSamples));
		if(SuperSample % 8 == 0)
			ParentWindow->ProcessEvents();
	}
	
	for(int Worker = 1; Worker < NWorkers; ++Worker)
		ParentWindow->ModelDll.DeleteDataSet(DataSets[Worker]);
	
#else
	std::mt19937_64 Generator;
	
	for(int Sample = 0; Sample < NSamples; ++Sample)
	{
		std::vector<double> Pars(Data->NPars);
		
		std::uniform_int_distribution<int> Dist(0, NumValues);
		int Draw = Dist(Generator);
		for(int Par = 0; Par < Data->NPars; ++Par) Pars[Par] = ParValues[Par][Draw];
		
		ParentWindow->OptimizationWin.SetParameterValues(DataSet, Pars.data(), Pars.size(), Parameters);
	
		ParentWindow->ModelDll.RunModel(DataSet);
		
		for(int TargetIdx = 0; TargetIdx < Targets.size(); ++TargetIdx)
		{
			double *ResultYValues = DataBlock[TargetIdx][Sample].data();
			String Legend;
			String Unit;
			ParentWindow->GetSingleSelectedResultSeries(PlotSetups[TargetIdx], DataSet, PlotSetups[TargetIdx].SelectedResults[0], Legend, Unit, ResultYValues);
			if(!ParametricOnly)
			{
				optimization_target &Target = Targets[TargetIdx];
				std::vector<double> ErrParam(Target.ErrParNum.size());
				for(int Idx = 0; Idx < ErrParam.size(); ++Idx) ErrParam[Idx] = Pars[Target.ErrParNum[Idx]];
				AddRandomError(ResultYValues, ResultTimesteps, ErrParam, Target.ErrStruct, Generator);
			}
		}
			
		ViewProjections.GenerateProgress.Set(Sample, NSamples);
		if(Sample % 50 == 0)
			ParentWindow->ProcessEvents();
	}
#endif
#undef MCMC_SAMPLE_PARALELLIZE

	std::vector<double> Pars(Data->NPars);
	//Run once with the median parameter set too.
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		timeseries_stats Stats;
		ComputeTimeseriesStats(Stats, ParValues[Par].data(), ParValues[Par].size(), ParentWindow->StatSettings, false);
		
		Pars[Par] = Stats.Median;
	}
	ParentWindow->OptimizationWin.SetParameterValues(DataSet, Pars.data(), Pars.size(), Parameters);
	ParentWindow->ModelDll.RunModel(DataSet);
	for(int TargetIdx = 0; TargetIdx < Targets.size(); ++TargetIdx)
	{
		double *ResultYValues = DataBlock[TargetIdx][NSamples].data();
		String Legend;
		String Unit;
		ParentWindow->GetSingleSelectedResultSeries(PlotSetups[TargetIdx], DataSet, PlotSetups[TargetIdx].SelectedResults[0], Legend, Unit, ResultYValues);
	}
	
	
	
	std::vector<double> Buffer(NSamples);
	for(int TargetIdx = 0; TargetIdx < Targets.size(); ++TargetIdx)
	{
		optimization_target &Target = Targets[TargetIdx];
		
		MyPlot &Plot = ProjectionPlots[TargetIdx];
		if(TargetIdx != 0) Plot.LinkedWith(ProjectionPlots[0]);
		Plot.PlotSetup = PlotSetups[TargetIdx];
		Plot.PlotSetup.ScatterInputs = true;
		
		double *ResultXValues = Plot.PlotData.Allocate(ResultTimesteps).data();
		ComputeXValues(ResultStartTime, ResultStartTime, ResultTimesteps, ParentWindow->TimestepSize, ResultXValues);
		
		double *InputYValues  = Plot.PlotData.Allocate(ResultTimesteps).data();
		
		String Legend;
		String Unit;
		Color GraphColor = Plot.PlotColors.Next();
		ParentWindow->GetSingleSelectedInputSeries(Plot.PlotSetup, DataSet, Plot.PlotSetup.SelectedInputs[0], Legend, Unit, InputYValues, true);
		Legend = "Observed";
		Plot.AddPlot(Legend, Unit, ResultXValues, InputYValues, ResultTimesteps, true, ResultStartTime, ResultStartTime, ParentWindow->TimestepSize, 0.0, 0.0, GraphColor);
		
		double *UpperYValues  = Plot.PlotData.Allocate(ResultTimesteps).data();
		double *MedianYValues = Plot.PlotData.Allocate(ResultTimesteps).data();
		double *LowerYValues  = Plot.PlotData.Allocate(ResultTimesteps).data();
		
		int NumObs = 0;
		int Coverage = 0;
		for(int Ts = 0; Ts < ResultTimesteps; ++Ts)
		{
			for(int Sample = 0; Sample < NSamples; ++Sample)
				Buffer[Sample] = DataBlock[TargetIdx][Sample][Ts];
			
			std::sort(Buffer.begin(), Buffer.end());
	
			UpperYValues[Ts]  = QuantileOfSorted(Buffer.data(), Buffer.size(), MaxConf*0.01);
			MedianYValues[Ts] = MedianOfSorted(Buffer.data(), Buffer.size());
			LowerYValues[Ts]  = QuantileOfSorted(Buffer.data(), Buffer.size(), MinConf*0.01);
			
			if(std::isfinite(InputYValues[Ts]))
			{
				++NumObs;
				if(InputYValues[Ts] >= LowerYValues[Ts] && InputYValues[Ts] <= UpperYValues[Ts]) ++Coverage;
			}
		}
		
		String Unit2 = Null;
		
		Legend = Format("%.2f percentile", MaxConf);
		GraphColor = Plot.PlotColors.Next();
		Plot.AddPlot(Legend, Unit2, ResultXValues, UpperYValues, ResultTimesteps, false, ResultStartTime, ResultStartTime, ParentWindow->TimestepSize, 0.0, 0.0, GraphColor);
		
		Legend = "Median";
		GraphColor = Plot.PlotColors.Next();
		Plot.AddPlot(Legend, Unit2, ResultXValues, MedianYValues, ResultTimesteps, false, ResultStartTime, ResultStartTime, ParentWindow->TimestepSize, 0.0, 0.0, GraphColor);
		
		Legend = Format("%.2f percentile", MinConf);
		GraphColor = Plot.PlotColors.Next();
		Plot.AddPlot(Legend, Unit2, ResultXValues, LowerYValues, ResultTimesteps, false, ResultStartTime, ResultStartTime, ParentWindow->TimestepSize, 0.0, 0.0, GraphColor);
		
		double CoveragePercent = 100.0*(double)Coverage/(double)NumObs;
		Plot.SetTitle(Format("\"%s\"  Coverage: %.2f%%", Target.ResultName.data(), CoveragePercent));
		
		Plot.FormatAxes(MajorMode_Regular, 0, ResultStartTime, ParentWindow->TimestepSize);
		Plot.SetLabelY(" ");
		Plot.Refresh();
		
		SetBetterGridLinePositions(Plot, 1);
		


		MyPlot &ResidPlot = ResidPlots[TargetIdx];
		
		//NOTE: This is the Y values of the median parameter set, not to be confused with the
		//median of the Y values over all the parameter sets.
		double *YValuesOfMedian = DataBlock[TargetIdx][NSamples].data();
		
		// Compute standardized residuals
		
		//Ooops, it is important that Pars still hold the median parameter set here.. This is
		//not clean code since somebody could inadvertently mess that up above.
		std::vector<double> ErrPar(Target.ErrParNum.size());
		for(int Idx = 0; Idx < ErrPar.size(); ++Idx) ErrPar[Idx] = Pars[Target.ErrParNum[Idx]];
	
		std::vector<double> &YValuesStored   = ResidPlot.PlotData.Allocate(0);
		YValuesStored.reserve(ResultTimesteps);
		
		//NOTE: if there is no input value, the residual value for this timestep will not be recorded below
		for(int Ts = 0; Ts < ResultTimesteps; ++Ts)
			if(std::isfinite(InputYValues[Ts])) YValuesStored.push_back(YValuesOfMedian[Ts]);
		
		std::vector<double> &ResidualYValues = ResidPlot.PlotData.Allocate(0);
		ResidualYValues.reserve(ResultTimesteps);
		ComputeStandardizedResiduals(InputYValues, YValuesOfMedian, ResultTimesteps, ErrPar, Target.ErrStruct, ResidualYValues);
		
		assert(YValuesStored.size() == ResidualYValues.size());
		
		GraphColor = ResidPlot.PlotColors.Next();
		ResidPlot.SetSequentialXAll(false);
		ResidPlot.SetTitle(Target.ResultName.data());
		ResidPlot.AddSeries(YValuesStored.data(), ResidualYValues.data(), YValuesStored.size()).Stroke(0.0, GraphColor).MarkColor(GraphColor).MarkStyle<CircleMarkPlot>();
		ResidPlot.SetLabelX("Simulated").SetLabelY("Standard residual").ShowLegend(false);
		ResidPlot.ZoomToFit(true, true);
		SetBetterGridLinePositions(ResidPlot, 0);
		SetBetterGridLinePositions(ResidPlot, 1);
		
		MyPlot &ResidHistogram = ResidHistograms[TargetIdx];
		ResidHistogram.SetTitle("St. resid. distr.");
		ResidHistogram.AddHistogram(Null, Null, ResidualYValues.data(), ResidualYValues.size());
		ResidHistogram.ZoomToFit(true, true);
		ResidHistogram.SetMouseHandling(false, false);
		ResidHistogram.ShowLegend(false);
		//TODO: Format axes better
		
		
		
		MyPlot &AcorPlot = AutoCorrPlots[TargetIdx];
		
		//TODO: It is debatable how much the auto-correlation makes sense when there is hole in
		//the data. Can be fixed with more sophisticated error structures

		std::vector<double> &Acor = AcorPlot.PlotData.Allocate(ResidualYValues.size());
		NormalizedAutocorrelation1D(ResidualYValues, Acor);
		
		GraphColor = Color(0, 0, 0);
		AcorPlot.AddSeries(Acor.data()+1, Acor.size()-1, 0.0, 1.0).NoMark().Stroke(1.5, GraphColor).Dash("").Legend("Autocorrelation of standardized residual");
		
		double ConfP = Conf/100.0;
		double Z = 0.5*std::erfc(0.5*(1.0 - ConfP)/(std::sqrt(2.0))); //NOTE: This is just the cumulative distribution function of the normal distribution
		Z /= std::sqrt((double)Acor.size());

		double *Lines = AcorPlot.PlotData.Allocate(4).data();
		Lines[0] = Lines[1] = Z;
		Lines[2] = Lines[3] = -Z;
		AcorPlot.AddSeries(Lines, 2, 0.0, (double)Acor.size()).NoMark().Stroke(1.5, GraphColor).Dash(LINE_DASHED).Legend(Format("%.2f percentile", MaxConf));
		AcorPlot.AddSeries(Lines+2, 2, 0.0, (double)Acor.size()).NoMark().Stroke(1.5, GraphColor).Dash(LINE_DASHED).Legend(Format("%.2f percentile", MinConf));
		
		AcorPlot.ZoomToFit(true, false).SetMouseHandling(false, true);
		AcorPlot.SetXYMin(Null, -1.0).SetRange(Null, 2.0);
		SetBetterGridLinePositions(AcorPlot, 0);
		SetBetterGridLinePositions(AcorPlot, 1);
		
		//AcorPlot.WhenZoomScroll << [&](){ AcorPlot.SetBetterGridLinePositions(1); }; // This
		//somehow screws up the center of the plot.
		
		AcorPlot.SetTitle(Format("\"%s\"", Target.ResultName.data()));
		AcorPlot.SetLabelY("Autocorrelation");
		AcorPlot.SetLabelX("Lag");
		
		AcorPlot.Refresh();
	}
	
	ParentWindow->ModelDll.DeleteDataSet(DataSet);
	
	ViewProjections.GenerateProgress.Hide();
}



















void MCMCResultWindow::SaveResults()
{
	if(!Data) return;
	
	FileSel Sel;
	Sel.Type("MCMC results", "*.mcmc");
	
	if(!ParentWindow->ParameterFile.empty())
		Sel.ActiveDir(GetFileFolder(ParentWindow->ParameterFile.data()));
	else
		Sel.ActiveDir(GetCurrentDirectory());
	
	Sel.ExecuteSaveAs();
	std::string Filename = Sel.Get().ToStd();
	
	if(Filename.size() == 0) return;
	
	std::ofstream File(Filename.data());
	
	if(File.fail()) return;
	
	File << Data->NPars << " " << Data->NWalkers << " " << Data->NSteps << " " << (int)BurninEdit.GetData() << " " << Data->NAccepted << "\n";
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		String &ParName = FreeSyms[Par];
		
		File << ParName.ToStd() << " " << MinBound[Par] << " " << MaxBound[Par] << "\n";
		
		for(int Step = 0; Step < Data->NSteps; ++Step)
		{
			for(int Walker = 0; Walker < Data->NWalkers; ++Walker)
				File << (*Data)(Walker, Par, Step) << "\t";
			File << "\n";
		}
	}
	File << "logprob\n";
	for(int Step = 0; Step < Data->NSteps; ++Step)
	{
		for(int Walker = 0; Walker  < Data->NWalkers; ++Walker)
			File << Data->LLValue(Walker, Step) << "\t";
		File << "\n";
	}
	
	String JsonData = ParentWindow->OptimizationWin.SaveToJsonString();
	File << JsonData.ToStd();
	
	File.close();
	
	ParentWindow->Log(Format("MCMC data saved to %s", Filename.data()));
}

bool MCMCResultWindow::LoadResults()
{
	//NOTE: Error handling is very rudimentary for now.
	
	if(!Data) Data = &ParentWindow->OptimizationWin.Data; //TODO: Maybe this window should own the data block instead..
	
	FileSel Sel;
	Sel.Type("MCMC results", "*.mcmc");
	
	if(!ParentWindow->ParameterFile.empty())
		Sel.ActiveDir(GetFileFolder(ParentWindow->ParameterFile.data()));
	else
		Sel.ActiveDir(GetCurrentDirectory());
	
	Sel.ExecuteOpen();
	std::string Filename = Sel.Get().ToStd();
	
	if(Filename.size() == 0) return false;
	
	std::ifstream File(Filename.data(), std::ifstream::in);
	
	if(File.fail()) return false;
	
	ClearPlots();
	
	std::string Line;
	std::getline(File, Line);
	if(File.eof() || File.bad() || File.fail()) return false;
	std::stringstream LL(Line, std::ios_base::in);
	
	int NPars, NWalkers, NSteps, BurninVal, NAccepted;
	LL >> NPars;
	LL >> NWalkers;
	LL >> NSteps;
	LL >> BurninVal;
	LL >> NAccepted;
	if(LL.bad() || LL.fail()) return false;
	
	//NOTE: we need to have locals of these and pass them to BeginNewPlots, otherwise they will
	//be cleared inside BeginNewPlots at the same time as it reads them. TODO: should pack
	//these inside the Data struct instead.
	std::vector<double> MinBound(NPars);
	std::vector<double> MaxBound(NPars);
	Array<String> FreeSyms;
	
	Data->Allocate(NWalkers, NPars, NSteps);
	Data->NAccepted = NAccepted;
	
	for(int Par = 0; Par < NPars; ++Par)
	{
		std::getline(File, Line);
		if(File.eof() || File.bad() || File.fail()) return false;
		std::stringstream LL(Line, std::ios_base::in);
		
		std::string Sym;
		double Min, Max;
		LL >> Sym;
		LL >> Min;
		LL >> Max;
		
		if(LL.bad() || LL.fail()) return false;
		
		FreeSyms.push_back(Sym.data());
		MinBound[Par] = Min;
		MaxBound[Par] = Max;
		
		for(int Step = 0; Step < NSteps; ++Step)
		{
			std::getline(File, Line);
			if(File.bad() || File.fail()) return false;
			std::stringstream LL(Line, std::ios_base::in);
			
			for(int Walker = 0; Walker < NWalkers; ++Walker)
			{
				double Val;
				LL >> Val;
				if(LL.bad() || LL.fail()) return false;
				
				(*Data)(Walker, Par, Step) = Val;
			}
		}
	}
	
	std::getline(File, Line);
	if(File.bad() || File.fail()) return false;
	if(Line != "logprob") return false;
	for(int Step = 0; Step < NSteps; ++Step)
	{
		std::getline(File, Line);
		if(File.bad() || File.fail()) return false;
		std::stringstream LL(Line, std::ios_base::in);
		
		for(int Walker = 0; Walker < NWalkers; ++Walker)
		{
			double Val;
			LL >> Val;
			if(LL.bad() || LL.fail()) return false;
			
			Data->LLValue(Walker, Step) = Val;
		}
	}
	
	std::string JsonData(std::istreambuf_iterator<char>(File), {});
	String JsonData2(JsonData.data());
	ParentWindow->OptimizationWin.LoadFromJsonString(JsonData2);
	ParentWindow->OptimizationWin.MCMCSetup.PushExtendRun.Enable();
	ParentWindow->OptimizationWin.ErrSymFixup();
	
	Parameters = ParentWindow->OptimizationWin.Parameters;
	Targets    = ParentWindow->OptimizationWin.Targets;

	BeginNewPlots(Data, MinBound.data(), MaxBound.data(), FreeSyms, 1);
	
	BurninSlider.SetData(BurninVal);
	BurninEdit.SetData(BurninVal);
	Burnin[0] = (double)BurninVal; Burnin[1] = (double)BurninVal;
	
	RefreshPlots();
	
	ParentWindow->Log(Format("MCMC data loaded from %s", Filename.data()));
	
	return true;
}

