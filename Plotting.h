#ifndef _MobiView_PlotDataStorage_h_
#define _MobiView_PlotDataStorage_h_

//NOTE: This has to match up to the major mode selector.
enum plot_major_mode
{
	MajorMode_Regular = 0,
	MajorMode_Histogram,
	MajorMode_Profile,
	MajorMode_Profile2D,
	MajorMode_CompareBaseline,
	MajorMode_Residuals,
	MajorMode_ResidualHistogram,
	MajorMode_QQ,
};

//NOTE: This has to match up to the aggregation selector.
enum aggregation_type
{
	Aggregation_Mean = 0,
	Aggregation_Sum,
	Aggregation_Min,
	Aggregation_Max,
};

//NOTE: This has to match up to the aggregation period selector.
enum aggregation_period
{	Aggregation_None = 0,
	Aggregation_Monthly,
	Aggregation_Yearly,
};

enum y_axis_mode
{
	YAxis_Regular = 0,
	YAxis_Normalized,
	YAxis_Logarithmic,
};

//const size_t NUM_PERCENTILES = 7;
//const double PERCENTILES[NUM_PERCENTILES] = {0.05, 0.15, 0.25, 0.5, 0.75, 0.85, 0.95};

struct timeseries_stats
{
	double Min;
	double Max;
	double Sum;
	double Median;
	std::vector<double> Percentiles;
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
	double LogNashSutcliffe;
	double R2;
	double IndexOfAgreement;
	double KlingGuptaEfficiency;
	double SpearmansRCC;
	size_t DataPoints;
};

struct StatisticsSettings
{
	bool DisplayMin          = true;
	bool DisplayMax          = true;
	bool DisplaySum          = true;
	bool DisplayMedian       = true;
	bool DisplayMean         = true;
	bool DisplayVariance     = true;
	bool DisplayStandardDev  = true;
	
	bool DisplayMeanError    = true;
	bool DisplayMAE          = true;
	bool DisplayRMSE         = true;
	bool DisplayNS           = true;
	bool DisplayLogNS        = true;
	bool DisplayR2           = true;
	bool DisplayIdxAgr       = true;
	bool DisplayKGE          = true;
	bool DisplaySRCC         = true;
	
	std::vector<double> Percentiles = {0.0, 5.0, 15.0, 25.0, 50.0, 75.0, 85.0, 95.0, 100.0};
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

struct plot_setup
{
	plot_major_mode    MajorMode;
	aggregation_type   AggregationType;
	aggregation_period AggregationPeriod;
	y_axis_mode        YAxisMode;
	bool               ScatterInputs;
	
	std::vector<std::string> SelectedResults;
	std::vector<std::string> SelectedInputs;
	
	std::vector<bool> IndexSetIsActive;
	std::vector<std::vector<std::string>> SelectedIndexes;
	
	
	int ProfileTimestep;
};


class MobiView;

void NullifyNans(double *Data, size_t Len);
void AdvanceTimesteps(Time &T, uint64 Timesteps, timestep_size TimestepSize);
int64 TimestepsBetween(Time &T1, Time &T2, timestep_size TimestepSize);
int GetSmallestStepResolution(aggregation_period IntervalType, timestep_size TimestepSize);
void ComputeXValues(Time &ReferenceTime, Time &StartTime, uint64 Timesteps, timestep_size TimestepSize, double *WriteX);

void AggregateData(Time &ReferenceTime, Time &StartTime, uint64 Timesteps, double *Data, aggregation_period IntervalType, aggregation_type AggregationType, timestep_size TimestepSize, std::vector<double> &XValues, std::vector<double> &YValues);


void ComputeTimeseriesStats(timeseries_stats &StatsOut, double *Data, size_t Len, StatisticsSettings &StatSettings);
void ComputeResidualStats(residual_stats &StatsOut, double *Obs, double *Mod, size_t Len);
void ComputeTrendStats(double *XData, double *YData, size_t Len, double YMean, double &XMeanOut, double &XVarOut, double &XYCovarOut);


void DisplayTimeseriesStats(timeseries_stats &Stats, String &Name, String &Unit, StatisticsSettings &StatSettings, DocEdit &PlotInfo);
void DisplayResidualStats(residual_stats &Stats, String &Name, StatisticsSettings &StatSettings, DocEdit &PlotInfo);

class PlotCtrl;

class MyPlot : public ScatterCtrl
{
public:
	
	MyPlot();
	
	void BuildPlot(MobiView *Parent, PlotCtrl *Control, bool IsMainPlot, DocEdit &PlotInfo);
	
	void AddPlot(String &Legend, String &Unit, double *XIn, double *Data, size_t Len, bool IsInput, Time &ReferenceTime, Time &StartTime, timestep_size TimestepSize, double MinY = 0.0, double MaxY = 0.0);
	int  AddHistogram(String &Legend, String &Unit, double *Data, size_t Len);
	void AddQQPlot(String &ModUnit, String &ObsUnit, String &ModName, String &ObsName, timeseries_stats &ModeledStats, timeseries_stats &ObservedStats, StatisticsSettings &StatSettings);
	void AddLine(const String &Legend, double X0, double X1, double Y0, double Y1, Color GraphColor = Null);
	void AddTrendLine(String &Legend, double XYCovar, double XVar, double YMean, double XMean, double StartX, double EndX);
	void AddNormalApproximation(String &Legend, int SampleCount, double Min, double Max, double Mean, double StdDev);
	
	
	void ClearAll();
	
	void AddPlotRecursive(MobiView *Parent, DocEdit &PlotInfo, std::string &Name, std::vector<char *> &IndexSets, std::vector<std::string> &CurrentIndexes, int Level, bool IsInput, uint64 Timesteps, Time &ReferenceTime, Time &StartTime, double *XIn);
	
	
	void ReplotProfile();
	
	
	void SetBetterGridLinePositions(int Dim);
	void UpdateDateGridLinesX(Vector<double> &LinesOut, Time InputStartTime, timestep_size TimestepSize);
	

	//TODO: Move to private when that is possible!
	plot_data_storage PlotData;
	plot_colors PlotColors;
	
	//NOTE: Only used when in profile mode.
	std::vector<std::string> ProfileLabels;
	String ProfileLegend;
	String ProfileUnit;
	size_t ProfileIndexesCount;
	
	plot_setup PlotSetup;
	
	bool PlotWasAutoResized = false;
private:
	Vector<String> QQLabels;
	
	Vector<double> SurfX;
	Vector<double> SurfY;
	Vector<double> SurfZ;
	TableDataVector SurfData;
};





#endif
