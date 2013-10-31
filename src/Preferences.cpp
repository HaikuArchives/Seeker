#include <File.h>
#include <stdio.h>
#include "Preferences.h"

//#define DEBUG_PREFS

Preferences::Preferences(const char *path)
{
	status=B_OK;
	fPath.SetTo(path);
}

status_t Preferences::Load()
{
	locker.Lock();
	BFile file;
	status=file.SetTo(fPath.String(), B_READ_ONLY);

	if(status!=B_OK)
	{
		locker.Unlock();
		return status;
	}

	status=Unflatten(&file);
	locker.Unlock();
#ifdef DEBUG_PREFS
printf("Prefs::Load"); PrintToStream();
#endif
	return status;
}

status_t Preferences::Save()
{
	locker.Lock();
#ifdef DEBUG_PREFS
printf("Prefs::Save"); PrintToStream();
#endif
	BFile file;
	status=file.SetTo(fPath.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);

	if(status!=B_OK)
	{
		locker.Unlock();
		return status;
	}

	status=Flatten(&file);
	locker.Unlock();
	return status;
}
