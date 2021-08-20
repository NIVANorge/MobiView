#include "MobiView.h"

#include <plugin/md/Markdown.h>

#define IMAGECLASS LogoImg
#define IMAGEFILE <MobiView/logos.iml>
#include <Draw/iml.h>


ModelInfoViewWindow::ModelInfoViewWindow()
{
	InfoBox.IsLogWindow = false;
	
	CtrlLayout(*this, "Model information");
	
	Sizeable().Zoomable();
}

bool StartsWith(const char *String, const char *Substring)
{
	if(!String || !Substring) return false;
	int Sublen = strlen(Substring);
	if(strlen(String) < Sublen) return false;
	for(int Idx = 0; Idx < Sublen; ++Idx)
		if(String[Idx] != Substring[Idx]) return false;
	return true;
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
	if(StartsWith(ModelName, "INCA"))
		Buf << "@@iml:1512*768`LogoImg:INCALogo`&&";
	else if(StartsWith(ModelName, "Simply"))
		Buf << "@@iml:2236*982`LogoImg:SimplyLogo`&&";
	else if(StartsWith(ModelName, "MAGIC"))
		Buf << "@@iml:2476*1328`LogoImg:MAGICLogo`&&";
	
	Buf << "[3 The model [* " << ModelName << "] contains the following modules:\n";
	
	MarkdownConverter Mdc;
	
	for(size_t Module = 0; Module < ModuleNames.size(); ++Module)
	{
		Buf += "\n[* ";
		Buf += ModuleNames[Module];
		Buf += " (V";
		Buf += ModuleVersions[Module];
		Buf += ")]\n";
		
		const char *ModuleDesc = ParentWindow->ModelDll.GetModuleDescription(ParentWindow->DataSet, ModuleNames[Module]);
		if(ModuleDesc && strlen(ModuleDesc) != 0)
			Buf += Mdc.ToQtf(String(ModuleDesc));
		else
			Buf += "(no description provided by model creator)";
		Buf += "\n";
	}
	Buf += "]";
	Buf.Replace("\n", "&");
	
	InfoBox.SetQTF(Buf);
}