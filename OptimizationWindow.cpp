

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

SensitivityRunSetup::SensitivityRunSetup()
{
	CtrlLayout(*this);
}

target_stat_class
GetStatClass(optimization_target &Target)
{
	target_stat_class Result = StatClass_Unknown;
	int Stat = (int)Target.Stat;
	if(Stat > (int)StatType_Offset && Stat < (int)StatType_End)
		Result = StatClass_Stat;
	else if(Stat > (int)ResidualType_Offset && Stat < (int)ResidualType_End)
		Result = StatClass_Residual;
	else if(Stat > (int)MCMCError_Offset && Stat < (int)MCMCError_End)
		Result = StatClass_LogLikelihood;
	
	return Result;
}


OptimizationWindow::OptimizationWindow()
{
	SetRect(0, 0, 740, 740);
	Title("MobiView optimization and MCMC setup").Sizeable().Zoomable();
	
	ParSetup.ParameterView.AddColumn(Id("__name"), "Name");
	ParSetup.ParameterView.AddColumn(Id("__indexes"), "Indexes");
	ParSetup.ParameterView.AddColumn(Id("__min"), "Min");
	ParSetup.ParameterView.AddColumn(Id("__max"), "Max");
	ParSetup.ParameterView.AddColumn(Id("__unit"), "Unit");
	ParSetup.ParameterView.AddColumn(Id("__sym"), "Symbol");
	ParSetup.ParameterView.AddColumn(Id("__expr"), "Expression");
	
	ParSetup.OptionUseExpr.Set((int)false);
	ParSetup.OptionUseExpr.WhenAction     = THISBACK(EnableExpressionsClicked);
	ParSetup.ParameterView.HeaderObject().HideTab(5);
	ParSetup.ParameterView.HeaderObject().HideTab(6);
	
	TargetSetup.TargetView.AddColumn(Id("__resultname"), "Result name");
	TargetSetup.TargetView.AddColumn(Id("__resultindexes"), "Result idxs.");
	TargetSetup.TargetView.AddColumn(Id("__inputname"), "Input name");
	TargetSetup.TargetView.AddColumn(Id("__inputindexes"), "Input idxs.");
	TargetSetup.TargetView.AddColumn(Id("__targetstat"), "Statistic");
	TargetSetup.TargetView.AddColumn(Id("__errparam"), "Error param(s).");
	TargetSetup.TargetView.AddColumn(Id("__weight"), "Weight");
	TargetSetup.TargetView.AddColumn(Id("__begin"), "Begin");
	TargetSetup.TargetView.AddColumn(Id("__end"), "End");
	
	//TargetSetup.TargetView.HeaderObject().HideTab(5);
	
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
	
	RunSetup.PushRun.WhenPush          = [&](){ RunClicked(0); };
	RunSetup.PushRun.SetImage(IconImg4::Run());
	
	RunSetup.EditMaxEvals.Min(1);
	RunSetup.EditMaxEvals.SetData(1000);
	
	
	MCMCSetup.PushRun.WhenPush         = [&](){ RunClicked(1); };
	MCMCSetup.PushRun.SetImage(IconImg4::Run());
	MCMCSetup.PushViewChains.WhenPush << [&]() { if(!ParentWindow->MCMCResultWin.IsOpen()) ParentWindow->MCMCResultWin.Open(); };
	MCMCSetup.PushViewChains.SetImage(IconImg4::ViewMorePlots());
	MCMCSetup.PushExtendRun.WhenPush   = [&](){ RunClicked(2); };
	MCMCSetup.PushExtendRun.Disable();
	MCMCSetup.PushExtendRun.SetImage(IconImg4::Run());
	
	MCMCSetup.EditSteps.Min(10);
	MCMCSetup.EditSteps.SetData(1000);
	MCMCSetup.EditWalkers.Min(10);
	MCMCSetup.EditWalkers.SetData(25);
	MCMCSetup.InitialTypeSwitch.SetData(0);
	
	MCMCSetup.SamplerParamView.AddColumn("Name");
	MCMCSetup.SamplerParamView.AddColumn("Value");
	MCMCSetup.SamplerParamView.AddColumn("Description");
	MCMCSetup.SamplerParamView.ColumnWidths("3 2 5");
	
	MCMCSetup.SelectSampler.Add((int)MCMCMethod_AffineStretch, "Affine stretch (recommended)");
	//MCMCSetup.SelectSampler.Add((int)MCMCMethod_AffineWalk, "Affine walk");     //NOTE: This
	//seems to be broken, at least it works very poorly.
	MCMCSetup.SelectSampler.Add((int)MCMCMethod_DifferentialEvolution, "Differential evolution");
	MCMCSetup.SelectSampler.GoBegin();
	MCMCSetup.SelectSampler.WhenAction << THISBACK(SamplerMethodSelected);
	SamplerMethodSelected(); //To set the sampler parameters for the initial selection
	
	SensitivitySetup.EditSampleSize.Min(10);
	SensitivitySetup.EditSampleSize.SetData(1000);
	SensitivitySetup.PushRun.WhenPush  = [&](){ RunClicked(3); };
	SensitivitySetup.PushViewResults.WhenPush = [&]() {	if(!ParentWindow->VarSensitivityWin.IsOpen()) ParentWindow->VarSensitivityWin.Open(); };
	SensitivitySetup.PushRun.SetImage(IconImg4::Run());
	SensitivitySetup.PushViewResults.SetImage(IconImg4::ViewMorePlots());
	
	AddFrame(Tool);
	Tool.Set(THISBACK(SubBar));
	
	TargetSetup.OptimizerTypeTab.Add(RunSetup.SizePos(), "Optimizer");
	TargetSetup.OptimizerTypeTab.Add(MCMCSetup.SizePos(), "MCMC");
	TargetSetup.OptimizerTypeTab.Add(SensitivitySetup.SizePos(), "Variance based sensitivity");
	
	TargetSetup.OptimizerTypeTab.WhenSet << THISBACK(TabChange);
	
	MainVertical.Vert();
	MainVertical.Add(ParSetup);
	MainVertical.Add(TargetSetup);
	Add(MainVertical.SizePos());
}

void OptimizationWindow::SetError(const String &ErrStr)
{
	TargetSetup.ErrorLabel.SetText(ErrStr);
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


bool OptimizationWindow::AddSingleParameter(indexed_parameter &Parameter, int SourceRow, bool ReadAdditionalData)
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
		//TODO: Maybe min and max should be put on the indexed_parameter, but that is pretty superfluous
		//for the other purposes this struct is used for.
		double Min = Null;
		double Max = Null;
		String ParUnit = "";
		String Symbol = Parameter.Symbol.data();
		String Expr   = Parameter.Expr.data();
		
		if(ReadAdditionalData)
		{
			Min     = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__min"));
			Max     = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__max"));
			ParUnit = ParentWindow->Params.ParameterView.Get(SourceRow, Id("__unit"));
			
			Symbol  = ParentWindow->ModelDll.GetParameterShortName(ParentWindow->DataSet, Parameter.Name.data());
		}
		
		Parameter.Symbol = Symbol.ToStd();
		Parameters.push_back(Parameter);
		
		ParSetup.ParameterView.Add(Parameter.Name.data(), Indexes, Min, Max, ParUnit, Symbol, Expr);
		
		int Row = Parameters.size()-1;
		
		EditMinCtrls.Create<EditDoubleNotNull>();
		EditMaxCtrls.Create<EditDoubleNotNull>();
		ParSetup.ParameterView.SetCtrl(Row, 2, EditMinCtrls[Row]);
		ParSetup.ParameterView.SetCtrl(Row, 3, EditMaxCtrls[Row]);
		
		EditSymCtrls.Create<EditField>();
		EditExprCtrls.Create<EditField>();
		ParSetup.ParameterView.SetCtrl(Row, 5, EditSymCtrls[Row]);
		ParSetup.ParameterView.SetCtrl(Row, 6, EditExprCtrls[Row]);
		
		EditSymCtrls[Row].WhenAction <<  [this, Row](){ SymbolEdited(Row); };
		EditExprCtrls[Row].WhenAction << [this, Row](){ ExprEdited(Row); };
	}
	else
	{
		ParSetup.ParameterView.Set(OverrideRow, 1, Indexes);
	}
	
	return true;
}

