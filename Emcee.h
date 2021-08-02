#ifndef _MobiView_Emcee_h_
#define _MobiView_Emcee_h_


#include <Core/Core.h>


double EmceeWalkStep(double A, const double *CurWalker, double *NewWalkerOut, size_t NPars, double PrevLL, const double *Ensemble, size_t NEnsemble, 
	double (*LogLikelyhood)(void *, double *), void *LLFunState);
	
void RunEmcee(size_t NWalkers, size_t NPars, size_t NSteps, double (*LogLikelyhood)(void *, double *), 
	void **LLFunStates, double *ParsOut, double *LLOut, double A);


#endif
