#ifndef _MobiView_Acor_h_
#define _MobiView_Acor_h_

void NormalizedAutocorrelation1D(const std::vector<double> &Chain, std::vector<double> &Acor);
double IntegratedTime(mcmc_data *Data, int Par, int C, int Tol, bool *BelowTolerance);

#endif
