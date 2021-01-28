#ifndef _MobiView_Statistics_h_
#define _MobiView_Statistics_h_


#include "MobiView.h"
#include "MyRichView.h"
#include <numeric>

void DisplayStat(String ValName, int PositiveGood, double ValOld, double ValNow, bool DisplayChange, String &Display, bool &TrackedFirst, int Precision)
{
	if(!TrackedFirst) Display << "::@W ";
	TrackedFirst = false;
	Display << ValName << "::@W " << FormatDouble(ValNow, Precision);
	if(DisplayChange && ValOld != ValNow)
	{
		if((PositiveGood==1 && ValNow > ValOld) || (PositiveGood==0 && ValNow < ValOld) || (PositiveGood==-1 && std::abs(ValNow) < std::abs(ValOld)))
			Display << "::@G ";
		else
			Display << "::@R ";
		if(ValNow > ValOld) Display << "+";
		Display << FormatDouble(ValNow - ValOld, Precision);
	}
	else
		Display << ":: ";
	Display << "\n";
}

void DisplayStat(String ValName, double Val, String &Display, bool &TrackedFirst, int Precision)
{
	if(!TrackedFirst) Display << "::@W ";
	TrackedFirst = false;
	Display << ValName << "::@W " << FormatDouble(Val, Precision);
}

void DisplayTimeseriesStats(timeseries_stats &Stats, String &Name, String &Unit, StatisticsSettings &StatSettings, MyRichView &PlotInfo, Color Col)
{
	int Precision = StatSettings.Precision;
	
	String Display = Name + " [" + Unit + "]:&";;
	Display.Replace("[", "`[");
	Display.Replace("]", "`]");
	Display.Replace("_", "`_");
	
	Display = Format("[*@(%d.%d.%d) %s]", Col.GetR(), Col.GetG(), Col.GetB(), Display);
	
	
	#define SET_SETTING(Handle, Name, Type) \
		if(StatSettings.Display##Handle)         DisplayStat(Name, Stats.Handle, Display, TrackedFirst, Precision);
	#define SET_RES_SETTING(Handle, Name, Type)
	
	bool TrackedFirst = true;
	Display << "{{1:1FWGW ";
	
	#include "SetStatSettings.h"
	
	#undef SET_SETTING
	#undef SET_RES_SETTING
	
	DisplayStat("data points", Stats.DataPoints, Display, TrackedFirst, Precision);
	Display << "}}&";
	
	PlotInfo.Append(Display);
	PlotInfo.ScrollEnd();
}

void DisplayResidualStats(residual_stats &Stats, residual_stats &CachedStats, String &Name, StatisticsSettings &StatSettings, MyRichView &PlotInfo)
{
	int Precision = StatSettings.Precision;
	
	String Display = Name;
	
	Display.Replace("[", "`[");
	Display.Replace("]", "`]");
	Display.Replace("_", "`_");
	
	Display = Format("[* %s]", Display);
	
	#define SET_RES_SETTING(Handle, Name, Type) \
		if(StatSettings.Display##Handle) DisplayStat(Name, Type, CachedStats.Handle, Stats.Handle, CachedStats.WasInitialized, Display, TrackedFirst, Precision);
	#define SET_SETTING(Handle, Name, Type)
	
	bool TrackedFirst = true;
	Display << "{{2:1:1FWGW ";
	
	#include "SetStatSettings.h"

	DisplayStat("data points", Stats.DataPoints, Display, TrackedFirst, Precision);
	Display << ":: }}&";
	
	#undef SET_RES_SETTING
	#undef SET_SETTING
	
	PlotInfo.Append(Display);
	PlotInfo.ScrollEnd();
}


void ComputeTimeseriesStats(timeseries_stats &StatsOut, double *Data, size_t Len, StatisticsSettings &StatSettings)
{
	double Sum = 0.0;
	size_t FiniteCount = 0;
	
	std::vector<double> SortedData(Len);
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double Val = Data[Idx];
		if(std::isfinite(Val) && !IsNull(Val))
		{
			SortedData[FiniteCount] = Val;
			Sum += Val;
			
			++FiniteCount;
		}
	}
	
	//TODO: Guard against FiniteCount==0
	
	SortedData.resize(FiniteCount);
	std::sort(SortedData.begin(), SortedData.end());
	
	double Mean = Sum / (double)FiniteCount;
	
	double Variance = 0.0;
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double Val = Data[Idx];
		if(std::isfinite(Val) && !IsNull(Val))
		{
			double Dev = Mean - Val;
			Variance += Dev*Dev;
		}
	}
	
	StatsOut.Percentiles.resize(StatSettings.Percentiles.size());
	for(size_t PercentileIdx = 0; PercentileIdx < StatSettings.Percentiles.size(); ++PercentileIdx)
	{
		if(FiniteCount > 0)
		{
			size_t Idx = (size_t)std::ceil(StatSettings.Percentiles[PercentileIdx] * 0.01 * (double)(FiniteCount-1)); // Should not lose precision since we don't usually have millions of timesteps
			StatsOut.Percentiles[PercentileIdx] = SortedData[Idx];
		}
		else
		{
			StatsOut.Percentiles[PercentileIdx] = Null;
		}
	}
	
	
	Variance /= (double)FiniteCount;
	
	if(FiniteCount > 0)
	{
		StatsOut.Min = SortedData[0];
		StatsOut.Max = SortedData[SortedData.size()-1];
		
		size_t Middle = FiniteCount / 2;
		if(FiniteCount % 2 == 0)
		{
			StatsOut.Median = 0.5 * (SortedData[Middle] + SortedData[Middle-1]);
		}
		else
		{
			StatsOut.Median = SortedData[Middle];
		}
	}
	else
	{
		StatsOut.Min = Null;
		StatsOut.Max = Null;
		StatsOut.Median = Null;
	}
	StatsOut.Sum = Sum;
	StatsOut.Mean = Mean;
	
	StatsOut.Variance = Variance;
	StatsOut.StandardDev = std::sqrt(Variance);
	StatsOut.DataPoints = FiniteCount;
}


