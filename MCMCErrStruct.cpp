#include "MCMCErrStruct.h"

#include <assert.h>



inline double LogPDFNormal(double X, double Mu, double SigmaSquared)
{
	static const double Log2PI = std::log(2.0*M_PI);
	double Factor = (X - Mu);
	return -0.5*std::log(SigmaSquared) - Log2PI - 0.5*Factor*Factor/SigmaSquared;
}

double ComputeLLValue(double *Obs, double *Sim, size_t Timesteps, const std::vector<double> &ErrParam, mcmc_error_structure ErrStruct)
{
	double Result = 0.0;
	
	double PrevEta = std::numeric_limits<double>::infinity();
	
	for(int Idx = 0; Idx < Timesteps; ++Idx)
	{
		if(!std::isfinite(Sim[Idx]))
			return -std::numeric_limits<double>::infinity();
		
		if(!std::isfinite(Obs[Idx]))
			continue;
		
		if(ErrStruct == MCMCError_Normal)
		{
			double StdDev = ErrParam[0];
			Result += LogPDFNormal(Obs[Idx], Sim[Idx], StdDev*StdDev);
		}
		else if(ErrStruct == MCMCError_NormalHet1)
		{
			double StdDev = ErrParam[0] * Sim[Idx];
			Result += LogPDFNormal(Obs[Idx], Sim[Idx], StdDev*StdDev);
		}
		else if(ErrStruct == MCMCError_WLS)
		{
			double StdDev = ErrParam[0] + ErrParam[1]*Sim[Idx];
			Result += LogPDFNormal(Obs[Idx], Sim[Idx], StdDev*StdDev);
		}
		else if(ErrStruct == MCMCError_WLSAR1)
		{
			double StdDev = ErrParam[0] + ErrParam[1]*Sim[Idx];
			double Eta = (Obs[Idx] - Sim[Idx]) / StdDev;
			if(!std::isfinite(PrevEta))
				Result += -std::log(StdDev) + LogPDFNormal(Eta, 0.0, 1.0 - ErrParam[2]*ErrParam[2]);
			else
			{
				double Y    = Eta - PrevEta*ErrParam[2];
				Result += -std::log(StdDev) + LogPDFNormal(Y, 0.0, 1.0);
			}
			PrevEta = Eta;
		}
		else
			assert(!"Error structure not supported");
	}
	return Result;
}

void AddRandomError(double* Series, size_t Timesteps, const std::vector<double> &ErrParam, mcmc_error_structure ErrStruct, std::mt19937_64 &Generator)
{
	double PrevEta = std::numeric_limits<double>::infinity();
	
	for(int Ts = 0; Ts < Timesteps; ++Ts)
	{
		if(ErrStruct == MCMCError_Normal)
		{
			double StdDev = ErrParam[0];
			std::normal_distribution<double> Distr(Series[Ts], StdDev);
			Series[Ts] = Distr(Generator);
		}
		else if(ErrStruct == MCMCError_NormalHet1)
		{
			double StdDev = ErrParam[0]*Series[Ts];
			std::normal_distribution<double> Distr(Series[Ts], StdDev);
			Series[Ts] = Distr(Generator);
		}
		else if(ErrStruct == MCMCError_WLS)
		{
			double StdDev = ErrParam[0] + ErrParam[1]*Series[Ts];
			std::normal_distribution<double> Distr(Series[Ts], StdDev);
			Series[Ts] = Distr(Generator);
		}
		else if(ErrStruct == MCMCError_WLSAR1)
		{
			std::normal_distribution<double> Distr(0.0, 1.0);
			double Y = Distr(Generator);
			double StdDev = ErrParam[0] + ErrParam[1]*Series[Ts];
			double Eta;
			if(!std::isfinite(PrevEta))
				Eta = Y;
			else
				Eta = ErrParam[2]*PrevEta + Y;
		
			Series[Ts] += StdDev*Eta;
			PrevEta = Eta;
		}
		else assert(!"Error structure not supported");
	}
}

void ComputeStandardizedResiduals(double *Obs, double *Sim, size_t Timesteps, const std::vector<double> &ErrParam, mcmc_error_structure ErrStruct, std::vector<double> &ResidualsOut)
{
	double PrevEta = std::numeric_limits<double>::infinity();
	
	for(int Ts = 0; Ts < Timesteps; ++Ts)
	{
		if(!std::isfinite(Obs[Ts]) || !std::isfinite(Sim[Ts])) continue;
		
		double Resid;
		if(ErrStruct == MCMCError_Normal)
		{
			double StdDev = ErrParam[0];
			Resid = (Obs[Ts] - Sim[Ts])/StdDev;
		}
		else if(ErrStruct == MCMCError_NormalHet1)
		{
			double StdDev = ErrParam[0]*Sim[Ts];
			Resid = (Obs[Ts] - Sim[Ts])/StdDev;
		}
		else if(ErrStruct == MCMCError_WLS)
		{
			double StdDev = ErrParam[0] + ErrParam[1]*Sim[Ts];
			Resid = (Obs[Ts] - Sim[Ts]) / StdDev;
		}
		else if(ErrStruct == MCMCError_WLSAR1)
		{
			double StdDev = ErrParam[0] + ErrParam[1]*Sim[Ts];
			double Eta    = (Obs[Ts] - Sim[Ts]) / StdDev;
			if(!std::isfinite(PrevEta))
				Resid = Eta;
			else
				Resid = Eta - ErrParam[2]*PrevEta;   //NOTE: The standardized residual is then Y, which is supposed to be normally (independently) distributed.
			
			PrevEta = Eta;
		}
		else assert(!"Error structure not supported");
				
		ResidualsOut.push_back(Resid);
	}
}
