
#include "MCMC.h"
#include <assert.h>

//NOTE: This is used for random number generators and threading. Somebody could replace it with something Non-Upp specific if they want to reuse this code.
#include <Core/Core.h>
using namespace Upp;


//NOTE: Using Box-Muller to draw independent normally distributed variables since u++ doesn't have a random normal distribution, and we don't
	    //want to use <random> since it is not thread safe.
void DrawIndependentStandardNormals(double *NormalsOut, int Size)
{
	double Z2;
	for(int Idx = 0; Idx < Size; ++Idx)
	{
		if(Idx % 2 == 0)
		{
			double U1 = Randomf();
			double U2 = Randomf();
			double A = std::sqrt(-2.0*std::log(U1));
			double Z1 = A*std::cos(2.0*M_PI*U2);
			       Z2 = A*std::sin(2.0*M_PI*U2);
			       
			NormalsOut[Idx] = Z1;
		}
		else
			NormalsOut[Idx] = Z2;
	}
}


typedef bool (*SamplerMove)(double *, double *, int, int, int, int, size_t, mcmc_data *, double (*LogLikelyhood)(void *, int, int), void *);

bool AffineStretchMove(double *SamplerParams, double *Scale, int Step, int Walker, int FirstEnsembleWalker, int EnsembleStep, size_t NEnsemble, mcmc_data *Data,
	double (*LogLikelyhood)(void *, int, int), void *LLFunState)
{
	/*
	This is a simple C++ implementation of the Affine-invariant ensemble sampler from https://github.com/dfm/emcee
	
	( Foreman-Mackey, Hogg, Lang & Goodman (2012) Emcee: the MCMC Hammer )

	*/
	
	double A = SamplerParams[0];
	
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
	
	double Q = std::pow(ZZ, (double)Data->NPars-1.0)*std::exp(LL-PrevLL);
	
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

bool AffineWalkMove(double *SamplerParams, double *Scale, int Step, int Walker, int FirstEnsembleWalker, int EnsembleStep, size_t NEnsemble, mcmc_data *Data,
	double (*LogLikelyhood)(void *, int, int), void *LLFunState)
{	
	int S0 = (int)SamplerParams[0];
	
	bool Accepted = true;
	
	double PrevLL = Data->LLValue(Walker, Step-1);
	
	double *Z = (double *)malloc(S0*sizeof(double));
	int    *Ens = (int *)malloc(S0*sizeof(int));
	
	DrawIndependentStandardNormals(Z, S0);
	
	double Z2;
	for(int S = 0; S < S0; ++S)
		Ens[S] = (int)Random(S0); // NOTE: Unlike in Differential Evolution, the members of the sub-ensemble could be repeating (or at least I can't see it mentioned in the paper).
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		double Xk = (*Data)(Walker, Par, Step-1);
		double Xsmean = 0.0;
		for(int S = 0; S < S0; ++S)
		{
			int EnsembleWalker = Ens[S] + FirstEnsembleWalker;
			Xsmean += (*Data)(EnsembleWalker, Par, EnsembleStep);
		}
		Xsmean /= (double)S0;
		double W = 0.0;
		for(int S = 0; S < S0; ++S)
		{
			int EnsembleWalker = Ens[S] + FirstEnsembleWalker;
			double Xj = (*Data)(EnsembleWalker, Par, EnsembleStep);
			W += Z[S]*(Xj - Xsmean);
		}
		(*Data)(Walker, Par, Step) = Xk + W;
	}
	
	free(Z);
	free(Ens);
	
	double LL = LogLikelyhood(LLFunState, Walker, Step);
	double R = Randomf();
	
	double Q = LL-PrevLL;
	
	if(!std::isfinite(LL) || std::log(R) > Q)  // Reject the proposal
	{
		for(int Par = 0; Par < Data->NPars; ++Par)
			(*Data)(Walker, Par, Step) = (*Data)(Walker, Par, Step-1);
			
		LL = PrevLL;
		Accepted = false;
	}
	
	Data->LLValue(Walker, Step) = LL;
	return Accepted;
}

bool DifferentialEvolutionMove(double *SamplerParams, double *Scale, int Step, int Walker, int FirstEnsembleWalker, int EnsembleStep, size_t NEnsemble, mcmc_data *Data,
	double (*LogLikelyhood)(void *, int, int), void *LLFunState)
{
	/*
	Based on
	
	Braak, C.J.F.T.
	A Markov Chain Monte Carlo version of the genetic algorithm Differential Evolution: easy Bayesian computing for real parameter spaces.
	Stat Comput 16, 239–249 (2006). https://doi.org/10.1007/s11222-006-8769-1
	*/
	
	double C = SamplerParams[0];
	double B = SamplerParams[1];
	double CR = SamplerParams[2];
	
	if(C < 0.0) C = 2.38 / std::sqrt(2.0 * (double)Data->NPars); // Default.
	
	bool Accepted = true;
	
	double PrevLL = Data->LLValue(Walker, Step-1);
	
	int EnsW1 = (int)Random(NEnsemble) + FirstEnsembleWalker;
	int EnsW2;
	do	
		EnsW2 = (int)Random(NEnsemble) + FirstEnsembleWalker;
	while(EnsW2 == EnsW1);
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		double Xk  = (*Data)(Walker, Par, Step-1);
		
		double Cross = Randomf();
		if(Cross <= CR)
		{
			double BS = B*Scale[Par];  //Scale relative to |max - min| for the parameter.
			double BB = -BS + Randomf()*2.0*BS; // Uniform [-BS, BS]
			
			double Xr1 = (*Data)(EnsW1, Par, EnsembleStep);
			double Xr2 = (*Data)(EnsW2, Par, EnsembleStep);
			
			(*Data)(Walker, Par, Step) = Xk + C*(Xr1 - Xr2) + BB;
		}
		else
			(*Data)(Walker, Par, Step) = Xk;
	}
	
	double LL = LogLikelyhood(LLFunState, Walker, Step);
	double R = Randomf();
	double Q = LL - PrevLL;
	
	if(Q < std::log(R))  // Reject
	{
		for(int Par = 0; Par < Data->NPars; ++Par)
			(*Data)(Walker, Par, Step) = (*Data)(Walker, Par, Step-1);
			
		LL = PrevLL;
		Accepted = false;
	}
	Data->LLValue(Walker, Step) = LL;
	
	return Accepted;
}

