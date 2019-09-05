#ifndef _MobiView_PlotDataStorage_h_
#define _MobiView_PlotDataStorage_h_

//NOTE: This has to match up to the major mode selecter.
enum plot_major_mode
{
	MajorMode_Regular = 0,
	MajorMode_Histogram,
	MajorMode_Profile,
	MajorMode_CompareBaseline,
	MajorMode_Residuals,
	MajorMode_ResidualHistogram,
	MajorMode_QQ,
};

const size_t NUM_PERCENTILES = 7;
const double PERCENTILES[NUM_PERCENTILES] = {0.05, 0.15, 0.25, 0.5, 0.75, 0.85, 0.95};

struct timeseries_stats
{
	double Min;
	double Max;
	double Sum;
	double Median;
	double Percentiles[NUM_PERCENTILES];
	double Mean;
	double Variance;
	double StandardDeviation;
	size_t DataPoints;
};

struct residual_stats
{
	double MinError;
	double MaxError;
	double MeanError;
	double MeanAbsoluteError;
	double RootMeanSquareError;
	double NashSutcliffe;
	double R2;
	double SpearmansRCC;
	size_t DataPoints;
};

struct plot_data_storage
{
	plot_data_storage()
	{
		Data.reserve(1024); //NOTE: We should never allow the outer vector to resize, because that could invalidate references to inner vectors.
	}
	
	std::vector<std::vector<double>> Data;
	
	std::vector<double> &Allocate(size_t Size)
	{
		Data.emplace_back(Size);
		return Data.back();
	}
	
	void Clear()
	{
		Data.clear();
	}
};

struct plot_colors
{
	plot_colors()
	{
		PlotColors = {{0, 130, 200}, {230, 25, 75}, {60, 180, 75}, {245, 130, 48}, {145, 30, 180},
                  {70, 240, 240}, {240, 50, 230}, {210, 245, 60}, {250, 190, 190}, {0, 128, 128}, {230, 190, 255},
                  {170, 110, 40}, {128, 0, 0}, {170, 255, 195}, {128, 128, 0}, {255, 215, 180}, {0, 0, 128}, {255, 225, 25}};
        NextAvailable = 0;
	}
	
	int NextAvailable;
	std::vector<Color> PlotColors;
	
	Color Next()
	{
		Color Result = PlotColors[NextAvailable];
		NextAvailable++;
		if(NextAvailable == PlotColors.size()) NextAvailable = 0;
		return Result;
	}
	
	void Reset()
	{
		NextAvailable = 0;
	}
};


class MobiView;

class PlotCtrl : public WithPlotCtrlLayout<ParentCtrl> {
public:
	typedef PlotCtrl CLASSNAME;
	
	PlotCtrl(MobiView* Parent);
	
	void PlotModeChange();
	
	void AggregateData(Date &ReferenceDate, Date &StartDate, uint64 Timesteps, double *Data, int IntervalType, int AggregationType, std::vector<double> &XValues, std::vector<double> &YValues);
	void AddPlot(String &Legend, String &Unit, double *Data, size_t Len, bool Scatter, bool LogY, bool NormalY, Date &ReferenceDate, Date &StartDate, double MinY = 0.0, double MaxY = 0.0);
	int  AddHistogram(String &Legend, String &Unit, double *Data, size_t Len);
	void AddQQPlot(String &ModUnit, String &ObsUnit, String &ModName, String &ObsName, timeseries_stats &ModeledStats, timeseries_stats &ObservedStats);
	void AddLine(const String &Legend, double X0, double X1, double Y0, double Y1, Color GraphColor = Null);
	void AddTrendLine(String &Legend, size_t Timesteps, double XYCovar, double XVar, double YMean, double XMean, Date &ReferenceDate, Date &StartDate);
	void AddNormalApproximation(String &Legend, int SampleCount, double Min, double Max, double Mean, double StdDev);
	void AddPlotRecursive(std::string &Name, int Mode, std::vector<char *> &IndexSets, std::vector<std::string> &CurrentIndexes, int Level, uint64 Timesteps, Date &ReferenceDate, Date &StartDate);
	
	void SetBetterGridLinePositions(int Dim);
	
	void RePlot();
	
	void TimestepSliderEvent();
	void TimestepEditEvent();
	void ReplotProfile();
	
	void NullifyNans(double *Data, size_t Len);
	
	
	
	void UpdateDateGridLinesX(Vector<double> &LinesOut);
	
	
	
	
	ArrayCtrl *EIndexList[MAX_INDEX_SETS];
	
	plot_data_storage PlotData;
	
	bool PlotWasAutoResized = false;
	
private:
	
	MobiView *Parent;
	
	std::vector<String> ProfileLabels;
	String ProfileLegend;
	String ProfileUnit;
	Date ProfileDisplayDate; //NOTE: Only currently used when in profile mode.
	size_t ProfileIndexesCount;
	
	Vector<String> QQLabels;
	
	plot_colors PlotColors;
	
	Date InputStartDate;
};


#endif
