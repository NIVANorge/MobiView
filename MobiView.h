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


#define MAX_INDEX_SETS 6

class MobiView : public WithMobiViewLayout<TopWindow> {
public:
	typedef MobiView CLASSNAME;
	MobiView();
	
	void SubBar(Bar &bar);
	
	void HandleDllError();
	bool CheckDllUserError();
	void Load();
	void SaveParameters();
	void RunModel();
	
	void PlotModeChange();
	void AddPlot(String &Legend, int PlotIdx, double *Data, int Len, bool Scatter, bool LogY, bool NormalY, Date &ReferenceDate, Date &StartDate);
	void AddHistogram(String &Legend, int PlotIdx, double *Data, size_t Len);
	void AddPlotRecursive(std::string &Name, int Mode, std::vector<char *> &IndexSets, std::vector<std::string> &CurrentIndexes, int Level, int &PlotIdx, uint64 Timesteps, Date &ReferenceDate, Date &StartDate);
	void RePlot();
	
	void GetSingleSelectedResultSeries(void *DataSet, String &Legend, double *WriteTo);
	void GetSingleSelectedInputSeries(void *DataSet, String &Legend, double *WriteTo, bool AlignWithResults);
	
	void RefreshParameterView();
	
	void RecursiveUpdateParameter(std::vector<char *> &IndexSetNames, int Level, std::vector<std::string> &CurrentIndexes, int Row);
	void ParameterEditAccepted(int Row);
	
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
	
	
	void *DataSet;
	
	
	std::vector<Color> PlotColors;
	
};

#endif
