#ifndef _MobiView_PlotDataStorage_h_
#define _MobiView_PlotDataStorage_h_


#include "MyRichView.h"

//NOTE: This has to match up to the aggregation selector. It should also match the override
//modes in the AdditionalPlotView
enum plot_major_mode
{
	MajorMode_Regular = 0,
	MajorMode_Stacked,
	MajorMode_StackedShare,
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

//NOTE: The matching of this with the selector should be dynamic, so no worries.
enum aggregation_period
{	Aggregation_None = 0,
	Aggregation_Weekly,
	Aggregation_Monthly,
	Aggregation_Yearly,
};

//NOTE: This has to match up to the y axis mode selector.
enum y_axis_mode
{
	YAxis_Regular = 0,
	YAxis_Normalized,
	YAxis_Logarithmic,
};

#define SET_SETTING(Handle, Name, Type) \
	double Handle;
#define SET_RES_SETTING(Handle, Name, Type)
	
struct timeseries_stats
{
	#include "SetStatSettings.h"
	
	std::vector<double> Percentiles;
	size_t DataPoints;
};

#undef SET_SETTING
#undef SET_RES_SETTING

#define SET_RES_SETTING(Handle, Name, Type) double Handle;
#define SET_SETTING(Handle, Name, Type)

struct residual_stats
{
	#include "SetStatSettings.h"
	
	//Could also have these in SetStatSettings.h, but it does not make that much sense to make
	//them displayable
	double MinError;
	double MaxError;
	
	size_t DataPoints;
	bool WasInitialized = false;
};

#undef SET_SETTING
#undef SET_RES_SETTING


#define SET_SETTING(Handle, Name, Type)
#define SET_RES_SETTING(Handle, Name, Type)   ResidualType_##Handle,
enum residual_type
{
	ResidualType_Offset = 100,
	#include "SetStatSettings.h"
	ResidualType_End    = 199,
};
#undef SET_SETTING
#undef SET_RES_SETTING

#define SET_SETTING(Handle, Name, Type) StatType_##Handle,
#define SET_RES_SETTING(Handle, Name, Type)
enum stat_type
{
	StatType_Offset = 0,
	#include "SetStatSettings.h"
	StatType_End    = 99,
};
#undef SET_SETTING
#undef SET_RES_SETTING


#define SET_SETTING(Handle, Name, Type) bool Display##Handle = true;
#define SET_RES_SETTING(Handle, Name, Type) SET_SETTING(Handle, Name, Type)

struct StatisticsSettings
{
	#include "SetStatSettings.h"
	
	std::vector<double> Percentiles = {2.5, 5.0, 15.0, 25.0, 50.0, 75.0, 85.0, 95.0, 97.5};
	
