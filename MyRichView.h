#ifndef _MobiView_MyLogBox_h_
#define _MobiView_MyLogBox_h_

class MyRichView : public RichTextView {
public:
	MyRichView() {
		zoomlevel = 7;
	}
	virtual void Layout() {
		RichTextView::Layout();
		PageWidth( int(zoomlevel*GetSize().cx) );	// Smaller the total, the bigger the text
	}
	double zoomlevel;
	
	void Append(const String &ToAppend) {
		String Data = GetQTF();
		Data += ToAppend;
		SetQTF(Data);
	}
};

#endif
