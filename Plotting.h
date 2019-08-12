#ifndef _MobiView_PlotDataStorage_h_
#define _MobiView_PlotDataStorage_h_


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


#endif
