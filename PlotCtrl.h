#ifndef _MobiView_PlotCtrl_h_
#define _MobiView_PlotCtrl_h_

class PlotCtrl : public WithPlotCtrlLayout<ParentCtrl>
{
public:
	typedef PlotCtrl CLASSNAME;
	
	PlotCtrl(MobiView* Parent);
	
	void PlotModeChange();
	
	void RePlot(bool CausedByReRun = false);
	
	void TimestepSliderEvent();
	void TimestepEditEvent();
	
	void GatherCurrentPlotSetup(plot_setup &PlotSetup);
	void SetMainPlotSetup(plot_setup &PlotSetup);
	
	ArrayCtrl *EIndexList[MAX_INDEX_SETS];
	
	
	Time ProfileDisplayTime;
	
private:
	
	MobiView *Parent;
};

#endif
