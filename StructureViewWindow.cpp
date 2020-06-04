#include "MobiView.h"



StructureViewWindow::StructureViewWindow()
{
	CtrlLayout(*this, "Model equation batch structure");
	
	Sizeable().Zoomable();
	
	StructureViewBox.SetColor(TextCtrl::PAPER_READONLY, StructureViewBox.GetColor(TextCtrl::PAPER_NORMAL));

}

void StructureViewWindow::RefreshText()
{
	StructureViewBox.Clear();
	
	uint64 BufLen = 8192;      //TODO: It is not that good that we have to set a fixed buffer length here..
	char *Buf = new char[BufLen];
	Buf[BufLen-1] = 0;
	
	ParentWindow->ModelDll.PrintResultStructure(ParentWindow->DataSet, Buf, BufLen-1);
	StructureViewBox.Set(String(Buf));

	delete[] Buf;
}