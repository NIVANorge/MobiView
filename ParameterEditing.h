#ifndef _MobiView_ParameterEditing_h_
#define _MobiView_ParameterEditing_h_


#include <Core/Core.h>
#include <vector>

#include "DllInterface.h"

//TODO: This enum should really be in a common file with the main Mobius code. For now we just
//have to keep that synced
enum parameter_type
{
	ParameterType_Double = 0,
	ParameterType_UInt,
	ParameterType_Bool,
	ParameterType_Time,
	ParameterType_Enum,
};

struct parameter_index
{
	bool Locked = false;
	std::string IndexSetName;
	std::string Name;  //May be invalid if Locked=true
};

struct indexed_parameter
{
	bool Valid = false;
	bool Virtual = false;
	std::string Name;
	parameter_type Type;
	std::vector<parameter_index> Indexes;
};



inline parameter_type
ParseParameterType(const char *Name)
{
	parameter_type Type;
	if(strcmp(Name, "double") == 0) Type = ParameterType_Double;
	else if(strcmp(Name, "uint") == 0) Type = ParameterType_UInt;
	else if(strcmp(Name, "bool") == 0) Type = ParameterType_Bool;
	else if(strcmp(Name, "time") == 0) Type = ParameterType_Time;
	else if(strcmp(Name, "enum") == 0) Type = ParameterType_Enum;
	//Error handling?
	return Type;
}

Upp::String MakeIndexString(const std::vector<char *> &Indexes);
Upp::String MakeIndexString(const std::vector<std::string> &Indexes);
Upp::String MakeParameterIndexString(const indexed_parameter &Parameter);
bool   ParameterIsSubsetOf(const indexed_parameter &Parameter, const indexed_parameter &CompareTo);
void   SetParameterValue(const indexed_parameter &Parameter, void *DataSet, Upp::Value Val, model_dll_interface &ModelDll);



#endif
