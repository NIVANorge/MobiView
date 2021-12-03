
#include <vector>
#include <complex>
#include <plugin/eigen/unsupported/Eigen/FFT>
#include "MCMC.h"
#include "Acor.h"

//NOTE: Code adapted from python,  https://github.com/dfm/emcee/blob/main/src/emcee/autocorr.py

int NextPow2(int N)
{
	//return 1 << (sizeof(int) - __builtin_clz(N - 1));     //Could be optimized using bit scan, but probably not worth it
	int I = 1;
	while(I < N) I <<= 1;
	return I;
}

void NormalizedAutocorrelation1D(const std::vector<double> &Chain, std::vector<double> &Acor)
{
	Eigen::FFT<double> Fft;
	
	std::vector<double> Series2 = Chain;
	double Mean = 0.0;
	for(int Idx = 0; Idx < Series2.size(); ++Idx) Mean += Series2[Idx];
	Mean /= (double)Series2.size();
	for(int Idx = 0; Idx < Series2.size(); ++Idx) Series2[Idx] -= Mean;
	
	Series2.resize(2*NextPow2(Series2.size())); //NOTE: To avoid autocorrelation between the beginning and end.
	std::vector<std::complex<double>> Transformed;
	Fft.fwd(Transformed, Series2);

	for(int Idx = 0; Idx < Transformed.size(); ++Idx) Transformed[Idx] *= std::conj(Transformed[Idx]);
	std::vector<std::complex<double>> Back;
	Fft.inv(Back, Transformed);
	Acor.resize(Chain.size());
	for(int Idx = 0; Idx < Acor.size(); ++Idx) Acor[Idx] = Back[Idx].real() / Back[0].real();
	
	/* //NOTE: *MUCH* slower implementation.. :
	double Mean = 0.0;
	for(int Idx = 0; Idx < Chain.size(); ++Idx) Mean += Chain[Idx];
	Mean /= (double)Chain.size();
	
	Acor.resize(Chain.size());
	int N = Chain.size();
	for(int h = 0; h < N; ++h)
	{
		double Sum = 0.0;
		for(int t = 0; t < N-h; ++t)
			Sum += (Chain[t] - Mean)*(Chain[t+h] - Mean);
		Acor[h] = Sum / (double)N;
		Acor[h] /= Acor[0];
	}
	*/
}

double IntegratedTime(mcmc_data *Data, int Par, int C, int Tol, bool *BelowTolerance)
{
	std::vector<double> Acor(Data->NSteps, 0.0);
	std::vector<double> Acor1D;
	std::vector<double> Chain(Data->NSteps);
	
	//Take the mean of all 1D autocorrelation series for each walker.
	for(int Walker = 0; Walker < Data->NWalkers; ++Walker)
	{
		for(int Step = 0; Step < Data->NSteps; ++Step) Chain[Step] = (*Data)(Walker, Par, Step);
		NormalizedAutocorrelation1D(Chain, Acor1D);
		for(int Step = 0; Step < Data->NSteps; ++Step) Acor[Step] += Acor1D[Step] / (double)Data->NWalkers;
	}
	
	double Cumulative = 0.0;
	double TauEst;
	for(int Step = 0; Step < Data->NSteps; ++Step)
	{
		Cumulative += Acor[Step];
		TauEst = 2.0*Cumulative - 1.0;
		
		if((double)Step > (double)C*TauEst) break;
	}
	
	*BelowTolerance = (double)Tol * TauEst > (double)Data->NSteps;
	return TauEst;
}
