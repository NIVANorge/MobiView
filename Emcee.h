#ifndef _MobiView_Emcee_h_
#define _MobiView_Emcee_h_


#include <Core/Core.h> //TODO: We should get rid of this dependency somehow.. //NOTE: Just for the random generators. Could be switched out.


struct mcmc_data
{
	double *ParData = nullptr;
	double *LLData  = nullptr;
	
	size_t NSteps, NWalkers, NPars;
	
	void Allocate(size_t NWalkers, size_t NPars, size_t NSteps)
	{
		ParData = (double *)malloc(sizeof(double)*NSteps*NWalkers*NPars);
		LLData  = (double *)malloc(sizeof(double)*NSteps*NWalkers);
		this->NSteps   = NSteps;
		this->NWalkers = NWalkers;
		this->NPars    = NPars;
	}
	
	void Free()
	{
		if(ParData) free(ParData);
		if(LLData)  free(LLData);
		ParData = nullptr;
		LLData  = nullptr;
	}
	
	/*
		IMPORTANT NOTICE: It is important for usage purposes that the steps of one parameter for one walker are
		stored sequentially. E.g. when increasing the step by one, but keeping the parameter
		and walker constant, the address should only change by one entry. This allows for easy
		plotting of the chains.
	*/
	
	double &operator()(int Walker, int Par, int Step)
	{
		return ParData[Step + NSteps*(Par + NPars*Walker)];
	}
	
	double &LLValue(int Walker, int Step)
	{
		return LLData[Walker*NSteps + Step];
	}
};

void RunEmcee(double (*LogLikelyhood)(void *, int, int), void *LLFunState, mcmc_data &Data, double A);


#endif
