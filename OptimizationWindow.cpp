

#ifdef _WIN32
	#include <winsock2.h>          //NOTE: For some reason dlib includes some windows headers in an order that upp's clang setup doesn't like
#endif

#define DLIB_NO_GUI_SUPPORT  //NOTE: Turns off dlib's own GUI since we are using upp.

#include "dlib/optimization.h"
#include "dlib/global_optimization.h"

#include "MobiView.h"

#include <unordered_map>

#define IMAGECLASS IconImg4
#define IMAGEFILE <MobiView/images.iml>
#include <Draw/iml.h>



OptimizationParameterSetup::OptimizationParameterSetup()
{
	CtrlLayout(*this);
}

OptimizationTargetSetup::OptimizationTargetSetup()
{
	CtrlLayout(*this);
}

OptimizationRunSetup::OptimizationRunSetup()
{
	CtrlLayout(*this);
}

MCMCRunSetup::MCMCRunSetup()
{
	CtrlLayout(*this);
}

OptimizationWindow::OptimizationWindow()
{
	//CtrlLayout(*this, "MobiView optimization setup");
	
	SetRect(0, 0, 740, 740);
	Title("MobiView optimization setup").Sizeable().Zoomable();
	
	ParSetup.ParameterView.AddColumn(Id("__name"), "Name");
	ParSetup.ParameterView.AddColumn(Id("__indexes"), "Indexes");
	ParSetup.ParameterView.AddColumn(Id("__min"), "Min");
	ParSetup.ParameterView.AddColumn(Id("__max"), "Max");
	ParSetup.ParameterView.AddColumn(Id("__unit"), "Unit");
	ParSetup.ParameterView.AddColumn(Id("__sym"), "Sym.");
	ParSetup.ParameterView.AddColumn(Id("__expr"), "Expr.");
	
	ParSetup.OptionUseExpr.Set((int)false);
	ParSetup.OptionUseExpr.WhenAction     = THISBACK(EnableExpressionsClicked);
	ParSetup.ParameterView.HeaderObject().HideTab(5);
	ParSetup.ParameterView.HeaderObject().HideTab(6);
	
	TargetSetup.TargetView.AddColumn(Id("__resultname"), "Result name");
	TargetSetup.TargetView.AddColumn(Id("__resultindexes"), "Result idxs.");
	TargetSetup.TargetView.AddColumn(Id("__inputname"), "Input name");
	TargetSetup.TargetView.AddColumn(Id("__inputindexes"), "Input idxs.");
	TargetSetup.TargetView.AddColumn(Id("__targetstat"), "Target stat.");
	TargetSetup.TargetView.AddColumn(Id("__errparam"), "Error param.");
	TargetSetup.TargetView.AddColumn(Id("__weight"), "Weight");
	
	TargetSetup.TargetView.HeaderObject().HideTab(5);
	
	ParSetup.PushAddParameter.WhenPush    = THISBACK(AddParameterClicked);
	ParSetup.PushAddGroup.WhenPush        = THISBACK(AddGroupClicked);
	ParSetup.PushRemoveParameter.WhenPush = THISBACK(RemoveParameterClicked);
	ParSetup.PushClearParameters.WhenPush = THISBACK(ClearParametersClicked);
	ParSetup.PushAddVirtual.WhenPush      = THISBACK(AddVirtualClicked);
	
	ParSetup.PushAddParameter.SetImage(IconImg4::Add());
	ParSetup.PushAddGroup.SetImage(IconImg4::AddGroup());
	ParSetup.PushRemoveParameter.SetImage(IconImg4::Remove());
	ParSetup.PushClearParameters.SetImage(IconImg4::RemoveGroup());
	ParSetup.PushAddVirtual.SetImage(IconImg4::Add());
	
	ParSetup.PushAddVirtual.Disable();
	ParSetup.PushAddVirtual.Hide();
	
	TargetSetup.PushAddTarget.WhenPush    = THISBACK(AddTargetClicked);
	TargetSetup.PushRemoveTarget.WhenPush = THISBACK(RemoveTargetClicked);
	TargetSetup.PushClearTargets.WhenPush = THISBACK(ClearTargetsClicked);
	TargetSetup.PushDisplay.WhenPush      = THISBACK(DisplayClicked);
	
	TargetSetup.PushAddTarget.SetImage(IconImg4::Add());
	TargetSetup.PushRemoveTarget.SetImage(IconImg4::Remove());
	TargetSetup.PushClearTargets.SetImage(IconImg4::RemoveGroup());
	TargetSetup.PushDisplay.SetImage(IconImg4::ViewMorePlots());
	
	RunSetup.PushRun.WhenPush          = THISBACK(RunClicked);
	RunSetup.PushRun.SetImage(IconImg4::Run());
	
	RunSetup.EditMaxEvals.Min(1);
	RunSetup.EditMaxEvals.SetData(1000);
	
	
	MCMCSetup.PushRun.WhenPush         = THISBACK(RunClicked);
	MCMCSetup.PushRun.SetImage(IconImg4::Run());
	MCMCSetup.PushViewChains.WhenPush << [&]() { ParentWindow->MCMCResultWin.Open(); };
	MCMCSetup.PushViewChains.SetImage(IconImg4::ViewMorePlots());
	
	MCMCSetup.EditSteps.Min(10);
	MCMCSetup.EditSteps.SetData(1000);
	MCMCSetup.EditWalkers.Min(10);
	MCMCSetup.EditWalkers.SetData(20);
	MCMCSetup.InitialTypeSwitch.SetData(0);
	
	
	AddFrame(Tool);
	Tool.Set(THISBACK(SubBar));
	
	TargetSetup.OptimizerTypeTab.Add(RunSetup, "Optimiser");
	TargetSetup.OptimizerTypeTab.Add(MCMCSetup, "MCMC");
	
	TargetSetup.OptimizerTypeTab.WhenSet << THISBACK(TabChange);
	
	MainVertical.Vert();
	MainVertical.Add(ParSetup);
	MainVertical.Add(TargetSetup);
	Add(MainVertical.SizePos());	
}

void OptimizationWindow::SubBar(Bar &bar)
{
	bar.Add(IconImg4::Open(), THISBACK(LoadFromJson)).Tip("Load setup from file");
	bar.Add(IconImg4::Save(), THISBACK(WriteToJson)).Tip("Save setup to file");
}

void OptimizationWindow::EnableExpressionsClicked()
{
	bool Value = (bool)ParSetup.OptionUseExpr.Get();
	
	if(Value)
	{
		ParSetup.ParameterView.HeaderObject().ShowTab(5);
		ParSetup.ParameterView.HeaderObject().ShowTab(6);
		ParSetup.PushAddVirtual.Enable();
		ParSetup.PushAddVirtual.Show();
	}
	else
	{
		ParSetup.ParameterView.HeaderObject().HideTab(5);
		ParSetup.ParameterView.HeaderObject().HideTab(6);
		ParSetup.PushAddVirtual.Disable();
		ParSetup.PushAddVirtual.Hide();
	}
}


