#include "MobiView.h"



EditStatSettingsWindow::EditStatSettingsWindow()
{
	CtrlLayout(*this, "Edit statistics settings");
	
	OKButton.WhenAction = THISBACK(SaveDataAndClose);
	
	ResetDefaultButton.WhenAction << [this]()
	{
		this->PercentilesEdit.SetText("0, 5, 15, 25, 50, 75, 85, 95, 100");
	};
	
	PercentilesEdit.WhenEnter = THISBACK(SaveDataAndClose);
}


void EditStatSettingsWindow::LoadData()
{
	StatisticsSettings &Stat = ParentWindow->StatSettings;
	
	#define SET_SETTING(Name) \
		Display##Name.Set((int)Stat.Display##Name);
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	
	String QuantileString;
	int Idx = 0;
	for(double Quantile : Stat.Percentiles)
	{
		QuantileString << Format("%g", Quantile);
		if(Idx != Stat.Percentiles.size()-1) QuantileString << ", ";
		++Idx;
	}
	
	PercentilesEdit.SetText(QuantileString);
	
	PrecisionEdit.SetData(Stat.Precision);
}

void EditStatSettingsWindow::SaveDataAndClose()
{
	StatisticsSettings &Stat = ParentWindow->StatSettings;
	
	#define SET_SETTING(Name) \
		Stat.Display##Name = (bool)Display##Name.Get();
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	
	std::vector<double> Percentiles;
	String PercStr = PercentilesEdit.GetText().ToString();
	bool Success = ParseDoubleList(PercStr, Percentiles);
	
	Stat.Precision = PrecisionEdit.GetData();
	
	if(Success && Percentiles.size() >= 1)
	{
		std::sort(Percentiles.begin(), Percentiles.end());
		Stat.Percentiles = Percentiles;
		
		Close();
		ParentWindow->PlotRebuild(); // To reflect the new settings!
	}
	else
	{
		PromptOK("The percentiles has to be a comma-separated list of numeric values between 0 and 100, containing at least one value.");
	}
}

bool EditStatSettingsWindow::ParseDoubleList(String &ListStr, std::vector<double> &Result)
{
	bool Success = true;
	
	Vector<String> SplitList = Split(ListStr, ",");
	for(String &Str : SplitList)
	{
		double Val = StrDbl(Str);
		if(!IsNull(Val) && Val >= 0.0 && Val <= 100.0)
			Result.push_back(Val);
		else
			Success = false;
	}
	return Success;
}