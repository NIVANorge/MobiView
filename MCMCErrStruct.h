#ifndef _MobiView_MCMCErrStruct_h_
#define _MobiView_MCMCErrStruct_h_

#include <vector>
#include <random>

#define SET_LL_SETTING(Handle, Name, NumErr) MCMCError_##Handle,
enum mcmc_error_structure
{
	MCMCError_Unknown = -1,
	MCMCError_Offset  = 200,
	#include "LLSettings.h"
	MCMCError_End     = 299,
};
#undef SET_LL_SETTING

void AddRandomError(double* Series, size_t Timesteps, const std::vector<double> &ErrParam, mcmc_error_structure ErrStruct, std::mt19937_64 &Generator);
void ComputeStandardizedResiduals(double *Obs, double *Sim, size_t Timesteps, const std::vector<double> &ErrParam, mcmc_error_structure ErrStruct, std::vector<double> &ResidualsOut);
double ComputeLLValue(double *Obs, double *Sim, size_t Timesteps, const std::vector<double> &ErrParam, mcmc_error_structure ErrStruct);

#endif
