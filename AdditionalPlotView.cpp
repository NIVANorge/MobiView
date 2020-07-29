#include "MobiView.h"


MiniPlot::MiniPlot()
{
	CtrlLayout(*this);
}

AdditionalPlotView::AdditionalPlotView()
{
	CtrlLayout(*this, "Additional plot view");
	
	MinimizeBox().Sizeable().Zoomable();
	
	Add(VerticalSplitter.VSizePos(25, 0));
	
	VerticalSplitter.Vert();

	EditNumRows.NotNull();
	EditNumRows.SetData(2);
	EditNumRows.Max(MAX_ADDITIONAL_PLOTS);
	
	LinkAll.NotNull();
	LinkAll.Set(1);
	
	EditNumRows.WhenAction = THISBACK(NumRowsChanged);
	
	LinkAll.WhenAction = THISBACK(UpdateLinkStatus);
	
	for(int Row = 0; Row < MAX_ADDITIONAL_PLOTS; ++Row)
	{
		Plots[Row].CopyMain.WhenPush << [this, Row](){ CopyMainPlot(Row); };
	}
	
	NumRowsChanged();
}

void AdditionalPlotView::NumRowsChanged()
{
	int NRows = EditNumRows.GetData();
	
	if(IsNull(NRows) || NRows <= 0) return;
	
	VerticalSplitter.Clear();
	
	for(int Row = 0; Row < NRows; ++Row)
	{
		VerticalSplitter.Add(Plots[Row]);
	}
	
	BuildAll();  //TODO: Technically only have to rebuild any newly added ones
}

void AdditionalPlotView::UpdateLinkStatus()
{
	if(LinkAll.Get())
	{
		int FirstLinkable = -1;
		for(int Row = 0; Row < MAX_ADDITIONAL_PLOTS; ++Row)
		{
			if(Plots[Row].Plot.GetMouseHandlingX())
			{
				if(FirstLinkable >= 0)
					Plots[Row].Plot.LinkedWith(Plots[FirstLinkable].Plot);
				else
					FirstLinkable = Row;
				
			}
		}
	}
	else
	{
		for(int Row = 0; Row < MAX_ADDITIONAL_PLOTS; ++Row)
		{
			Plots[Row].Plot.Unlinked();
		}
	}
}

void AdditionalPlotView::CopyMainPlot(int Which)
{
	Plots[Which].Plot.PlotSetup = ParentWindow->Plotter.MainPlot.PlotSetup;
	Plots[Which].Plot.BuildPlot(ParentWindow, nullptr, false, Plots[Which].PlotInfo);
	UpdateLinkStatus();
}

void AdditionalPlotView::BuildAll()
{
	int NRows = EditNumRows.GetData();
	for(int Row = 0; Row < NRows; ++Row)
	{
		Plots[Row].Plot.BuildPlot(ParentWindow, nullptr, false, Plots[Row].PlotInfo);
	}
	UpdateLinkStatus();
}

void AdditionalPlotView::ClearAll()
{
	for(int Row = 0; Row < MAX_ADDITIONAL_PLOTS; ++Row)
	{
		Plots[Row].Plot.ClearAll();
	}
}