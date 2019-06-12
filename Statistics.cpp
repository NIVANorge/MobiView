#ifndef _MobiView_Statistics_h_
#define _MobiView_Statistics_h_


#include "MobiView.h"

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
	Display << "Mean error (bias): "  << FormatDouble(Stats.MeanError, 3) << "\n";
	Display << "MAE: "                << FormatDouble(Stats.MeanAbsoluteError, 3) << "\n";
	Display << "RMSE: "               << FormatDouble(Stats.RootMeanSquareError, 3) << "\n";
	Display << "N-S: "                << FormatDouble(Stats.NashSutcliffe, 3) << "\n";
	Display << "common data points: " << Stats.DataPoints << "\n";
	Display << "\n";
	
	PlotInfo.Append(Display);
	PlotInfo.SetCursor(INT64_MAX);
}


void MobiView::ComputeTimeseriesStats(timeseries_stats &StatsOut, double *Data, size_t Len, Date &StartDate)
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
	std::sort(SortedData.begin(), SortedData.end()); //TODO: There exist faster algorithms for this than sorting...
	
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


void MobiView::ComputeResidualStats(residual_stats &StatsOut, double *Residuals, double VarObs, size_t Len, Date &StartDate)
{
	double Sum = 0.0;
	double SumAbs = 0.0;
	double SumSquare = 0.0;
	size_t FiniteCount = 0;
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double Val = Residuals[Idx];
		if(std::isfinite(Val))
		{
			Sum += Val;
			SumAbs += std::abs(Val);
			SumSquare += Val*Val;
			
			++FiniteCount;
		}
	}
	
	double MeanError = Sum / (double)FiniteCount;
	
	double MeanSquareError = SumSquare / (double)FiniteCount;
	
	double NS = 1.0 - MeanSquareError / VarObs;
	
	StatsOut.MeanError = MeanError;
	StatsOut.MeanAbsoluteError = SumAbs / (double)FiniteCount;
	StatsOut.RootMeanSquareError = std::sqrt(MeanSquareError);
	StatsOut.NashSutcliffe = NS;
	StatsOut.DataPoints = FiniteCount;
}

void MobiView::ComputeTrendStats(double *YData, size_t Len, double MeanY, double &XMeanOut, double &XVarOut, double &XYCovarOut)
{
	double SumX = 0.0;
	size_t FiniteCount = 0;
	
	for(size_t Idx = 0; Idx < Len; ++Idx)
	{
		double YVal = YData[Idx];
		if(std::isfinite(YVal))
		{
			SumX += (double)Idx;
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
			double DevX = ((double)Idx - MeanX);
			CovarAcc += (Val - MeanY)*DevX;
			XVarAcc += DevX*DevX;
		}
	}
	
	XMeanOut = MeanX;
	XVarOut = XVarAcc / (double)FiniteCount;
	XYCovarOut = CovarAcc / (double)FiniteCount;
}


#endif