void OptimizationWindow::SymbolEdited(int Row)
{
	String Symbol = ParSetup.ParameterView.Get(Row, "__sym");
	Parameters[Row].Symbol = Symbol.ToStd();
}

void OptimizationWindow::ExprEdited(int Row)
{
	String Expr = ParSetup.ParameterView.Get(Row, "__expr");
	Parameters[Row].Expr = Expr.ToStd();
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
	
	int TargetStat = (int)Target.Stat;;
	
	TargetSetup.TargetView.Add(Target.ResultName.data(), ResultIndexStr, Target.InputName.data(), InputIndexStr, TargetStat, Target.ErrParSym.data(), Target.Weight, Target.Begin.data(), Target.End.data());
	
	TargetStatCtrls.Create<DropList>();
	DropList &SelectStat = TargetStatCtrls.Top();
	
	TargetErrCtrls.Create<EditField>();
	EditField &ErrCtrl = TargetErrCtrls.Top();
	
	int TabNum = TargetSetup.OptimizerTypeTab.Get(); 
	
	if(TabNum == 2)
	{
		#define SET_SETTING(Handle, Name, Type) SelectStat.Add((int)StatType_##Handle, Name);
		#define SET_RES_SETTING(Handle, Name, Type)
		#include "SetStatSettings.h"
		#undef SET_SETTING
		#undef SET_RES_SETTING
	}
	if(TabNum == 0 || TabNum == 2)
	{
		#define SET_SETTING(Handle, Name, Type)
		#define SET_RES_SETTING(Handle, Name, Type) if(Type != -1) SelectStat.Add((int)ResidualType_##Handle, Name);
		#include "SetStatSettings.h"
		#undef SET_SETTING
		#undef SET_RES_SETTING
	}
	if(TabNum == 0 || TabNum == 1)
	{
		#define SET_LL_SETTING(Handle, Name, NumErr) SelectStat.Add((int)MCMCError_##Handle, Name);
		#include "LLSettings.h"
		#undef SET_LL_SETTING
	}
	
	
	int Row = TargetSetup.TargetView.GetCount() - 1;
	int Col = TargetSetup.TargetView.GetPos(Id("__targetstat"));
	TargetSetup.TargetView.SetCtrl(Row, Col, SelectStat);
	SelectStat.WhenAction << [this, Row](){ StatEdited(Row); };
	
	Col     = TargetSetup.TargetView.GetPos(Id("__errparam"));
	TargetSetup.TargetView.SetCtrl(Row, Col, ErrCtrl);
	ErrCtrl.WhenAction << [this, Row](){ ErrSymEdited(Row); };
	
	TargetWeightCtrls.Create<EditDoubleNotNull>();
	EditDoubleNotNull &EditWt = TargetWeightCtrls.Top();
	EditWt.Min(0.0);
	Col     = TargetSetup.TargetView.GetPos(Id("__weight"));
	TargetSetup.TargetView.SetCtrl(Row, Col, EditWt);
	EditWt.WhenAction << [this, Row](){ WeightEdited(Row); };
	
	TargetBeginCtrls.Create<EditTimeNotNull>();
	EditTimeNotNull &EditBegin = TargetBeginCtrls.Top();
	Col = TargetSetup.TargetView.GetPos(Id("__begin"));
	TargetSetup.TargetView.SetCtrl(Row, Col, EditBegin);
	EditBegin.WhenAction << [this, Row](){ BeginEdited(Row); };
	
	TargetEndCtrls.Create<EditTimeNotNull>();
	EditTimeNotNull &EditEnd = TargetEndCtrls.Top();
	Col = TargetSetup.TargetView.GetPos(Id("__end"));
	TargetSetup.TargetView.SetCtrl(Row, Col, EditEnd);
	EditEnd.WhenAction << [this, Row](){ EndEdited(Row); };
}

void OptimizationWindow::StatEdited(int Row)
{
	int IntValue = (int)TargetSetup.TargetView.Get(Row, "__targetstat");
	
	if(IntValue > (int)StatType_Offset && IntValue < (int)StatType_End)
		Targets[Row].Stat              = (stat_type)IntValue;
	else if(IntValue > (int)ResidualType_Offset && IntValue < (int)ResidualType_End)
		Targets[Row].ResidualStat      = (residual_type)IntValue;
	else if(IntValue > (int)MCMCError_Offset && IntValue < (int)MCMCError_End)
		Targets[Row].ErrStruct         = (mcmc_error_structure)IntValue;
}

void OptimizationWindow::ErrSymEdited(int Row)
{
	Targets[Row].ErrParSym = TargetSetup.TargetView.Get(Row, "__errparam").ToStd();
}

void OptimizationWindow::WeightEdited(int Row)
{
	Targets[Row].Weight = (double)TargetSetup.TargetView.Get(Row, "__weight");
}

void OptimizationWindow::BeginEdited(int Row)
{
	Targets[Row].Begin = TargetSetup.TargetView.Get(Row, "__begin").ToString().ToStd();
}

void OptimizationWindow::EndEdited(int Row)
{
	Targets[Row].End   = TargetSetup.TargetView.Get(Row, "__end").ToString().ToStd();
}


void OptimizationWindow::AddTargetClicked()
{
	plot_setup PlotSetup;
	ParentWindow->Plotter.GatherCurrentPlotSetup(PlotSetup);
	
	if(PlotSetup.SelectedResults.size() != 1 || PlotSetup.SelectedInputs.size() > 1)
	{
		SetError("This only works with a single selected result series and at most one input series.");
		return;
	}
	
	for(int Idx = 0; Idx < PlotSetup.SelectedIndexes.size(); ++Idx)
	{
		if(PlotSetup.SelectedIndexes[Idx].size() != 1 && PlotSetup.IndexSetIsActive[Idx])
		{
			SetError("This currently only works with a single selected index combination");
			return;
		}
	}
	
	optimization_target Target = {};
	Target.ResultName = PlotSetup.SelectedResults[0];
	std::vector<char *> ResultIndexes;
	bool Success      = ParentWindow->GetSelectedIndexesForSeries(PlotSetup, ParentWindow->DataSet, Target.ResultName, 0, ResultIndexes);
	if(!Success) return;
	//Ugh, super annoying to have to convert back and forth between char* and string to ensure
	//storage...
	for(const char *Idx : ResultIndexes)
			Target.ResultIndexes.push_back(std::string(Idx));
	
	if(PlotSetup.SelectedInputs.size() == 1)
	{
		Target.InputName  = PlotSetup.SelectedInputs[0];
		std::vector<char *> InputIndexes;
		bool Success = ParentWindow->GetSelectedIndexesForSeries(PlotSetup, ParentWindow->DataSet, Target.InputName, 1, InputIndexes);
		if(!Success) return;
		for(const char *Idx : InputIndexes)
			Target.InputIndexes.push_back(std::string(Idx));
	}
	
	int TabNum = TargetSetup.OptimizerTypeTab.Get(); 
	//NOTE: Defaults.
	if(TabNum == 0) Target.ResidualStat = ResidualType_MAE;
	else if(TabNum == 1) Target.ErrStruct = MCMCError_NormalHet1; //This is the easiest one to start with maybe. Or should we just do Normal?
	else if(TabNum == 2) Target.Stat = StatType_Mean;

	Target.Weight = 1.0;
	
	//NOTE for you to be able to add a target, the model has to have been run once any way, so
	//there is no problem using these time stamps for that.
	char TimeStr[256];
	uint64 ResultTimesteps = ParentWindow->ModelDll.GetTimesteps(ParentWindow->DataSet);
	ParentWindow->ModelDll.GetStartDate(ParentWindow->DataSet, TimeStr);
	Time ResultStartTime;
	StrToTime(ResultStartTime, TimeStr);

	Time GofStartTime, GofEndTime;
	int64 GofOffset, GofTimesteps;
	ParentWindow->GetGofOffsets(ResultStartTime, ResultTimesteps, GofStartTime, GofEndTime, GofOffset, GofTimesteps);
	Target.Begin = Format(GofStartTime).ToStd();
	Target.End   = Format(GofEndTime).ToStd();
	
	AddOptimizationTarget(Target);
}


