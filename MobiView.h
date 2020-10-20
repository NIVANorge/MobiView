#ifndef _MobiView_MobiView_h
#define _MobiView_MobiView_h



#include <CtrlLib/CtrlLib.h>
#include <ScatterCtrl/ScatterCtrl.h>

#include <map>
#include <vector>


#define MAX_INDEX_SETS 6


#include "StarOption.h"

using namespace Upp;

#include "DllInterface.h"
#include "Plotting.h"


#include "MyRichView.h"


#define LAYOUTFILE <MobiView/MobiView.lay>
#include <CtrlCore/lay.h>


#include "PlotCtrl.h"


//TODO: This stuff should really be in a common file with the main Mobius code.
enum parameter_type
{
	ParameterType_Double = 0,
	ParameterType_UInt,
	ParameterType_Bool,
	ParameterType_Time,
	ParameterType_Enum,
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
	
	SearchWindow();
	
	MobiView *ParentWindow;
	
	void Find();
	void SelectItem();
};

class ChangeIndexesWindow;

class VisualizeBranches : public Ctrl {
public :
	VisualizeBranches();
	
	MobiView *ParentWindow;
	ChangeIndexesWindow *OtherParent;
	
	virtual void Paint(Draw &W);
};

class StructureViewWindow : public WithStructureViewLayout<TopWindow> {
public:
	typedef StructureViewWindow CLASSNAME;
	
	StructureViewWindow();
	
	MobiView *ParentWindow;
	
	void RefreshText();
};

class ModelInfoViewWindow : public WithModelInfoLayout<TopWindow> {
public:
	typedef ModelInfoViewWindow CLASSNAME;
	
	ModelInfoViewWindow();
	
	MobiView *ParentWindow;
	
	void RefreshText();
};

class EditStatSettingsWindow : public WithEditStatSettingsLayout<TopWindow> {
public:
	typedef EditStatSettingsWindow CLASSNAME;
	
	EditStatSettingsWindow();
	
	MobiView *ParentWindow;
	
	void LoadData();
	void SaveDataAndClose();
	
	bool ParseDoubleList(String &ListStr, std::vector<double> &Result);
};



class ChangeIndexesWindow : public WithChangeIndexesLayout<TopWindow> {
public:
	typedef ChangeIndexesWindow CLASSNAME;
	
	ChangeIndexesWindow();
	
	MobiView *ParentWindow;
	
	VisualizeBranches Branches;
	
	Label     *IndexSetName[MAX_INDEX_SETS];
	LineEdit  *IndexList[MAX_INDEX_SETS];
	ArrayCtrl *BranchList[MAX_INDEX_SETS];
	
	Array<Array<Ctrl>> BranchControls;
	Array<Array<Ctrl>> NameControls;
	
	bool ParseIntList(String &ListStr, std::vector<int> &Result, int Row);
	
	void RefreshData();
	void DoIndexUpdate();
	void AddIndexPushed();
	void DeleteSelectedPushed();
	
	void SelectedBranchListUpdate();
	
	void BuildConnectionEditFromDataset();
};

#define MAX_ADDITIONAL_PLOTS 10

class MiniPlot : public WithMiniPlotLayout<ParentCtrl>
{
public:
	typedef MiniPlot CLASSNAME;
	
	MiniPlot();
};

class AdditionalPlotView : public WithAdditionalPlotViewLayout<TopWindow>
{
public:
	typedef AdditionalPlotView CLASSNAME;
	
	AdditionalPlotView();
	
	void CopyMainPlot(int Which);
	
	void BuildAll(bool CausedByReRun = false);
	void ClearAll();
	
	MobiView *ParentWindow;
	
	void NumRowsChanged(bool Rebuild = true);
	void UpdateLinkStatus();
	
	void WriteToJson();
	void LoadFromJson();
	
private:
	Splitter VerticalSplitter;
	
