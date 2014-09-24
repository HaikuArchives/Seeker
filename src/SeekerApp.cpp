#include "SeekerWindow.h"
#include "SeekerApp.h"
#include "AboutWindow.h"
#include <String.h>

Preferences *prefs;

int main()
{
	SeekerApp* app=new SeekerApp();
	app->Run();

	delete app;	
	return(0);
}

SeekerApp::SeekerApp():BApplication(Seeker_APP_SIG)
{
	prefs=new Preferences("/boot/home/config/settings/SeekerPrefs");
	prefs->Load();

	BRect rect;
	if(prefs->FindRect("mainframe",&rect)!=B_OK)
	{
		rect.Set(50, 50, 600, 400);
		prefs->AddRect("mainframe",rect);
	}
	
	iMainWindow=new SeekerWindow(this,rect);
}

SeekerApp::~SeekerApp()
{
	prefs->Save();
	delete prefs;
}

void SeekerApp::ReadyToRun()
{
	iMainWindow->Show();
}

void SeekerApp::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_REFS_RECEIVED:
			iMainWindow->PostMessage(msg);
			break;
		case MSG_RENAME_WINDOW_CANCEL_ALL:
		{
			BString title;
			BWindow *win;
			for(int32 i=0;i<CountWindows();i++)
			{
				win=WindowAt(i);
				if(!win)
					continue;
				title=win->Title();
				if(title=="Rename")
					win->PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		case MSG_INFO_WINDOW_CLOSE_ALL:
		{
			BString title;
			BWindow *win;
			for(int32 i=0;i<CountWindows();i++)
			{
				win=WindowAt(i);
				if(!win)
					continue;
				title=win->Title();
				if(title.FindFirst("Info: ")==B_OK)
					win->PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		default:
			BApplication::MessageReceived(msg);
	}
}

void SeekerApp::AboutRequested(void)
{
	// Check to see if we have one open
	BString title;
	BWindow *win;
	for(int32 i=0;i<CountWindows();i++)
	{
		win=WindowAt(i);
		if(!win)
			continue;
		title=win->Title();
		if(title.FindFirst("About")==B_OK)
			return;
	}
	
	win=new AboutWindow();
	win->Show();
}