bool OptimizationWindow::AddSingleParameter(const indexed_parameter &Parameter, int SourceRow, bool ReadAdditionalData)
{
	if(SourceRow == -1) return false;
	
	if(Parameter.Type != ParameterType_Double) return false; //TODO: Dlib has provision for allowing integer parameters
	
	int OverrideRow = -1;
	int Row = 0;
	if(!Parameter.Virtual)
	{
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
		String Symbol = Null;
		
		if(ReadAdditionalData)
		{
			Min     = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__min"));
			Max     = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__max"));
			ParUnit = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__unit"));
			
			Symbol  = ParentWindow->ModelDll.GetParameterShortName(ParentWindow->DataSet, Parameter.Name.data());
		}
		
		ParSetup.ParameterView.Add(Parameter.Name.data(), Indexes, Min, Max, ParUnit);
		
		int Row = Parameters.size()-1;
		
		EditMinCtrls.Create<EditDoubleNotNull>();
		EditMaxCtrls.Create<EditDoubleNotNull>();
		ParSetup.ParameterView.SetCtrl(Row, 2, EditMinCtrls[Row]);
		ParSetup.ParameterView.SetCtrl(Row, 3, EditMaxCtrls[Row]);
		
		EditSymCtrls.Create<EditField>();
		EditExprCtrls.Create<EditField>();
		ParSetup.ParameterView.SetCtrl(Row, 5, EditSymCtrls[Row]);
		ParSetup.ParameterView.SetCtrl(Row, 6, EditExprCtrls[Row]);
		
		if(!IsNull(Symbol) && Symbol != "") EditSymCtrls[Row].SetData(Symbol);
	}
	else
	{
		ParSetup.ParameterView.Set(OverrideRow, 1, Indexes);
	}
	
	return true;
}

void OptimizationWindow::AddParameterClicked()
{
	int SelectedRow              = ParentWindow->FindSelectedParameterRow();
	indexed_parameter &Parameter = ParentWindow->CurrentSelectedParameter;
	if(SelectedRow==-1 || !Parameter.Valid) return;  //TODO: Some kind of error message explaining how to use the feature?
	
	AddSingleParameter(Parameter, SelectedRow);
}

void OptimizationWindow::AddVirtualClicked()
{
	indexed_parameter Parameter = {};
	Parameter.Name    = "(virtual)";
	Parameter.Valid   = true;
	Parameter.Virtual = true;
	Parameter.Type    = ParameterType_Double;
	AddSingleParameter(Parameter, 0, false);
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
	for(int Row = 0; Row < ParSetup.ParameterView.GetCount(); ++Row)
	{
		if(ParSetup.ParameterView.IsSel(Row))
		{
			SelectedRow = Row;
			break;
		}
	}
	
	if(SelectedRow == -1) return;
	
	ParSetup.ParameterView.Remove(SelectedRow);
	Parameters.erase(Parameters.begin()+SelectedRow);
	EditMinCtrls.Remove(SelectedRow);
	EditMaxCtrls.Remove(SelectedRow);
	EditSymCtrls.Remove(SelectedRow);
	EditExprCtrls.Remove(SelectedRow);
}

void OptimizationWindow::ClearParametersClicked()
{
	ParSetup.ParameterView.Clear();
	Parameters.clear();
	EditMinCtrls.Clear();
	EditMaxCtrls.Clear();
	EditSymCtrls.Clear();
	EditExprCtrls.Clear();
}