	int Precision = 5;
	double EckhardtFilterParam = 0.925;
};

#undef SET_SETTING
#undef SET_RES_SETTING

struct plot_data_storage
{
	plot_data_storage()
	{
		//NOTE: We should never allow the outer vector to resize, because that could invalidate references to inner vectors.
		//TODO: This is a really crappy way of doing it, and is error prone.
		Data.reserve(1024);
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
		PlotColors = {{0, 130, 200}, {230, 25, 75}, {245, 130, 48}, {145, 30, 180}, {60, 180, 75},
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

//void NullifyNans(double *Data, size_t Len);
void AdvanceTimesteps(Time &T, uint64 Timesteps, timestep_size TimestepSize);
int64 TimestepsBetween(const Time &T1, const Time &T2, timestep_size TimestepSize);
int GetSmallestStepResolution(aggregation_period IntervalType, timestep_size TimestepSize);
void ComputeXValues(Time &ReferenceTime, Time &StartTime, uint64 Timesteps, timestep_size TimestepSize, double *WriteX);

void AggregateData(Time &ReferenceTime, Time &StartTime, uint64 Timesteps, double *Data, aggregation_period IntervalType, aggregation_type AggregationType, timestep_size TimestepSize, std::vector<double> &XValues, std::vector<double> &YValues);


void ComputeTimeseriesStats(timeseries_stats &StatsOut, double *Data, size_t Len, const StatisticsSettings &StatSettings, bool AlreadySorted = false);
void ComputeResidualStats(residual_stats &StatsOut, double *Obs, double *Mod, size_t Len);
void ComputeTrendStats(double *XData, double *YData, size_t Len, double YMean, double &XMeanOut, double &XVarOut, double &XYCovarOut);


void DisplayTimeseriesStats(timeseries_stats &Stats, String &Name, String &Unit, const StatisticsSettings &StatSettings, MyRichView &PlotInfo, Color Col = Color(0, 0, 0));
void DisplayResidualStats(residual_stats &Stats, residual_stats &CachedStats, String &Name, const StatisticsSettings &StatSettings, MyRichView &PlotInfo, bool DisplayChange);

void SetBetterGridLinePositions(ScatterDraw &Scatter, int Dim);


class PlotCtrl;

class MyPlot : public ScatterCtrl
{
public:
	
	MyPlot();
	
	void BuildPlot(MobiView *Parent, PlotCtrl *Control, bool IsMainPlot, MyRichView &PlotInfo, bool CausedByReRun = false, int OverrideMode = -100);
	
	void FormatAxes(plot_major_mode PlotMajorMode, int NBinsHistogram, Time InputStartTime, timestep_size TimestepSize);
	
	Color AddPlot(String &Legend, String &Unit, double *XIn, double *Data, size_t Len, bool IsInput, Time &ReferenceTime, Time &StartTime, timestep_size TimestepSize, double MinY = 0.0, double MaxY = 0.0, Color OverrideColor=Null, plot_major_mode MajorMode=MajorMode_Regular);
	int  AddHistogram(const String &Legend, const String &Unit, double *Data, size_t Len);
	void AddQQPlot(String &ModUnit, String &ObsUnit, String &ModName, String &ObsName, timeseries_stats &ModeledStats, timeseries_stats &ObservedStats, StatisticsSettings &StatSettings);
	void AddLine(const String &Legend, double X0, double X1, double Y0, double Y1, Color GraphColor = Null);
	void AddTrendLine(String &Legend, double XYCovar, double XVar, double YMean, double XMean, double StartX, double EndX);
	void AddNormalApproximation(String &Legend, int SampleCount, double Min, double Max, double Mean, double StdDev);
	
	
	void ClearAll(bool FullClear = true);
	
	void AddPlotRecursive(MobiView *Parent, MyRichView &PlotInfo, std::string &Name, std::vector<char *> &IndexSets,
		std::vector<std::string> &CurrentIndexes, int Level, bool IsInput, uint64 Timesteps, Time &ReferenceTime,
		Time &StartTime, double *XIn, plot_major_mode MajorMode, int64 GofOffset, int64 GofTimesteps);
	
	
	void ReplotProfile();
	
	void UpdateDateGridLinesX(Vector<double> &LinesOut, Time InputStartTime, timestep_size TimestepSize);
	
	
	
	//NOTE: To facilitate stacked plots:
	ScatterDraw &MyAddSeries(double *XValues, double *YValues, size_t Len, bool IsInput, const Color Col, plot_major_mode MajorMode);
	void StackedPlotFixup(plot_major_mode MajorMode);
	
	std::vector<double *> CachedStackY;
	size_t CachedStackLen;
	
	
	

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
	
	
	virtual void Layout() override
	{
		ScatterCtrl::Layout();
		//NOTE: Always let the save/export size be the same as the window size.
		Size PlotSize = GetSize();
		SetSaveSize(PlotSize);
	}
	
	
private:
	Vector<String> QQLabels;
	
	//TODO: TableDataVector should be replaced with TableDataCArray, and then we can get rid of the SurfX,
	//SurfY, SurfZ stuff.
	Vector<double> SurfX;
	Vector<double> SurfY;
	Vector<double> SurfZ;
	TableDataVector SurfData;
	
	residual_stats CachedStats;
};





#endif
