#ifndef _MobiView_ParameterEditing_h_
#define _MobiView_ParameterEditing_h_




void MobiView::RefreshParameterView()
{
	ParameterView.Clear();
	
	//if(ParameterGroupSelecter.GetSelectCount() <= 0) return;   //Why doesn't this work?
	
	Value SelectedGroup = ParameterGroupSelecter.Get(0);
	std::string SelectedGroupName = SelectedGroup.ToString().ToStd();
	
	uint64 IndexSetCount = ModelDll.GetParameterGroupIndexSetsCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> IndexSetNames(IndexSetCount);
	ModelDll.GetParameterGroupIndexSets(DataSet, SelectedGroupName.data(), IndexSetNames.data());
	
	for(size_t Idx = 0; Idx < MAX_INDEX_SETS; ++Idx)
	{
		IndexList[Idx]->Disable();
	}
	
	std::vector<std::string> Indexes_String(IndexSetCount);
	std::vector<char *> Indexes(IndexSetCount);
	
	for(size_t Idx = 0; Idx < IndexSetCount; ++Idx)
	{
		size_t Id = IndexSetNameToId[IndexSetNames[Idx]];
		IndexList[Id]->Enable();
		
		//Indexes_String[Idx] = IndexList[Id]->GetList().Get(0).ToString().ToStd();
		Indexes_String[Idx] = IndexList[Id]->Get().ToString().ToStd();
		Indexes[IndexSetCount - Idx - 1] = (char *)Indexes_String[Idx].data(); //NOTE: Have to reverse since GetParameterGroupIndexSets returns in reverse order. May want to fix that!
	}
	
	uint64 ParameterCount = ModelDll.GetAllParametersCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> ParameterNames(ParameterCount);
	std::vector<char *> ParameterTypes(ParameterCount);
	ModelDll.GetAllParameters(DataSet, ParameterNames.data(), ParameterTypes.data(), SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	ParameterControls.Clear();
	CurrentParameterTypes.clear();
	for(size_t Idx = 0; Idx < ParameterCount; ++Idx)
	{
		const char *Name = ParameterNames[Idx];
		const char *Type = ParameterTypes[Idx];
		
		Value ParVal;
		Value ParMin;
		Value ParMax;
		Value ParUnit;
		Value ParDesc;
		const char *Unit = ModelDll.GetParameterUnit(DataSet, Name);
		if(Unit) ParUnit = Unit;
		const char *Description = ModelDll.GetParameterDescription(DataSet, Name);
		if(Description) ParDesc = Description;
		
		if(strcmp(Type, "double") == 0)
		{
			ParVal = ModelDll.GetParameterDouble(DataSet, Name, Indexes.data(), IndexSetCount);
			double Min, Max;
			ModelDll.GetParameterDoubleMinMax(DataSet, Name, &Min, &Max);
			ParMin = Min;
			ParMax = Max;
			
			ParameterControls.Create<EditDoubleNotNull>();
			CurrentParameterTypes.push_back(ParameterType_Double);
			
			if (CheckDllUserError()) return;
		}
		else if(strcmp(Type, "uint") == 0)
		{
			//TODO: Converting to int potentially loses precision. However Value has no uint64
			//subtype
			ParVal = (int64)ModelDll.GetParameterUInt(DataSet, Name, Indexes.data(), IndexSetCount);
			uint64 Min, Max;
			ModelDll.GetParameterUIntMinMax(DataSet, Name, &Min, &Max);
			int64 M = (int64)Max;
			if(M < 0) M = INT64_MAX; //Stupid stopgap. We should do this better and actually display uint64's.
			ParMin = (int64)Min;
			ParMax = M;
			
			ParameterControls.Create<EditInt64NotNull>();
			CurrentParameterTypes.push_back(ParameterType_UInt);
			
			if (CheckDllUserError()) return;
		}
		else if(strcmp(Type, "bool") == 0)
		{
			ParVal = ModelDll.GetParameterBool(DataSet, Name, Indexes.data(), IndexSetCount);
			if(CheckDllUserError()) return;
			
			ParameterControls.Create<Option>();
			CurrentParameterTypes.push_back(ParameterType_Bool);
		}
		else if(strcmp(Type, "time") == 0)
		{
			char TimeVal[256];
			ModelDll.GetParameterTime(DataSet, Name, Indexes.data(), IndexSetCount, TimeVal);
			Date D;
			StrToDate(D, TimeVal); //Error handling? But should not be necessary.
			ParVal = D;
			
			ParameterControls.Create<EditDateNotNull>();
			CurrentParameterTypes.push_back(ParameterType_Time);
		}
		ParameterView.Add(String(Name), ParVal, ParMin, ParMax, ParUnit, ParDesc);
		ParameterView.SetCtrl((int)Idx, 1, ParameterControls.Top());
		ParameterControls.Top().WhenAction = [=]() { ParameterEditAccepted((int)Idx); };
	}
}


void MobiView::RecursiveUpdateParameter(std::vector<char *> &IndexSetNames, int Level, std::vector<std::string> &CurrentIndexes, int Row)
{
	if(Level == IndexSetNames.size())
	{
		//Do the actual update.
		size_t IndexCount = CurrentIndexes.size();
		std::vector<char *> Indexes(IndexCount);
		for(size_t Idx = 0; Idx < IndexCount; ++Idx)
		{
			Indexes[IndexCount - Idx - 1] = (char *)CurrentIndexes[Idx].data(); //NOTE: Have to do the reversing since the GetParameterGroupIndexSets returns them in "reverse order". We may want to fix that..
		}
		
		std::string Name = ParameterView.Get(Row, 0).ToString().ToStd();
		parameter_type Type = CurrentParameterTypes[Row];
		
		switch(Type)
		{
			case ParameterType_Double:
			{
				double V = ParameterView.Get(1);
				ModelDll.SetParameterDouble(DataSet, Name.data(), Indexes.data(), Indexes.size(), V);
			} break;
			
			case ParameterType_UInt:
			{
				int64 V = ParameterView.Get(1);
				ModelDll.SetParameterUInt(DataSet, Name.data(), Indexes.data(), Indexes.size(), (uint64)V);
			} break;
			
			case ParameterType_Bool:
			{
				Ctrl *ctrl = ParameterView.GetCtrl(Row, 1);
				bool V = (bool)((Option*)ctrl)->Get();
				ModelDll.SetParameterBool(DataSet, Name.data(), Indexes.data(), Indexes.size(), V);
			} break;
			
			case ParameterType_Time:
			{
				Ctrl *ctrl = ParameterView.GetCtrl(Row, 1);
				Date D = ((EditDateNotNull*)ctrl)->GetData();
				std::string V = Format(D).ToStd();
				ModelDll.SetParameterTime(DataSet, Name.data(), Indexes.data(), Indexes.size(), V.data());
			} break;
		}
		CheckDllUserError();
	}
	else
	{
		const char *IndexSetName = IndexSetNames[Level];
		size_t Id = IndexSetNameToId[IndexSetName];
		
		if(IndexLock[Id]->Get())
		{
			size_t IndexCount = ModelDll.GetIndexCount(DataSet, IndexSetName);
			std::vector<char *> IndexNames(IndexCount);
			ModelDll.GetIndexes(DataSet, IndexSetName, IndexNames.data());
			for(size_t Idx = 0; Idx < IndexCount; ++Idx)
			{
				CurrentIndexes[Level] = IndexNames[Idx];
				RecursiveUpdateParameter(IndexSetNames, Level + 1, CurrentIndexes, Row);
			}
		}
		else
		{
			CurrentIndexes[Level] = IndexList[Id]->Get().ToString().ToStd();
			RecursiveUpdateParameter(IndexSetNames, Level + 1, CurrentIndexes, Row);
		}
	}
}


void MobiView::ParameterEditAccepted(int Row)
{
	//TODO: High degree of copypaste from above. Factor this out.
	Value SelectedGroup = ParameterGroupSelecter.Get(0);
	std::string SelectedGroupName = SelectedGroup.ToString().ToStd();
	
	uint64 IndexSetCount = ModelDll.GetParameterGroupIndexSetsCount(DataSet, SelectedGroupName.data());
	if (CheckDllUserError()) return;
	
	std::vector<char *> IndexSetNames(IndexSetCount);
	ModelDll.GetParameterGroupIndexSets(DataSet, SelectedGroupName.data(), IndexSetNames.data());
	
	std::vector<std::string> CurrentIndexes(IndexSetCount);
	RecursiveUpdateParameter(IndexSetNames, 0, CurrentIndexes, Row);
}

void MobiView::SaveParameters()
{
	if(!hinstModelDll || !ModelDll.WriteParametersToFile || !CurrentParameterFile.size())
	{
		Log("Parameters can only be saved once a model and parameter file is loaded");
		return;
	}
	//TODO: Mechanism for determining if there has actually been edits that need to be saved.
	//TODO: Maybe also a "do you really want to overwrite <filename>".
	ModelDll.WriteParametersToFile(DataSet, CurrentParameterFile.data());
	//TODO log
	CheckDllUserError();
}

void MobiView::SaveParametersAs()
{
	if(!hinstModelDll || !ModelDll.WriteParametersToFile || !CurrentParameterFile.size())
	{
		Log("Parameters can only be saved once a model and parameter file is loaded");
		return;
	}
	
	FileSel Sel;
	Sel.Type("Parameter dat files", "*.dat");
	String ParameterFile = CurrentParameterFile.data();
	Sel.PreSelect(ParameterFile);
	Sel.ExecuteSaveAs("Save parameters as");
	
	std::string NewFile = Sel.Get().ToStd();
	if(NewFile.size())
	{
		ModelDll.WriteParametersToFile(DataSet, NewFile.data());
		CurrentParameterFile = NewFile;
		//TODO log
		CheckDllUserError();
	}
}


#endif
