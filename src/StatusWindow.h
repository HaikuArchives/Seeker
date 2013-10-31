#ifndef STATUS_WINDOW_H
#define STATUS_WINDOW_H

#include <Window.h>
#include <StatusBar.h>

enum
{
	SW_SET_VALUE=100,
	SW_ADD_VALUE,
	SW_RESET,
	SW_SET_MAX_VALUE,
	SW_SET_TEXT,
	SW_SET_COLOR
};

class StatusWindow : public BWindow
{
public:
	StatusWindow(BRect frame, const char *title, window_type type, uint32 flags,
		uint32 workspaces=B_CURRENT_WORKSPACE);
	virtual void MessageReceived(BMessage *msg);
protected:
	BStatusBar *statusbar;
};

#endif