void OptimizationWindow::AddOptimizationTarget(optimization_target &Target)
{
	Targets.push_back(Target);
	
	String InputIndexStr = MakeIndexString(Target.InputIndexes);
	String ResultIndexStr = MakeIndexString(Target.ResultIndexes);
	
	TargetSetup.TargetView.Add(Target.ResultName.data(), ResultIndexStr, Target.InputName.data(), InputIndexStr, (int)Target.Stat, Target.ErrParSym.data(), Target.Weight);
	
	TargetStatCtrls.Create<DropList>();
	DropList &SelectStat = TargetStatCtrls.Top();
	
	TargetErrCtrls.Create<EditField>();
	EditField &ErrCtrl = TargetErrCtrls.Top();
	
	#define SET_SETTING(Handle, Name, Type)
	#define SET_RES_SETTING(Handle, Name, Type) SET_SETTING(Handle, Name, Type) \
		if(Type != -1) SelectStat.Add((int)ResidualType_##Handle, Name);
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	#undef SET_RES_SETTING
	
	int Row = TargetSetup.TargetView.GetCount() - 1;
	int Col = TargetSetup.TargetView.GetPos(Id("__targetstat"));
	TargetSetup.TargetView.SetCtrl(Row, Col, SelectStat);
	
	Col     = TargetSetup.TargetView.GetPos(Id("__errparam"));
	TargetSetup.TargetView.SetCtrl(Row, Col, ErrCtrl);
	
	TargetWeightCtrls.Create<EditDoubleNotNull>();
	EditDoubleNotNull &EditWt = TargetWeightCtrls.Top();
	EditWt.Min(0.0);
	Col     = TargetSetup.TargetView.GetPos(Id("__weight"));
	TargetSetup.TargetView.SetCtrl(Row, Col, EditWt);
}

void OptimizationWindow::AddTargetClicked()
{
	plot_setup PlotSetup;
	ParentWindow->Plotter.GatherCurrentPlotSetup(PlotSetup);
	
	if(PlotSetup.SelectedResults.size() != 1 || PlotSetup.SelectedInputs.size() != 1)
	{
		TargetSetup.ErrorLabel.SetText("This only works with a single selected result series and input series.");
		return;
	}
	
	for(int Idx = 0; Idx < PlotSetup.SelectedIndexes.size(); ++Idx)
	{
		if(PlotSetup.SelectedIndexes[Idx].size() != 1 && PlotSetup.IndexSetIsActive[Idx])
		{
			TargetSetup.ErrorLabel.SetText("This currently only works with a single selected index combination");
			return;
		}
	}
	
	optimization_target Target = {};
	Target.ResultName = PlotSetup.SelectedResults[0];
	Target.InputName  = PlotSetup.SelectedInputs[0];
	
	std::vector<char *> InputIndexes;
	std::vector<char *> ResultIndexes;
	
	bool Success = ParentWindow->GetSelectedIndexesForSeries(PlotSetup, ParentWindow->DataSet, Target.InputName, 1, InputIndexes);
	if(!Success) return;
	Success      = ParentWindow->GetSelectedIndexesForSeries(PlotSetup, ParentWindow->DataSet, Target.ResultName, 0, ResultIndexes);
	if(!Success) return;
	
	//Ugh, super annoying to have to convert back and forth between char* and string to ensure
	//storage...
	for(const char *Idx : InputIndexes)
		Target.InputIndexes.push_back(std::string(Idx));
	for(const char *Idx : ResultIndexes)
		Target.ResultIndexes.push_back(std::string(Idx));
	
	Target.Stat = ResidualType_MAE;
	Target.Weight = 1.0;
	
	AddOptimizationTarget(Target);
}

void OptimizationWindow::DisplayClicked()
{
	if(!ParentWindow->DataSet || !ParentWindow->ModelDll.IsLoaded())
	{
		TargetSetup.ErrorLabel.SetText("Can't display plots before a model is loaded");
		return;
	}
	
	if(Targets.empty())
	{
		TargetSetup.ErrorLabel.SetText("There are no targets to display the plots of");
	 	return;
	}
	
	TargetSetup.ErrorLabel.SetText("");
	
	std::vector<plot_setup> PlotSetups;
	PlotSetups.reserve(Targets.size());
	
	for(optimization_target &Target : Targets)
	{
		plot_setup PlotSetup = {};
		PlotSetup.SelectedIndexes.resize(MAX_INDEX_SETS);
		PlotSetup.IndexSetIsActive.resize(MAX_INDEX_SETS);
		
		PlotSetup.MajorMode = MajorMode_Regular;//MajorMode_Residuals;
		PlotSetup.AggregationPeriod = Aggregation_None;
		PlotSetup.ScatterInputs = true;
		
		PlotSetup.SelectedResults.push_back(Target.ResultName);
		PlotSetup.SelectedInputs.push_back(Target.InputName);
		
		std::vector<char *> IndexSets;
		bool Success = ParentWindow->GetIndexSetsForSeries(ParentWindow->DataSet, Target.InputName, 1, IndexSets);
		
		if(!Success) return;
		
		for(int IdxIdx = 0; IdxIdx < IndexSets.size(); ++IdxIdx)
		{
			size_t Id = ParentWindow->IndexSetNameToId[IndexSets[IdxIdx]];
			PlotSetup.IndexSetIsActive[Id] = true;
			PlotSetup.SelectedIndexes[Id].push_back(Target.InputIndexes[IdxIdx]);
		}

		IndexSets.clear();
		Success = ParentWindow->GetIndexSetsForSeries(ParentWindow->DataSet, Target.ResultName, 0, IndexSets);
		if(!Success) return;
		
		for(int IdxIdx = 0; IdxIdx < IndexSets.size(); ++IdxIdx)
		{
			size_t Id = ParentWindow->IndexSetNameToId[IndexSets[IdxIdx]];
			PlotSetup.IndexSetIsActive[Id] = true;
			PlotSetup.SelectedIndexes[Id].push_back(Target.ResultIndexes[IdxIdx]);
		}
		
		PlotSetups.push_back(PlotSetup);
	}
	
	AdditionalPlotView *Plots = &ParentWindow->OtherPlots;
	
	Plots->SetAll(PlotSetups);
	
	if(!Plots->IsOpen())
		Plots->Open();
}



void OptimizationWindow::RemoveTargetClicked()
{
	int SelectedRow = -1;
	for(int Row = 0; Row < TargetSetup.TargetView.GetCount(); ++Row)
	{
		if(TargetSetup.TargetView.IsSel(Row))
		{
			SelectedRow = Row;
			break;
		}
	}
	
	if(SelectedRow == -1) return;
	
	TargetSetup.TargetView.Remove(SelectedRow);
	Targets.erase(Targets.begin()+SelectedRow);
	TargetWeightCtrls.Remove(SelectedRow);
	TargetStatCtrls.Remove(SelectedRow);
	TargetErrCtrls.Remove(SelectedRow);
}

void OptimizationWindow::ClearTargetsClicked()
{
	TargetSetup.TargetView.Clear();
	Targets.clear();
	TargetWeightCtrls.Clear();
	TargetStatCtrls.Clear();
	TargetErrCtrls.Clear();
}
	
void OptimizationWindow::ClearAll()
{
	ClearParametersClicked();
	ClearTargetsClicked();
}



typedef dlib::matrix<double,0,1> column_vector;

inline int
SetParameters(void *DataSet, std::vector<indexed_parameter> *Parameters, const column_vector& Par, bool UseExpr, const Array<String> &Syms, const Array<String> &Exprs, model_dll_interface &ModelDll)
{
	//NOTE: The length of Par has to be equal to the length of Parameters - ExprCount
	
	if(!UseExpr)
	{
		int ParIdx = 0;
		for(indexed_parameter &Param : *Parameters)
		{
			if(!Param.Virtual)
				SetParameterValue(Param, DataSet, Par(ParIdx), ModelDll);
			++ParIdx;
		}
	}
	else
	{
		EvalExpr Expression;
		Expression.SetErrorUndefined(true); //NOTE: Necessary for it to return Null when encountering an undefined variable
		
		int ParIdx = 0;
		int ActiveIdx = 0;
		for(indexed_parameter &Param : *Parameters)
		{
			double Val;
			if(!IsNull(Exprs[ParIdx]))
			{
				auto Res = Expression.Eval(Exprs[ParIdx]);
				if(IsNull(Res)) return ParIdx;
				Val = Res.val;
			}
			else
			{
				Val = Par(ActiveIdx);
				++ActiveIdx;
			}
			
			if(!IsNull(Syms[ParIdx]))
			{
				auto Res = Expression.AssignVariable(Syms[ParIdx], Val);
				if(IsNull(Res)) return ParIdx;
			}
			
			if(!Param.Virtual)
				SetParameterValue(Param, DataSet, Val, ModelDll);
			++ParIdx;
		}
	}
	
	return -1;
}

void OptimizationWindow::SetParameterValues(void *DataSet, double *Pars, size_t NPars)
{
	//NOTE: This one is only here to be used by the MCMC result window when it wants to write
	//back parameter sets. TODO: This may not be the best way of organizing it...
	
	std::unordered_map<std::string, std::pair<int,int>> SymRow;
	
	int ParCount = ParSetup.ParameterView.GetCount();  //NOTE: May be different from NPars.
	
	//TODO: Shouldn't these be packed into indexed_parameter instead??
	Array<String> Syms(ParCount);
	Array<String> Exprs(ParCount);

	int ExprCount = 0;
	for(int Row = 0; Row < ParCount; ++Row)
	{
		Syms[Row]  = ParSetup.ParameterView.Get(Row, Id("__sym"));
		Exprs[Row] = ParSetup.ParameterView.Get(Row, Id("__expr"));
		if(!IsNull(Exprs[Row]) && Exprs[Row] != "") ++ExprCount;
	}
	
	column_vector Pars2(NPars);
	for(int Par = 0; Par < NPars; ++Par) Pars2(Par) = Pars[Par];

	SetParameters(DataSet, &Parameters, Pars2, ExprCount > 0, Syms, Exprs, ParentWindow->ModelDll);
}


double ComputeLLValue(double *Obs, double *Sim, size_t Timesteps, double ErrParam);

struct optimization_model
{
	MobiView                         *ParentWindow;
	Label                            *ProgressLabel;
	
	std::vector<indexed_parameter>   *Parameters;
	std::vector<optimization_target> *Targets;
	
	bool                             ValuesLoadedOnce = false;
	std::vector<std::vector<double>> InputData;
	
	int64 GofOffset;
	int64 GofTimesteps;
	
	int ExprCount;
	int FreeParCount;
	const Array<String> *Syms;
	const Array<String> *Exprs;
	
	int64 NumEvals = 0;
	double InitialScore, BestScore;
	bool  IsMaximizing;
	int   UpdateStep = 100;
	
	int RunType = 0;
	
	optimization_model(MobiView *ParentWindow, std::vector<indexed_parameter> *Parameters, std::vector<optimization_target> *Targets,
		const Array<String> &Syms, const Array<String> &Exprs, Label *ProgressLabel, int RunType)
	{
		this->ParentWindow = ParentWindow;
		this->Parameters   = Parameters;
		this->Targets      = Targets;
		this->Syms         = &Syms;
		this->Exprs        = &Exprs;
		this->ProgressLabel = ProgressLabel;
		this->RunType      = RunType;
		
		//TODO: Assert length of Parameters, Syms and Exprs are the same?
		
		ExprCount = 0;
		for(const String &Expr : Exprs)
			if(!IsNull(Expr) && Expr != "")
				ExprCount++;
		FreeParCount = Exprs.size() - ExprCount;
	}
	
	double EvaluateObjectives(void *DataSet, const column_vector &Par)
	{
		SetParameters(DataSet, Parameters, Par, ExprCount > 0, *Syms, *Exprs, ParentWindow->ModelDll);
		
		ParentWindow->ModelDll.RunModel(DataSet);
		
		// Extract result and input values to compute them.
		
		if(!ValuesLoadedOnce)
		{
			uint64 Timesteps = ParentWindow->ModelDll.GetTimesteps(DataSet);
			InputData.resize(Targets->size());
			
			for(int Obj = 0; Obj < Targets->size(); ++Obj)
			{
				optimization_target &Target = (*Targets)[Obj];
				InputData[Obj].resize(Timesteps);
				std::vector<const char *> InputIndexes(Target.InputIndexes.size());
				for(int Idx = 0; Idx < InputIndexes.size(); ++Idx)
					InputIndexes[Idx] = Target.InputIndexes[Idx].data();
				
				//NOTE: The final 'true' signifies that we align with the result series, so that it
				//is in fact safe to use the result timesteps for the size here.
				ParentWindow->ModelDll.GetInputSeries(DataSet, Target.InputName.data(), (char**)InputIndexes.data(), InputIndexes.size(), InputData[Obj].data(), true);
			
				char TimeStr[256];
				uint64 ResultTimesteps = ParentWindow->ModelDll.GetTimesteps(DataSet);
				ParentWindow->ModelDll.GetStartDate(DataSet, TimeStr);
				Time ResultStartTime;
				StrToTime(ResultStartTime, TimeStr);
			
				Time GofStartTime;
				Time GofEndTime;
				ParentWindow->GetGofOffsets(ResultStartTime, ResultTimesteps, GofStartTime, GofEndTime, this->GofOffset, this->GofTimesteps);
			}
			
			ValuesLoadedOnce = true;
		}
		
		double Aggregate = 0.0;
		
		//NOTE: We have to allocate this for each call, for thread safety. There is no other
		//way unless Dlib could tell us what thread Id we are in, which it doesn't.
		//NOTE: Although, right now we haven't got threading to work any way...
		std::vector<double> ResultData(InputData[0].size());
		
		for(int Obj = 0; Obj < Targets->size(); ++Obj)
		{
			optimization_target &Target = (*Targets)[Obj];
			
			std::vector<const char *> ResultIndexes(Target.ResultIndexes.size());
			for(int Idx = 0; Idx < ResultIndexes.size(); ++Idx)
				ResultIndexes[Idx] = Target.ResultIndexes[Idx].data();
			
			ParentWindow->ModelDll.GetResultSeries(DataSet, Target.ResultName.data(), (char**)ResultIndexes.data(), ResultIndexes.size(), ResultData.data());
			
			double Value;
			
			if(RunType == 0)
			{
				//NOTE: It may seem a little wasteful to compute all of them, but it is a bit messy to
				//untangle their computations.
				residual_stats ResidualStats;
				ComputeResidualStats(ResidualStats, InputData[Obj].data() + GofOffset, ResultData.data() + GofOffset, GofTimesteps);
				
				if(false){}
				#define SET_SETTING(Handle, Name, Type)
				#define SET_RES_SETTING(Handle, Name, Type) \
					else if(Target.Stat == ResidualType_##Handle) Value = ResidualStats.Handle;     //TODO: Could do this with an array lookup, but would be a little hacky
		
				#include "SetStatSettings.h"
				
				#undef SET_SETTING
				#undef SET_RES_SETTING
			}
			else if(RunType == 1)
			{
				double ErrParam = Par(Target.ErrParNum);
				Value = ComputeLLValue(InputData[Obj].data() + GofOffset, ResultData.data() + GofOffset, GofTimesteps, ErrParam);
			}
			
			Aggregate += Value*Target.Weight;
		}	
		
		return Aggregate;
	}
	
	double operator()(const column_vector& Par)
	{
		//NOTE: This one is for use by the Dlib optimizer
		
		void *DataSetCopy = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false);
		
		double Value = EvaluateObjectives(DataSetCopy, Par);
		
		
		// TODO: The following is about updating the UI during optimization
		// It is not thread safe, so should be turned off if we
		// eventually are able to run the optimizer multi-threaded:
		
		++NumEvals;
		
		/////////// BEGIN UI UPDATE
		
		if(IsMaximizing)
			BestScore = std::max(BestScore, Value);
		else
			BestScore = std::min(BestScore, Value);
		
		AdditionalPlotView *View = &ParentWindow->OtherPlots;
		if(ProgressLabel && View->IsOpen() && BestScore==Value)
			ParentWindow->ModelDll.CopyData(DataSetCopy, ParentWindow->DataSet, true, false, true); //Copy over parameter and result data.
		
		if(ProgressLabel && (NumEvals%UpdateStep==0))
		{
			ProgressLabel->SetText(Format("Current evaluations: %d, best score: %g (initial: %g)", NumEvals, BestScore, InitialScore));
			
			if(View->IsOpen())
				View->BuildAll(true);
			
			ParentWindow->ProcessEvents();
		}
		
		////////// END UI UPDATE
		
		ParentWindow->ModelDll.DeleteDataSet(DataSetCopy);
		
		return Value;
	}
	
	double Evaluate(const column_vector &Pars)
	{
		//NOTE: This one is for use by the MCMC sampler.
		
		void *DataSetCopy = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false);

		double Value = EvaluateObjectives(DataSetCopy, Pars);
		
		ParentWindow->ModelDll.DeleteDataSet(DataSetCopy);
		
		return Value;
	}
	
};


