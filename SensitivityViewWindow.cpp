#include "MobiView.h"

// NOTE: A little bad we have to re-include this here just to generate the Run icon.
#define IMAGECLASS IconImg3
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>


SensitivityViewWindow::SensitivityViewWindow()
{
	CtrlLayout(*this, "MobiView perturbation view");
	
	Sizeable().Zoomable();
	
	EditSteps.Min(2);
	EditSteps.SetData(5);
	
	PushRun.SetImage(IconImg3::Run());
	PushRun.WhenPush = THISBACK(Run);
	
	RunProgress.Hide();
}

int
SensitivityViewWindow::FindSelectedParameterRow()
{
	ParameterCtrl &Params = ParentWindow->Params;
	int SelectedRow = -1;
	for(int Row = 0; Row < Params.ParameterView.GetCount(); ++Row)
	{
		if(Params.ParameterView.IsSel(Row))
		{
			SelectedRow = Row;
			break;
		}
	}
	return SelectedRow;
}

void
SensitivityViewWindow::Update()
{
	ErrorLabel.SetText("");
	
	int SelectedRow = FindSelectedParameterRow();
	
	bool Error = (SelectedRow == -1);
	
	if(!Error)
	{
		parameter_type Type = ParentWindow->CurrentParameterTypes[SelectedRow];
		if(Type != ParameterType_Double)
		{
			//TODO: Allow UInts too?
			Error = true;
			ErrorLabel.SetText("Can only perturb parameters of type double");
		}
	}
	else
		ErrorLabel.SetText("Select a parameter in the main view");
	
	if(!Error && ParentWindow->SecondExpandedSetLocal >= 0)
	{
		Error = true;
		ErrorLabel.SetText("We currently don't have this functionality for matrix parameters");
	}
	
	if(Error)
	{
		EditMin.SetData(Null);
		EditMax.SetData(Null);
		return;
	}
	
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
	
	int SelectedRow = FindSelectedParameterRow();
	
	if(ParentWindow->SecondExpandedSetLocal >= 0)
	{
		ErrorLabel.SetText("We currently don't have this functionality for matrix parameters");
		return;
	}
	
	if(SelectedRow == -1)
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
	
	plot_setup PlotSetup;
	ParentWindow->Plotter.GatherCurrentPlotSetup(PlotSetup);
	
	if(PlotSetup.SelectedResults.size() != 1 || PlotSetup.SelectedInputs.size() > 1)
	{
		ErrorLabel.SetText("This currently only works with a single selected result series, and at most one input series");
		return;
	}
	for(int Idx = 0; Idx < PlotSetup.SelectedIndexes.size(); ++Idx)
	{
		if(PlotSetup.SelectedIndexes[Idx].size() != 1 && PlotSetup.IndexSetIsActive[Idx])
		{
			ErrorLabel.SetText("This currently only works with a single index combination for the result");
			return;
		}
	}
	
	if(PlotSetup.YAxisMode == YAxis_Normalized)
		PlotSetup.YAxisMode = YAxis_Regular;
	PlotSetup.MajorMode = MajorMode_Regular;
	
	int NSteps = EditSteps.GetData();
	
	double Stride = (Max - Min) / (double) (NSteps - 1);
	
	void *DataSetCopy = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false);
	ParentWindow->CheckDllUserError();
	
	
	
	char TimeStr[256];
	ParentWindow->ModelDll.GetInputStartDate(DataSetCopy, TimeStr);
	Time InputStartTime;
	StrToTime(InputStartTime, TimeStr);
	
	
	RunProgress.Set(0);
	RunProgress.SetTotal(NSteps);
	RunProgress.Show();
	this->ProcessEvents();
	
	int GUIUpdateFreq = std::min(10, NSteps / 10);
	GUIUpdateFreq = std::max(1, GUIUpdateFreq);
	
	Plot.ClearAll(false);
	Plot.PlotData.Data.reserve(2*NSteps);
	
	//TODO: This should say indexes too!
	Plot.SetTitle(Format("Sensitivity of \"%s\" to \"%s\"",
		PlotSetup.SelectedResults[0].data(),
		ParentWindow->Params.ParameterView.Get(SelectedRow, Id("__name").ToString())));
	
	int NStep = 0;
	for(int NStep = 0; NStep < NSteps; ++NStep)
	{
		//TODO: Also allow selection of logarithmic spacing
		double Val = Min + ((double)NStep)*Stride;
		
		// Write the value into the DataSet
		ParentWindow->ParameterEditAccepted(SelectedRow, Id("__value"), DataSetCopy, Val);
		
		// Run the model
		ParentWindow->ModelDll.RunModel(DataSetCopy);
		
		
		
		// Extract the result data and do the plotting
		
		uint64 ResultTimesteps = ParentWindow->ModelDll.GetTimesteps(DataSetCopy);
		ParentWindow->ModelDll.GetStartDate(DataSetCopy, TimeStr);
		Time ResultStartTime;
		StrToTime(ResultStartTime, TimeStr);
		
		double *ResultXValues = Plot.PlotData.Allocate(ResultTimesteps).data();
		ComputeXValues(InputStartTime, ResultStartTime, ResultTimesteps, ParentWindow->TimestepSize, ResultXValues);
		
		double *ResultYValues = Plot.PlotData.Allocate(ResultTimesteps).data();
		String Legend;
		String Unit;
		ParentWindow->GetSingleSelectedResultSeries(PlotSetup, DataSetCopy, PlotSetup.SelectedResults[0], Legend, Unit, ResultYValues);
		NullifyNans(ResultYValues, ResultTimesteps);

		//Note: override the legend:
		Legend = Format("%g", Val);
		Unit = Null; //NOTE: to avoid it showing in the legend (ideally we want to be able to turn that off).
		
		//PromptOK(Format("%g %g %g", ResultXValues[0], ResultXValues[1], ResultXValues[ResultTimesteps-1]));
		
		//timeseries_stats Stats = {};
		//ComputeTimeseriesStats(Stats, ResultYValues, ResultTimesteps, ParentWindow->StatSettings);
		
		//PromptOK(Format("%d", (int)ResultTimesteps));
		
		//NOTE: It doesn't matter that we pass 0 as min and max since we disallow normalized Y
		//axis above.
		
		
		//Plot.AddPlot(Legend, Unit, ResultXValues, ResultYValues, ResultTimesteps, false, InputStartTime, ResultStartTime, ParentWindow->TimestepSize, 0.0, 0.0);
		
		//TODO: Figure out why AddPlot doesn't work!
		Color GraphColor = GradientColor(Color(255, 0, 0), Color(0, 0, 255), NStep, NSteps);
		Plot.AddSeries(ResultXValues, ResultYValues, ResultTimesteps).NoMark().Stroke(1.5, GraphColor).Dash("").Units(Unit).Legend(Legend);
		
		Plot.FormatAxes(MajorMode_Regular, 0, InputStartTime, ParentWindow->TimestepSize);
		Plot.Refresh();
		
		
		bool Error = ParentWindow->CheckDllUserError();
		if(Error)
		{
			ErrorLabel.SetText("An error occurred while running the model. See the main log box");
			break;
		}
		
		if(NStep % GUIUpdateFreq == 0)
		{
			RunProgress.Set(NStep+1);

			this->ProcessEvents();
		}
	}
	
	Plot.FormatAxes(MajorMode_Regular, 0, InputStartTime, ParentWindow->TimestepSize);
	RunProgress.Hide();
	
	ParentWindow->ModelDll.DeleteDataSet(DataSetCopy);
	
	//PromptOK(Format("x: %g %g,   y: %g, %g,   ts: %d, resultstart: %s", ResultXValues[0], ResultXValues[10], ResultYValues[0], ResultYValues[10], (int)ResultTimesteps, ResultStartTime));
	
}