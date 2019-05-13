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
	void EquationOrInputSelected();
	void RePlot();
	void RefreshParameterView();
	void ParameterEditAccepted(int Row);
	
private:
	ToolBar Tool;
	
	Array<Ctrl> ParameterControls;
	std::vector<parameter_type> CurrentParameterTypes;
	
	HINSTANCE hinstModelDll;
	model_dll_interface ModelDll;
	
	std::string CurrentParameterFile;
	
	
	Label    *IndexSetName[6]; //TODO: Allow dynamic amount of index sets, not just 6. But how?
	DropList *IndexList[6];
	ArrayCtrl *EIndexList[6];
	
	std::map<std::string, size_t> IndexSetNameToId;
	
	std::vector<std::vector<double>> PlotData; //TODO: Better caching system
	
	void *DataSet;
	
	
	std::vector<Color> PlotColors;
	
};

#endif
