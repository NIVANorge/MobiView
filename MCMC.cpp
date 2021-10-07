
#include "MCMC.h"


//NOTE: This is used for random number generators and threading. Somebody could replace it with something Non-Upp specific if they want to reuse this code.
#include <Core/Core.h>
using namespace Upp;


bool EmceeStretchMove(double A, int Step, int Walker, int FirstEnsembleWalker, int EnsembleStep, size_t NEnsemble, mcmc_data *Data,
	double (*LogLikelyhood)(void *, int, int), void *LLFunState)
{
	/*
	This is a simple C++ implementation of the Affine-invariant ensemble sampler from https://github.com/dfm/emcee
	
	( Foreman-Mackey, Hogg, Lang & Goodman (2012) Emcee: the MCMC Hammer )

	*/
	
	bool Accepted = true;
	
	double PrevLL = Data->LLValue(Walker, Step-1);
	
	//NOTE: The random generators are supposed to be thread safe.
	// Draw a random stretch factor
	double U = Randomf(); //uniform between 0,1
	double ZZ = (A - 1.0)*U + 1.0;
	ZZ = ZZ*ZZ/A;
	
	int EnsembleWalker = (int)Random(NEnsemble) + FirstEnsembleWalker; //NOTE: Only works for the particular way the Ensembles are ordered right now.
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		double Xj = (*Data)(EnsembleWalker, Par, EnsembleStep);
		double Xk = (*Data)(Walker, Par, Step-1);
		(*Data)(Walker, Par, Step) = Xj + ZZ*(Xk - Xj);
	}
	
	double LL = LogLikelyhood(LLFunState, Walker, Step);
	double R = Randomf();
	
	double Q = std::min(1.0, std::pow(ZZ, (double)Data->NPars-1.0)*std::exp(LL-PrevLL));
	
	if(!std::isfinite(LL) || R > Q)  // Reject the proposal
	{
		for(int Par = 0; Par < Data->NPars; ++Par)
			(*Data)(Walker, Par, Step) = (*Data)(Walker, Par, Step-1);
			
		LL = PrevLL;
		Accepted = false;
	}
	
	Data->LLValue(Walker, Step) = LL;
	return Accepted;
}


//NOTE: Not currently in use:
bool DifferentialEvolutionMove(double B, double C, int Step, int Walker, int FirstEnsembleWalker, int EnsembleStep, size_t NEnsemble, mcmc_data *Data,
	double (*LogLikelyhood)(void *, int, int), void *LLFunState)
{
	/*
	Based on
	
	Braak, C.J.F.T.
	A Markov Chain Monte Carlo version of the genetic algorithm Differential Evolution: easy Bayesian computing for real parameter spaces.
	Stat Comput 16, 239–249 (2006). https://doi.org/10.1007/s11222-006-8769-1
	*/
	
	//TODO: Also implement crossover probability?
	
	bool Accepted = true;
	
	double PrevLL = Data->LLValue(Walker, Step-1);
	
	int EnsW1 = (int)Random(NEnsemble) + FirstEnsembleWalker;
	int EnsW2;
	do
		EnsW2 = (int)Random(NEnsemble) + FirstEnsembleWalker;
	while(EnsW1 == EnsW2);
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		//TODO: Shouldn't BB be scaled relative to the par |max-min| ??
		double BB = -B + Randomf()*2.0*B; // Uniform [-B, B]
		
		double Xr1 = (*Data)(EnsW1, Par, EnsembleStep);
		double Xr2 = (*Data)(EnsW2, Par, EnsembleStep);
		double Xk  = (*Data)(Walker, Par, Step-1);
		(*Data)(Walker, Par, Step) = Xk + C*(Xr1 - Xr2) + BB;
	}
	
	double LL = LogLikelyhood(LLFunState, Walker, Step);
	double R = Randomf();
	double Q = LL - PrevLL;
	if(Q < std::log(R))
	{
		for(int Par = 0; Par < Data->NPars; ++Par)
			(*Data)(Walker, Par, Step) = (*Data)(Walker, Par, Step-1);
			
		LL = PrevLL;
		Accepted = false;
	}
	
	Data->LLValue(Walker, Step) = LL;
	return Accepted;
}




#define MCMC_MULTITHREAD 1

bool RunMCMC(double (*LogLikelyhood)(void *, int, int), void *LLFunState, mcmc_data *Data, double A, bool (*Callback)(void *, int), void *CallbackState, int CallbackInterval, int InitialStep)
{
	size_t NEns1 = Data->NWalkers / 2;
	size_t NEns2 = NEns1;
	if(Data->NWalkers % 2 != 0) NEns2++;
	
	// Compute the LLs of the initial walkers (Can be paralellized too, but probably unnecessary)
	// NOTE: This assumes that Data(Walker, Par, 0) has been filled with an initial ensemble.
	if(InitialStep == 0) //NOTE: If the initial step is not 0, this is a continuation of an earlier run, and so the LL value will already have been computed.
		for(int Walker = 0; Walker < Data->NWalkers; ++Walker)
			Data->LLValue(Walker, 0) = LogLikelyhood(LLFunState, Walker, 0);

	Array<AsyncWork<int>> Workers;
	Workers.InsertN(0, NEns2);
	
	for(int Step = InitialStep+1; Step < Data->NSteps; ++Step)
	{
		//First complementary ensemble is ensemble2 from previous step
		int FirstEnsembleWalker = NEns1;
		int EnsembleStep        = Step-1;
		
#if MCMC_MULTITHREAD
		for(int Walker = 0; Walker < NEns1; ++Walker)
			Workers[Walker].Do([=]()->int {return (int)EmceeStretchMove(A, Step, Walker, FirstEnsembleWalker, EnsembleStep, NEns2, Data, LogLikelyhood, LLFunState);});
		
		for(int Walker = 0; Walker < NEns1; ++Walker)
			Data->NAccepted += Workers[Walker].Get();
			
#else
		for(int Walker = 0; Walker < NEns1; ++Walker)
			Data->NAccepted += (int)EmceeStretchMove(A, Step, Walker, FirstEnsembleWalker, EnsembleStep, NEns2, Data, LogLikelyhood, LLFunState);
#endif
		
		
		//Second complementary ensemble is ensemble1 from this step
		FirstEnsembleWalker = 0;
		EnsembleStep        = Step;

#if MCMC_MULTITHREAD
		for(int Walker = NEns1; Walker < Data->NWalkers; ++Walker)
			Workers[Walker-NEns1].Do([=]()->int {return (int)EmceeStretchMove(A, Step, Walker, FirstEnsembleWalker, EnsembleStep, NEns1, Data, LogLikelyhood, LLFunState); });
			
		//for(auto &Worker : Workers2)
		for(int Walker = NEns1; Walker < Data->NWalkers; ++Walker)
			Data->NAccepted += Workers[Walker-NEns1].Get();
	
#else
		for(int Walker = NEns1; Walker < Data->NWalkers; ++Walker)
			Data->NAccepted += (int)EmceeStretchMove(A, Step, Walker, FirstEnsembleWalker, EnsembleStep, NEns1, Data, LogLikelyhood, LLFunState);
#endif
		
		bool Halt = false;
		if(((Step-InitialStep) % CallbackInterval == 0) || (Step==Data->NSteps-1)) Halt = !Callback(CallbackState, Step);
		
		if(Halt) return false;
	}
	
	return true;
}