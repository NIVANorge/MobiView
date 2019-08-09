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


#endif
