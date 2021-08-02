
#include "Emcee.h"

/*
	This is a simple C++ implementation of the Affine-invariant ensemble sampler from https://github.com/dfm/emcee
	
	( Foreman-Mackey, Hogg, Lang & Goodman (2012) Emcee: the MCMC Hammer )

*/

using namespace Upp;

double EmceeWalkStep(double A, const double *CurWalker, double *NewWalkerOut, size_t NPars, double PrevLL, const double *Ensemble, size_t NEnsemble, 
	double (*LogLikelyhood)(void *, double *), void *LLFunState)
{
	// Draw a random stretch factor
	double U = Randomf(); //uniform between 0,1   TODO: replace with something that is guaranteed to be thread safe.
	double ZZ = (A - 1.0)*U + 1.0;
	ZZ = ZZ*ZZ/A;
	
	int EnsembleIdx = (int)Random(NEnsemble);
	
	for(int Idx = 0; Idx < NPars; ++Idx)
	{
		double Xj = Ensemble[EnsembleIdx*NPars + Idx];
		NewWalkerOut[Idx] = Xj + ZZ*(CurWalker[Idx] - Xj);
	}
	
	double LL = LogLikelyhood(LLFunState, NewWalkerOut);
	double R = Randomf();
	
	double Q = std::pow(ZZ, (double)NPars-1.0)*LL/PrevLL;
	
	if(R > Q)  // Don't accept the proposal
	{
		for(int Idx = 0; Idx < NPars; ++Idx)
			NewWalkerOut[Idx] = CurWalker[Idx];
			
		LL = PrevLL;
	}
	
	return LL;
}

void RunEmcee(size_t NWalkers, size_t NPars, size_t NSteps, double (*LogLikelyhood)(void *, double *), 
	void **LLFunStates, double *ParsOut, double *LLOut, double A)
{
	size_t NEns1 = NWalkers / 2;
	size_t NEns2 = NEns1;
	if(NWalkers % 2 != 0)
		NEns2++;
	
	// Compute the LLs of the initial walkers (Can be paralellized too, but probably unnecessary)
	for(int WalkIdx = 0; WalkIdx < NWalkers; ++WalkIdx)
	{
		double *Pars = ParsOut + WalkIdx*NPars;
		double LL = LogLikelyhood(LLFunStates[WalkIdx], Pars);
		LLOut[WalkIdx] = LL;
	}
	
	for(int Step = 1; Step < NSteps; ++Step)
	{
		//First complementary ensemble is ensemble2 from previous step
		double *Ensemble = ParsOut + ((Step - 1)*NWalkers + NEns1)*NPars;
		
		//TODO: This for loop should be parallellized:
		for(int WalkIdx = 0; WalkIdx < NEns1; ++WalkIdx)
		{
			double *PrevWalker = ParsOut + ((Step - 1)*NWalkers + WalkIdx)*NPars;
			double *NextWalker = ParsOut + (Step*NWalkers + WalkIdx)*NPars;
			double PrevLL = LLOut[(Step-1)*NWalkers + WalkIdx];
			
			double LL = EmceeWalkStep(A, PrevWalker, NextWalker, NPars, PrevLL, Ensemble, NEns2, LogLikelyhood, LLFunStates[WalkIdx]);
			
			LLOut[Step*NWalkers + WalkIdx] = LL;
		}
		
		//Second complementary ensemble is ensemble1 from this step
		Ensemble = ParsOut + Step*NWalkers*NPars;
		
		//TODO: Paralellize
		for(int WalkIdx = 0; WalkIdx < NEns2; ++WalkIdx)
		{
			double *PrevWalker = ParsOut + ((Step - 1)*NWalkers + WalkIdx + NEns1)*NPars;
			double *NextWalker = ParsOut + (Step*NWalkers + WalkIdx + NEns1)*NPars;
			double PrevLL = LLOut[(Step-1)*NWalkers + WalkIdx + NEns1];
			
			double LL = EmceeWalkStep(A, PrevWalker, NextWalker, NPars, PrevLL, Ensemble, NEns1, LogLikelyhood, LLFunStates[WalkIdx]);
			
			LLOut[Step*NWalkers + WalkIdx + NEns1] = LL;
		}
	}
}