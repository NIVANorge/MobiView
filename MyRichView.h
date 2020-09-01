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
		ScrollEnd();     //TODO: This is not ideal, but it is better than it always scrolling to top.
	}
	double zoomlevel;
	
	void Append(const String &ToAppend) {
		String Data = GetQTF();
		Data += ToAppend;
		SetQTF(Data);
	}
	
	virtual void MouseWheel(Point p, int zdelta, dword keyflags) {
		if (keyflags == K_CTRL) {		// Zooms font
			zoomlevel += zdelta/240.;
			if (zoomlevel < 1)
				zoomlevel = 10;
			else if (zoomlevel > 9)
				zoomlevel = 1;
			RefreshLayoutDeep();
		} else				// Scrolls down
			RichTextView::MouseWheel(p, zdelta, keyflags);
	}
};

#endif