struct mcmc_run_data
{
	optimization_model *Model;
	mcmc_data          *Data;
	double *MinBound;
	double *MaxBound;
};

struct mcmc_callback_data
{
	MobiView *ParentWindow;
};

bool MCMCCallbackFun(void *CallbackData, int CurStep)
{
	mcmc_callback_data *CBD = (mcmc_callback_data *)CallbackData;
	
	/*
	if(CBD->ParentWindow->MCMCResultWin.HaltWasPushed)
		return false;
	*/
	
	CBD->ParentWindow->MCMCResultWin.RefreshPlots(CurStep);
	CBD->ParentWindow->ProcessEvents();
	
	return true;
}

double MCMCLogLikelyhoodEval(void *RunData, int Walker, int Step)
{
	mcmc_run_data *RunData0 = (mcmc_run_data *)RunData;
	mcmc_data     *Data     = RunData0->Data;
	
	column_vector Pars(Data->NPars);
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		double Val = (*Data)(Walker, Par, Step);
		
		if(Val < RunData0->MinBound[Par] || Val > RunData0->MaxBound[Par])
			return -std::numeric_limits<double>::infinity();
		
		Pars(Par) = Val;
	}
	
	return RunData0->Model->Evaluate(Pars);
}

double ComputeLLValue(double *Obs, double *Sim, size_t Timesteps, double ErrParam)
{
	//TODO: We should allow different error models.
	const double Root2Pi = std::sqrt(2.0*M_PI);
	
	double Result = 0.0;
	for(int Idx = 0; Idx < Timesteps; ++Idx)
	{
		if(!std::isfinite(Sim[Idx]))
			return -std::numeric_limits<double>::infinity();
		
		if(!std::isfinite(Obs[Idx]))
			continue;
		
		double StdDev = ErrParam * Sim[Idx];
		double Factor = (Obs[Idx] - Sim[Idx]) / StdDev;
		
		Result += -std::log(StdDev*Root2Pi) - 0.5*Factor*Factor;
	}
	return Result;
}

