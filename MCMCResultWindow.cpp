#include "MobiView.h"


#define IMAGECLASS IconImg6
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>

#include <fstream>


MCMCResultWindow::MCMCResultWindow()
{
	CtrlLayout(*this);
	
	SetRect(0, 0, 1200, 900);
	Title("MobiView MCMC results").Sizeable().Zoomable();
	
	ChoosePlotsTab.Add(ViewChainPlots.SizePos(), "Chain plots");
	ChoosePlotsTab.Add(ViewTrianglePlots.SizePos(), "Triangle plot");
	ChoosePlotsTab.WhenSet << [&](){ RefreshPlots(); };
	
	BurninSlider.WhenAction = THISBACK(BurninSliderEvent);
	BurninEdit.WhenAction   = THISBACK(BurninEditEvent);
	
	BurninSlider.SetData(0);
	BurninSlider.MinMax(0,1);
	
	BurninEdit.SetData(0);
	BurninEdit.Min(0);
	
	BurninEdit.Disable();
	BurninSlider.Disable();
	
	//TODO: This is stupid...
	BurninPlotY[0] = -1e6;
	BurninPlotY[1] =  1e6;
	
	AddFrame(Tool);
	Tool.Set(THISBACK(SubBar));
}

void MCMCResultWindow::SubBar(Bar &bar)
{
	//bar.Add(IconImg6::Open(), THISBACK(LoadFromJson)).Tip("Load setup from file");
	bar.Add(IconImg6::Save(), THISBACK(SaveResults)).Tip("Save setup to file");
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
}

void MCMCResultWindow::RefreshPlots(int CurStep)
{
	if(!Data) return;
	
	int ShowPlot = ChoosePlotsTab.Get();
	

	
	if(ShowPlot == 0)
	{
		// Chain plots
		for(ScatterCtrl &Plot : ChainPlots) Plot.Refresh();
	}
	else if(ShowPlot == 1)
	{
	
		// Triangle plots
		if(CurStep < 0) CurStep = Data->NSteps-1;
		
		int BurninVal = Burnin[0];
		
		int NumSteps = CurStep - BurninVal;
		if(NumSteps > 0)
		{
			std::vector<std::vector<double>> ParValues;
			ParValues.resize(Data->NPars);
			
			int NumValues = NumSteps * Data->NWalkers;
			int LowerQuant = (int)((double)NumValues * 0.025);
			int UpperQuant = (int)((double)NumValues * 0.975);
			
			for(int Par = 0; Par < Data->NPars; ++Par)
			{
				ParValues[Par].resize(NumValues);
				
				for(int Walker = 0; Walker < Data->NWalkers; ++Walker)
				{
					for(int Step = BurninVal; Step < CurStep; ++Step)
						ParValues[Par][Walker*NumSteps + Step-BurninVal] = (*Data)(Walker, Par, Step);	
				}
				
				std::sort(ParValues[Par].begin(), ParValues[Par].end());
			}
			
			
			
			
			int PlotIdx = 0;
			if(Data->NPars > 1)
			{
				for(int Par1 = 0; Par1 < Data->NPars; ++Par1)
				{
					std::vector<double> &Par1Data = ParValues[Par1];
					
					double Minx = Par1Data[LowerQuant];
					double Maxx = Par1Data[UpperQuant];
					double MedianX = Par1Data[Par1Data.size()/2]; //TODO: Make more precise.
					
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
						
						
						double Miny = Par2Data[LowerQuant];
						double Maxy = Par2Data[UpperQuant];
						
						
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
}

void MCMCResultWindow::ResizePlots()
{
	int PlotCount = ChainPlots.size();
	
	int WinWidth  = GetRect().GetWidth();
	int WinHeight = GetRect().GetHeight();
	
	int NumCols = 4;
	
	int PlotWidth = WinWidth / NumCols;
	//int PlotHeight = (3 * WinHeight) / PlotCount - 50;   //TODO: Fix this
	int PlotHeight = 160;

	for(int PlotIdx = 0; PlotIdx < PlotCount; ++PlotIdx)
	{
		int XX = PlotIdx % NumCols;
		int YY = PlotIdx / NumCols;
		
		ChainPlots[PlotIdx].LeftPos(XX*PlotWidth, PlotWidth);
		ChainPlots[PlotIdx].TopPos(YY*PlotHeight, PlotHeight);
	}
}

void MCMCResultWindow::BeginNewPlots(mcmc_data *Data, double *MinBound, double *MaxBound, const Array<String> &FreeSyms)
{
	this->FreeSyms.clear();
	for(const String &Str : FreeSyms)
		this->FreeSyms << Str;
	
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
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		ScatterCtrl &Plot = ChainPlots[Par];
		
		Plot.SetMode(ScatterDraw::MD_ANTIALIASED);
		Plot.SetPlotAreaLeftMargin(35).SetPlotAreaBottomMargin(15).SetPlotAreaTopMargin(4).SetTitle(FreeSyms[Par]); //NOTE: Seems like title height is auto added to top margin.
		Plot.SetMouseHandling(true, false);
		Plot.SetTitleFont(Plot.GetTitleFont().Height(14));
		Plot.SetReticleFont(Plot.GetReticleFont().Height(8));
		
		for(int Walker = 0; Walker < Data->NWalkers; ++Walker)
			Plot.AddSeries(&(*Data)(Walker, Par, 0), Data->NSteps, 0.0, 1.0).ShowLegend(false).NoMark().Stroke(1.0, GraphColor).Dash("").Opacity(0.4);
		
		Plot.AddSeries((double*)&Burnin[0], (double*)&BurninPlotY[0], (int)2).ShowLegend(false).NoMark().Stroke(1.0, BurninColor).Dash("");
		
		Plot.SetXYMin(0.0, MinBound[Par]);
		Plot.SetRange((double)Data->NSteps, MaxBound[Par]-MinBound[Par]);
		
		if(Par > 0)
			Plot.LinkedWith(ChainPlots[0]);
	}
	
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
	
	ResizePlots();
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
	String Filename = Sel.Get();
	
	if(Filename.GetLength() == 0) return;
	
	std::string Fn = Filename.ToStd();
	
	std::ofstream File(Fn.data());
	
	if(File.fail()) return;
	
	File << Data->NPars << " " << Data->NWalkers << " " << Data->NSteps << "\n";
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		String &ParName = FreeSyms[Par];
		
		File << ParName.ToStd() << "\n";
		
		for(int Step = 0; Step < Data->NSteps; ++Step)
		{
			for(int Walker = 0; Walker < Data->NWalkers; ++Walker)
				File << (*Data)(Par, Walker, Step) << "\t";
			File << "\n";
		}
	}
	
	
	
	File.close();
}

