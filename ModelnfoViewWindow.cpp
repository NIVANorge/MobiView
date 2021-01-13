#include "MobiView.h"


#define IMAGECLASS LogoImg
#define IMAGEFILE <MobiView/logos.iml>
#include <Draw/iml.h>


ModelInfoViewWindow::ModelInfoViewWindow()
{
	InfoBox.IsLogWindow = false;
	
	CtrlLayout(*this, "Model information");
	
	Sizeable().Zoomable();
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
	
	String Buf;
	
	const char *ModelName = ParentWindow->ModelDll.GetModelName(ParentWindow->DataSet);
	if(ModelName && strlen(ModelName) >= 4 && ModelName[0]=='I' && ModelName[1]=='N' && ModelName[2]=='C' && ModelName[3]=='A')
		Buf << "@@iml:1512*768`LogoImg:INCALogo`&&";
	else if(ModelName && strlen(ModelName) >= 6 && ModelName[0]=='S' && ModelName[1]=='i' && ModelName[2]=='m' && ModelName[3]=='p' && ModelName[4]=='l' && ModelName[5]=='y')
		Buf << "@@iml:2236*982`LogoImg:SimplyLogo`&&";
	else if(ModelName && strlen(ModelName) >= 5 && ModelName[0]=='M' && ModelName[1]=='A' && ModelName[2]=='G' && ModelName[3]=='I' && ModelName[4]=='C')
		Buf << "@@iml:2476*1328`LogoImg:MAGICLogo`&&";
	
	Buf << "[3 The model [* " << ModelName << "] contains the following modules:\n";
	
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