bool OptimizationWindow::RunMobivewMCMC(size_t NWalkers, size_t NSteps, optimization_model *OptimModel, 
	double *InitialValue, double *MinBound, double *MaxBound, int InitialType, int CallbackInterval)
{
	constexpr double A = 2.0; //NOTE: Could eventually make this configurable, but according to the Emcee guys, this is a good value
	
	size_t NPars = OptimModel->FreeParCount;
	
	MCMCResultWindow *ResultWin = &ParentWindow->MCMCResultWin;
	ResultWin->ClearPlots();
	
	Data.Free();
	Data.Allocate(NWalkers, NPars, NSteps);
	
	Array<String> FreeSyms;
	for(int Par = 0; Par < OptimModel->Parameters->size(); ++Par)
		if(IsNull((*OptimModel->Exprs)[Par])) FreeSyms.push_back((*OptimModel->Syms)[Par]);
	
	ResultWin->ChoosePlotsTab.Set(0);
	ResultWin->BeginNewPlots(&Data, MinBound, MaxBound, FreeSyms);
	if(!ResultWin->IsOpen())
		ResultWin->Open();
	
	ParentWindow->ProcessEvents();
	
	std::mt19937_64 Generator;
	
	for(int Walker = 0; Walker < NWalkers; ++Walker)
	{
		for(int Par = 0; Par < NPars; ++Par)
		{
			double Initial = InitialValue[Par];
			double Draw;
			if(InitialType == 0)
			{
				//Initializing walkers to a gaussian ball around the initial parameter values.
				double StdDev = (MaxBound[Par] - MinBound[Par])/400.0;   //TODO: How to choose scaling coefficient?
				
				std::normal_distribution<double> Distribution(Initial, StdDev);
				
				Draw = Distribution(Generator);
				Draw = std::max(MinBound[Par], Draw);
				Draw = std::min(MaxBound[Par], Draw);
	
			}
			else if(InitialType == 1)
			{
				std::uniform_real_distribution<double> Distribution(MinBound[Par], MaxBound[Par]);
				Draw = Distribution(Generator);
			}
			else
				PromptOK("Internal error, wrong intial type");
			
			Data(Walker, Par, 0) = Draw;
		}
	}
	
	mcmc_run_data RunData;
	RunData.Model = OptimModel;
	RunData.Data  = &Data;
	RunData.MinBound = MinBound;
	RunData.MaxBound = MaxBound;
	
	mcmc_callback_data CallbackData;
	CallbackData.ParentWindow = ParentWindow;
	
	bool Finished = RunEmcee(MCMCLogLikelyhoodEval, (void *)&RunData, Data, A, MCMCCallbackFun, (void *)&CallbackData, CallbackInterval);
	
	return Finished;
}


