#include "MobiView.h"
#include <Painter/Painter.h>

VisualizeBranches::VisualizeBranches()
{

}

struct reach_node
{
	int Index;
	int Level;
	std::vector<int> InIndexes;
	String Name;
};


int DetermineLevels(std::vector<reach_node> &Reaches, int AtIdx, int AtLevel)
{
	Reaches[AtIdx].Level = AtLevel;
	
	int MaxLevel = AtLevel;
	for(int In : Reaches[AtIdx].InIndexes)
	{
		MaxLevel = std::max(MaxLevel, DetermineLevels(Reaches, In, AtLevel+1));
	}
	
	return MaxLevel;
}


inline void RadiusComponents(double Angle, double &RX, double &RY)
{
	//NOTE: The factor r is just to stretch it out a little more to fill out space better. No
	//"scientific backing", only visual tweaking behind it
	double a = -8.0/(M_PI*M_PI);
	double b = -a*M_PI/2.0;
	double r = (a*Angle*Angle + b*Angle)*(std::sqrt(2.0)-1.0);
	RX = std::cos(Angle)*(1.0 + r);
	RY = std::sin(Angle)*(1.0 + r);
}

void RecursiveDrawReach(std::vector<reach_node> &Reaches, std::vector<int> &SpentAtLevel, std::vector<int> &CountAtLevel, int Current, DrawPainter &P, double Width, double Height, double ToX, double ToY)
{
	reach_node &Reach = Reaches[Current];
	
	//Compute FromX, FromY
	int Level = Reach.Level;
	int PosAtLevel = SpentAtLevel[Level]++;
	
	double RadiusX = Width / (double)(CountAtLevel.size()+1);
	double RadiusY = Height / (double)(CountAtLevel.size()+1);

	double Angle = M_PI * 0.5 * (double)(PosAtLevel + 1)/(double)(CountAtLevel[Level]+1);
	
	double RX, RY;
	RadiusComponents(Angle, RX, RY);

	double FromX = Width  - RadiusX * (double)(Level + 2) * RX;
	double FromY = Height - RadiusY * (double)(Level + 2) * RY;

	// (special case if Reach.Level == 0. In that case compute ToX, ToY);
	if(Level == 0)
	{
		double RX, RY;
		RadiusComponents(Angle, RX, RY);
		ToX = Width  - RadiusX * (double)(Level + 1) * RX;
		ToY = Height - RadiusY * (double)(Level + 1) * RY;
	}
	
	
	//Shorter line at initial reaches (otherwise there are often crossings)
	if(Reach.InIndexes.empty())
	{
		FromX = 0.4*ToX + 0.6*FromX;
		FromY = 0.4*ToY + 0.6*FromY;
	}
	
	//Draw from (FromX, FromY) to (ToX, ToY);
	P.Move(FromX, FromY).Line(ToX, ToY).Stroke(2, Blue());
	
	P.Translate(ToX, ToY);
	P.Circle(0, 0, 3).Stroke(1, Blue()).Fill(Blue());
	P.Translate(-ToX, -ToY);
	
	Font Fnt(Roman(14));
	double FontX = GetTextSize(Reach.Name, Fnt).cx;
	
	double TextAngle = std::atan2(ToY - FromY, ToX - FromX);
	
	double Len = std::sqrt((ToY - FromY)*(ToY-FromY) + (ToX - FromX)*(ToX - FromX));
	
	double TextX = FromX + 0.5*(1.0 - FontX/Len) * (ToX - FromX);
	double TextY = FromY + 0.5*(1.0 - FontX/Len) * (ToY - FromY);
	
	P.Translate(TextX, TextY);
	P.Rotate(TextAngle);
	P.Text(0, 0, Reach.Name, Fnt).Fill(Blue());
	P.Rotate(-TextAngle);
	P.Translate(-TextX, -TextY);
	
	for(int In : Reach.InIndexes)
	{
		RecursiveDrawReach(Reaches, SpentAtLevel, CountAtLevel, In, P, Width, Height, FromX, FromY);
	}
}


