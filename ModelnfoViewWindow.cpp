#include "MobiView.h"


ModelInfoViewWindow::ModelInfoViewWindow()
{
	CtrlLayout(*this, "Model information");
	
	Sizeable().Zoomable();
	
	//InfoBox.SetColor(TextCtrl::PAPER_READONLY, InfoBox.GetColor(TextCtrl::PAPER_NORMAL));
}

void ModelInfoViewWindow::RefreshText()
{
	InfoBox.Clear();
	
	if(!ParentWindow->ModelDll.IsLoaded() || !ParentWindow->DataSet)
	{
		InfoBox.SetQTF("[3 No model is loaded.]");
		return;
	}
	
	uint64 ModuleCount = ParentWindow->ModelDll.GetAllModulesCount(ParentWindow->DataSet);
	std::vector<char *> ModuleNames(ModuleCount);
	std::vector<char *> ModuleVersions(ModuleCount);
	ParentWindow->ModelDll.GetAllModules(ParentWindow->DataSet, ModuleNames.data(), ModuleVersions.data());
	if (ParentWindow->CheckDllUserError()) return;
	
	String Buf = "[3 The model ";
	Buf += ParentWindow->ModelDll.GetModelName(ParentWindow->DataSet);
	Buf += " contains the following modules:\n";
	
	for(size_t Module = 0; Module < ModuleNames.size(); ++Module)
	{
		Buf += "\n[* ";
		Buf += ModuleNames[Module];
		Buf += " (V";
		Buf += ModuleVersions[Module];
		Buf += ")]\n";
		
		const char *ModuleDesc = ParentWindow->ModelDll.GetModuleDescription(ParentWindow->DataSet, ModuleNames[Module]);
		if(ModuleDesc)
			Buf += ModuleDesc;
		else
			Buf += "(no description provided by model creator)";
		Buf += "\n";
	}
	Buf += "]";
	Buf.Replace("\n", "&");
	
	InfoBox.SetQTF(Buf);
}