void OptimizationWindow::TargetsToPlotSetups(std::vector<optimization_target> &Targets, std::vector<plot_setup> &PlotSetups)
{
	PlotSetups.clear();
	PlotSetups.reserve(Targets.size());
	
	for(optimization_target &Target : Targets)
	{
		plot_setup PlotSetup = {};
		PlotSetup.SelectedIndexes.resize(MAX_INDEX_SETS);
		PlotSetup.IndexSetIsActive.resize(MAX_INDEX_SETS);
		
		PlotSetup.MajorMode = MajorMode_Regular;//MajorMode_Residuals;
		PlotSetup.AggregationPeriod = Aggregation_None;
		PlotSetup.ScatterInputs = true;
		
		std::vector<char *> IndexSets;
		
		if(Target.InputName != "")
		{
			PlotSetup.SelectedInputs.push_back(Target.InputName);
		
			bool Success = ParentWindow->GetIndexSetsForSeries(ParentWindow->DataSet, Target.InputName, 1, IndexSets);
			if(!Success) return;
			
			for(int IdxIdx = 0; IdxIdx < IndexSets.size(); ++IdxIdx)
			{
				size_t Id = ParentWindow->IndexSetNameToId[IndexSets[IdxIdx]];
				PlotSetup.IndexSetIsActive[Id] = true;
				PlotSetup.SelectedIndexes[Id].push_back(Target.InputIndexes[IdxIdx]);
			}
		}

		PlotSetup.SelectedResults.push_back(Target.ResultName);

		IndexSets.clear();
		bool Success = ParentWindow->GetIndexSetsForSeries(ParentWindow->DataSet, Target.ResultName, 0, IndexSets);
		if(!Success) return;
		
		for(int IdxIdx = 0; IdxIdx < IndexSets.size(); ++IdxIdx)
		{
			size_t Id = ParentWindow->IndexSetNameToId[IndexSets[IdxIdx]];
			PlotSetup.IndexSetIsActive[Id] = true;
			PlotSetup.SelectedIndexes[Id].push_back(Target.ResultIndexes[IdxIdx]);
		}
		
		PlotSetups.push_back(PlotSetup);
	}
}


void OptimizationWindow::DisplayClicked()
{
	if(!ParentWindow->DataSet || !ParentWindow->ModelDll.IsLoaded())
	{
		SetError("Can't display plots before a model is loaded");
		return;
	}
	
	if(Targets.empty())
	{
		SetError("There are no targets to display the plots of");
		return;
	}
	
	SetError("");
	
	std::vector<plot_setup> PlotSetups;
	TargetsToPlotSetups(Targets, PlotSetups);
	
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
	TargetBeginCtrls.Remove(SelectedRow);
	TargetEndCtrls.Remove(SelectedRow);
}

void OptimizationWindow::ClearTargetsClicked()
{
	TargetSetup.TargetView.Clear();
	Targets.clear();
	TargetWeightCtrls.Clear();
	TargetStatCtrls.Clear();
	TargetErrCtrls.Clear();
	TargetBeginCtrls.Clear();
	TargetEndCtrls.Clear();
}
	
void OptimizationWindow::ClearAll()
{
	ClearParametersClicked();
	ClearTargetsClicked();
}

void OptimizationWindow::SamplerMethodSelected()
{
	mcmc_sampler_method Method = (mcmc_sampler_method)(int)MCMCSetup.SelectSampler.GetData();
	
	MCMCSetup.SamplerParamView.Clear();
	MCMCSamplerParamCtrls.Clear();
	
	switch(Method)
	{
		case MCMCMethod_AffineStretch :
		{
			MCMCSetup.SamplerParamView.Add("a", 2.0, "Max. stretch factor");
		} break;
		
		case MCMCMethod_AffineWalk :
		{
			MCMCSetup.SamplerParamView.Add("|S|", 10.0, "Size of sub-ensemble. Has to be between 2 and NParams/2.");
		} break;
		
		case MCMCMethod_DifferentialEvolution :
		{
			MCMCSetup.SamplerParamView.Add("\u03B3", 0.1, "Stretch factor. Should be about 2.38/sqrt(2*dim)");
			MCMCSetup.SamplerParamView.Add("b", 1e-4, "Max. random walk step");
			//TODO: Crossover probability
		} break;
	}
	
	int Rows = MCMCSetup.SamplerParamView.GetCount();
	MCMCSamplerParamCtrls.InsertN(0, Rows);
	for(int Row = 0; Row < Rows; ++Row)
		MCMCSetup.SamplerParamView.SetCtrl(Row, 1, MCMCSamplerParamCtrls[Row]);
}



typedef dlib::matrix<double,0,1> column_vector;

inline int
SetParameters(void *DataSet, std::vector<indexed_parameter> *Parameters, const column_vector& Par, bool UseExpr, model_dll_interface &ModelDll)
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
			if(Param.Expr != "")
			{
				auto Res = Expression.Eval(Param.Expr.data());
				if(IsNull(Res)) return ParIdx;
				Val = Res.val;
			}
			else
			{
				Val = Par(ActiveIdx);
				++ActiveIdx;
			}
			
			if(Param.Symbol != "")
			{
				auto Res = Expression.AssignVariable(Param.Symbol.data(), Val);
				if(IsNull(Res)) return ParIdx;
			}
			
			if(!Param.Virtual)
				SetParameterValue(Param, DataSet, Val, ModelDll);
			++ParIdx;
		}
	}
	
	return -1;
}


double ComputeLLValue(double *Obs, double *Sim, size_t Timesteps, const std::vector<double> &ErrParam, mcmc_error_structure ErrStruct);

struct optimization_model
{
	MobiView                         *ParentWindow;
	Label                            *ProgressLabel;
	
	std::vector<indexed_parameter>   *Parameters;
	std::vector<optimization_target> *Targets;
	
	bool                             ValuesLoadedOnce = false;
	std::vector<std::vector<double>> InputData;
	
	std::vector<int64> GofOffsets;
	std::vector<int64> GofTimesteps;
	
	int ExprCount;
	int FreeParCount;
	
	int64 NumEvals = 0;
	double InitialScore, BestScore;
	bool  IsMaximizing = true;
	int   UpdateStep = 100;
	
	void *DataSetBase = nullptr;
	
	
	optimization_model(MobiView *ParentWindow, std::vector<indexed_parameter> *Parameters, std::vector<optimization_target> *Targets,
		Label *ProgressLabel, void *DataSetBase)
	{
		this->ParentWindow = ParentWindow;
		this->Parameters   = Parameters;
		this->Targets      = Targets;
		this->ProgressLabel = ProgressLabel;
		
		ExprCount = 0;
		for(indexed_parameter &Parameter : *Parameters)
			if(Parameter.Expr != "")
				ExprCount++;
		FreeParCount = Parameters->size() - ExprCount;
		
		this->DataSetBase = DataSetBase;
	}
	