void OptimizationWindow::RunClicked()
{
	TargetSetup.ErrorLabel.SetText("");
	
	if(Parameters.empty())
	{
		TargetSetup.ErrorLabel.SetText("At least one parameter must be added before running");
		ProcessEvents();
		return;
	}
	
	if(Targets.empty())
	{
		TargetSetup.ErrorLabel.SetText("There must be at least one optimization target.");
		return;
	}
	
	int Cl = 1;
	if(ParentWindow->ParametersWereChangedSinceLastSave)
		Cl = PromptYesNo("The main parameter set has been edited since the last save. If run the optimizer now it will overwrite these changes. Do you still want to run the optimizer?");
	if(!Cl)
		return;
	
	
	int RunType = TargetSetup.OptimizerTypeTab.Get(); // 0=Optimizer, 1=MCMC
	
	
	
	int ParCount = ParSetup.ParameterView.GetCount();
	
	std::unordered_map<std::string, std::pair<int,int>> SymRow;
	
	Array<String> Syms(ParCount);
	Array<String> Exprs(ParCount);
	int ExprCount = 0;
	int ActiveIdx = 0;
	if((bool)ParSetup.OptionUseExpr.Get() || RunType==1)    //TODO: Non-ideal. Instead we should maybe force use exprs for RunType MCMC
	{
		//EvalExpr TestExpression;
		for(int Row = 0; Row < ParCount; ++Row)
		{
			String Sym  = ParSetup.ParameterView.Get(Row, Id("__sym"));
			String Expr = ParSetup.ParameterView.Get(Row, Id("__expr"));
			
			if(Sym == "") Sym = Null;
			if(Expr == "") Expr = Null;
			
			if(!IsNull(Sym))
			{
				std::string Sym2 = Sym.ToStd();
				if(SymRow.find(Sym2) != SymRow.end())
				{
					TargetSetup.ErrorLabel.SetText(Format("The parameter symbol %s appears twice", Sym2.data()));
					return;
				}
				SymRow[Sym2] = {Row, ActiveIdx};
			}
			
			if(!IsNull(Expr))
				++ExprCount;
			else
				++ActiveIdx;
				
			Syms[Row]  = Sym;
			Exprs[Row] = Expr;
		}
	}
	
	
	//NOTE: Since we haven't wired all of the background data to the edit fields, we have to
	//gather some of it here.
	int RowCount = TargetSetup.TargetView.GetCount();
	bool PositiveGood;
	for(int Row = 0; Row < RowCount; ++Row)
	{
		residual_type Stat = (residual_type)(int)TargetSetup.TargetView.Get(Row, Id("__targetstat"));
		double Weight      = (double)TargetSetup.TargetView.Get(Row, Id("__weight"));
		String ErrSym      = TargetSetup.TargetView.Get(Row, Id("__errparam"));
		std::string ErrSym2 = ErrSym.ToStd();
		
		if(RunType == 0)
		{
			bool PositiveGoodLocal;
			if(false){}
			#define SET_SETTING(Handle, Name, Type)
			#define SET_RES_SETTING(Handle, Name, Type) \
				else if(Stat == ResidualType_##Handle) PositiveGoodLocal = (Type==1);
			
			#include "SetStatSettings.h"
			
			#undef SET_SETTING
			#undef SET_RES_SETTING
			
			//NOTE: We could allow people to set negative weights in order to mix different types
			//of target, but I don't see a good use case for it currently.
			
			
			
			if(Row != 0 && (PositiveGoodLocal != PositiveGood))
			{
				TargetSetup.ErrorLabel.SetText("All optimization targets have to be either minimized or maximized, no mixing is allowed.");
				return;
			}
			PositiveGood = PositiveGoodLocal;
			
			Targets[Row].Stat   = Stat;
		}
		
		if(Weight < 0.0) //NOTE: The interface should already have prevented this, but let's be safe.
		{
			TargetSetup.ErrorLabel.SetText("Negative weights are not allowed.");
			return;
		}
		
		if(RunType == 1)
		{
			if(SymRow.find(ErrSym2) == SymRow.end())
			{
				TargetSetup.ErrorLabel.SetText("At least one target lacks a valid error symbol");
				return;
			}
			
			int ParSymRow = SymRow[ErrSym2].first;
			indexed_parameter &Par = Parameters[SymRow[ErrSym2].first];
			if(!Par.Virtual)
			{
				TargetSetup.ErrorLabel.SetText("Only virtual parameters can be error parameters.");
				return;
			}
			if(!IsNull(Exprs[ParSymRow]))
			{
				TargetSetup.ErrorLabel.SetText("Only virtual parameters can not be results of expressions.");
				return;
			}
			
			Targets[Row].ErrParSym = ErrSym2;
			Targets[Row].ErrParNum = SymRow[ErrSym2].second;
		}
		

		Targets[Row].Weight = Weight;
	}
	
	Label *ProgressLabel = nullptr;
	if(RunSetup.OptionShowProgress.Get())
		ProgressLabel = &RunSetup.ProgressLabel;
	
	optimization_model OptimizationModel(ParentWindow, &Parameters, &Targets, Syms, Exprs, ProgressLabel, RunType);
	
	// Initial evaluation on the parameters given in the main dataset.
	column_vector InitialPars(OptimizationModel.FreeParCount);
	column_vector MinBound(OptimizationModel.FreeParCount);
	column_vector MaxBound(OptimizationModel.FreeParCount);
	
	ActiveIdx = 0;
	for(int Row = 0; Row < ParCount; ++Row)
	{
		if(IsNull(Exprs[Row]) || Exprs[Row]=="")
		{
			
			MinBound(ActiveIdx) = (double)ParSetup.ParameterView.Get(Row, Id("__min"));
			MaxBound(ActiveIdx) = (double)ParSetup.ParameterView.Get(Row, Id("__max"));
			
			if(MinBound(ActiveIdx) >= MaxBound(ActiveIdx))
			{
				TargetSetup.ErrorLabel.SetText(Format("The min value is larger than or equal to the max value for the parameter %s", Parameters[Row].Name.data()));
				return;
			}
			
			++ActiveIdx;
		}
	}
	
	int ParIdx = 0;
	ActiveIdx = 0;
	for(indexed_parameter Par : Parameters)
	{
		if(IsNull(Exprs[ParIdx]))
		{
			if(Par.Virtual)
			{
				//NOTE: The best guess we have for the initial value of a virtual parameter is
				//in the middle of its range. We could add a field to allow people to fill it
				//in though.
				InitialPars(ActiveIdx) = 0.5 * (MinBound(ActiveIdx) + MaxBound(ActiveIdx));
			}
			else
			{
				std::vector<const char *> Indexes;
				for(parameter_index &Index : Par.Indexes)
					Indexes.push_back(Index.Name.data());
				
				InitialPars(ActiveIdx) = ParentWindow->ModelDll.GetParameterDouble(ParentWindow->DataSet, Par.Name.data(), (char**)Indexes.data(), Indexes.size());
			}
			if(InitialPars(ActiveIdx) < MinBound(ActiveIdx))
			{
				TargetSetup.ErrorLabel.SetText(Format("The parameter \"%s\" %s has an initial value that is smaller than the assigned min bound", Par.Name.data(), MakeParameterIndexString(Par)));
				return;
			}
			if(InitialPars(ActiveIdx) > MaxBound(ActiveIdx))
			{
				TargetSetup.ErrorLabel.SetText(Format("The parameter \"%s\" %s has an initial value that is larger than the assigned max bound", Par.Name.data(), MakeParameterIndexString(Par)));
				return;
			}
			
			++ActiveIdx;
		}
		
		++ParIdx;
	}
	if(ParentWindow->CheckDllUserError()) return;
	
	//NOTE: This sets the parameters twice, but that should not be a problem
	int ErrorAtRow = SetParameters(ParentWindow->DataSet, &Parameters, InitialPars, ExprCount > 0, Syms, Exprs, ParentWindow->ModelDll);
	if(ErrorAtRow != -1)
	{
		TargetSetup.ErrorLabel.SetText(Format("Unable to parse the expression \"%s\" at row %d", Exprs[ErrorAtRow], ErrorAtRow));
		return;
	}
	
	auto Begin = std::chrono::high_resolution_clock::now();
	
	double InitialScore;
	if(RunType == 0)
	{
		InitialScore = OptimizationModel(InitialPars);
	}
	else
	{
		InitialScore = OptimizationModel.Evaluate(InitialPars);
	}
	
	auto End = std::chrono::high_resolution_clock::now();
	double Ms = std::chrono::duration_cast<std::chrono::milliseconds>(End - Begin).count();
	
	OptimizationModel.InitialScore = InitialScore;
	OptimizationModel.BestScore    = InitialScore;
	OptimizationModel.NumEvals     = 0;
	OptimizationModel.IsMaximizing = PositiveGood;
	//NOTE: Try to update about every 4 seconds (in practice it is faster since the first run is slower):
	int UpdateStep = std::ceil(4000.0/Ms);
	if(UpdateStep <= 0) UpdateStep = 1;
	OptimizationModel.UpdateStep = UpdateStep;
		
		
	if(RunType == 0)
	{
		dlib::function_evaluation InitialEval;
		InitialEval.x = InitialPars;
		InitialEval.y = PositiveGood ? InitialScore : -InitialScore;
		
		std::vector<dlib::function_evaluation> InitialEvals = {InitialEval};
		
		if(ParentWindow->CheckDllUserError())
			return;
		
		int MaxFunctionCalls = RunSetup.EditMaxEvals.GetData();
		double ExpectedDuration = Ms*1e-3*(double)MaxFunctionCalls;
			
		ParentWindow->Log(Format("Running optimization. Expected duration around %g seconds. The window will be frozen and unresponsive until the optimization is finished.", ExpectedDuration));
	
		TargetSetup.ErrorLabel.SetText("Running optimization. This may take a few minutes or more.");
		
		ParentWindow->ProcessEvents();
		
		
		dlib::function_evaluation Result;
		
		auto BeginTime = std::chrono::high_resolution_clock::now();
		
		//Note to self: For some reason, using threads crashes the application. The debugger is not
		//able to catch it. Is there an incompatibility with upp here?
		
		RunSetup.PushRun.Disable();
		MCMCSetup.PushRun.Disable();
		
		if(PositiveGood)		
			Result = dlib::find_max_global(OptimizationModel, MinBound, MaxBound, dlib::max_function_calls(MaxFunctionCalls), dlib::FOREVER, 0, InitialEvals);
		else		
			Result = dlib::find_min_global(OptimizationModel, MinBound, MaxBound, dlib::max_function_calls(MaxFunctionCalls), dlib::FOREVER, 0, InitialEvals);
		
		auto EndTime = std::chrono::high_resolution_clock::now();
		double Duration = std::chrono::duration_cast<std::chrono::seconds>(EndTime - BeginTime).count();
		
		double NewScore = Result.y;
		
		if((PositiveGood && (NewScore < InitialScore)) || (!PositiveGood && (NewScore > InitialScore)))
		{
			ParentWindow->Log("The optimizer was unable to find a better result using the given number of function evaluations");
		}
		else
		{
			SetParameters(ParentWindow->DataSet, &Parameters, Result.x, ExprCount > 0, Syms, Exprs, ParentWindow->ModelDll);
			ParentWindow->Log(Format("Optimization finished after %g seconds, with new best aggregate score: %g (old: %g). Remember to save these parameters to a different file if you don't want to overwrite your old parameter set.", 
				Duration, NewScore, InitialScore));
			ParentWindow->RunModel();  // We call the RunModel function of the ParentWindow instead of doing it directly on the dll so that plots etc. are updated.
		}
		
		RunSetup.PushRun.Enable();
		MCMCSetup.PushRun.Enable();
	}
	else if(RunType == 1)
	{
		int NWalkers    = MCMCSetup.EditWalkers.GetData();
		int NSteps      = MCMCSetup.EditSteps.GetData();
		int InitialType = MCMCSetup.InitialTypeSwitch.GetData();
		
		int NPars = OptimizationModel.FreeParCount;
		
		std::vector<double> InitialVals(NPars);
		std::vector<double> MinVals(NPars);
		std::vector<double> MaxVals(NPars);
		
		for(int Idx = 0; Idx < NPars; ++Idx)
		{
			InitialVals[Idx] = InitialPars(Idx);
			MinVals[Idx]     = MinBound(Idx);
			MaxVals[Idx]     = MaxBound(Idx);
		}
		
		auto BeginTime = std::chrono::high_resolution_clock::now();
		
		//TODO: Reduce this when threading is implemented.
		double ExpectedDuration = Ms*1e-3*(double)NWalkers*(double)NSteps;
		ParentWindow->Log(Format("Running MCMC. Expected duration around %g seconds. The window will be frozen and unresponsive until the run is finished.", ExpectedDuration));
		ParentWindow->ProcessEvents();
		
		
		int CallbackInterval = 50; //TODO: Set this up properly
		
		RunSetup.PushRun.Disable();
		MCMCSetup.PushRun.Disable();
		
		bool Finished = RunMobivewMCMC(NWalkers, NSteps, &OptimizationModel, InitialVals.data(), MinVals.data(), MaxVals.data(), InitialType, CallbackInterval);
		
		auto EndTime = std::chrono::high_resolution_clock::now();
		double Duration = std::chrono::duration_cast<std::chrono::seconds>(EndTime - BeginTime).count();
		
		if(Finished)
			ParentWindow->Log(Format("MCMC run finished after %g seconds.", Duration));
		else
			ParentWindow->Log("MCMC run halted by user");
		
		RunSetup.PushRun.Enable();
		MCMCSetup.PushRun.Enable();
	}
	
	
	RunSetup.ProgressLabel.SetText("");
	TargetSetup.ErrorLabel.SetText("");
	
	//Close(); //If we don't do this, some times the window is lost behind the main one and difficult to find...
	//TODO: This is not ideal either since it makes it stay on top of other unrelated windows...
	//TopMost(true, false);
}

void OptimizationWindow::TabChange()
{
	int TabNum = TargetSetup.OptimizerTypeTab.Get();
	
	if (TabNum == 0)             // Regular optimizer  -- Show target stat
	{
		TargetSetup.TargetView.HeaderObject().ShowTab(4);
		TargetSetup.TargetView.HeaderObject().HideTab(5);
	}
	else if (TabNum == 1)        // MCMC               -- Target stat is differently determined
	{
		TargetSetup.TargetView.HeaderObject().HideTab(4);
		TargetSetup.TargetView.HeaderObject().ShowTab(5);
	}
}













//// Serialization:
	
void OptimizationWindow::LoadFromJsonString(String &JsonData)
{
	ClearAll();
	
	Value SetupJson  = ParseJSON(JsonData);	
	
	Value MaxEvalsJson = SetupJson["MaxEvals"];
	if(!IsNull(MaxEvalsJson))
		RunSetup.EditMaxEvals.SetData((int)MaxEvalsJson);

	
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
			
			Value VirtualVal = ParamJson["Virtual"];
			if(!IsNull(VirtualVal)) Parameter.Virtual = (bool)VirtualVal;
			
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
				ParSetup.ParameterView.Set(ValidRow, Id("__unit"), UnitVal);
				Value MinVal  = ParamJson["Min"];
				ParSetup.ParameterView.Set(ValidRow, Id("__min"), MinVal);
				Value MaxVal  = ParamJson["Max"];
				ParSetup.ParameterView.Set(ValidRow, Id("__max"), MaxVal);
				
				Value SymVal  = ParamJson["Sym"];
				ParSetup.ParameterView.Set(ValidRow, Id("__sym"), SymVal);
				Value ExprVal = ParamJson["Expr"];
				ParSetup.ParameterView.Set(ValidRow, Id("__expr"), ExprVal);
				
				
				++ValidRow;
			}
		}
	}
	
	Value EnableExpr = SetupJson["EnableExpr"];
	if(!IsNull(EnableExpr))
		ParSetup.OptionUseExpr.Set((int)EnableExpr);
	
	Value ShowProgress = SetupJson["ShowProgress"];
	if(!IsNull(ShowProgress))
		RunSetup.OptionShowProgress.Set((int)ShowProgress);
	
	Value NWalkers = SetupJson["Walkers"];
	if(!IsNull(NWalkers))
		MCMCSetup.EditWalkers.SetData(NWalkers);
	
	Value NSteps   = SetupJson["Steps"];
	if(!IsNull(NSteps))
		MCMCSetup.EditSteps.SetData(NSteps);
	
	Value InitType = SetupJson["InitType"];
	if(!IsNull(InitType))
		MCMCSetup.InitialTypeSwitch.SetData(InitType);
	
	Value RunType  = SetupJson["RunType"];
	if(!IsNull(RunType))
		TargetSetup.OptimizerTypeTab.SetData(RunType);
	
	ValueArray TargetArr = SetupJson["Targets"];
	if(!IsNull(TargetArr))
	{
		int Count = TargetArr.GetCount();
		for(int Row = 0; Row < Count; ++Row)
		{
			Value TargetJson = TargetArr[Row];
			
			optimization_target Target = {};
			
			String ResultName = TargetJson["ResultName"];
			if(!IsNull(ResultName))
				Target.ResultName = ResultName.ToStd();
			
			String InputName  = TargetJson["InputName"];
			if(!IsNull(InputName))
				Target.InputName  = InputName.ToStd();
			
			Target.Stat = ResidualType_MAE;
			
			String StatName = TargetJson["Stat"];
			if(!IsNull(StatName))
			{
				if(false){}
				#define SET_SETTING(Handle, Name, Type)
				#define SET_RES_SETTING(Handle, Name, Type) \
					else if(String(Name) == StatName) Target.Stat = ResidualType_##Handle;
				
				#include "SetStatSettings.h"
				
				#undef SET_SETTING
				#undef SET_RES_SETTING	
			}
			
			ValueArray ResultIndexArr = TargetJson["ResultIndexes"];
			if(!IsNull(ResultIndexArr))
			{
				int Count2 = ResultIndexArr.GetCount();
				for(int Row2 = 0; Row2 < Count2; ++Row2)
				{
					String Index = ResultIndexArr[Row2];
					Target.ResultIndexes.push_back(Index.ToStd());
				}
			}
			
			ValueArray InputIndexArr = TargetJson["InputIndexes"];
			if(!IsNull(InputIndexArr))
			{
				int Count2 = InputIndexArr.GetCount();
				for(int Row2 = 0; Row2 < Count2; ++Row2)
				{
					String Index = InputIndexArr[Row2];
					Target.InputIndexes.push_back(Index.ToStd());
				}
			}
			
			Value WeightVal = TargetJson["Weight"];
			if(!IsNull(WeightVal)) Target.Weight = (double)WeightVal;
			
			Value ErrPar = TargetJson["ErrPar"];
			if(!IsNull(ErrPar)) Target.ErrParSym = ErrPar.ToString().ToStd();
			
			AddOptimizationTarget(Target);
		}
	}
	
	EnableExpressionsClicked();
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
	
	String JsonData = LoadFile(Filename);
	
	LoadFromJsonString(JsonData);
}

