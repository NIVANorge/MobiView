
DLL_FUNCTION(int,          EncounteredError,                 char *Errmsgout, uint64_t ErrBufLen)
DLL_FUNCTION(int,          EncounteredWarning,               char *Warnmsgout, uint64_t WarnBufLen)

DLL_FUNCTION(void *,       SetupModel,                       const char *ParFile, const char *InputFile)
DLL_FUNCTION(void *,       SetupModelBlankIndexSets,         const char *InputFile)
DLL_FUNCTION(void,         ReadInputs,                       void *DataSetPtr, const char *InputFile)
DLL_FUNCTION(void,         SetIndexes,                       void *DataSetPtr, const char *IndexSetName, uint64_t IndexCount, char **IndexNames)
DLL_FUNCTION(void,         SetBranchIndexes,                 void *DataSetPtr, const char *IndexSetName, uint64_t IndexCount, dll_branch_index *Indexes)
DLL_FUNCTION(const char *, GetModelName,                     void *DataSetPtr)
DLL_FUNCTION(void,         RunModel,                         void *DataSetPtr)
DLL_FUNCTION(void *,       CopyDataSet,                      void *DataSetPtr, bool CopyResults)
DLL_FUNCTION(void,         DeleteDataSet,                    void *DataSetPtr)
DLL_FUNCTION(void,         DeleteModelAndDataSet,            void *DataSetPtr)
DLL_FUNCTION(void,         CopyData,                         void *SourceDataSetPtr, void *TargetDataSetPtr, bool CopyParams, bool CopyInputs, bool CopyResults)


DLL_FUNCTION(uint64_t,     GetTimesteps,                     void *DataSetPtr)
DLL_FUNCTION(uint64_t,     GetNextTimesteps,                 void *DataSetPtr)
DLL_FUNCTION(void,         GetStartDate,                     void *DataSetPtr, char *DateOut)
DLL_FUNCTION(void,         GetNextStartDate,                 void *DataSetPtr, char *DateOut)
DLL_FUNCTION(uint64_t,     GetInputTimesteps,                void *DataSetPtr)
DLL_FUNCTION(void,         GetInputStartDate,                void *DataSetPtr, char *DateOut)
DLL_FUNCTION(void,         GetResultSeries,                  void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount, double *WriteTo)
DLL_FUNCTION(void,         GetInputSeries,                   void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount, double *WriteTo, bool AlignWithResults)
DLL_FUNCTION(void,         SetParameterDouble,               void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount, double Value)
DLL_FUNCTION(void,         SetParameterUInt,                 void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount, uint64_t Value)
DLL_FUNCTION(void,         SetParameterBool,                 void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount, bool Value)
DLL_FUNCTION(void,         SetParameterTime,                 void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount, const char *Value)
DLL_FUNCTION(void,         SetParameterEnum,                 void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount, const char *Value)
DLL_FUNCTION(double,       GetParameterDouble,               void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount)
DLL_FUNCTION(uint64_t,     GetParameterUInt,                 void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount)
DLL_FUNCTION(bool,         GetParameterBool,                 void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount)
DLL_FUNCTION(void,         GetParameterTime,                 void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount, char *WriteTo)
DLL_FUNCTION(const char *, GetParameterEnum,                 void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount)