	double EvaluateObjectives(void *DataSet, const column_vector &Par)
	{
		SetParameters(DataSet, Parameters, Par, ExprCount > 0, ParentWindow->ModelDll);
		
		ParentWindow->ModelDll.RunModel(DataSet);
		
		// Extract result and input values to compute them.
		
		if(!ValuesLoadedOnce)
		{
			uint64 Timesteps = ParentWindow->ModelDll.GetTimesteps(DataSet);
			InputData.resize(Targets->size());
			
			GofOffsets.resize(Targets->size());
			GofTimesteps.resize(Targets->size());
			
			char TimeStr[256];
			ParentWindow->ModelDll.GetStartDate(DataSet, TimeStr);
			Time ResultStartTime;
			StrToTime(ResultStartTime, TimeStr);
			
			for(int Obj = 0; Obj < Targets->size(); ++Obj)
			{
				optimization_target &Target = (*Targets)[Obj];
				InputData[Obj].resize(Timesteps);   //TODO: This is sometimes unnecessary, but the size is used below.
				if(Target.InputName != "")
				{
					std::vector<const char *> InputIndexes(Target.InputIndexes.size());
					for(int Idx = 0; Idx < InputIndexes.size(); ++Idx)
						InputIndexes[Idx] = Target.InputIndexes[Idx].data();
					
					//NOTE: The final 'true' signifies that we align with the result series, so that it
					//is in fact safe to use the result timesteps for the size here.
					ParentWindow->ModelDll.GetInputSeries(DataSet, Target.InputName.data(), (char**)InputIndexes.data(), InputIndexes.size(), InputData[Obj].data(), true);
				}
				
				Time Begin;
				Time End;
				StrToTime(Begin, Target.Begin.data());
				StrToTime(End, Target.End.data());
				
				GofOffsets[Obj]   = TimestepsBetween(ResultStartTime, Begin, ParentWindow->TimestepSize);
				GofTimesteps[Obj] = TimestepsBetween(Begin, End, ParentWindow->TimestepSize) + 1; //NOTE: if start time = end time, there is still one timestep.
			}
			
			ValuesLoadedOnce = true;
		}
		
		double Aggregate = 0.0;
		
		//NOTE: We have to allocate this for each call, for thread safety.
		std::vector<double> ResultData(InputData[0].size());
		
		for(int Obj = 0; Obj < Targets->size(); ++Obj)
		{
			optimization_target &Target = (*Targets)[Obj];
			
			std::vector<const char *> ResultIndexes(Target.ResultIndexes.size());
			for(int Idx = 0; Idx < ResultIndexes.size(); ++Idx)
				ResultIndexes[Idx] = Target.ResultIndexes[Idx].data();
			
			ParentWindow->ModelDll.GetResultSeries(DataSet, Target.ResultName.data(), (char**)ResultIndexes.data(), ResultIndexes.size(), ResultData.data());
			
			double Value;
			
			
			switch(GetStatClass(Target))
			{
				case StatClass_Stat:
				{
					timeseries_stats Stats;
					ComputeTimeseriesStats(Stats, ResultData.data() + GofOffsets[Obj], GofTimesteps[Obj], ParentWindow->StatSettings, false);
					
					if(false){}
					#define SET_SETTING(Handle, Name, Type) \
						else if(Target.Stat == StatType_##Handle) Value = Stats.Handle;
					#define SET_RES_SETTING(Handle, Name, Type)
					#include "SetStatSettings.h"
					#undef SET_SETTING
					#undef SET_RES_SETTING
				} break;
				
				case StatClass_Residual:
				{
					//NOTE: It may seem a little wasteful to compute all of them, but it is a bit messy to
					//untangle their computations.
					residual_stats ResidualStats;
					ComputeResidualStats(ResidualStats, InputData[Obj].data() + GofOffsets[Obj], ResultData.data() + GofOffsets[Obj], GofTimesteps[Obj]);
					
					if(false){}
					#define SET_SETTING(Handle, Name, Type)
					#define SET_RES_SETTING(Handle, Name, Type) \
						else if(Target.ResidualStat == ResidualType_##Handle) Value = ResidualStats.Handle;     //TODO: Could do this with an array lookup, but would be a little hacky
					#include "SetStatSettings.h"
					#undef SET_SETTING
					#undef SET_RES_SETTING
				} break;
				
				case StatClass_LogLikelihood:
				{
					std::vector<double> ErrParam(Target.ErrParNum.size());
					for(int Idx = 0; Idx < ErrParam.size(); ++Idx) ErrParam[Idx] = Par(Target.ErrParNum[Idx]);
					Value = ComputeLLValue(InputData[Obj].data() + GofOffsets[Obj], ResultData.data() + GofOffsets[Obj], GofTimesteps[Obj], ErrParam, Target.ErrStruct);
				} break;
				
				default :
					assert(!"Unknown stat class!");
			}
			
			Aggregate += Value*Target.Weight;
		}
		
		return Aggregate;
	}

	
	double operator()(const column_vector& Par)
	{
		//NOTE: This one is for use by the Dlib optimizer
		
		//NOTE: Since the optimizer is not multithreaded, there is no reason to do an
		//additional copy each run.
		//If we ever do get threading to work for this, we have to have a separate copy per
		//paralell run.
		
		//void *DataSetCopy = ParentWindow->ModelDll.CopyDataSet(DataSetBase, false);
		void *DataSetCopy = DataSetBase;
		
		double Value = EvaluateObjectives(DataSetCopy, Par);
		
		
		// NOTE: The following is about updating the UI during optimization
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
			GuiLock Lock;
			
			ProgressLabel->SetText(Format("Current evaluations: %d, best score: %g (initial: %g)", NumEvals, BestScore, InitialScore));
			
			if(View->IsOpen())
				View->BuildAll(true);
			
			ParentWindow->ProcessEvents();
		}
		
		////////// END UI UPDATE
		
		//NOTE: See note about threading above
		//ParentWindow->ModelDll.DeleteDataSet(DataSetCopy);
		
		return IsMaximizing ? Value : -Value;
	}
	
};


void OptimizationWindow::SetParameterValues(void *DataSet, double *ParVal, size_t NPars, std::vector<indexed_parameter> &Parameters)
{
	//NOTE: This one is only here to be used by the MCMC result window when it wants to write
	//back parameter sets. TODO: This may not be the best way of organizing it...
	
	int ExprCount = 0;
	for(indexed_parameter &Parameter : Parameters)
		if(Parameter.Expr != "") ++ExprCount;
	
	column_vector ParVal2(NPars);
	for(int Par = 0; Par < NPars; ++Par) ParVal2(Par) = ParVal[Par];

	SetParameters(DataSet, &Parameters, ParVal2, ExprCount > 0, ParentWindow->ModelDll);
}


struct mcmc_run_data
{
	optimization_model *Model;
	mcmc_data          *Data;
	std::vector<void *> DataSets;
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
	
	GuiLock Lock;
	
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
	
