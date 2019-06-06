#ifndef _MobiView_MobiView_h
#define _MobiView_MobiView_h



#include <CtrlLib/CtrlLib.h>
#include <ScatterCtrl/ScatterCtrl.h>

#include "StarOption.h"

using namespace Upp;

#define LAYOUTFILE <MobiView/MobiView.lay>
#include <CtrlCore/lay.h>

#include <map>

#include <vector>

#include "DllInterface.h"


/*
Major TODO's:
	- Check what is up with slowness of scatter input series if it is the third one.
	- Don't allow to run with null parameter values
*/








//TODO: This stuff should really be in a common file with the main Mobius code.
enum parameter_type
{
	ParameterType_Double = 0,
	ParameterType_UInt,
	ParameterType_Bool,
	ParameterType_Time,
};

inline parameter_type
ParseParameterType(const char *Name)
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

class MobiView;


class SearchWindow : public WithSearchLayout<TopWindow> {
public:
	typedef SearchWindow CLASSNAME;
	
	SearchWindow(MobiView *ParentWindow);
	
	MobiView *ParentWindow;
	
	void Find();
	void SelectItem();
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
	void StoreSettings();
	
	void OpenSearch();
	SearchWindow *Search = nullptr;
	
	
	void ClosingChecks();
	
	
	void UpdateEquationSelecter();
	
	void PlotModeChange();
	
	void AggregateData(Date &ReferenceDate, Date &StartDate, uint64 Timesteps, double *Data, int IntervalType, int AggregationType, std::vector<double> &XValues, std::vector<double> &YValues);
	void AddPlot(String &Legend, String &Unit, int PlotIdx, double *Data, size_t Len, bool Scatter, bool LogY, bool NormalY, Date &ReferenceDate, Date &StartDate, double MinY = 0.0, double MaxY = 0.0);
	int  AddHistogram(String &Legend, String &Unit, int PlotIdx, double *Data, size_t Len);
	void AddTrendLine(String &Legend, int PlotIdx, size_t Timesteps, double XYCovar, double XVar, double YMean, double XMean, Date &ReferenceDate, Date &StartDate);
	void AddNormalApproximation(String &Legend, int PlotIdx, int SampleCount, double Min, double Max, double Mean, double StdDev);
	void AddPlotRecursive(std::string &Name, int Mode, std::vector<char *> &IndexSets, std::vector<std::string> &CurrentIndexes, int Level, int &PlotIdx, uint64 Timesteps, Date &ReferenceDate, Date &StartDate);
	
	
	void RePlot();
	
	void TimestepSliderEvent();
	void TimestepEditEvent();
	void ReplotProfile();
	
	
	void GetSingleSelectedResultSeries(void *DataSet, String &Legend, String &Unit, double *WriteTo);
	void GetSingleSelectedInputSeries(void *DataSet, String &Legend, String &Unit, double *WriteTo, bool AlignWithResults);
	
	void GetSingleResultSeries(void *DataSet, double *WriteTo, size_t SelectRowFor, int Row);
	void GetSingleInputSeries(void *DataSet, double *WriteTo, size_t SelectRowFor, int Row);
	
	
	void RefreshParameterView();
	
	void RecursiveUpdateParameter(std::vector<char *> &IndexSetNames, int Level, std::vector<std::string> &CurrentIndexes, int Row);
	void ParameterEditAccepted(int Row);
	
	
	void NullifyNans(double *Data, size_t Len);
	
	
	
	void ComputeTimeseriesStats(timeseries_stats &StatsOut, double *Data, size_t Len, Date &StartDate);
	void ComputeResidualStats(residual_stats &StatsOut, double *Residuals, double VarObs, size_t Len, Date &StartDate);
	void ComputeTrendStats(double *YData, size_t Len, double YMean, double &XMeanOut, double &XVarOut, double &XYCovarOut);
	
	void DisplayTimeseriesStats(timeseries_stats &Stats, String &Name, String &Unit);
	void DisplayResidualStats(residual_stats &Stats, String &Name);
	
	
	void GetResultDataRecursive(std::string &Name, std::vector<char *> &IndexSets, std::vector<std::string> &CurrentIndexes, int Level, uint64 Timesteps, std::vector<std::vector<double>> &PushTo, std::vector<std::string> &PushNamesTo);
	void SaveToCsv();
	

	ToolBar Tool;
	
	Array<Ctrl> ParameterControls;
	std::vector<parameter_type> CurrentParameterTypes;
	
	Array<Ctrl> EquationSelecterFavControls;
	
	
	HINSTANCE hinstModelDll;
	model_dll_interface ModelDll;
	
	
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
	String ProfileUnit;
	Date ProfileDisplayDate; //NOTE: Only currently used when in profile mode.
	
	void *DataSet = nullptr;
	void *BaselineDataSet = nullptr;
	
	
	bool ParametersWereChangedSinceLastSave = false;
	
	
	std::vector<Color> PlotColors;
	
	
	std::string DllFile;
	std::string InputFile;
	std::string CurrentParameterFile;
	
};


#endif
