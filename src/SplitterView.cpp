#include <stdlib.h>
#include <ScrollBar.h>

#include "Colors.h"
#include "SplitterView.h"
#include <stdio.h>

CSplitterView::CSplitterView(BRect frame, const char *name,uint32 resizingMode, uint32 flags)
 : BView(frame, name, resizingMode, flags),
	left(NULL),
	right(NULL),
	iTracking(false),
	iSplitFactor(0.50),
	iSplitLineWidth(4.0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	// set color of split bar
	rgb_color splitColor;
	splitColor.red=203; splitColor.blue=203; splitColor.green=203; splitColor.alpha=255;
	SetHighColor(splitColor);
	
	leftframe=Bounds();
	rightframe=Bounds();

	leftframe.right/=2;
	leftframe.right-=iSplitLineWidth/2;
	iSplitLineRect.Set(leftframe.right+1,0,leftframe.right+iSplitLineWidth,Bounds().bottom);
	rightframe.left=iSplitLineRect.right+1;
}	

void CSplitterView::AddChild(BView *aView, uint32 pane)
{
	if(pane!=C_LEFT && pane!=C_RIGHT)
		return;

	if ( (left && pane==C_LEFT) || (right && pane==C_RIGHT) )
		debugger("You cannot add a view to a pane which already owns a view");

	if(pane==C_LEFT)
	{
		left=aView;
		aView->ResizeTo(leftframe.right,leftframe.bottom);
		left->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM );
	}
	else
	{
		right=aView;
		aView->MoveTo(rightframe.left,0);
		right->SetResizingMode(B_FOLLOW_ALL);
	}

	BView::AddChild(aView);
}

bool CSplitterView::RemoveChild(BView *aView)
{
	if(aView==right)
		right=NULL;
	else
	if(aView==left)
		left=NULL;
	else
		return false;
	BView::RemoveChild(aView);
	
	return true;
}

BRect CSplitterView::PaneBounds(uint8 which)
{
	if(which==C_RIGHT)
		return rightframe.OffsetToCopy(0,0);
	return leftframe.OffsetToCopy(0,0);
}

BRect CSplitterView::PaneFrame(uint8 which)
{
	if(which==C_RIGHT)
		return rightframe;
	return leftframe;
}

void CSplitterView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if(iTracking)
	{
		if(point.x<1)
			point.x=1;
		else
		if(point.x>Bounds().right-iSplitLineWidth)
			point.x=Bounds().right-iSplitLineWidth;
		
		float delta=iSplitLineRect.left-point.x;
		
		// recalculate split factor
		iSplitLineRect.OffsetTo(point.x,0);
		iSplitFactor=iSplitLineRect.left/Bounds().right;
		
		// recalculate bounds of children
		leftframe.right=point.x-1;
		rightframe.left=iSplitLineRect.right+1;
		
		if(right)
		{
			right->MoveTo(rightframe.left,0);
//			right->ResizeTo(rightframe.right,rightframe.bottom);

			// This *should* fix the resizing bug which caused us to lose our scrollbar
			// in the FileView when moving the divider
			right->ResizeBy(delta,0);
		}
		if(left)
			left->ResizeTo(leftframe.right,leftframe.bottom);

		Draw(iSplitLineRect);
	}
}

void CSplitterView::MouseDown(BPoint point)
{
	if(iTracking)
		iTracking=false;
	
	if(iSplitLineRect.Contains(point))
	{
		iTracking=true;
//		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}
}

void CSplitterView::MouseUp(BPoint point)
{
	if(iTracking)
		iTracking=false;
}

void CSplitterView::FrameResized(float width, float height)
{
	if(left)
		leftframe=left->Bounds();
	if(right)
		rightframe=right->Frame();
}

void CSplitterView::SetDividerWidth(float dwidth)
{
	// set the width of the divider line

	if(dwidth<4 || dwidth>Bounds().Width()/2)
		return;
	
	iSplitLineRect.right=iSplitLineRect.left+dwidth;
	rightframe.left=iSplitLineRect.right+1;
	
	if(right)
	{
		right->MoveTo(rightframe.left,0);
		right->ResizeTo(rightframe.right,rightframe.bottom);
	}
	iSplitLineWidth=dwidth;

	Draw(iSplitLineRect);
}

void CSplitterView::SetSplitPoint(float spoint)
{
	// Takes a percentage value >0 and <1 which determines how much
	// room the left and right panes take up

	if(spoint<=0 || spoint >=1.0)
		return;
	
	// recalculate split factor
	leftframe.right=Bounds().right * spoint;
	leftframe.right-=iSplitLineWidth/2;
	iSplitLineRect.Set(leftframe.right+1,0,leftframe.right+1+iSplitLineWidth,Bounds().bottom);
	rightframe.left=iSplitLineRect.right+1;

	iSplitFactor=spoint;

	if(right)
	{
		right->MoveTo(rightframe.left,0);
		right->ResizeTo(rightframe.right,rightframe.bottom);
	}
	if(left)
		left->ResizeTo(leftframe.right,leftframe.bottom);

	Draw(iSplitLineRect);
}