	return RunData0->Model->EvaluateObjectives(RunData0->DataSets[Walker], Pars);
}

bool OptimizationWindow::RunMobiviewMCMC(mcmc_sampler_method Method, double *SamplerParams, size_t NWalkers, size_t NSteps, optimization_model *OptimModel,
	double *InitialValue, double *MinBound, double *MaxBound, int InitialType, int CallbackInterval, int RunType)
{
	size_t NPars = OptimModel->FreeParCount;
	
	int InitialStep = 0;
	{
		GuiLock Lock;
		
		MCMCResultWindow *ResultWin = &ParentWindow->MCMCResultWin;
		ResultWin->ClearPlots();
		
		if(RunType == 2) // NOTE RunType==2 means extend the previous run.
		{
			if(Data.NSteps == 0)
			{
				SetError("Can't extend a run before the model has been run once");
				return false;
			}
			if(Data.NSteps >= NSteps)
			{
				SetError(Format("To extend the run, you must set a higher number of timesteps than previously. Previous was %d.", (int)Data.NSteps));
				return false;
			}
			if(Data.NWalkers != NWalkers)
			{
				SetError("To extend the run, you must have the same amount of walkers as before.");
				return false;
			}
			if(ResultWin->Parameters != Parameters || ResultWin->Targets != Targets)
			{
				//TODO: Could alternatively let you continue with the old parameter set.
				SetError("You can't extend the run since the parameters or targets have changed.");
				return false;
			}
			InitialStep = Data.NSteps-1;
			
			Data.ExtendSteps(NSteps);
		}
		else
		{
			Data.Free();
			Data.Allocate(NWalkers, NPars, NSteps);
		}
		
		Array<String> FreeSyms;
		for(indexed_parameter &Parameter : Parameters)
			if(Parameter.Expr == "") FreeSyms.push_back(String(Parameter.Symbol.data()));
		
		
		//NOTE: These have to be cached so that we don't have to rely on the optimization window
		//not being edited (and going out of sync with the data)
		ResultWin->Parameters = Parameters;
		ResultWin->Targets    = Targets;
		
		ResultWin->ChoosePlotsTab.Set(0);
		ResultWin->BeginNewPlots(&Data, MinBound, MaxBound, FreeSyms, RunType);
		
		ParentWindow->ProcessEvents();
		
		if(RunType==1)
		{
			std::mt19937_64 Generator;
			for(int Walker = 0; Walker < NWalkers; ++Walker)
			{
				for(int Par = 0; Par < NPars; ++Par)
				{
					double Initial = InitialValue[Par];
					double Draw = Initial;
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
		}
	}
	
	ParentWindow->ProcessEvents();
	
	mcmc_run_data RunData;
	RunData.Model = OptimModel;
	RunData.Data  = &Data;
	RunData.MinBound = MinBound;
	RunData.MaxBound = MaxBound;
	RunData.DataSets.resize(NWalkers);
	
	//TODO: For Emcee we really only need a set of datasets that is the size of a
	//partial ensemble, which is about half of the total ensemble.
	for(int Walker = 0; Walker < NWalkers; ++Walker)
		RunData.DataSets[Walker] = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false);
	
	mcmc_callback_data CallbackData;
	CallbackData.ParentWindow = ParentWindow;
	
	//TODO: We have to check the SamplerParams for correctness somehow!
	
	bool Finished = RunMCMC(Method, SamplerParams, MCMCLogLikelyhoodEval, (void *)&RunData, &Data, MCMCCallbackFun, (void *)&CallbackData, CallbackInterval, InitialStep);
	
	for(int Walker = 0; Walker < NWalkers; ++Walker)
		ParentWindow->ModelDll.DeleteDataSet(RunData.DataSets[Walker]);
	
	return Finished;
}





bool OptimizationWindow::RunVarianceBasedSensitivity(int NSamples, optimization_model *Optim, double *MinBound, double *MaxBound)
{
	
	int ProgressInterval = 50; //TODO: Make the caller set this.
	
	VarianceSensitivityWindow &SensWin = ParentWindow->VarSensitivityWin;
	if(!SensWin.IsOpen()) SensWin.Open();
	
	SensWin.Plot.ClearAll(true);
	
	SensWin.ShowProgress.Show();
	SensWin.ResultData.Clear();
	
	for(indexed_parameter &Par : *Optim->Parameters)
	{
		if(Par.Expr == "") SensWin.ResultData.Add(Par.Symbol.data(), Null, Null);
	}
	ParentWindow->ProcessEvents();
	
	int NPars = Optim->FreeParCount;
	
	std::vector<double> matA(NSamples*NPars);
	std::vector<double> matB(NSamples*NPars);
	
	std::mt19937_64 Generator;
	
	for(int J = 0; J < NSamples; ++J)
		for(int I = 0; I < NPars; ++I)
		{
			std::uniform_real_distribution<double> Distr(MinBound[I], MaxBound[I]);
			matA[J*NPars + I] = Distr(Generator);
		}
	
	for(int J = 0; J < NSamples; ++J)
		for(int I = 0; I < NPars; ++I)
		{
			std::uniform_real_distribution<double> Distr(MinBound[I], MaxBound[I]);
			matB[J*NPars + I] = Distr(Generator);
		}
	
	std::vector<double> f0(NSamples*2);
	double *fA = f0.data();
	double *fB = fA + NSamples;
	
	int TotalEvals = NSamples*(NPars + 2);
	int Evals = 0;
	
	Array<AsyncWork<double>> Workers;
	auto NThreads = std::thread::hardware_concurrency();
	int NWorkers = std::max(32, (int)NThreads);
	Workers.InsertN(0, NWorkers);
	
	std::vector<void *> DataSets(NWorkers);
	for(int Worker = 0; Worker < NWorkers; ++Worker)
		DataSets[Worker] = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false);
	
	for(int SuperSample = 0; SuperSample < NSamples/NWorkers+1; ++SuperSample)
	{
		for(int Worker = 0; Worker < NWorkers; ++Worker)
		{
			int J = SuperSample*NWorkers + Worker;
			if(J >= NSamples) break;
			Workers[Worker].Do([=, & matA, & matB, & fA, & fB, &DataSets] () -> double {
				column_vector Pars(NPars);
				for(int I = 0; I < NPars; ++I)
					Pars(I) = matA[J*NPars + I];
				
				fA[J] = Optim->EvaluateObjectives(DataSets[Worker], Pars);
		
				
				for(int I = 0; I < NPars; ++I)
					Pars(I) = matB[J*NPars + I];
				
				fB[J] = Optim->EvaluateObjectives(DataSets[Worker], Pars);
				return 0.0; //NOTE: Just to be able to reuse the same workers. we have them returning double
			});
		}
		for(auto &Worker : Workers) Worker.Get();
		
		Evals += NWorkers*2;
		if(Evals > NSamples*2) Evals = NSamples*2;
		if(SuperSample % 8 == 0)
		{
			SensWin.ShowProgress.Set(Evals, TotalEvals);
			ParentWindow->ProcessEvents();
		}
	}
	
	int NBinsHistogram = SensWin.Plot.AddHistogram(Null, Null, f0.data(), 2*NSamples);
	Time Dummy;
	SensWin.Plot.FormatAxes(MajorMode_Histogram, NBinsHistogram, Dummy, ParentWindow->TimestepSize);
	SensWin.Plot.ShowLegend(false);
	
	//TODO: What should we use to compute overall variance?
	double mA = 0.0;
	double vA = 0.0;
	for(int J = 0; J < 2*NSamples; ++J) mA += f0[J];
	mA /= 2.0*(double)NSamples;
	for(int J = 0; J < 2*NSamples; ++J) vA += (f0[J]-mA)*(f0[J]-mA);
	vA /= 2.0*(double)NSamples;
	
	for(int I = 0; I < NPars; ++I)
	{
		double Main  = 0;
		double Total = 0;
		
		for(int SuperSample = 0; SuperSample < NSamples/NWorkers+1; ++SuperSample)
		{
			for(int Worker = 0; Worker < NWorkers; ++Worker)
			{
				int J = SuperSample*NWorkers + Worker;
				if(J >= NSamples) break;
				
				Workers[Worker].Do([=, & matA, & matB, & DataSets] () -> double {
					column_vector Pars(NPars);
					for(int II = 0; II < NPars; ++II)
					{
						if(II == I) Pars(II) = matB[J*NPars + II];
						else        Pars(II) = matA[J*NPars + II];
					}
					return Optim->EvaluateObjectives(DataSets[Worker], Pars);
				});
			}
			for(int Worker = 0; Worker < NWorkers; ++Worker)
			{
				int J = SuperSample*NWorkers + Worker;
				if(J >= NSamples) break;
				double fABij = Workers[Worker].Get();
				
				Main  += fB[J]*(fABij - fA[J]);
				Total += (fA[J] - fABij)*(fA[J] - fABij);
				
				Evals++;
			}
			if(SuperSample % 8 == 0)
			{
				SensWin.ShowProgress.Set(Evals, TotalEvals);
				ParentWindow->ProcessEvents();
			}
		}
		
		Main /= (vA * (double)NSamples);
		Total /= (vA * 2.0 * (double)NSamples);
		
		SensWin.ResultData.Set(I, "__main", FormatDouble(Main, ParentWindow->StatSettings.Precision));
		SensWin.ResultData.Set(I, "__total", FormatDouble(Total, ParentWindow->StatSettings.Precision));
		
		ParentWindow->ProcessEvents();
	}
	
	SensWin.ShowProgress.Hide();
	
	for(int Worker = 0; Worker < NWorkers; ++Worker)
		ParentWindow->ModelDll.DeleteDataSet(DataSets[Worker]);
	
	return true;
	
	#undef DO_PROGRESS
}

VarianceSensitivityWindow::VarianceSensitivityWindow()
{
	CtrlLayout(*this, "MobiView variance based sensitivity results");
	MinimizeBox().Sizeable().Zoomable();

	ShowProgress.Set(0, 1);
	ShowProgress.Hide();
	
	ResultData.AddColumn("__par", "Parameter");
	ResultData.AddColumn("__main", "First-order sensitivity coefficient");
	ResultData.AddColumn("__total", "Total effect index");
}




bool OptimizationWindow::ErrSymFixup()
{
	std::unordered_map<std::string, std::pair<int,int>> SymRow;
	
	//if((bool)ParSetup.OptionUseExpr.Get() || RunType==1 || RunType==2 || RunType==3)    //TODO: Non-ideal. Instead we should maybe force use exprs for MCMC RunTypes
	//{
	int ActiveIdx = 0;
	int Row = 0;
	for(indexed_parameter &Parameter : Parameters)
	{
		if(Parameter.Symbol != "")
		{
			if(SymRow.find(Parameter.Symbol) != SymRow.end())
			{
				SetError(Format("The parameter symbol %s appears twice", Parameter.Symbol.data()));
				return false;
			}
			SymRow[Parameter.Symbol] = {Row, ActiveIdx};
		}
		
		if(Parameter.Expr == "")
			++ActiveIdx;
		
		++Row;
	}
	//}
	
	//if(RunType == 1 || RunType == 2)
	//{
	for(optimization_target &Target : Targets)
	{
		Target.ErrParNum.clear();
			
		Vector<String> Symbols = Split(Target.ErrParSym.data(), ',', true);
		
		if(GetStatClass(Target) == StatClass_LogLikelihood)
		{
			for(String &Symbol : Symbols) { Symbol = TrimLeft(TrimRight(Symbol)); }
			
			#define SET_LL_SETTING(Handle, Name, NumErr) \
				else if(Target.ErrStruct == MCMCError_##Handle && Symbols.size() != NumErr) {\
					SetError(Format("The error structure %s requires %d error parameters. Only %d were provided. Provide them as a comma-separated list of parameter symbols", Name, NumErr, Symbols.size())); \
					return false; \
				}
			if(false) {}
			#include "LLSettings.h"
			#undef SET_LL_SETTING
			
			for(String &Symbol : Symbols)
			{
				std::string Sym = Symbol.ToStd();
				
				if(SymRow.find(Sym) == SymRow.end())
				{
					SetError(Format("The error parameter symbol %s is not the symbol of a parameter in the parameter list.", Sym.data()));
					return false;
				}
				
				int ParSymRow = SymRow[Sym].first;
				indexed_parameter &Par = Parameters[SymRow[Sym].first];
				if(!Par.Virtual)
				{
					SetError(Format("Only virtual parameters can be error parameters. The parameter with symbol %s is not virtual", Sym.data()));
					return false;
				}
				if(Par.Expr != "")
				{
					SetError(Format("Error parameters can not be results of expressions. The parameter with symbol %s was set as an error parameter, but also has an expression", Sym.data()));
					return false;
				}
				
				Target.ErrParNum.push_back(SymRow[Sym].second);
			}
		}
		else if(GetStatClass(Target) == StatClass_Residual && !Symbols.empty())
		{
			SetError("An error symbol was provided for a stat does not use one");
			return false;
		}
	}
	//}
	
	return true;
}


void OptimizationWindow::RunClicked(int RunType)
{
	//RunType:
	//	0 = Optimizer
	//  1 = New MCMC
	//  2 = Extend MCMC
	//  3 = Variance based sensitivity
	
	SetError("");
	
	if(Parameters.empty())
	{
		SetError("At least one parameter must be added before running");
		ProcessEvents();
		return;
	}
	
	if(Targets.empty())
	{
		SetError("There must be at least one optimization target.");
		return;
	}
	
	int Cl = 1;
	if(ParentWindow->ParametersWereChangedSinceLastSave)
		Cl = PromptYesNo("The main parameter set has been edited since the last save. If run the optimizer now it will overwrite these changes. Do you still want to run the optimizer?");
	if(!Cl)
		return;
	
	bool Success = ErrSymFixup();
	if(!Success) return;
	
	//TODO: This does not work unless the model has been run!! Should use GetNextTimesteps and
	//GetNextStartDate!!
	char TimeStr[256];
	uint64 ResultTimesteps = ParentWindow->ModelDll.GetTimesteps(ParentWindow->DataSet);
	ParentWindow->ModelDll.GetStartDate(ParentWindow->DataSet, TimeStr);
	Time ResultStartTime;
	StrToTime(ResultStartTime, TimeStr);
	Time ResultEndTime = ResultStartTime;
	AdvanceTimesteps(ResultEndTime, ResultTimesteps-1, ParentWindow->TimestepSize);
	
	//NOTE: Since we haven't wired all of the background data to the edit fields, we have to
	//gather some of it here.
	bool PositiveGood;
	int Row = 0;
	for(optimization_target &Target : Targets)
	{
		target_stat_class StatClass = GetStatClass(Target);
		
		if(
			(StatClass == StatClass_Stat && (RunType==0||RunType==1||RunType==2))
		  ||(StatClass == StatClass_LogLikelihood && (RunType==3))
		  ||(StatClass == StatClass_Residual && (RunType==1||RunType==2))
		  )
		{
			SetError("The selected stat for one of the targets is not valid for this type of run.");
			return;
		}
		
		if(RunType == 0)
		{
			bool PositiveGoodThis;
			if(StatClass == StatClass_Residual)
			{
				if(false){}
				#define SET_SETTING(Handle, Name, Type)
				#define SET_RES_SETTING(Handle, Name, Type) \
					else if(Target.ResidualStat == ResidualType_##Handle) PositiveGoodThis = (Type==1);
				#include "SetStatSettings.h"
				#undef SET_SETTING
				#undef SET_RES_SETTING
			}
			else if(StatClass == StatClass_LogLikelihood)
				PositiveGoodThis = true;
			else
				assert(!"Unsupported stat type for optimization");
			
			//NOTE: We could allow people to set negative weights in order to mix different types
			//of target, but I don't see a good use case for it currently.
			
			if(Row != 0 && (PositiveGoodThis != PositiveGood))
			{
				SetError("All optimization targets have to be either minimized or maximized, no mixing is allowed.");
				return;
			}
			PositiveGood = PositiveGoodThis;
		}
		++Row;
		
		if(Target.Weight < 0.0) //NOTE: The interface should already have prevented this, but let's be safe.
		{
			SetError("Negative weights are not allowed.");
			return;
		}
		
		if(StatClass != StatClass_Stat && Target.InputName == "")
		{
			SetError(Format("Targets that compute an error need an input observed series. No comparison series were provided for result \"%s\"", Target.ResultName.data()));
			return;
		}
		
		Time Begin, End;
		StrToTime(Begin, Target.Begin.data());
		StrToTime(End, Target.End.data());
		if(    !Begin.IsValid() || IsNull(Begin) || Begin < ResultStartTime || Begin > ResultEndTime || Begin > End
			|| !End.IsValid()   || IsNull(End)   || End   < ResultStartTime || End   > ResultEndTime)
		{
			SetError("One of the targets has a begin-end interval that is not valid or does not lie within the model run interval");
			return;
		}
	}
	
	Label *ProgressLabel = nullptr;
	if(RunSetup.OptionShowProgress.Get())
		ProgressLabel = &RunSetup.ProgressLabel;
	
	void *DataSetBase = nullptr;
	if(RunType==0) DataSetBase = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false);
	optimization_model OptimizationModel(ParentWindow, &Parameters, &Targets, ProgressLabel, DataSetBase);
	
	// Initial evaluation on the parameters given in the main dataset.
	column_vector InitialPars(OptimizationModel.FreeParCount);
	column_vector MinBound(OptimizationModel.FreeParCount);
	column_vector MaxBound(OptimizationModel.FreeParCount);
	
	int ActiveIdx = 0;
	for(int Row = 0; Row < Parameters.size(); ++Row)
	{
		if(Parameters[Row].Expr == "")
		{
			MinBound(ActiveIdx) = (double)ParSetup.ParameterView.Get(Row, Id("__min"));
			MaxBound(ActiveIdx) = (double)ParSetup.ParameterView.Get(Row, Id("__max"));
			
			if(MinBound(ActiveIdx) >= MaxBound(ActiveIdx))
			{
				SetError(Format("The min value is larger than or equal to the max value for the parameter %s", Parameters[Row].Name.data()));
				return;
			}
			
			++ActiveIdx;
		}
	}

	int ExprCount = 0;
	ActiveIdx = 0;
	for(indexed_parameter Par : Parameters)
	{
		if(Par.Expr == "")
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
				SetError(Format("The parameter \"%s\" %s has an initial value that is smaller than the assigned min bound", Par.Name.data(), MakeParameterIndexString(Par)));
				return;
			}
			if(InitialPars(ActiveIdx) > MaxBound(ActiveIdx))
			{
				SetError(Format("The parameter \"%s\" %s has an initial value that is larger than the assigned max bound", Par.Name.data(), MakeParameterIndexString(Par)));
				return;
			}
			
			if((RunType==1 || RunType==2 || RunType==3) && Par.Symbol == "")
			{
				SetError("You should provide a symbol for all the parameters that don't have an expression.");
				return;
			}
			
			++ActiveIdx;
		}
		else
			++ExprCount;

	}
	if(ParentWindow->CheckDllUserError()) return;
	
	//NOTE: This sets the parameters twice, but that should not be a problem
	int ErrorAtRow = SetParameters(ParentWindow->DataSet, &Parameters, InitialPars, ExprCount > 0, ParentWindow->ModelDll);
	if(ErrorAtRow != -1)
	{
		SetError(Format("Unable to parse the expression \"%s\" at row %d", Parameters[ErrorAtRow].Expr.data(), ErrorAtRow));
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
		void *DataSet = ParentWindow->ModelDll.CopyDataSet(ParentWindow->DataSet, false);
		InitialScore = OptimizationModel.EvaluateObjectives(DataSet, InitialPars);
		ParentWindow->ModelDll.DeleteDataSet(DataSet);
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
		
	bool PushExtendEnabled = MCMCSetup.PushExtendRun.IsEnabled();
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
			
		ParentWindow->Log(Format("Running optimization. Expected duration around %g seconds.", ExpectedDuration));
	
		SetError("Running optimization. This may take a few minutes or more.");
		
		ParentWindow->ProcessEvents();
		
		RunSetup.PushRun.Disable();
		MCMCSetup.PushRun.Disable();
		MCMCSetup.PushExtendRun.Disable();
		SensitivitySetup.PushRun.Disable();
		
		auto BeginTime = std::chrono::high_resolution_clock::now();
		
		//TODO: We need to disable some interactions with the main dataset when this is running
		//(such as running the model or editing parameters!)
		
		//TODO: Something goes wrong when we capture the OptimizationModel by reference. It somehow loses the cached
		//input data inside the run... This should not happen, as the input data should be
		//cached in the first initialization run above, and not changed after that!
		
		//TODO: Threading here sometimes crashes on some machines, but not all!
		Thread().Run([=, & InitialEvals](){
			
			dlib::function_evaluation Result = dlib::find_max_global(OptimizationModel, MinBound, MaxBound, dlib::max_function_calls(MaxFunctionCalls), dlib::FOREVER, 0, InitialEvals);
			auto EndTime = std::chrono::high_resolution_clock::now();
			double Duration = std::chrono::duration_cast<std::chrono::seconds>(EndTime - BeginTime).count();
			
			double NewScore = Result.y;
			if(!PositiveGood) NewScore = -NewScore;
			
			
			GuiLock Lock;
			if((PositiveGood && (NewScore <= InitialScore)) || (!PositiveGood && (NewScore >= InitialScore)) || !std::isfinite(NewScore))
			{
				//ParentWindow->Log(Format("Initial %g new %g", InitialScore, NewScore));
				ParentWindow->Log("The optimizer was unable to find a better result using the given number of function evaluations");
			}
			else
			{
				SetParameters(ParentWindow->DataSet, &Parameters, Result.x, ExprCount > 0, ParentWindow->ModelDll);
				ParentWindow->Log(Format("Optimization finished after %g seconds, with new best aggregate score: %g (old: %g). Remember to save these parameters to a different file if you don't want to overwrite your old parameter set.",
					Duration, NewScore, InitialScore));
				ParentWindow->RunModel();  // We call the RunModel function of the ParentWindow instead of doing it directly on the dll so that plots etc. are updated.
			}
			
			RunSetup.PushRun.Enable();
			MCMCSetup.PushRun.Enable();
			if(PushExtendEnabled) MCMCSetup.PushExtendRun.Enable();
			SensitivitySetup.PushRun.Enable();
			
			SetError("");
			RunSetup.ProgressLabel.SetText("");
		});
	}
	else if(RunType == 1 || RunType == 2)
	{
		int NWalkers    = MCMCSetup.EditWalkers.GetData();
		int NSteps      = MCMCSetup.EditSteps.GetData();
		int InitialType = MCMCSetup.InitialTypeSwitch.GetData();
		
		int NPars = OptimizationModel.FreeParCount;
		
		mcmc_sampler_method Method = (mcmc_sampler_method)(int)MCMCSetup.SelectSampler.GetData();
		double SamplerParams[20]; //NOTE: We should not encounter a sampler with more parameters than this...
		for(int Par = 0; Par < MCMCSetup.SamplerParamView.GetCount(); ++Par)
			SamplerParams[Par] = MCMCSetup.SamplerParamView.Get(Par, 1);
		
		
		std::vector<double> InitialVals(NPars);
		std::vector<double> MinVals(NPars);
		std::vector<double> MaxVals(NPars);
		
		for(int Idx = 0; Idx < NPars; ++Idx)
		{
			InitialVals[Idx] = InitialPars(Idx);
			MinVals[Idx]     = MinBound(Idx);
			MaxVals[Idx]     = MaxBound(Idx);
		}
		
		auto NCores = std::thread::hardware_concurrency();
		int NActualSteps = (RunType==1) ? (NSteps) : (NSteps - Data.NSteps);
		String Prefix = (RunType==1) ? "Running MCMC." : "Extending MCMC run.";
		
		double ExpectedDuration1 = Ms*1e-3*(double)NWalkers*(double)NActualSteps/(double)NCores;
		double ExpectedDuration2 = 2.0*ExpectedDuration1;   //TODO: This is just because we are not able to get the number of physical cores..
		
		ParentWindow->Log(Format("%s Expected duration around %.1f to %.1f seconds. Number of logical cores: %d.", Prefix, ExpectedDuration1, ExpectedDuration2, (int)NCores));
		ParentWindow->ProcessEvents();
		
		
		int CallbackInterval = 50; //TODO: Make this configurable?
		
		RunSetup.PushRun.Disable();
		MCMCSetup.PushRun.Disable();
		MCMCSetup.PushExtendRun.Disable();
		SensitivitySetup.PushRun.Disable();
		
		if(!ParentWindow->MCMCResultWin.IsOpen())
			ParentWindow->MCMCResultWin.Open();
		
		//NOTE: If we launch this from a separate thread, it can't launch its own threads, and
		//the GUI doesn't update properly :(
		
		//Thread().Run([=, & OptimizationModel, & InitialVals, & MinVals, & MaxVals](){
			
			auto BeginTime = std::chrono::system_clock::now();
			
			bool Finished = RunMobiviewMCMC(Method, &SamplerParams[0], NWalkers, NSteps, &OptimizationModel, InitialVals.data(), MinVals.data(), MaxVals.data(),
											InitialType, CallbackInterval, RunType);
			
			auto EndTime = std::chrono::system_clock::now();
			double Duration = std::chrono::duration_cast<std::chrono::seconds>(EndTime - BeginTime).count();
			
			GuiLock Lock;
			
			if(Finished)
				ParentWindow->Log(Format("MCMC run finished after %g seconds.", Duration));
			else
				ParentWindow->Log("MCMC run unsuccessful.");
			
			RunSetup.PushRun.Enable();
			MCMCSetup.PushRun.Enable();
			MCMCSetup.PushExtendRun.Enable();
			SensitivitySetup.PushRun.Enable();
			
			RunSetup.ProgressLabel.SetText("");
			SetError("");
		//});
	}
	else if(RunType == 3)
	{
		int NSamples = SensitivitySetup.EditSampleSize.GetData();
		
		RunSetup.PushRun.Disable();
		MCMCSetup.PushRun.Disable();
		MCMCSetup.PushExtendRun.Disable();
		SensitivitySetup.PushRun.Disable();
		
		int NPars = OptimizationModel.FreeParCount;
		std::vector<double> MinVals(NPars);
		std::vector<double> MaxVals(NPars);
		
		for(int Idx = 0; Idx < NPars; ++Idx)
		{
			MinVals[Idx]     = MinBound(Idx);
			MaxVals[Idx]     = MaxBound(Idx);
		}
		
		auto NCores = std::thread::hardware_concurrency();
		double ExpectedDuration1 = Ms*1e-3*(double)NSamples*(2.0 + (double)NPars) / (double)NCores;
		double ExpectedDuration2 = 2.0*ExpectedDuration1;   //TODO: This is just because we are not able to get the number of physical cores..
		
		ParentWindow->Log(Format("Running variance based sensitiviy sampling. Expected duration around %.1f to %.1f seconds. Number of logical cores: %d.", ExpectedDuration1, ExpectedDuration2, (int)NCores));
		
		auto BeginTime = std::chrono::system_clock::now();
		
		RunVarianceBasedSensitivity(NSamples, &OptimizationModel, MinVals.data(), MaxVals.data());
		
		auto EndTime = std::chrono::system_clock::now();
		double Duration = std::chrono::duration_cast<std::chrono::seconds>(EndTime - BeginTime).count();
		ParentWindow->Log(Format("Variance based sensitivity sampling finished after %g seconds.", Duration));
		
		
		RunSetup.PushRun.Enable();
		MCMCSetup.PushRun.Enable();
		MCMCSetup.PushExtendRun.Enable();
		SensitivitySetup.PushRun.Enable();
	}
}

