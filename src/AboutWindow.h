#ifndef ABOUTWINDOW_H_
#define ABOUTWINDOW_H_

#include <Window.h>
#include <View.h>
#include <Bitmap.h>
#include <String.h>

class AboutView : public BView
{
public:
	AboutView(BRect r);
	~AboutView(void);
	void Draw(BRect update);
	BBitmap *logo;
	char version[64];
	float versionwidth, writtenwidth, namewidth;
};

class AboutWindow : public BWindow
{
public:
	AboutWindow(void);
	~AboutWindow(void);
};

#endif