	MiniPlot Plots[MAX_ADDITIONAL_PLOTS];
	
	ToolBar Tool;
	
	void SubBar(Bar &bar);
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
	
	ParentCtrl PlotInfoRect;
	EditTime   CalibrationIntervalStart;
	EditTime   CalibrationIntervalEnd;
	Label      CalibrationIntervalLabel;
	
	MyRichView PlotInfo;
	MyRichView LogBox;

	ParentCtrl EquationSelecterRect;
	ArrayCtrl  EquationSelecter;
	Option     ShowFavorites;
	ArrayCtrl  InputSelecter;
	
	PlotCtrl Plotter;
	
	bool EquationSelecterIsSorted = false;
	bool InputSelecterIsSorted = false;
	
	
	
	void SubBar(Bar &bar);
	
	void Log(String Msg, bool Error = false);
	
	void HandleDllError();
	bool CheckDllUserError();
	void Load();
	void SaveParameters();
	void SaveParametersAs();
	void RunModel();
	void SaveBaseline();
	void StoreSettings(bool OverwriteFavorites = true);
	
	void CleanInterface();
	void BuildInterface();
	
	void OpenSearch();
	SearchWindow Search;
	
	
	void OpenStatSettings();
	EditStatSettingsWindow EditStatSettings;
	
	StatisticsSettings StatSettings;
	
	
	void OpenStructureView();
	StructureViewWindow StructureView;
	
	
	void OpenChangeIndexes();
	ChangeIndexesWindow ChangeIndexes;
	
	void OpenAdditionalPlotView();
	AdditionalPlotView OtherPlots;
	
	void OpenModelInfoView();
	ModelInfoViewWindow ModelInfo;
	
	
	void AddParameterGroupsRecursive(int ParentId, const char *ParentName, int ChildCount);
	
	void ClosingChecks();
	
	
	
	void PlotModeChange();
	void PlotRebuild();
	void UpdateEquationSelecter();
	
	
	void GetSingleSelectedResultSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, String &Legend, String &Unit, double *WriteTo);
	void GetSingleSelectedInputSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, String &Legend, String &Unit, double *WriteTo, bool AlignWithResults);
	
	void GetSingleResultSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, double *WriteTo, size_t SelectRowFor, std::string &Row);
	void GetSingleInputSeries(plot_setup &PlotSetup, void *DataSet, std::string &Name, double *WriteTo, size_t SelectRowFor, std::string &Row);

	
	void ExpandIndexSetClicked(size_t IndexSet);
	void RefreshParameterView(bool RefreshValuesOnly = false);
	//void RefreshParameterViewValues();
	
	void RecursiveUpdateParameter(std::vector<char *> &IndexSetNames, int Level, std::vector<std::string> &CurrentIndexes, int Row, Id ValueColumn, int ExpandedSetLocal, int SecondExpandedSetLocal);
	void ParameterEditAccepted(int Row, Id ValueColumn, int ExpandedSetLocal, int SecondExpandedSetLocal);
	
	
	void GetResultDataRecursive(std::string &Name, std::vector<char *> &IndexSets, std::vector<std::string> &CurrentIndexes, int Level, uint64 Timesteps, std::vector<std::vector<double>> &PushTo, std::vector<std::string> &PushNamesTo);
	void SaveToCsv();
	
	

	ToolBar Tool;
	
	Array<Ctrl> ParameterControls;
	std::vector<parameter_type> CurrentParameterTypes;
	
	Array<Ctrl> EquationSelecterFavControls;
	
	model_dll_interface ModelDll;
	timestep_size TimestepSize;
	
	Label     *IndexSetName[MAX_INDEX_SETS]; //TODO: Allow dynamic amount of index sets, not just 6. But how?
	DropList  *IndexList[MAX_INDEX_SETS];
	Option    *IndexLock[MAX_INDEX_SETS];
	Option    *IndexExpand[MAX_INDEX_SETS];
	
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