void ComputeResidualStats(residual_stats &StatsOut, double *Obs, double *Mod, size_t Len)
{
	double Sum = 0.0;
	double SumAbs = 0.0;
	double SumSquare = 0.0;
	size_t FiniteCount = 0;
	
	double SumObs = 0.0;
	double SumMod = 0.0;
	
	double SumLogObs = 0.0;
	double SumLogSquare = 0.0;
	
	double Min = DBL_MAX;
	double Max = -DBL_MAX;
	
	std::vector<double> FiniteObs;
	std::vector<double> FiniteMod;
	FiniteObs.reserve(Len);
	FiniteMod.reserve(Len);
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		if(std::isfinite(Obs[Idx]) && !IsNull(Obs[Idx]) && std::isfinite(Mod[Idx]) && !IsNull(Mod[Idx]))
		{
			double Val = Obs[Idx] - Mod[Idx];
			
			Sum += Val;
			SumAbs += std::abs(Val);
			SumSquare += Val*Val;
			
			Min = std::min(Min, Val);
			Max = std::max(Max, Val);
			
			SumObs += Obs[Idx];
			SumMod += Mod[Idx];
			
			SumLogObs += std::log(Obs[Idx]);
			double LogRes = std::log(Obs[Idx]) - std::log(Mod[Idx]);
			SumLogSquare += LogRes*LogRes;
			
			++FiniteCount;
			
			FiniteObs.push_back(Obs[Idx]);
			FiniteMod.push_back(Mod[Idx]);
		}
	}
	
	//NOTE: We can NOT just reuse these from timeseries_stats computation, because here we can
	//only count values where BOTH Obs and Mod are not NaN!!
	double MeanObs = SumObs / (double)FiniteCount;
	double MeanMod = SumMod / (double)FiniteCount;
	
	double MeanLogObs = SumLogObs / (double)FiniteCount;
	
	double SSObs = 0.0;
	double SSMod = 0.0;
	double Cov = 0.0;
	
	double SSLogObs = 0.0;
	
	double AgreementDenom = 0.0;
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		if(std::isfinite(Obs[Idx]) && !IsNull(Obs[Idx]) && std::isfinite(Mod[Idx]) && !IsNull(Mod[Idx]))
		{
			double Val = Obs[Idx] - Mod[Idx];
			
			SSObs += (Obs[Idx] - MeanObs)*(Obs[Idx] - MeanObs);
			SSMod += (Mod[Idx] - MeanMod)*(Mod[Idx] - MeanMod);
			Cov += (Obs[Idx] - MeanObs)*(Mod[Idx] - MeanMod);
			SSLogObs += (std::log(Obs[Idx]) - MeanLogObs)*(std::log(Obs[Idx]) - MeanLogObs);
			
			double Agreement = std::abs(Mod[Idx] - MeanObs) + std::abs(Obs[Idx] - MeanObs);
			AgreementDenom += Agreement*Agreement;
		}
	}
	Cov /= (double)FiniteCount;
	
	double StdObs = std::sqrt(SSObs/(double)FiniteCount);
	double StdMod = std::sqrt(SSMod/(double)FiniteCount);
	double CvarObs = StdObs/MeanObs;
	double CvarMod = StdMod/MeanMod;
	
	double Beta = MeanMod / MeanObs;
	double Delta = CvarMod / CvarObs;
	double RR = Cov / (StdObs * StdMod);
	
	
	
	double MeanError = Sum / (double)FiniteCount;
	
	double MeanSquareError = SumSquare / (double)FiniteCount;
	
	double NS = 1.0 - SumSquare / SSObs;  //TODO: do something here if SSObs == 0?
	
	double logNS = 1.0 - SumLogSquare / SSLogObs;
	
	double IA = 1.0 - SumSquare / AgreementDenom;
	
	double KGE = 1.0 - std::sqrt((RR-1.0)*(RR-1.0) + (Beta-1.0)*(Beta-1.0) + (Delta-1.0)*(Delta-1.0));
	
	StatsOut.MinError  = Min;
	StatsOut.MaxError  = Max;
	
	StatsOut.MeanError = MeanError;
	StatsOut.MAE       = SumAbs / (double)FiniteCount;
	StatsOut.RMSE      = std::sqrt(MeanSquareError);
	StatsOut.NS        = NS;
	StatsOut.LogNS     = logNS;
	StatsOut.R2        = RR*RR;
	StatsOut.IdxAgr    = IA;
	StatsOut.KGE       = KGE;
	StatsOut.DataPoints = FiniteCount;
	
	
	
	//Determining ranks in order to compute Spearman's rank correlation coefficient
	std::vector<size_t> OrderObs(FiniteCount);
	std::vector<size_t> OrderMod(FiniteCount);
	std::iota(OrderObs.begin(), OrderObs.end(), 0);
	std::iota(OrderMod.begin(), OrderMod.end(), 0);
	
	std::sort(OrderObs.begin(), OrderObs.end(),
		[&FiniteObs](size_t I1, size_t I2) {return FiniteObs[I1] < FiniteObs[I2];});
	
	std::sort(OrderMod.begin(), OrderMod.end(),
		[&FiniteMod](size_t I1, size_t I2) {return FiniteMod[I1] < FiniteMod[I2];});
	
	std::vector<size_t> RankObs(FiniteCount);
	std::vector<size_t> RankMod(FiniteCount);
	
	
	for(size_t Idx = 0; Idx < FiniteCount; ++Idx)
	{
		RankObs[OrderObs[Idx]] = Idx + 1;
		RankMod[OrderMod[Idx]] = Idx + 1;
	}
	
	double SumSquareRankDiff = 0.0;
	for(size_t Idx = 0; Idx < FiniteCount; ++Idx)
	{
		double RankDiff = (double)RankObs[Idx] - (double)RankMod[Idx];
		SumSquareRankDiff += RankDiff*RankDiff;
	}
	
	double FC = (double)FiniteCount;
	StatsOut.SRCC = 1.0 - 6.0 * SumSquareRankDiff / (FC * (FC*FC - 1.0));
}

void ComputeTrendStats(double *XData, double *YData, size_t Len, double MeanY, double &XMeanOut, double &XVarOut, double &XYCovarOut)
{
	double SumX = 0.0;
	size_t FiniteCount = 0;
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double YVal = YData[Idx];
		if(std::isfinite(YVal))
		{
			SumX += XData[Idx];
			FiniteCount++;
		}
	}
	
	double MeanX = SumX / (double)FiniteCount;
	
	double CovarAcc = 0.0;
	double XVarAcc = 0.0;
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double Val = YData[Idx];
		if(std::isfinite(Val))
		{
			double DevX = (XData[Idx] - MeanX);
			CovarAcc += (Val - MeanY)*DevX;
			XVarAcc += DevX*DevX;
		}
	}
	
	XMeanOut = MeanX;
	XVarOut = XVarAcc / (double)FiniteCount;
	XYCovarOut = CovarAcc / (double)FiniteCount;
}


#endif