String OptimizationWindow::SaveToJsonString()
{
	Json MainFile;
	
	MainFile("MaxEvals", RunSetup.EditMaxEvals.GetData());
	
	JsonArray ParameterArr;
	
	//TODO: Factor out a serialization method for an indexed parameter?
	int Row = 0;
	for(indexed_parameter &Par : Parameters)
	{
		Json ParJson;
		ParJson("Name", Par.Name.data());
		ParJson("Virtual", Par.Virtual);
		
		ParJson("Unit", ParSetup.ParameterView.Get(Row, Id("__unit")));
		ParJson("Min", (double)ParSetup.ParameterView.Get(Row, Id("__min")));
		ParJson("Max", (double)ParSetup.ParameterView.Get(Row, Id("__max")));
		ParJson("Sym", ParSetup.ParameterView.Get(Row, Id("__sym")));
		ParJson("Expr", ParSetup.ParameterView.Get(Row, Id("__expr")));
		
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
	
	bool EnableExpr = (bool)ParSetup.OptionUseExpr.Get();
	MainFile("EnableExpr", EnableExpr);
	
	bool ShowProgress = (bool)RunSetup.OptionShowProgress.Get();
	MainFile("ShowProgress", ShowProgress);
	
	int NWalkers = MCMCSetup.EditWalkers.GetData();
	MainFile("Walkers", NWalkers);
	
	int NSteps   = MCMCSetup.EditSteps.GetData();
	MainFile("Steps", NSteps);
	
	int InitType = MCMCSetup.InitialTypeSwitch.GetData();
	MainFile("InitType", InitType);
	
	int RunType  = TargetSetup.OptimizerTypeTab.Get();
	MainFile("RunType", RunType);
	
	JsonArray TargetArr;
	
	Row = 0;
	for(optimization_target &Target : Targets)
	{
		Json TargetJson;
		TargetJson("ResultName", Target.ResultName.data());
		TargetJson("InputName", Target.InputName.data());
		
		JsonArray ResultIndexArr;
		for(std::string &Index : Target.ResultIndexes)
			ResultIndexArr << Index.data();
		TargetJson("ResultIndexes", ResultIndexArr);
		
		JsonArray InputIndexArr;
		for(std::string &Index : Target.InputIndexes)
			InputIndexArr << Index.data();
		TargetJson("InputIndexes", InputIndexArr);
		
		String StatName = TargetStatCtrls[Row].GetValue();
		double Weight   = TargetSetup.TargetView.Get(Row, Id("__weight"));
		String ErrName  = TargetSetup.TargetView.Get(Row, Id("__errparam"));
		
		TargetJson("Stat", StatName);
		TargetJson("Weight", Weight);
		TargetJson("ErrPar", ErrName);
		
		TargetArr << TargetJson;
		
		++Row;
	}
	
	MainFile("Targets", TargetArr);
	
	return MainFile.ToString();
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
	
	String JsonData = SaveToJsonString();
	
	SaveFile(Filename, JsonData);
}








/*
//#define MOBIVIEW_USE_THREADS

#ifdef MOBIVIEW_USE_THREADS
	
	unsigned long ThreadCount = 0;
	bool Found = false;
	try
    {
        char* nt = getenv("DLIB_NUM_THREADS");
        if (nt)
        {
            ThreadCount = dlib::string_cast<unsigned long>(nt);
            Found = true;
        }
    }
    catch(dlib::string_cast_error&) {}
    
    if(!Found)
        ThreadCount = std::thread::hardware_concurrency();

	ParentWindow->Log(Format("Running optimization using %d parallell threads. This may take a few minutes or more. The window will be frozen and unresponsive until the optimization is finished.", (int64)ThreadCount));
#else		
	ParentWindow->Log("Running optimization. This may take a few minutes or more. The window will be frozen and unresponsive until the optimization is finished.");
#endif

	TargetSetup.ErrorLabel.SetText("Running optimization. This may take a few minutes or more.");
	
	ParentWindow->ProcessEvents();
	
	int MaxFunctionCalls = RunSetup.EditMaxEvals.GetData();
	dlib::function_evaluation Result;
	
	auto BeginTime = std::chrono::high_resolution_clock::now();
	
#ifdef MOBIVIEW_USE_THREADS
	//dlib::thread_pool &ThreadPool = dlib::default_thread_pool();
	dlib::thread_pool ThreadPool(ThreadCount);
#endif
	//Note to self: For some reason, using threads crashes the application. The debugger is not
	//able to catch it. Is there an incompatibility with upp here?
	
	if(PositiveGood)
#ifdef MOBIVIEW_USE_THREADS
		Result = dlib::find_max_global(ThreadPool, OptimizationModel , MinBound, MaxBound, dlib::max_function_calls(MaxFunctionCalls), dlib::FOREVER, 0, InitialEvals);
#else		
		Result = dlib::find_max_global(OptimizationModel, MinBound, MaxBound, dlib::max_function_calls(MaxFunctionCalls), dlib::FOREVER, 0, InitialEvals);
#endif
	else
#ifdef MOBIVIEW_USE_THREADS
		Result = dlib::find_min_global(ThreadPool, OptimizationModel , MinBound, MaxBound, dlib::max_function_calls(MaxFunctionCalls), dlib::FOREVER, 0, InitialEvals);
#else		
		Result = dlib::find_min_global(OptimizationModel, MinBound, MaxBound, dlib::max_function_calls(MaxFunctionCalls), dlib::FOREVER, 0, InitialEvals);
#endif
*/