bool MetropolisMove(double *SamplerParams, double *Scale, int Step, int Walker, int FirstEnsembleWalker, int EnsembleStep, size_t NEnsemble, mcmc_data *Data,
	double (*LogLikelyhood)(void *, int, int), void *LLFunState)
{
	// Single-chain Metropolis hastings.
	assert(Data->NWalkers == 1);
	
	double B = SamplerParams[0];   // Scale of normal perturbation.
	
	double *Perturb = (double *)malloc(sizeof(double)*Data->NPars);
	DrawIndependentStandardNormals(Perturb, Data->NPars);
	
	bool Accepted = true;
	double PrevLL = Data->LLValue(Walker, Step-1);
	
	for(int Par = 0; Par < Data->NPars; ++Par)
	{
		double Xk  = (*Data)(Walker, Par, Step-1);
		double P = B*Perturb[Par]*Scale[Par];   // Scale[Par] is a scaling relative to the par |max - min|
		(*Data)(Walker, Par, Step) = Xk + P;
	}
	
	double LL = LogLikelyhood(LLFunState, Walker, Step);
	double R = Randomf();
	double Q = LL - PrevLL;
	if(Q < std::log(R))  // Reject
	{
		for(int Par = 0; Par < Data->NPars; ++Par)
			(*Data)(Walker, Par, Step) = (*Data)(Walker, Par, Step-1);
			
		LL = PrevLL;
		Accepted = false;
	}
	Data->LLValue(Walker, Step) = LL;
	
	free(Perturb);
	return Accepted;
}



#define MCMC_MULTITHREAD 1

bool RunMCMC(mcmc_sampler_method Method, double *SamplerParams, double *Scale, double (*LogLikelyhood)(void *, int, int), void *LLFunState, mcmc_data *Data, bool (*Callback)(void *, int), void *CallbackState, int CallbackInterval, int InitialStep)
{
	SamplerMove Move;
	switch(Method)
	{
		case MCMCMethod_AffineStretch :
			Move = AffineStretchMove;
			break;
		case MCMCMethod_AffineWalk :
			Move = AffineWalkMove;
			break;
		case MCMCMethod_DifferentialEvolution :
			Move = DifferentialEvolutionMove;
			break;
		case MCMCMethod_MetropolisHastings :
			Move = MetropolisMove;
			break;
		default:
			assert(!"MCMC method not implemented!");
	}
	
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
			Workers[Walker].Do([=]()->int {return (int)Move(SamplerParams, Scale, Step, Walker, FirstEnsembleWalker, EnsembleStep, NEns2, Data, LogLikelyhood, LLFunState);});
		
		for(int Walker = 0; Walker < NEns1; ++Walker)
			Data->NAccepted += Workers[Walker].Get();
			
#else
		for(int Walker = 0; Walker < NEns1; ++Walker)
			Data->NAccepted += (int)Move(SamplerParams, Scale, Step, Walker, FirstEnsembleWalker, EnsembleStep, NEns2, Data, LogLikelyhood, LLFunState);
#endif
		
		
		//Second complementary ensemble is ensemble1 from this step
		FirstEnsembleWalker = 0;
		EnsembleStep        = Step;

#if MCMC_MULTITHREAD
		for(int Walker = NEns1; Walker < Data->NWalkers; ++Walker)
			Workers[Walker-NEns1].Do([=]()->int {return (int)Move(SamplerParams, Scale, Step, Walker, FirstEnsembleWalker, EnsembleStep, NEns1, Data, LogLikelyhood, LLFunState); });
			
		for(int Walker = NEns1; Walker < Data->NWalkers; ++Walker)
			Data->NAccepted += Workers[Walker-NEns1].Get();
	
#else
		for(int Walker = NEns1; Walker < Data->NWalkers; ++Walker)
			Data->NAccepted += (int)Move(SamplerParams, Scale, Step, Walker, FirstEnsembleWalker, EnsembleStep, NEns1, Data, LogLikelyhood, LLFunState);
#endif
		
		bool Halt = false;
		if(((Step-InitialStep) % CallbackInterval == 0) || (Step==Data->NSteps-1)) Halt = !Callback(CallbackState, Step);
		
		if(Halt) return false;
	}
	
	return true;
}