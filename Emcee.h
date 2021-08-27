#ifndef _MobiView_Emcee_h_
#define _MobiView_Emcee_h_

#include <limits>
#include <stdlib.h>

struct mcmc_data
{
	double *ParData = nullptr;
	double *LLData  = nullptr;
	size_t NSteps=0, NWalkers=0, NPars=0;
	size_t NAccepted = 0;
	
	void Allocate(size_t NWalkers, size_t NPars, size_t NSteps)
	{
		Free();
		
		ParData = (double *)malloc(sizeof(double)*NSteps*NWalkers*NPars);
		LLData  = (double *)malloc(sizeof(double)*NSteps*NWalkers);
		this->NSteps   = NSteps;
		this->NWalkers = NWalkers;
		this->NPars    = NPars;
		
		for(int Idx = 0; Idx < NSteps*NWalkers*NPars; ++Idx) ParData[Idx] = std::numeric_limits<double>::quiet_NaN();
		for(int Idx = 0; Idx < NSteps*NWalkers; ++Idx)       LLData[Idx]  = std::numeric_limits<double>::quiet_NaN();
		
		NAccepted = 0;
	}
	
	void ExtendSteps(size_t NStepsNew)
	{
		if(!ParData || NStepsNew <= NSteps) return;
		
		double *ParDataNew = (double *)malloc(sizeof(double)*NStepsNew*NWalkers*NPars);
		double *LLDataNew  = (double *)malloc(sizeof(double)*NStepsNew*NWalkers);
		
		for(int Walker = 0; Walker < NWalkers; ++Walker)
		{
			for(int Par = 0; Par < NPars; ++Par)
			{
				for(int Step = 0; Step < NStepsNew; ++Step)
				{
					if(Step < NSteps)
						ParDataNew[Step + NStepsNew*(Par + NPars*Walker)] = (*this)(Walker, Par, Step);
					else
						ParDataNew[Step + NStepsNew*(Par + NPars*Walker)] = std::numeric_limits<double>::quiet_NaN();
				}
			}
			
			for(int Step = 0; Step < NStepsNew; ++Step)
			{
				if(Step < NSteps)
					LLDataNew[Walker*NStepsNew + Step] = LLValue(Walker, Step);
				else
					LLDataNew[Walker*NStepsNew + Step] = std::numeric_limits<double>::quiet_NaN();
			}
		}
		NSteps = NStepsNew;
		free(ParData);
		free(LLData);
		ParData = ParDataNew;
		LLData  = LLDataNew;
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
	
	void GetMAPIndex(int Burnin, int CurStep, int &BestW, int &BestS)
	{
		//NOTE: Doesn't do error handling, all bounds have to be checked externally
		
		double Best = -std::numeric_limits<double>::infinity();
		for(int Step = Burnin; Step <= CurStep; ++Step)
		{
			for(int Walker = 0; Walker < NWalkers; ++Walker)
			{
				double Val = LLValue(Walker, Step);
				if(Val > Best)
				{
					Best = Val;
					BestW = Walker;
					BestS = Step;
				}
			}
		}
	}
};

bool RunEmcee(double (*LogLikelyhood)(void *, int, int), void *LLFunState, mcmc_data *Data, double A, bool (*Callback)(void *, int), void *CallbackState, int CallbackInterval, int InitialStep);


#endif
