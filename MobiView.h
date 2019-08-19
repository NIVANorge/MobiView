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




#define MAX_INDEX_SETS 6

#include "Plotting.h"


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




class MobiView;


class ParameterCtrl : public WithParameterCtrlLayout<ParentCtrl> {
public:
	typedef ParameterCtrl CLASSNAME;
	
	ParameterCtrl();
};


class SearchWindow : public WithSearchLayout<TopWindow> {
public:
	typedef SearchWindow CLASSNAME;
	
	SearchWindow(MobiView *ParentWindow);
	
	MobiView *ParentWindow;
	
	void Find();
	void SelectItem();
};

class StructureViewWindow : public WithStructureViewLayout<TopWindow> {
public:
	typedef StructureViewWindow CLASSNAME;
	
	StructureViewWindow(MobiView *ParentWindow);
};

class VisualizeBranches : public TopWindow {
public :
	VisualizeBranches(MobiView *ParentWindow);
	
	MobiView *ParentWindow;
	
	virtual void Paint(Draw &W);
};


class MobiView : public TopWindow {
	
public:
	typedef MobiView CLASSNAME;
	MobiView();
	
	
	
	Splitter MainVertical;
	Splitter UpperHorizontal;
	Splitter LowerHorizontal;
	
	TreeCtrl ParameterGroupSelecter;
	ParameterCtrl Params;
	DocEdit PlotInfo;
	DocEdit LogBox;

	ParentCtrl EquationSelecterRect;
	ArrayCtrl  EquationSelecter;
	Option     ShowFavorites;
	ArrayCtrl  InputSelecter;
	
	PlotCtrl Plotter;
	
	bool EquationSelecterIsSorted = false;
	bool InputSelecterIsSorted = false;
	
	
	
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
	
	
	void OpenVisualizeBranches();
	VisualizeBranches *Visualize = nullptr;
	
	
	void OpenStructureView();
	StructureViewWindow *StructureView = nullptr;
	
	
	void AddParameterGroupsRecursive(int ParentId, const char *ParentName, int ChildCount);
	
	void ClosingChecks();
	
	
	
	void PlotModeChange();
	void UpdateEquationSelecter();
	
	
	void GetSingleSelectedResultSeries(void *DataSet, String &Legend, String &Unit, double *WriteTo);
	void GetSingleSelectedInputSeries(void *DataSet, String &Legend, String &Unit, double *WriteTo, bool AlignWithResults);
	
	void GetSingleResultSeries(void *DataSet, double *WriteTo, size_t SelectRowFor, int Row);
	void GetSingleInputSeries(void *DataSet, double *WriteTo, size_t SelectRowFor, int Row);
	
	
	void RefreshParameterView();
	
	void RecursiveUpdateParameter(std::vector<char *> &IndexSetNames, int Level, std::vector<std::string> &CurrentIndexes, int Row, int Col, bool EditAsRow);
	void ParameterEditAccepted(int Row, int Col, bool EditAsRow);
	
	
	void GetResultDataRecursive(std::string &Name, std::vector<char *> &IndexSets, std::vector<std::string> &CurrentIndexes, int Level, uint64 Timesteps, std::vector<std::vector<double>> &PushTo, std::vector<std::string> &PushNamesTo);
	void SaveToCsv();
	
	
	
	void ComputeTimeseriesStats(timeseries_stats &StatsOut, double *Data, size_t Len, Date &StartDate);
	void ComputeResidualStats(residual_stats &StatsOut, double *Obs, double *Mod, size_t Len, Date &StartDate);
	void ComputeTrendStats(double *YData, size_t Len, double YMean, double &XMeanOut, double &XVarOut, double &XYCovarOut);
	
	void DisplayTimeseriesStats(timeseries_stats &Stats, String &Name, String &Unit);
	void DisplayResidualStats(residual_stats &Stats, String &Name);
	
	
	
	
	

	ToolBar Tool;
	
	Array<Ctrl> ParameterControls;
	std::vector<parameter_type> CurrentParameterTypes;
	
	Array<Ctrl> EquationSelecterFavControls;
	
	
	HINSTANCE hinstModelDll;
	model_dll_interface ModelDll;
	
	
	Label     *IndexSetName[MAX_INDEX_SETS]; //TODO: Allow dynamic amount of index sets, not just 6. But how?
	DropList  *IndexList[MAX_INDEX_SETS];
	Option    *IndexLock[MAX_INDEX_SETS];
	
	std::map<std::string, size_t> IndexSetNameToId;
	
	
	void *DataSet = nullptr;
	void *BaselineDataSet = nullptr;
	
	
	bool ParametersWereChangedSinceLastSave = false;
	
	
	//NOTE: These should hold the names of what files were last opened (in an earlier
	//session or this session), regardless of whether or not files are currently open.)
	std::string DllFile;
	std::string InputFile;
	std::string ParameterFile;
	
};


#endif
