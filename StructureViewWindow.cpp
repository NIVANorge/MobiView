#include "MobiView.h"



StructureViewWindow::StructureViewWindow(MobiView *ParentWindow)
{
	CtrlLayout(*this, "Model equation batch structure");
	
	Sizeable().Zoomable();
	
	StructureViewBox.SetColor(TextCtrl::PAPER_READONLY, StructureViewBox.GetColor(TextCtrl::PAPER_NORMAL));
	
	uint64 BufLen = 8192;      //TODO: It is not that good that we have to set a fixed buffer length here..
	char *Buf = new char[BufLen];
	
	ParentWindow->ModelDll.PrintResultStructure(ParentWindow->DataSet, Buf, BufLen);
	
	StructureViewBox.Set(String(Buf));
	
	WhenClose << [ParentWindow](){ delete ParentWindow->StructureView; ParentWindow->StructureView = nullptr; }; //TODO: Is this always a safe way of doing it?? No, apparently not!!
	
	delete[] Buf;
	
	Open();
}