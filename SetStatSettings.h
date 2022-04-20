
// The third argument is only used for the "PositiveGood" argument to DisplayStat, and is used
// when displaying colors for if a change in the stat was good, and also for the optimizer.

SET_SETTING(Min, "min", 0)
SET_SETTING(Max, "max", 0)
SET_SETTING(Sum, "sum", 0)
SET_SETTING(Median, "median", 0)
SET_SETTING(Mean, "mean", 0)
SET_SETTING(Variance, "variance", 0)
SET_SETTING(StandardDev, "std.dev", 0)
SET_SETTING(Flashiness, "flashiness", 0)
SET_SETTING(EstBFI, "est.bfi", 0)

SET_RES_SETTING(MeanError, "Mean error (bias)", -1)
SET_RES_SETTING(MAE, "MAE", 0)
SET_RES_SETTING(RMSE, "RMSE", 0)
SET_RES_SETTING(NS, "N-S", 1)
SET_RES_SETTING(LogNS, "log N-S", 1)
SET_RES_SETTING(R2, "r2", 1)
SET_RES_SETTING(IdxAgr, "Idx. of agr.", 1)
SET_RES_SETTING(KGE, "KGE", 1)
SET_RES_SETTING(SRCC, "Spearman's RCC", 1)
