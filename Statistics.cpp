#ifndef _MobiView_Statistics_h_
#define _MobiView_Statistics_h_


#include "MobiView.h"
#include <numeric>

void MobiView::DisplayTimeseriesStats(timeseries_stats &Stats, String &Name, String &Unit)
{
	String Display = Name + " [" + Unit + "]:\n";
	
	Display << "min: "         << FormatDouble(Stats.Min, 2) << "\n";
	Display << "max: "         << FormatDouble(Stats.Max, 2) << "\n";
	Display << "sum: "         << FormatDouble(Stats.Sum, 2) << "\n";
	Display << "mean: "        << FormatDouble(Stats.Mean, 2) << "\n";
	Display << "median: "      << FormatDouble(Stats.Median, 2) << "\n";
	Display << "variance: "    << FormatDouble(Stats.Variance, 2) << "\n";
	Display << "std.dev.: "    << FormatDouble(Stats.StandardDeviation, 2) << "\n";
	Display << "data points: " << Stats.DataPoints << "\n";
	Display << "\n";
	
	PlotInfo.Append(Display);
	PlotInfo.SetCursor(INT64_MAX);
}

void MobiView::DisplayResidualStats(residual_stats &Stats, String &Name)
{
	String Display = Name;
	
	Display << "\n";
	Display << "Mean error (bias): "  << FormatDouble(Stats.MeanError, 5) << "\n";
	Display << "MAE: "                << FormatDouble(Stats.MeanAbsoluteError, 5) << "\n";
	Display << "RMSE: "               << FormatDouble(Stats.RootMeanSquareError, 5) << "\n";
	Display << "N-S: "                << FormatDouble(Stats.NashSutcliffe, 5) << "\n";
	Display << "log N-S: "            << FormatDouble(Stats.LogNashSutcliffe, 5) << "\n";
	Display << "r2: "                 << FormatDouble(Stats.R2, 5) << "\n";
	Display << "Idx. of agr.: "       << FormatDouble(Stats.IndexOfAgreement, 5) << "\n";
	Display << "Spearman's RCC: "     << FormatDouble(Stats.SpearmansRCC, 5) << "\n";
	Display << "common data points: " << Stats.DataPoints << "\n";
	Display << "\n";
	
	PlotInfo.Append(Display);
	PlotInfo.SetCursor(INT64_MAX);
}


void MobiView::ComputeTimeseriesStats(timeseries_stats &StatsOut, double *Data, size_t Len)
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
	
	for(size_t PercentileIdx = 0; PercentileIdx < NUM_PERCENTILES; ++PercentileIdx)
	{
		size_t Idx = (size_t)std::ceil(PERCENTILES[PercentileIdx] * (double)FiniteCount); // Should not lose precision since we don't usually have millions of timesteps
		StatsOut.Percentiles[PercentileIdx] = SortedData[Idx];
	}
	
	
	Variance /= (double)FiniteCount;
	
	StatsOut.Min = SortedData[0];
	StatsOut.Max = SortedData[SortedData.size()-1];
	StatsOut.Sum = Sum;
	StatsOut.Mean = Mean;
	size_t Middle = FiniteCount / 2;
	if(FiniteCount % 2 == 0)
	{
		StatsOut.Median = 0.5 * (SortedData[Middle] + SortedData[Middle-1]);
	}
	else
	{
		StatsOut.Median = SortedData[Middle];
	}
	StatsOut.Variance = Variance;
	StatsOut.StandardDeviation = std::sqrt(Variance);
	StatsOut.DataPoints = FiniteCount;
}


void MobiView::ComputeResidualStats(residual_stats &StatsOut, double *Obs, double *Mod, size_t Len)
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
		double Val = Obs[Idx] - Mod[Idx];
		if(std::isfinite(Val))
		{
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
	//ONLY count in values where both Obs and Mod are not NaN!!
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
		if(std::isfinite(Obs[Idx]) && std::isfinite(Mod[Idx]))
		{
			SSObs += (Obs[Idx] - MeanObs)*(Obs[Idx] - MeanObs);
			SSMod += (Mod[Idx] - MeanMod)*(Mod[Idx] - MeanMod);
			Cov += (Obs[Idx] - MeanObs)*(Mod[Idx] - MeanMod);
			
			SSLogObs += (std::log(Obs[Idx]) - MeanLogObs)*(std::log(Obs[Idx]) - MeanLogObs);
			
			double Agreement = std::abs(Mod[Idx] - MeanObs) + std::abs(Obs[Idx] - MeanObs);
			AgreementDenom += Agreement*Agreement;
		}
	}
	
	double RR = Cov / (std::sqrt(SSObs) * std::sqrt(SSMod));
	
	
	double MeanError = Sum / (double)FiniteCount;
	
	double MeanSquareError = SumSquare / (double)FiniteCount;
	
	double NS = 1.0 - SumSquare / SSObs;  //TODO: do something here if SSObs == 0?
	
	double logNS = 1.0 - SumLogSquare / SSLogObs;
	
	double IA = 1.0 - SumSquare / AgreementDenom;
	
	StatsOut.MinError  = Min;
	StatsOut.MaxError  = Max;
	StatsOut.MeanError = MeanError;
	StatsOut.MeanAbsoluteError = SumAbs / (double)FiniteCount;
	StatsOut.RootMeanSquareError = std::sqrt(MeanSquareError);
	StatsOut.NashSutcliffe = NS;
	StatsOut.LogNashSutcliffe = logNS;
	StatsOut.R2 = RR*RR;
	StatsOut.IndexOfAgreement = IA;
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
	StatsOut.SpearmansRCC = 1.0 - 6.0 * SumSquareRankDiff / (FC * (FC*FC - 1.0));
}

void MobiView::ComputeTrendStats(double *XData, double *YData, size_t Len, double MeanY, double &XMeanOut, double &XVarOut, double &XYCovarOut)
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
