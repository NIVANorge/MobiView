#ifndef _MobiView_MobiView_h
#define _MobiView_MobiView_h



#include <CtrlLib/CtrlLib.h>
#include <ScatterCtrl/ScatterCtrl.h>



using namespace Upp;

#define LAYOUTFILE <MobiView/MobiView.lay>
#include <CtrlCore/lay.h>

#include <map>

#define IMAGECLASS IconImg
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>
#include <vector>

#include "DllInterface.h"


//TODO: This stuff should really be in a common file with the main Mobius code.
enum parameter_type
{
	ParameterType_Double = 0,
	ParameterType_UInt,
	ParameterType_Bool,
	ParameterType_Time,
};

parameter_type ParseParameterType(const char *Name)
{
	parameter_type Type;
	if(strcmp(Name, "double") == 0) Type = ParameterType_Double;
	else if(strcmp(Name, "uint") == 0) Type = ParameterType_UInt;
	else if(strcmp(Name, "bool") == 0) Type = ParameterType_Bool;
	else if(strcmp(Name, "time") == 0) Type = ParameterType_Time;
	//Error handling?
	return Type;
}


struct timeseries_stats
{
	double Min;
	double Max;
	double Sum;
	double Median;
	double Mean;
	double Variance;
	double StandardDeviation;
	size_t DataPoints;
};

struct residual_stats
{
	double MeanError;
	double MeanAbsoluteError;
	double RootMeanSquareError;
	double NashSutcliffe;
	//double R2;
	size_t DataPoints;
};


//NOTE: This has to match up to the major mode selecter.
enum plot_major_mode
{
	MajorMode_Regular = 0,
	MajorMode_Histogram,
	MajorMode_Profile,
	MajorMode_CompareBaseline,
	MajorMode_Residuals,
	MajorMode_ResidualHistogram,
};


#define MAX_INDEX_SETS 6

class MobiView : public WithMobiViewLayout<TopWindow> {
public:
	typedef MobiView CLASSNAME;
	MobiView();
	
	void SubBar(Bar &bar);
	
	void Log(String Msg);
	
	void HandleDllError();
	bool CheckDllUserError();
	void Load();
	void SaveParameters();
	void SaveParametersAs();
	void RunModel();
	void SaveBaseline();
	
	void PlotModeChange();
	void AddPlot(String &Legend, int PlotIdx, double *Data, size_t Len, bool Scatter, bool LogY, bool NormalY, Date &ReferenceDate, Date &StartDate, double MinY = 0.0, double MaxY = 0.0);
	void AddHistogram(String &Legend, int PlotIdx, double *Data, size_t Len);
	void AddTrendLine(String &Legend, int PlotIdx, size_t Timesteps, double XYCovar, double XVar, double YMean, double XMean, Date &ReferenceDate, Date &StartDate);
	
	void TimestepSliderEvent();
	void TimestepEditEvent();
	void ReplotProfile();
	
	void AddPlotRecursive(std::string &Name, int Mode, std::vector<char *> &IndexSets, std::vector<std::string> &CurrentIndexes, int Level, int &PlotIdx, uint64 Timesteps, Date &ReferenceDate, Date &StartDate);
	void RePlot();
	
	void GetSingleSelectedResultSeries(void *DataSet, String &Legend, double *WriteTo);
	void GetSingleSelectedInputSeries(void *DataSet, String &Legend, double *WriteTo, bool AlignWithResults);
	
	void GetSingleResultSeries(void *DataSet, double *WriteTo, size_t SelectRowFor, int Row);
	void GetSingleInputSeries(void *DataSet, double *WriteTo, size_t SelectRowFor, int Row);
	
	
	void RefreshParameterView();
	
	void RecursiveUpdateParameter(std::vector<char *> &IndexSetNames, int Level, std::vector<std::string> &CurrentIndexes, int Row);
	void ParameterEditAccepted(int Row);
	
	
	void NullifyNans(double *Data, size_t Len);
	
	
	
	void ComputeTimeseriesStats(timeseries_stats &StatsOut, double *Data, size_t Len, Date &StartDate);
	void ComputeResidualStats(residual_stats &StatsOut, double *Residuals, double VarObs, size_t Len, Date &StartDate);
	void ComputeTrendStats(double *YData, size_t Len, double YMean, double &XMeanOut, double &XVarOut, double &XYCovarOut);
	
	void DisplayTimeseriesStats(timeseries_stats &Stats, String &Name);
	void DisplayResidualStats(residual_stats &Stats, String &Name);
	
private:
	ToolBar Tool;
	
	Array<Ctrl> ParameterControls;
	std::vector<parameter_type> CurrentParameterTypes;
	
	HINSTANCE hinstModelDll;
	model_dll_interface ModelDll;
	
	std::string CurrentParameterFile;
	
	
	Label    *IndexSetName[MAX_INDEX_SETS]; //TODO: Allow dynamic amount of index sets, not just 6. But how?
	DropList *IndexList[MAX_INDEX_SETS];
	Option   *IndexLock[MAX_INDEX_SETS];
	ArrayCtrl *EIndexList[MAX_INDEX_SETS];
	
	std::map<std::string, size_t> IndexSetNameToId;
	
	std::vector<std::vector<double>> PlotData; //TODO: Better caching system
	std::vector<std::vector<double>> AggregateX;
	std::vector<std::vector<double>> AggregateY;
	bool PlotWasAutoResized = false;
	
	std::vector<String> ProfileLabels;
	String ProfileLegend;
	Date CurrentStartDate; //NOTE: Only currently used when in profile mode.
	
	void *DataSet = nullptr;
	void *BaselineDataSet = nullptr;
	
	std::vector<Color> PlotColors;
	
};

#endif
