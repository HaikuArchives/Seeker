#ifndef _PIONEERAPP_H_
#define _PIONEERAPP_H_

#include <Application.h>

#include "SeekerWindow.h"
#include "Preferences.h"

#define Seeker_APP_SIG "application/x-vnd.wgp-Seeker"

class SeekerApp: public BApplication
{
	public:
		
		SeekerApp();
		~SeekerApp();
		virtual void ReadyToRun();
		virtual void MessageReceived(BMessage *msg);
		virtual void AboutRequested(void);
	private:
		SeekerWindow* iMainWindow;
};

extern Preferences *prefs;

#endif