void VisualizeBranches::Paint(Draw &W)
{
	DrawPainter P(W, GetSize());
	P.Clear(White());
	
	if(OtherParent->SelectBranchedSet.GetCount() > 0)
	{
		int Select = OtherParent->SelectBranchedSet.GetIndex();
		
		ArrayCtrl *BranchList = OtherParent->BranchList[Select];
		
		int ErrorLine = -1;
		int ErrorLine2 = -1;
		int ErrorLine3 = -1;
		
		int IndexCount = BranchList->GetCount();
		std::vector<reach_node> Reaches(IndexCount);
		std::vector<bool> IsInput(IndexCount);
		
		for(int Idx = 0; Idx < IndexCount; ++Idx)
		{
			Reaches[Idx].Index = Idx;
			//IndexNameToIdx[Indexes[Idx]] = Idx;
			Reaches[Idx].Level = -1;
			Reaches[Idx].Name = BranchList->Get(Idx, 1);
			String BranchListStr = BranchList->Get(Idx,2);
			
			//NOTE: This may be a little expensive to do inside the draw call... but seems fine
			//so far.
			for(int Idx2 = 0; Idx2 < Idx; ++Idx2)
			{
				if(Reaches[Idx2].Name == Reaches[Idx].Name)
				{
					ErrorLine2 = Idx;
					break;
				}
			}
			if(ErrorLine2 >= 0) break;
			
			bool Success = OtherParent->ParseIntList(BranchListStr, Reaches[Idx].InIndexes, Idx);
			if(!Success) ErrorLine = Idx;
			
			for(int Idx2 : Reaches[Idx].InIndexes)
			{
				if(IsInput[Idx2])
				{
					ErrorLine3 = Idx;
					break;
				}
				IsInput[Idx2] = true;
			}
			if(ErrorLine3 >= 0) break;
		}
		
		int ErrorY = 100;
		const int ErrorYOffset = 20;
		if(ErrorLine >= 0)
		{
			Font Fnt(Roman(14));
			P.Text(10, ErrorY, Format("Error at id %d.", ErrorLine), Fnt).Fill(Blue());
			ErrorY += ErrorYOffset;
			P.Text(10, ErrorY, Format("The input ids has to be a comma-separated list of numbers", ErrorLine), Fnt).Fill(Blue());
			ErrorY += ErrorYOffset;
			P.Text(10, ErrorY, Format("smaller than the line id", ErrorLine), Fnt).Fill(Blue());
			ErrorY += 2*ErrorYOffset;
		}
		if(ErrorLine2 >= 0)
		{
			Font Fnt(Roman(14));
			P.Text(10, ErrorY, Format("Error at id %d.", ErrorLine2), Fnt).Fill(Blue());
			ErrorY += ErrorYOffset;
			P.Text(10, ErrorY, Format("The name of the index is not unique", ErrorLine), Fnt).Fill(Blue());
		}
		if(ErrorLine3 >= 0)
		{
			Font Fnt(Roman(14));
			P.Text(10, ErrorY, Format("Error at id %d.", ErrorLine3), Fnt).Fill(Blue());
			ErrorY += ErrorYOffset;
			P.Text(10, ErrorY, Format("This index has an input that has already appeared as an input to an earlier index.", ErrorLine), Fnt).Fill(Blue());
			ErrorY += ErrorYOffset;
			P.Text(10, ErrorY, Format("Currently, one branch splitting into two or more in this way is not allowed.", ErrorLine), Fnt).Fill(Blue());
		}
		
		if(ErrorLine == -1 && ErrorLine2 == -1 && ErrorLine3 == -1)
		{
		
			int MaxLevel = 0;
			for(int Idx = 0; Idx < IndexCount; ++Idx)
			{
				int LookupIdx = (int)Reaches.size()-1-Idx;
				if(Reaches[LookupIdx].Level >= 0) continue;
				MaxLevel = std::max(MaxLevel, DetermineLevels(Reaches, LookupIdx, 0));
			}
			
			std::vector<int> CountAtLevel(MaxLevel+1);
			std::vector<int> SpentAtLevel(MaxLevel+1);
			
			for(reach_node &Reach : Reaches)
			{
				CountAtLevel[Reach.Level]++;
			}
			
			double Width = GetSize().cx;
			double Height = GetSize().cy;
			
			for(int Idx = 0; Idx < IndexCount; ++Idx)
			{
				if(Reaches[Idx].Level == 0)
					RecursiveDrawReach(Reaches, SpentAtLevel, CountAtLevel, Idx, P, Width, Height, 0.0, 0.0);
			}
		}
	}
	else
	{
		Font Fnt(Roman(14));
		P.Text(100, 100, "No branched index sets to display", Fnt).Fill(Blue());
	}
}