#include <Message.h>
#include <String.h>
#include "StatusWindow.h"

StatusWindow::StatusWindow(BRect frame, const char *title, window_type type, uint32 flags,
	uint32 workspaces)
//	uint32 workspaces=B_CURRENT_WORKSPACE)
 : BWindow(frame,title,type,flags | B_NOT_CLOSABLE,workspaces)
{
	BView *top=new BView(Bounds(),NULL,B_FOLLOW_ALL,B_WILL_DRAW);
	top->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(top);
	
	BRect r(Bounds());
	r.InsetBy(10,10);
	statusbar=new BStatusBar(r,"StatusWindow::StatusBar");
	AddChild(statusbar);
	ResizeTo(r.Width()+20,r.Height()-10);
	
	statusbar->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	statusbar->SetResizingMode(B_FOLLOW_ALL);
}

void StatusWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case SW_SET_VALUE:
		{
			float value;
			if(msg->FindFloat("value",&value)==B_OK)
				statusbar->Update(value-statusbar->CurrentValue());
			break;
		}
		case SW_ADD_VALUE:
		{
			float value;
			if(msg->FindFloat("value",&value)==B_OK)
				statusbar->Update(value);
			break;
		}
		case SW_RESET:
		{
			statusbar->Reset();
			break;
		}
		case SW_SET_MAX_VALUE:
		{
			float value;
			if(msg->FindFloat("value",&value)==B_OK)
				statusbar->SetMaxValue(value);
			break;
		}
		case SW_SET_TEXT:
		{
			BString text;
			if(msg->FindString("text",&text)==B_OK)
				statusbar->SetText(text.String());
			break;
		}
		case SW_SET_COLOR:
		{
			break;
		}
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}