void OptimizationWindow::TabChange()
{
	int TabNum = TargetSetup.OptimizerTypeTab.Get();
	
	if (TabNum == 2)             // variance based    -- hide err params
		TargetSetup.TargetView.HeaderObject().HideTab(5);
	else                         // MCMC or optimizer -- show err params
		TargetSetup.TargetView.HeaderObject().ShowTab(5);
	
	//TODO: Alternatively we could just switch out the column...
	
	for(int TargetIdx = 0; TargetIdx < Targets.size(); ++TargetIdx)
	{
		DropList &SelectStat = TargetStatCtrls[TargetIdx];
		SelectStat.Clear();

		if(TabNum == 2)
		{
			#define SET_SETTING(Handle, Name, Type) SelectStat.Add((int)StatType_##Handle, Name);
			#define SET_RES_SETTING(Handle, Name, Type)
			#include "SetStatSettings.h"
			#undef SET_SETTING
			#undef SET_RES_SETTING
		}
		if(TabNum == 0 || TabNum == 2)
		{
			#define SET_SETTING(Handle, Name, Type)
			#define SET_RES_SETTING(Handle, Name, Type) if(Type != -1) SelectStat.Add((int)ResidualType_##Handle, Name);
			#include "SetStatSettings.h"
			#undef SET_SETTING
			#undef SET_RES_SETTING
		}
		if(TabNum == 0 || TabNum == 1)
		{
			#define SET_LL_SETTING(Handle, Name, NumErr) SelectStat.Add((int)MCMCError_##Handle, Name);
			#include "LLSettings.h"
			#undef SET_LL_SETTING
		}
		
		
		optimization_target &Target = Targets[TargetIdx];
		SelectStat.SetValue((int)Target.Stat);
		TargetSetup.TargetView.Set(TargetIdx, "__targetstat", (int)Target.Stat);
		//PromptOK(Format("Target stat is %d", (int)Target.Stat));
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
			
			Value SymVal  = ParamJson["Sym"];
			if(!IsNull(SymVal)) Parameter.Symbol = SymVal.ToStd();
			Value ExprVal = ParamJson["Expr"];
			if(!IsNull(ExprVal)) Parameter.Expr = ExprVal.ToStd();
			
			bool Added = AddSingleParameter(Parameter, 0, false);
			
			if(Added)
			{
				Value UnitVal = ParamJson["Unit"];
				ParSetup.ParameterView.Set(ValidRow, Id("__unit"), UnitVal);
				Value MinVal  = ParamJson["Min"];
				ParSetup.ParameterView.Set(ValidRow, Id("__min"), MinVal);
				Value MaxVal  = ParamJson["Max"];
				ParSetup.ParameterView.Set(ValidRow, Id("__max"), MaxVal);
				
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
	
	Value Method = SetupJson["Sampler"];
	if(!IsNull(Method))
	{
		int Key = MCMCSetup.SelectSampler.FindValue(Method);
		MCMCSetup.SelectSampler.SetData(Key);
	}
	SamplerMethodSelected();
	ValueArray SamplerPars = SetupJson["SamplerPars"];
	if(!IsNull(SamplerPars) && SamplerPars.GetCount() == MCMCSetup.SamplerParamView.GetCount())
	{
		for(int Par = 0; Par < SamplerPars.GetCount(); ++Par)
			MCMCSetup.SamplerParamView.Set(Par, 1, (double)SamplerPars[Par]);
	}
	
	
	Value NSamples = SetupJson["Samples"];
	if(!IsNull(NSamples))
		SensitivitySetup.EditSampleSize.SetData(NSamples);
	
	Value RunType  = SetupJson["RunType"];
	if(!IsNull(RunType))
		TargetSetup.OptimizerTypeTab.Set(RunType);
	
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
			
			Target.Stat         = StatType_Mean;
			Target.ResidualStat = ResidualType_MAE;
			Target.ErrStruct    = MCMCError_NormalHet1;
			
			String StatName = TargetJson["Stat"];
			if(!IsNull(StatName))
			{
				if(false){}
				#define SET_SETTING(Handle, Name, Type)
				#define SET_RES_SETTING(Handle, Name, Type) else if(Name == StatName) Target.ResidualStat = ResidualType_##Handle;
				#include "SetStatSettings.h"
				#undef SET_SETTING
				#undef SET_RES_SETTING

				if(false){}
				#define SET_SETTING(Handle, Name, Type) else if(Name == StatName) Target.Stat = StatType_##Handle;
				#define SET_RES_SETTING(Handle, Name, Type)
				#include "SetStatSettings.h"
				#undef SET_SETTING
				#undef SET_RES_SETTING

				#define SET_LL_SETTING(Handle, Name, NumErr) else if(Name == StatName) Target.ErrStruct = MCMCError_##Handle;
				if(false){}
				#include "LLSettings.h"
				#undef SET_LL_SETTING
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
			
			Value BeginVal = TargetJson["Begin"];
			if(!IsNull(BeginVal)) Target.Begin = BeginVal.ToString().ToStd();
			
			Value EndVal   = TargetJson["End"];
			if(!IsNull(EndVal)) Target.End = EndVal.ToString().ToStd();
			
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
		ParJson("Sym", Par.Symbol.data());
		ParJson("Expr", Par.Expr.data());
		
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
	
	String Method = MCMCSetup.SelectSampler.GetValue();
	MainFile("Sampler", Method);
	JsonArray SamplerPars;
	for(int Row = 0; Row < MCMCSetup.SamplerParamView.GetCount(); ++Row)
		SamplerPars << MCMCSetup.SamplerParamView.Get(Row, 1);
	MainFile("SamplerPars", SamplerPars);
	
	int RunType  = TargetSetup.OptimizerTypeTab.Get();
	MainFile("RunType", RunType);
	
	int NSamples = SensitivitySetup.EditSampleSize.GetData();
	MainFile("Samples", NSamples);
	
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
		
		String StatName = Null;
		#define SET_SETTING(Handle, Name, Type)
		#define SET_RES_SETTING(Handle, Name, Type) else if(Target.ResidualStat == ResidualType_##Handle) StatName = Name;
		if(false){}
		#include "SetStatSettings.h"
		#undef SET_SETTING
		#undef SET_RES_SETTING
		
		#define SET_SETTING(Handle, Name, Type) else if(Target.Stat == StatType_##Handle) StatName = Name;
		#define SET_RES_SETTING(Handle, Name, Type)
		if(false){}
		#include "SetStatSettings.h"
		#undef SET_SETTING
		#undef SET_RES_SETTING
		
		#define SET_LL_SETTING(Handle, Name, NumErr) else if(Target.ErrStruct == MCMCError_##Handle) StatName = Name;
		if(false){}
		#include "LLSettings.h"
		#undef SET_LL_SETTING
		
		//TODO: Fix this one when we make multiple error params possible
		String ErrName  = TargetSetup.TargetView.Get(Row, Id("__errparam"));
		
		TargetJson("Stat", StatName);
		TargetJson("Weight", Target.Weight);
		TargetJson("ErrPar", ErrName);
		TargetJson("Begin", Target.Begin.data());
		TargetJson("End", Target.End.data());
		
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

	SetError("Running optimization. This may take a few minutes or more.");
	
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