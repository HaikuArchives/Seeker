#ifndef _SPLITTERVIEW_H_
#define _SPLITTERVIEW_H_

#include <View.h>
#include <Rect.h>
#include <List.h>

#define C_LEFT		0
#define C_RIGHT		1

class CSplitterView : public BView 
{
	public:
	
		CSplitterView(BRect frame,const char *name=NULL,
			uint32 ResizingMode=B_FOLLOW_ALL,uint32 flags=B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);

		void AddChild(BView *aView, uint32 pane);
		bool RemoveChild(BView *aView);
		BRect PaneBounds(uint8 which);
		BRect PaneFrame(uint8 which);

		virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
		virtual void MouseDown(BPoint point);
		virtual void MouseUp(BPoint point);
		virtual void FrameResized(float width, float height);

		float DividerWidth() const { return iSplitLineWidth; }
		void SetDividerWidth(float dwidth);
		float SplitPoint() const { return iSplitFactor; }
		void SetSplitPoint(float spoint);
	private:

		BView	*left, *right;
		bool	iTracking;
		float	iSplitFactor;
		float	iSplitLineWidth;
		BRect	iSplitLineRect, leftframe,rightframe;
};

#endif