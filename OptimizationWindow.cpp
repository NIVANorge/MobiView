

#ifdef _WIN32
	#include <winsock2.h>          //NOTE: For some reason dlib includes some windows headers in an order that upp's clang setup doesn't like
#endif

#define DLIB_NO_GUI_SUPPORT  //NOTE: Turns off dlib's own GUI since we are using upp.


#include "dlib/optimization.h"
#include "dlib/global_optimization.h"

#include "MobiView.h"

// NOTE: A little bad we have to re-include this here just to generate the Run icon.
#define IMAGECLASS IconImg4
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>



#define SET_SETTING(Handle, Name, Type)
#define SET_RES_SETTING(Handle, Name, Type)   ResidualType_##Handle,
enum residual_type
{
	#include "SetStatSettings.h"
};
#undef SET_SETTING
#undef SET_RES_SETTING

OptimizationWindow::OptimizationWindow()
{
	CtrlLayout(*this, "MobiView optimization setup");
	
	Sizeable().Zoomable();
	
	PushRun.SetImage(IconImg4::Run());
	
	
	#define SET_SETTING(Handle, Name, Type)
	#define SET_RES_SETTING(Handle, Name, Type) SET_SETTING(Handle, Name, Type) \
		if(Type != -1) SelectStat.Add((int)ResidualType_##Handle, Name);
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	#undef SET_RES_SETTING
	
	SelectStat.GoBegin();
	
	ParameterView.AddColumn(Id("__name"), "Name");
	ParameterView.AddColumn(Id("__indexes"), "Indexes");
	ParameterView.AddColumn(Id("__min"), "Min");
	ParameterView.AddColumn(Id("__max"), "Max");
	ParameterView.AddColumn(Id("__unit"), "Unit");
	
	
	PushAddParameter.WhenPush    = THISBACK(AddParameterClicked);
	PushAddGroup.WhenPush        = THISBACK(AddGroupClicked);
	PushRemoveParameter.WhenPush = THISBACK(RemoveParameterClicked);
	PushClearParameters.WhenPush = THISBACK(ClearAllClicked);
	PushRun.WhenPush             = THISBACK(RunClicked);
	
	AddFrame(Tool);
	Tool.Set(THISBACK(SubBar));
	
	
	EditMaxEvals.Min(1);
	EditMaxEvals.SetData(1000);
}

void OptimizationWindow::SubBar(Bar &bar)
{
	bar.Add(IconImg4::Open(), THISBACK(LoadFromJson)).Tip("Load setup from file");
	bar.Add(IconImg4::Save(), THISBACK(WriteToJson)).Tip("Save setup to file");
}


bool OptimizationWindow::AddSingleParameter(indexed_parameter &Parameter, int SourceRow, bool ReadAdditionalData)
{
	if(SourceRow == -1) return false;
	
	if(Parameter.Type != ParameterType_Double) return false; //TODO: Dlib has provision for allowing integer parameters
	
	int OverrideRow = -1;
	int Row = 0;
	for(indexed_parameter &Par2 : Parameters)
	{
		if(ParameterIsSubsetOf(Parameter, Par2)) return false;
		
		if(ParameterIsSubsetOf(Par2, Parameter))
		{
			Par2 = Parameter;
			OverrideRow = Row;
		}	
		++Row;
	}
	
	String Indexes = MakeParameterIndexString(Parameter);
	
	if(OverrideRow < 0)
	{
		Parameters.push_back(Parameter);
		
		//TODO: Maybe these should be put on the indexed_parameter, but that is pretty superfluous
		//for the other purposes this struct is used for.
		double Min = Null;
		double Max = Null;
		String ParUnit = "";
		
		if(ReadAdditionalData)
		{
			Min     = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__min"));
			Max     = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__max"));
			ParUnit = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__unit"));
		}
		
		ParameterView.Add(Parameter.Name.data(), Indexes, Min, Max, ParUnit);
		
		EditMinCtrls.Create<EditDoubleNotNull>();
		EditMaxCtrls.Create<EditDoubleNotNull>();
		
		int Row = Parameters.size()-1;
		ParameterView.SetCtrl(Row, 2, EditMinCtrls[Row]);
		ParameterView.SetCtrl(Row, 3, EditMaxCtrls[Row]);
	}
	else
	{
		ParameterView.Set(OverrideRow, 1, Indexes);
	}
	
	return true;
}

void OptimizationWindow::AddParameterClicked()
{
	int SelectedRow             = ParentWindow->FindSelectedParameterRow();
	indexed_parameter Parameter = ParentWindow->GetSelectedParameter();
	if(SelectedRow==-1 || !Parameter.Valid) return;  //TODO: Some kind of error message explaining how to use the feature?
	
	AddSingleParameter(Parameter, SelectedRow);
}

void OptimizationWindow::AddGroupClicked()
{
	int RowCount = ParentWindow->Params.ParameterView.GetCount();
	//PromptOK(Format("Row count %d", RowCount));
	for(int Row = 0; Row < RowCount; ++Row)
	{
		indexed_parameter Parameter = ParentWindow->GetParameterAtRow(Row);
		if(Parameter.Valid)
			AddSingleParameter(Parameter, Row);
	}
}

void OptimizationWindow::RemoveParameterClicked()
{
	int SelectedRow = -1;
	for(int Row = 0; Row < ParameterView.GetCount(); ++Row)
	{
		if(ParameterView.IsSel(Row))
		{
			SelectedRow = Row;
			break;
		}
	}
	
	if(SelectedRow == -1) return;
	
	ParameterView.Remove(SelectedRow);
	Parameters.erase(Parameters.begin()+SelectedRow);
	EditMinCtrls.Remove(SelectedRow);
	EditMaxCtrls.Remove(SelectedRow);
}

void OptimizationWindow::ClearAllClicked()
{
	ParameterView.Clear();
	Parameters.clear();
	EditMinCtrls.Clear();
	EditMaxCtrls.Clear();
}


typedef dlib::matrix<double,0,1> column_vector;


inline void
SetParameters(MobiView *ParentWindow, void *DataSet, std::vector<indexed_parameter> *Parameters, const column_vector& Par)
{
	int ParIdx = 0;
	for(indexed_parameter &Param : *Parameters)
	{
		ParentWindow->ParameterEditAccepted(Param, DataSet, Par(ParIdx));
		++ParIdx;
	}
}

struct optimization_model
{
	MobiView                       *ParentWindow;
	std::vector<indexed_parameter> *Parameters;
	residual_type                   Res;
	
	bool                            ValuesLoadedOnce = false;
	std::vector<double>             InputData;
	
	std::string                     InputName;
	std::string                     ResultName;
	std::vector<char *>             InputIndexes;
	std::vector<char *>             ResultIndexes;
	
	optimization_model(MobiView *ParentWindow, std::vector<indexed_parameter> *Parameters, residual_type Res,
		std::string &InputName, const std::vector<char *> &InputIndexes, std::string &ResultName, const std::vector<char *> &ResultIndexes)
	{
		this->ParentWindow = ParentWindow;
		this->Parameters   = Parameters;
		this->Res          = Res;
		
		this->InputName    = InputName;
		this->ResultName   = ResultName;
		this->InputIndexes = InputIndexes;
		this->ResultIndexes= ResultIndexes;
	}
	
	double operator()(const column_vector& Par)
	{
		void *DataSetCopy = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false);   //NOTE: This is for thread safety.
		
		SetParameters(ParentWindow, DataSetCopy, Parameters, Par);
		
		ParentWindow->ModelDll.RunModel(DataSetCopy);
		
		
		// Extract result and input values to compute them.
		
		//TODO: We should use the GOF interval!!
		
		if(!ValuesLoadedOnce)
		{
			uint64 Timesteps = ParentWindow->ModelDll.GetTimesteps(DataSetCopy);
			InputData.resize(Timesteps);
			//NOTE: The final 'true' signifies that we align with the result series, so that it
			//is in fact safe to use the result timesteps for the size here.
			ParentWindow->ModelDll.GetInputSeries(DataSetCopy, InputName.data(), (char**)InputIndexes.data(), InputIndexes.size(), InputData.data(), true); 
		}
		
		//NOTE: We have to allocate this for each call, for thread safety. There is no other
		//way unless Dlib could tell us what thread Id we are in, which it doesn't.
		std::vector<double> ResultData(InputData.size());
		
		ParentWindow->ModelDll.GetResultSeries(DataSetCopy, ResultName.data(), (char**)ResultIndexes.data(), ResultIndexes.size(), ResultData.data());
		
		//NOTE: It may seem a little wasteful to compute all of them, but it is a bit messy to
		//untangle their computations.
		residual_stats ResidualStats;
		ComputeResidualStats(ResidualStats, InputData.data(), ResultData.data(), ResultData.size());
		
		double Value;
		
		if(false){}
		#define SET_SETTING(Handle, Name, Type)
		#define SET_RES_SETTING(Handle, Name, Type) \
			else if(Res == ResidualType_##Handle) Value = ResidualStats.Handle;     //TODO: Could do this with an array lookup, but would be a little hacky

		#include "SetStatSettings.h"
		
		#undef SET_SETTING
		#undef SET_RES_SETTING
		
		
		ParentWindow->ModelDll.DeleteDataSet(DataSetCopy);
		
		//ParentWindow->Log(Format("Ran once, value is %g", Value));
		//ParentWindow->ProcessEvents();
		
		return Value;
	}
};


void OptimizationWindow::RunClicked()
{
	ErrorLabel.SetText("");
	
	if(Parameters.empty())
	{
		ErrorLabel.SetText("At least one parameter must be added before running");
		return;
	}
	
	plot_setup PlotSetup;
	ParentWindow->Plotter.GatherCurrentPlotSetup(PlotSetup);
	
	if(PlotSetup.SelectedResults.size() != 1 || PlotSetup.SelectedInputs.size() != 1)
	{
		ErrorLabel.SetText("This only works with a single selected result series and input series.");
		return;
	}
	
	for(int Idx = 0; Idx < PlotSetup.SelectedIndexes.size(); ++Idx)
	{
		if(PlotSetup.SelectedIndexes[Idx].size() != 1 && PlotSetup.IndexSetIsActive[Idx])
		{
			ErrorLabel.SetText("This currently only works with a single selected index combination");
			return;
		}
	}
	
	std::string &InputName = PlotSetup.SelectedInputs[0];
	std::string &ResultName = PlotSetup.SelectedResults[0];
	
	std::vector<char *> InputIndexes;
	std::vector<char *> ResultIndexes;
	
	bool Success = ParentWindow->GetSelectedIndexesForSeries(PlotSetup, ParentWindow->DataSet, InputName, 1, InputIndexes);
	if(!Success) return;
	Success      = ParentWindow->GetSelectedIndexesForSeries(PlotSetup, ParentWindow->DataSet, ResultName, 0, ResultIndexes);
	if(!Success) return;
	
	residual_type Res = (residual_type)(int)SelectStat.GetKey(SelectStat.GetIndex());
	
	bool PositiveGood;
	if(false){}
	#define SET_SETTING(Handle, Name, Type)
	#define SET_RES_SETTING(Handle, Name, Type) \
		else if(Res == ResidualType_##Handle) PositiveGood = (Type==1);
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	#undef SET_RES_SETTING
	
	int RowCount = ParameterView.GetCount();
	column_vector MinBound(RowCount);
	column_vector MaxBound(RowCount);
	
	for(int Row = 0; Row < RowCount; ++Row)
	{
		MinBound(Row) = (double)ParameterView.Get(Row, Id("__min"));
		MaxBound(Row) = (double)ParameterView.Get(Row, Id("__max"));
		
		if(MinBound(Row) >= MaxBound(Row))
		{
			ErrorLabel.SetText(Format("The min value is larger than or equal to the max value for the parameter %s", Parameters[Row].Name.data()));
			return;
		}
	}
	
	optimization_model OptimizationModel(ParentWindow, &Parameters, Res, InputName, InputIndexes, ResultName, ResultIndexes);
	
	ParentWindow->Log("Running optimization. This can take a few minutes or more.");
	
	ParentWindow->ProcessEvents();
	
	int MaxFunctionCalls = EditMaxEvals.GetData();
	dlib::function_evaluation Result;
	
	if(PositiveGood)
		//Result = dlib::find_max_global(dlib::default_thread_pool(), OptimizationModel , MinBound, MaxBound, dlib::max_function_calls(MaxFunctionCalls));
		Result = dlib::find_max_global(OptimizationModel , MinBound, MaxBound, dlib::max_function_calls(MaxFunctionCalls));
	else
		//Result = dlib::find_min_global(dlib::default_thread_pool(), OptimizationModel, MinBound, MaxBound, dlib::max_function_calls(MaxFunctionCalls));
		Result = dlib::find_min_global(OptimizationModel, MinBound, MaxBound, dlib::max_function_calls(MaxFunctionCalls));
	
	SetParameters(ParentWindow, ParentWindow->DataSet, &Parameters, Result.x);
	ParentWindow->Log(Format("Optimization finished, with new best %s : %g. Remember to save to a different file if you don't want to lose your old calibration.",
		SelectStat.GetValue().ToString(), Result.y));
	ParentWindow->RunModel();
	
	Close();
}

void OptimizationWindow::LoadFromJson()
{
	FileSel Sel;
	
	Sel.Type("Calibration setups", "*.json");
	
	if(!ParentWindow->ParameterFile.empty())
		Sel.ActiveDir(GetFileFolder(ParentWindow->ParameterFile.data()));
	else
		Sel.ActiveDir(GetCurrentDirectory());
	
	Sel.ExecuteOpen();
	String Filename = Sel.Get();
	
	if(!FileExists(Filename)) return;
	
	if(GetFileName(Filename) == "settings.json")
	{
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store calibration setups.");
		return;
	}
	
	ClearAllClicked();
	
	String SetupFile = LoadFile(Filename);
	
	Value SetupJson  = ParseJSON(SetupFile);
	
	
	Value MaxEvalsJson = SetupJson["MaxEvals"];
	if(!IsNull(MaxEvalsJson))
		EditMaxEvals.SetData((int)MaxEvalsJson);

	

	Value StatNameJson = SetupJson["TargetStat"];
	if(!IsNull(StatNameJson))
	{
		residual_type Res;

		if(false){}
		#define SET_SETTING(Handle, Name, Type)
		#define SET_RES_SETTING(Handle, Name, Type) \
			else if(String(Name) == StatNameJson.ToString()) Res = ResidualType_##Handle;
		
		#include "SetStatSettings.h"
		
		#undef SET_SETTING
		#undef SET_RES_SETTING	
		
		SelectStat.SetData((int)Res);
	}
	else
		SelectStat.GoBegin();
	
	ValueArray ParameterArr = SetupJson["Parameters"];
	if(!IsNull(ParameterArr))
	{
		int Count = ParameterArr.GetCount();
		int ValidRow = 0;
		for(int Row = 0; Row < Count; ++Row)
		{
			Value ParamJson = ParameterArr[Row];
			
			indexed_parameter Parameter = {};
			Parameter.Valid = true;
			Parameter.Type  = ParameterType_Double;
			
			Value NameVal = ParamJson["Name"];
			if(!IsNull(NameVal)) Parameter.Name = NameVal.ToString().ToStd();
			else continue; // Should never happen though
			
			ValueArray IndexesVal = ParamJson["Indexes"];
			for(int Idx = 0; Idx < IndexesVal.GetCount(); ++Idx)
			{
				Value IndexJson = IndexesVal[Idx];
				parameter_index Index;
				
				Value NameVal = IndexJson["Name"];
				if(!IsNull(NameVal)) Index.Name = NameVal.ToString().ToStd();
				Value IndexSetVal = IndexJson["IndexSetName"];
				if(!IsNull(IndexSetVal)) Index.IndexSetName = IndexSetVal.ToString().ToStd();
				Value LockedVal = IndexJson["Locked"];
				if(!IsNull(LockedVal)) Index.Locked = (bool)LockedVal;
				
				Parameter.Indexes.push_back(Index);
			}
			
			bool Added = AddSingleParameter(Parameter, 0, false);
			
			if(Added)
			{
				Value UnitVal = ParamJson["Unit"];
				ParameterView.Set(ValidRow, Id("__unit"), UnitVal);
				Value MinVal  = ParamJson["Min"];
				ParameterView.Set(ValidRow, Id("__min"), MinVal);
				Value MaxVal  = ParamJson["Max"];
				ParameterView.Set(ValidRow, Id("__max"), MaxVal);
				
				++ValidRow;
			}
		}
	}
}

void OptimizationWindow::WriteToJson()
{
	FileSel Sel;

	Sel.Type("Calibration setups", "*.json");
	
	if(!ParentWindow->ParameterFile.empty())
		Sel.ActiveDir(GetFileFolder(ParentWindow->ParameterFile.data()));
	else
		Sel.ActiveDir(GetCurrentDirectory());
	
	Sel.ExecuteSaveAs();
	String Filename = Sel.Get();
	
	if(Filename.GetLength() == 0) return;
	
	if(GetFileName(Filename) == "settings.json")
	{
		PromptOK("settings.json is used by MobiView to store various settings and should not be used to store calibration setups.");
		return;
	}
	
	Json MainFile;
	
	MainFile("MaxEvals", EditMaxEvals.GetData());
	
	String StatName = SelectStat.GetValue();
	
	MainFile("TargetStat", StatName);
	
	JsonArray ParameterArr;
	
	//TODO: Factor out a serialization method for an indexed parameter?
	int Row = 0;
	for(indexed_parameter &Par : Parameters)
	{
		Json ParJson;
		ParJson("Name", Par.Name.data());
		
		ParJson("Unit", ParameterView.Get(Row, Id("__unit")));
		ParJson("Min", (double)ParameterView.Get(Row, Id("__min")));
		ParJson("Max", (double)ParameterView.Get(Row, Id("__max")));
		
		JsonArray IndexArr;
		for(parameter_index &Index : Par.Indexes)
		{
			Json IndexJson;
			IndexJson("Name", Index.Name.data());
			IndexJson("IndexSetName", Index.IndexSetName.data());
			IndexJson("Locked", Index.Locked);
			IndexArr << IndexJson;
		}
		ParJson("Indexes", IndexArr);
		
		ParameterArr << ParJson;
		
		++Row;
	}
	
	MainFile("Parameters", ParameterArr);
	
	
	SaveFile(Filename, MainFile.ToString());
}