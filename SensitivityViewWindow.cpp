#include "MobiView.h"

// NOTE: A little bad we have to re-include this here just to generate the Run icon.
#define IMAGECLASS IconImg3
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>


StatPlotCtrl::StatPlotCtrl()
{
	CtrlLayout(*this);
}

SensitivityViewWindow::SensitivityViewWindow()
{
	CtrlLayout(*this, "MobiView perturbation view");
	
	Sizeable().Zoomable();
	
	EditSteps.Min(2);
	EditSteps.SetData(5);
	
	PushRun.SetImage(IconImg3::Run());
	PushRun.WhenPush = THISBACK(Run);
	
	RunProgress.Hide();
	
	StatPlot.SelectStat.Add("(none)");
	
	#define SET_SETTING(Handle, Name, Type) \
		StatPlot.SelectStat.Add(Name);
	#define SET_RES_SETTING(Handle, Name, Type) SET_SETTING(Handle, Name, Type)
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	#undef SET_RES_SETTING
	
	StatPlot.SelectStat.SetIndex(0);
	
	MainHorizontal.Horz();
	MainHorizontal.Add(Plot);
	MainHorizontal.Add(StatPlot);
	MainHorizontal.SetPos(7500, 0);
	
	Add(MainHorizontal.VSizePos(52, 32).HSizePos());
}

void
SensitivityViewWindow::Update()
{
	ErrorLabel.SetText("");
	ParamLabel.SetText("");
	
	int SelectedRow = ParentWindow->FindSelectedParameterRow();

	indexed_parameter &CurrentParameter = ParentWindow->CurrentSelectedParameter;
	
	if((SelectedRow == -1) || !CurrentParameter.Valid)
	{
		ErrorLabel.SetText("Select a parameter in the main view");
		EditMin.SetData(Null);
		EditMax.SetData(Null);
		return;
	}
	
	if(CurrentParameter.Type != ParameterType_Double)
	{
		//TODO: Allow UInts too?
		ErrorLabel.SetText("Can only perturb parameters of type double");
		EditMin.SetData(Null);
		EditMax.SetData(Null);
		return;
	}
	
	
	String ParUnit = ParentWindow->Params.ParameterView.Get(SelectedRow, Id("__unit"));
	
	String LabelText = Format("%s [%s] ", CurrentParameter.Name.data(), ParUnit);
	LabelText << MakeParameterIndexString(CurrentParameter);
	
	ParamLabel.SetText(LabelText);
	
	
	double Min = ParentWindow->Params.ParameterView.Get(SelectedRow, Id("__min"));
	double Max = ParentWindow->Params.ParameterView.Get(SelectedRow, Id("__max"));
	
	EditMin.SetData(Min);
	EditMax.SetData(Max);
}

