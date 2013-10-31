#include "AboutWindow.h"
#include <Roster.h>
#include <Application.h>
#include <AppFileInfo.h>
#include <Mime.h>
#include <Font.h>
#include <File.h>
#include <stdio.h>

AboutWindow::AboutWindow(void)
 :BWindow(BRect(0,0,200,100),"About Seeker", B_TITLED_WINDOW, B_NOT_ZOOMABLE |
 	B_NOT_RESIZABLE)
{
	AddChild(new AboutView(Bounds()));
	MoveBy(300,200);
}

AboutWindow::~AboutWindow(void)
{
}

AboutView::AboutView(BRect r)
 :BView(r,"AboutWindow::AboutView",B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	logo=new BBitmap(BRect(0,0,31,31),B_CMAP8);

	BMimeType mime("application/x-vnd.wgp-Seeker");
	mime.GetIcon(logo,B_LARGE_ICON);
	
	app_info ai;
	version_info vi;
	be_app->GetAppInfo(&ai);
	BFile file(&ai.ref,B_READ_ONLY);
	BAppFileInfo appinfo(&file);
	appinfo.GetVersionInfo(&vi,B_APP_VERSION_KIND);

	BString variety;
	switch(vi.variety)
	{
		case 0:
			variety="Development";
			break;
		case 1:
			variety="Alpha";
			break;
		case 2:
			variety="Beta";
			break;
		case 3:
			variety="Gamma";
			break;
		case 4:
			variety="Golden Master";
			break;
		default:
			variety="Final";
			break;
	}

	sprintf(version,"Version %lu.%lu.%lu %s %lu",vi.major,vi.middle,vi.minor,
		variety.String(),vi.internal);

	BFont font;
	GetFont(&font);

	font.SetSize(24.0);
	namewidth=font.StringWidth("Seeker");

	font.SetSize(12.0);
	writtenwidth=font.StringWidth("Written by DarkWyrm");
	versionwidth=font.StringWidth(version);
}

AboutView::~AboutView(void)
{
	delete logo;
}

void AboutView::Draw(BRect update)
{
	float center=40+((Bounds().right-40)/2);
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(logo,BPoint(10,10));

	SetDrawingMode(B_OP_COPY);
	SetFontSize(24.0);
	DrawString("Seeker",BPoint(center-(namewidth/2),33));
	
	SetFontSize(12.0);
	DrawString(version,BPoint(center-(versionwidth/2),60));
	DrawString("Written by DarkWyrm",BPoint(center-(writtenwidth/2),75));
}
