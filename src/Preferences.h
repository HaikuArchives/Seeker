#ifndef PREFERENCES_H_
#define PREFERENCES_H_

#include <Message.h>
#include <String.h>
#include <Locker.h>

class Preferences : public BMessage
{
public:
	Preferences(const char *path);
	status_t InitCheck() { return status; }
	status_t Load();
	status_t Save();
private:
	status_t status;
	BString fPath;
	BLocker locker;
};
#endif