void
SensitivityViewWindow::Run()
{
	double Min = EditMin.GetData();
	double Max = EditMax.GetData();
	
	int SelectedRow = ParentWindow->FindSelectedParameterRow();
	
	ErrorLabel.SetText("");
	
	indexed_parameter &CurrentParameter = ParentWindow->CurrentSelectedParameter;
	
	if(SelectedRow == -1 || !CurrentParameter.Valid)
	{
		ErrorLabel.SetText("Select a parameter in the main view");
		return;
	}
	
	if(IsNull(Min) || IsNull(Max))
	{
		ErrorLabel.SetText("Min and Max must be real values");
		return;
	}
	
	if(Max <= Min)
	{
		ErrorLabel.SetText("Max must be larger than Min");
		return;
	}
	
	ParentWindow->Plotter.GatherCurrentPlotSetup(Plot.PlotSetup);
	
	if(Plot.PlotSetup.SelectedResults.size() != 1 || Plot.PlotSetup.SelectedInputs.size() > 1)
	{
		ErrorLabel.SetText("This currently only works with a single selected result series, and at most one input series");
		return;
	}
	for(int Idx = 0; Idx < Plot.PlotSetup.SelectedIndexes.size(); ++Idx)
	{
		if(Plot.PlotSetup.SelectedIndexes[Idx].size() != 1 && Plot.PlotSetup.IndexSetIsActive[Idx])
		{
			ErrorLabel.SetText("This currently only works with a single index combination for the result");
			return;
		}
	}
	
	if(Plot.PlotSetup.YAxisMode == YAxis_Normalized)
		Plot.PlotSetup.YAxisMode = YAxis_Regular;
	Plot.PlotSetup.MajorMode = MajorMode_Regular;
	
	
	int NSteps = EditSteps.GetData();
	
	double Stride = (Max - Min) / (double) (NSteps - 1);
	
	void *DataSetCopy = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, true, true);
	ParentWindow->CheckDllUserError();
	
	
	
	char TimeStr[256];
	ParentWindow->ModelDll.GetInputStartDate(DataSetCopy, TimeStr);
	Time InputStartTime;
	StrToTime(InputStartTime, TimeStr);
	
	uint64 InputTimesteps = ParentWindow->ModelDll.GetInputTimesteps(DataSetCopy);
	
	RunProgress.Set(0);
	RunProgress.SetTotal(NSteps);
	RunProgress.Show();
	this->ProcessEvents();
	
	int GUIUpdateFreq = std::min(10, NSteps / 10);
	GUIUpdateFreq = std::max(1, GUIUpdateFreq);
	
	Plot.ClearAll(false);
	Plot.PlotData.Data.reserve(2*NSteps + 3);
	
	StatPlot.Plot.RemoveAllSeries();
	StatPlot.Plot.SetTitle(" ");
	StatPlot.Plot.SetLabelX(" ");
	StatPlot.Plot.SetLabelY(" ");
	
	//TODO: This should say indexes too!
	Plot.SetTitle(Format("Sensitivity of \"%s\" [%s] to %s",
		Plot.PlotSetup.SelectedResults[0].data(),
		ParentWindow->ModelDll.GetResultUnit(ParentWindow->DataSet, Plot.PlotSetup.SelectedResults[0].data()),
		ParamLabel.GetText()));
	
	bool HasInput = Plot.PlotSetup.SelectedInputs.size() == 1;
	
	double *StatData = Plot.PlotData.Allocate(NSteps).data();
	double *ParValues = Plot.PlotData.Allocate(NSteps).data();
	
	for(int NStep = 0; NStep < NSteps; ++NStep)
	{
		StatData[NStep] = Null;
		ParValues[NStep] = Min + ((double)NStep)*Stride;   //TODO: Also allow selection of logarithmic spacing
	}
	
	double *InputYValues;
	if(HasInput)
	{
		double *InputXValues = Plot.PlotData.Allocate(InputTimesteps).data();
		ComputeXValues(InputStartTime, InputStartTime, InputTimesteps, ParentWindow->TimestepSize, InputXValues);
		
		InputYValues = Plot.PlotData.Allocate(InputTimesteps).data();
		String Legend;
		String Unit;
		ParentWindow->GetSingleSelectedInputSeries(Plot.PlotSetup, DataSetCopy, Plot.PlotSetup.SelectedInputs[0], Legend, Unit, InputYValues, false);
		//NullifyNans(InputYValues, InputTimesteps);
		
		Color InputColor(0, 130, 200);
		
		Plot.AddPlot(Legend, Unit, InputXValues, InputYValues, InputTimesteps, true, InputStartTime, InputStartTime, ParentWindow->TimestepSize, 0.0, 0.0, InputColor);
			
		//Plot.FormatAxes(MajorMode_Regular, 0, InputStartTime, ParentWindow->TimestepSize);
	}
	
	String StatName = StatPlot.SelectStat.Get();
	
	if(StatName != "(none)")
	{
		Color StatColor(0, 130, 200);
		StatPlot.Plot.AddSeries(ParValues, StatData, NSteps).MarkBorderColor(StatColor).MarkColor(StatColor).Stroke(1.5, StatColor).Opacity(0.5).MarkStyle<CircleMarkPlot>();
		
		StatPlot.Plot.SetLabels(CurrentParameter.Name.data(), StatName);
		StatPlot.Plot.ShowLegend(false);
		StatPlot.Plot.SetMouseHandling(false, false);
		
		//This is a little stupid, but whatever...
		Size PlotReticleSize = GetTextSize("00000", StatPlot.Plot.GetReticleFont());
		Size PlotUnitSize    = GetTextSize("[dummy]", StatPlot.Plot.GetLabelsFont());
		StatPlot.Plot.SetPlotAreaLeftMargin(PlotReticleSize.cx + PlotUnitSize.cy + 20);
		StatPlot.Plot.SetPlotAreaBottomMargin(PlotReticleSize.cy + PlotUnitSize.cy + 20);
		
		StatPlot.Plot.SetXYMin(Min);
		StatPlot.Plot.SetRange(Max-Min);
		StatPlot.Plot.SetMajorUnitsNum(std::min(NSteps-1, 9));     //TODO: This should be better!
	}
	else
	{
		StatPlot.Plot.SetTitle("Select a statistic to display");
	}
	
	Time GofStartTime;
	Time GofEndTime;
	bool HasResidual = false;
	
	int NStep = 0;
	for(int NStep = 0; NStep < NSteps; ++NStep)
	{
		
		double Val = ParValues[NStep];
		
		// Write the value into the DataSet
		SetParameterValue(CurrentParameter, DataSetCopy, Val, ParentWindow->ModelDll);
		
		// Run the model
		ParentWindow->ModelDll.RunModel(DataSetCopy, -1);
		
		
		
		// Extract the result data and do the plotting
		
		uint64 ResultTimesteps = ParentWindow->ModelDll.GetTimesteps(DataSetCopy);
		ParentWindow->ModelDll.GetStartDate(DataSetCopy, TimeStr);
		Time ResultStartTime;
		StrToTime(ResultStartTime, TimeStr);
		
		//TODO: It is wasteful to do this for each loop!!
		double *ResultXValues = Plot.PlotData.Allocate(ResultTimesteps).data();
		ComputeXValues(InputStartTime, ResultStartTime, ResultTimesteps, ParentWindow->TimestepSize, ResultXValues);
		
		double *ResultYValues = Plot.PlotData.Allocate(ResultTimesteps).data();
		String Legend;
		String Unit;
		ParentWindow->GetSingleSelectedResultSeries(Plot.PlotSetup, DataSetCopy, Plot.PlotSetup.SelectedResults[0], Legend, Unit, ResultYValues);
		//NullifyNans(ResultYValues, ResultTimesteps);
		
		//Note: override the legend:
		Legend = Format("%g", Val);
		Unit = Null; //NOTE: to avoid it showing in the legend (ideally we want to be able to turn that off).
			
		Color GraphColor = GradientColor(Color(245, 130, 48), Color(145, 30, 180), NStep, NSteps);
		//NOTE: It doesn't matter that we pass 0 as min and max since we disallow normalized Y
		//axis above.
		Plot.AddPlot(Legend, Unit, ResultXValues, ResultYValues, ResultTimesteps, false, InputStartTime, ResultStartTime, ParentWindow->TimestepSize, 0.0, 0.0, GraphColor);
		
		//TODO: There is a lot of stuff in the following call that it is wasteful to do for
		//each loop!
		Plot.FormatAxes(MajorMode_Regular, 0, InputStartTime, ParentWindow->TimestepSize);
		Plot.Refresh();
		
		residual_stats ResidualStats = {};
		timeseries_stats TimeseriesStats = {};
		
		//TODO: It is a tiny bit inefficient to do the string comparison here every time, but
		//it should be fine
		
		
		if(StatName != "(none)")
		{
			
			int64 GofOffset;
			int64 GofTimesteps;
			ParentWindow->GetGofOffsets(ResultStartTime, ResultTimesteps, GofStartTime, GofEndTime, GofOffset, GofTimesteps);
			
			int64 ResultOffset = TimestepsBetween(InputStartTime, ResultStartTime, ParentWindow->TimestepSize);
			
			#define SET_SETTING(Handle, Name, Type) \
				else if(Name == StatName)           \
				{                                   \
					ComputeTimeseriesStats(TimeseriesStats, ResultYValues+GofOffset, GofTimesteps, ParentWindow->StatSettings, false); \
					StatData[NStep] = TimeseriesStats.Handle;                                                            \
				}
			//TODO: Use the GOF interval!
			#define SET_RES_SETTING(Handle, Name, Type) \
				else if(Name == StatName)               \
				{                                       \
					if(HasInput)                        \
					{                                   \
						ComputeResidualStats(ResidualStats, InputYValues+ResultOffset+GofOffset, ResultYValues+GofOffset, GofTimesteps);     \
						StatData[NStep] = ResidualStats.Handle; \
						HasResidual = true;             \
					}                                   \
					else                                \
						StatPlot.Plot.SetTitle("Select an input series to compute the residual stat"); \
				}
			
			if(StatName == "(none)") {}
			#include "SetStatSettings.h"
			
			#undef SET_SETTING
			#undef SET_RES_SETTING
		
		}
		
		
		
		
		bool Error = ParentWindow->CheckDllUserError();
		if(Error)
		{
			ErrorLabel.SetText("An error occurred while running the model. See the main log box");
			break;
		}
		
		if(NStep % GUIUpdateFreq == 0)
		{
			RunProgress.Set(NStep+1);
			
			StatPlot.Plot.ZoomToFit(true, true);
			//StatPlot.Refresh();
			SetBetterGridLinePositions(StatPlot.Plot, 1);
			
			this->ProcessEvents();
		}
	}
	
	//Plot.FormatAxes(MajorMode_Regular, 0, InputStartTime, ParentWindow->TimestepSize);
	RunProgress.Hide();
	
	StatPlot.Plot.ZoomToFit(true, true);
	SetBetterGridLinePositions(StatPlot.Plot, 1);
	
	if(StatName != "(none)")
	{
		double StartX = (double)(GofStartTime - InputStartTime);
		double EndX   = (double)(GofEndTime   - InputStartTime);
		
		Plot.AddLine(Null, StartX, StartX, Plot.GetYMin(), Plot.GetYMax(), Red());
		Plot.AddLine(Null, EndX, EndX,     Plot.GetYMin(), Plot.GetYMax(), Red());
	}
	
	Plot.Refresh();
	
	ParentWindow->ModelDll.DeleteDataSet(DataSetCopy);
	
	//PromptOK(Format("x: %g %g,   y: %g, %g,   ts: %d, resultstart: %s", ResultXValues[0], ResultXValues[10], ResultYValues[0], ResultYValues[10], (int)ResultTimesteps, ResultStartTime));
	
}