DLL_FUNCTION(uint64_t,     GetEnumValuesCount,               void *DataSetPtr, const char *Name)
DLL_FUNCTION(void,         GetEnumValues,                    void *DataSetPtr, const char *Name, char **NamesOut)
DLL_FUNCTION(void,         GetParameterDoubleMinMax,         void *DataSetPtr, const char *Name, double *MinOut, double *MaxOut)
DLL_FUNCTION(void,         GetParameterUIntMinMax,           void *DataSetPtr, const char *Name, uint64_t *MinOut, uint64_t *MaxOut)
DLL_FUNCTION(const char *, GetParameterDescription,          void *DataSetPtr, const char *Name)
DLL_FUNCTION(const char *, GetParameterUnit,                 void *DataSetPtr, const char *Name)
DLL_FUNCTION(const char *, GetResultUnit,                    void *DataSetPtr, const char *Name)
DLL_FUNCTION(const char *, GetInputUnit,                     void *DataSetPtr, const char *Name)
DLL_FUNCTION(const char *, GetModuleDescription,             void *DataSetPtr, const char *ModuleName)
DLL_FUNCTION(bool,         InputWasProvided,                 void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount)
DLL_FUNCTION(bool,         ResultWasComputed,                void *DataSetPtr, const char *Name, char **IndexNames, uint64_t IndexCount)
DLL_FUNCTION(bool,         IsParameterGroupName,             void *DataSetPtr, const char *Name)
DLL_FUNCTION(timestep_size, GetTimestepSize,                 void *DataSetPtr)
DLL_FUNCTION(const char *, GetParameterShortName,            void *DataSetPtr, const char *Name)

DLL_FUNCTION(void,         WriteParametersToFile,            void *DataSetPtr, const char *Filename)
DLL_FUNCTION(void,         PrintResultStructure,             void *DataSetPtr, char *Buf, uint64_t BufLen)

DLL_FUNCTION(uint64_t,     GetIndexSetsCount,                void *DataSetPtr)
DLL_FUNCTION(void,         GetIndexSets,                     void *DataSetPtr, char **NamesOut, char **TypesOut)
DLL_FUNCTION(uint64_t,     GetIndexCount,                    void *DataSetPtr, const char *IndexSetName)
DLL_FUNCTION(void,         GetIndexes,                       void *DataSetPtr, const char *IndexSetName, char **NamesOut)
DLL_FUNCTION(uint64_t,     GetParameterGroupIndexSetsCount,  void *DataSetPtr, const char *ParameterGroupName)
DLL_FUNCTION(void,         GetParameterGroupIndexSets,       void *DataSetPtr, const char *ParameterGroupName, char **NamesOut)
DLL_FUNCTION(uint64_t,     GetResultIndexSetsCount,          void *DataSetPtr, const char *ResultName)
DLL_FUNCTION(void,         GetResultIndexSets,               void *DataSetPtr, const char *ResultName, char **NamesOut)
DLL_FUNCTION(uint64_t,     GetInputIndexSetsCount,           void *DataSetPtr, const char *InputName)
DLL_FUNCTION(void,         GetInputIndexSets,                void *DataSetPtr, const char *InputName, char **NamesOut)
DLL_FUNCTION(uint64_t,     GetAllParameterGroupsCount,       void *DataSetPtr, const char *ModuleName)
DLL_FUNCTION(void,         GetAllParameterGroups,            void *DataSetPtr, char **NamesOut, const char *ModuleName)
DLL_FUNCTION(uint64_t,     GetAllModulesCount,               void *DataSetPtr)
DLL_FUNCTION(void,         GetAllModules,                    void *DataSetPtr, char **NamesOut, char **VersionsOut)
DLL_FUNCTION(uint64_t,     GetAllParametersCount,            void *DataSetPtr, const char *GroupName)
DLL_FUNCTION(void,         GetAllParameters,                 void *DataSetPtr, char **NamesOut, char **TypesOut, const char *GroupName)
DLL_FUNCTION(uint64_t,     GetAllInputsCount,                void *DataSetPtr)
DLL_FUNCTION(void,         GetAllInputs,                     void *DataSetPtr, char **NamesOut, char **TypesOut)
DLL_FUNCTION(uint64_t,     GetAllResultsCount,               void *DataSetPtr, const char *ModuleName)
DLL_FUNCTION(void,         GetAllResults,                    void *DataSetPtr, char **NamesOut, char **TypesOut, const char *ModuleName)
DLL_FUNCTION(uint64_t,     GetBranchInputsCount,             void *DataSetPtr, const char *IndexSetName, const char *IndexName)
DLL_FUNCTION(void,         GetBranchInputs,                  void *DataSetPtr, const char *IndexSetName, const char *IndexName, char **BranchInputsOut)

