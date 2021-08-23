
#include "Emcee.h"

/*
	This is a simple C++ implementation of the Affine-invariant ensemble sampler from https://github.com/dfm/emcee
	
	( Foreman-Mackey, Hogg, Lang & Goodman (2012) Emcee: the MCMC Hammer )

*/

using namespace Upp; //NOTE: Just for the random generators. Could be switched out.


bool EmceeStretchMove(double A, int Step, int Walker, int FirstEnsembleWalker, int EnsembleStep, size_t NEnsemble, mcmc_data &Data,
	double (*LogLikelyhood)(void *, int, int), void *LLFunState)
{
	bool Accepted = true;
	
	double PrevLL = Data.LLValue(Walker, Step-1);
	
	// Draw a random stretch factor
	double U = Randomf(); //uniform between 0,1   TODO: replace with something that is guaranteed to be thread safe.
	double ZZ = (A - 1.0)*U + 1.0;
	ZZ = ZZ*ZZ/A;
	
	int EnsembleWalker = (int)Random(NEnsemble) + FirstEnsembleWalker; //NOTE: Only works for the particular way the Ensembles are ordered right now.
	
	for(int Par = 0; Par < Data.NPars; ++Par)
	{
		double Xj = Data(EnsembleWalker, Par, EnsembleStep);
		double Xk = Data(Walker, Par, Step-1);
		Data(Walker, Par, Step) = Xj + ZZ*(Xk - Xj);
	}
	
	double LL = LogLikelyhood(LLFunState, Walker, Step);
	double R = Randomf();
	
	double Q = std::min(1.0, std::pow(ZZ, (double)Data.NPars-1.0)*std::exp(LL-PrevLL));
	
	if(!std::isfinite(LL) || R > Q)  // Reject the proposal
	{
		for(int Par = 0; Par < Data.NPars; ++Par)
			Data(Walker, Par, Step) = Data(Walker, Par, Step-1);
			
		LL = PrevLL;
		Accepted = false;
	}
	
	Data.LLValue(Walker, Step) = LL;
	return Accepted;
}

bool RunEmcee(double (*LogLikelyhood)(void *, int, int), void *LLFunState, mcmc_data &Data, double A, bool (*Callback)(void *, int), void *CallbackState, int CallbackInterval, int InitialStep)
{
	size_t NEns1 = Data.NWalkers / 2;
	size_t NEns2 = NEns1;
	if(Data.NWalkers % 2 != 0) NEns2++;
	
	// Compute the LLs of the initial walkers (Can be paralellized too, but probably unnecessary)
	// NOTE: This assumes that Data(Walker, Par, 0) has been filled with an initial ensemble.
	if(InitialStep == 0) //NOTE: If the initial step is not 0, this is a continuation of an earlier run, and so the LL value will already have been computed.
		for(int Walker = 0; Walker < Data.NWalkers; ++Walker)
			Data.LLValue(Walker, 0) = LogLikelyhood(LLFunState, Walker, 0);

	for(int Step = InitialStep+1; Step < Data.NSteps; ++Step)
	{
		//First complementary ensemble is ensemble2 from previous step
		int FirstEnsembleWalker = NEns1;
		int EnsembleStep        = Step-1;
		
		//TODO: This for loop should be parallellized:
		for(int Walker = 0; Walker < NEns1; ++Walker)
			Data.NAccepted += (int)EmceeStretchMove(A, Step, Walker, FirstEnsembleWalker, EnsembleStep, NEns2, Data, LogLikelyhood, LLFunState);
		
		//Second complementary ensemble is ensemble1 from this step
		FirstEnsembleWalker = 0;
		EnsembleStep        = Step;
		
		//TODO: Paralellize
		for(int Walker = NEns1; Walker < Data.NWalkers; ++Walker)
			Data.NAccepted += (int)EmceeStretchMove(A, Step, Walker, FirstEnsembleWalker, EnsembleStep, NEns1, Data, LogLikelyhood, LLFunState);
		
		bool Halt = false;
		if(((Step-InitialStep) % CallbackInterval == 0) || (Step==Data.NSteps-1)) Halt = !Callback(CallbackState, Step);
		
		if(Halt) return false;
	}
	
	return true;
}