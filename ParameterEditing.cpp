#include "ParameterEditing.h"


using namespace Upp;

String MakeIndexString(const std::vector<char *> &Indexes)
{
	String Result = "";
	if(Indexes.size() > 0)
	{
		Result << "(";
		int Idx = 0;
		for(char *Index : Indexes)
		{
			Result << "\"" << Index << "\"";
			if(Idx != Indexes.size()-1) Result << " ";
			++Idx;
		}
		Result << ")";
	}
	return Result;
}

String MakeIndexString(const std::vector<std::string> &Indexes)
{
	String Result = "";
	if(Indexes.size() > 0)
	{
		Result << "(";
		int Idx = 0;
		for(const std::string &Index : Indexes)
		{
			Result << "\"" << Index.data() << "\"";
			if(Idx != Indexes.size()-1) Result << " ";
			++Idx;
		}
		Result << ")";
	}
	return Result;
}

String MakeParameterIndexString(const indexed_parameter &Parameter)
{
	String Result = "";
	if(Parameter.Indexes.size() > 0)
	{
		Result << "(";
		int Idx = 0;
		for(const parameter_index &Index : Parameter.Indexes)
		{
			if(Index.Locked)
				Result << "<locked \"" << Index.IndexSetName.data() << "\">";               //TODO: This is a bit unclear as to what index set is locked
			else
				Result << "\"" << Index.Name.data() << "\"";
			if(Idx != Parameter.Indexes.size()-1) Result << " ";
			++Idx;
		}
		Result << ")";
	}
	return Result;
}

bool ParameterIsSubsetOf(const indexed_parameter &Parameter, const indexed_parameter &CompareTo)
{
	if(Parameter.Name == CompareTo.Name)
	{
		for(int Idx = 0; Idx < Parameter.Indexes.size(); ++Idx)
		{
			if(CompareTo.Indexes[Idx].Locked) continue;
			if(Parameter.Indexes[Idx].Locked) return false;
			if(CompareTo.Indexes[Idx].Name != Parameter.Indexes[Idx].Name) return false;
		}
		return true;
	}
	return false;
}

static void
RecursiveUpdateParameter(int Level, std::vector<std::string> &CurrentIndexes, const indexed_parameter &Parameter, void *DataSet, Value Val, model_dll_interface &ModelDll)
{
	if(IsNull(Val)) return;
	
	if(Level == Parameter.Indexes.size())
	{
		//Do the actual update.
		size_t IndexCount = CurrentIndexes.size();
		std::vector<char *> Indexes(IndexCount);
		for(size_t Idx = 0; Idx < IndexCount; ++Idx)
		{
			Indexes[Idx] = (char *)CurrentIndexes[Idx].data();
		}
		
		const std::string &Name = Parameter.Name;
		parameter_type Type = Parameter.Type;
		
		switch(Type)
		{
			case ParameterType_Double:
			{
				ModelDll.SetParameterDouble(DataSet, Name.data(), Indexes.data(), Indexes.size(), (double)Val);
			} break;
			
			case ParameterType_UInt:
			{
				ModelDll.SetParameterUInt(DataSet, Name.data(), Indexes.data(), Indexes.size(), (uint64)(int64)Val);
			} break;
			
			case ParameterType_Bool:
			{
				ModelDll.SetParameterBool(DataSet, Name.data(), Indexes.data(), Indexes.size(), (bool)Val);
			} break;
			
			case ParameterType_Time:
			{
				Time D = (Time)Val;
				
				std::string V = Format(D, true).ToStd();
				if(V.size() > 0)    // Seems like D.IsValid() and !IsNull(D)  don't work correctly, so we do this instead.
					ModelDll.SetParameterTime(DataSet, Name.data(), Indexes.data(), Indexes.size(), V.data());
			} break;
			
			case ParameterType_Enum:
			{
				std::string V2 = Val.ToString().ToStd();
				ModelDll.SetParameterEnum(DataSet, Name.data(), Indexes.data(), Indexes.size(), V2.data());
			} break;
		}
		//CheckDllUserError();
	}
	else
	{
		const char *IndexSetName = Parameter.Indexes[Level].IndexSetName.data();
		
		if(Parameter.Indexes[Level].Locked)
		{
			size_t IndexCount = ModelDll.GetIndexCount(DataSet, IndexSetName);
			std::vector<char *> IndexNames(IndexCount);
			ModelDll.GetIndexes(DataSet, IndexSetName, IndexNames.data());
			for(size_t Idx = 0; Idx < IndexCount; ++Idx)
			{
				CurrentIndexes[Level] = IndexNames[Idx];
				RecursiveUpdateParameter(Level + 1, CurrentIndexes, Parameter, DataSet, Val, ModelDll);
			}
		}
		else
		{
			CurrentIndexes[Level] = Parameter.Indexes[Level].Name;
			
			RecursiveUpdateParameter(Level + 1, CurrentIndexes, Parameter, DataSet, Val, ModelDll);
		}
	}
}


void SetParameterValue(const indexed_parameter &Parameter, void *DataSet, Value Val, model_dll_interface &ModelDll)
{
	std::vector<std::string> CurrentIndexes(Parameter.Indexes.size());
	
	RecursiveUpdateParameter(0, CurrentIndexes, Parameter, DataSet, Val, ModelDll);
}

