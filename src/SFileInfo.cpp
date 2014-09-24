#include <stdio.h>

#include <VolumeRoster.h>
#include <Directory.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Alert.h>
#include <Button.h>

#include "SeekerApp.h"
#include "Preferences.h"
#include "Colors.h"

#include "SeekerWindow.h"
#include "SFileView.h"

#include "SFileInfo.h"

#define CAPACITY_STRING "Capacity"
#define SIZE_STRING "Size"
#define CREATED_STRING "Created"
#define MODIFIED_STRING "Modified"
#define KIND_STRING "Kind"
#define PATH_STRING "Path"

SFileInfoView::SFileInfoView(SFileInfoWindow *infoWindow, const SFile *file,
	BRect frame, const char *name, bool multiple,uint32 resizingMode, uint32 flags)
  :BView(frame, name, resizingMode, flags),
	iInfoWindow(infoWindow),
	iFile(0),
	iTextOffset(0.0)
{
	iFile=new SFile(file);
	
	// set font height
	font_height FontAttributes;
	be_plain_font->GetHeight(&FontAttributes);
	
	iTextOffset=ceil(FontAttributes.ascent) + 
				  ceil(FontAttributes.descent) +
				  ceil(FontAttributes.leading);
	iTextOffset *=1.5;

	shadowRect.Set(0,0,60,Bounds().Height());

	float maxstringwidth=60;	// width of shadowRect

	GetFont(&font);
	maxstringwidth=MAX(maxstringwidth,font.StringWidth(CAPACITY_STRING)+20);
	maxstringwidth=MAX(maxstringwidth,font.StringWidth(SIZE_STRING)+20);
	maxstringwidth=MAX(maxstringwidth,font.StringWidth(CREATED_STRING)+20);
	maxstringwidth=MAX(maxstringwidth,font.StringWidth(MODIFIED_STRING)+20);
	maxstringwidth=MAX(maxstringwidth,font.StringWidth(KIND_STRING)+20);
	maxstringwidth=MAX(maxstringwidth,font.StringWidth(PATH_STRING)+20);
	shadowRect.right=maxstringwidth;

	
	bitmapRect.right=shadowRect.right-10;
	bitmapRect.left=bitmapRect.right-31;
	bitmapRect.top=10.0;
	bitmapRect.bottom=bitmapRect.top + 31.0;
	
	if(multiple)
	{
		BButton *closeall=new BButton(BRect(0,0,50,25),"FileInfoWindow::CloseAll",
			"Close All",new BMessage(MSG_INFO_WINDOW_CLOSE_ALL));
		closeall->ResizeToPreferred();
		closeall->MoveTo(Bounds().right-closeall->Bounds().Width()-10,
			Bounds().bottom-closeall->Bounds().Height()-10);
		AddChild(closeall);
		closeall->SetTarget(be_app);
	}
}

SFileInfoView::~SFileInfoView()
{
	if(iFile) delete iFile;
}

void SFileInfoView::Draw(BRect rect)
{
	// draw shadow
	SetDrawingMode(B_OP_COPY);
	SetHighColor(BeInactiveControlGrey);
	FillRect(shadowRect);

	// draw icon
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(iFile->LargeIcon(), BRect(0.0, 0.0, 31.0, 31.0), bitmapRect);
	SetDrawingMode(B_OP_COPY);

	// draw name
	GetFont(&font);
	font.SetSize(14.0);
	SetFont(&font);
	MovePenTo(shadowRect.right + 10.0, bitmapRect.bottom);
	SetHighColor(Black);
	DrawString(iFile->Name());
	SetFont(be_plain_font);	

	// draw information strings
	font_height fontHeight;
	char strg[400];
	float x=0.0, y=50.0;

	GetFont(&font);
	font.GetHeight(&fontHeight);
 	SetDrawingMode(B_OP_OVER);
	
	// SIZE
	
	// label
	if(iFile->IsVolume())
		strcpy(strg, CAPACITY_STRING);
	else
		strcpy(strg, SIZE_STRING);

//	x=50.0 - font.StringWidth(strg);
	x=shadowRect.right - font.StringWidth(strg) - 10;
	y +=iTextOffset; 
	MovePenTo(x, y);
	SetHighColor(Red);
	DrawString(strg);

	// data
	x=shadowRect.right + 10.0;
	MovePenTo(x, y);
	if(iFile->IsVolume())
	{
		char capacityStrg[20], usedStrg[20], freeStrg[20];
		iFile->CapacityDesc(capacityStrg);
		iFile->UsedSpaceDesc(usedStrg);
		iFile->FreeSpaceDesc(freeStrg);
		sprintf(strg, "%s (%s used -- %s free)", capacityStrg, usedStrg, freeStrg);
	}
	else
		iFile->SizeDesc(strg);

	SetHighColor(Black);
	DrawString(strg);

	// CREATED
	
	// label
	strcpy(strg, CREATED_STRING);
	x=shadowRect.right - font.StringWidth(strg) - 10;
	y +=iTextOffset; 
	MovePenTo(x, y);
	SetHighColor(Red);
	DrawString(strg);

	// data
	x=shadowRect.right + 10.0;
	MovePenTo(x, y);
	iFile->CreatedTmDesc(strg);
	SetHighColor(Black);
	DrawString(strg);

	// MODIFIED
	
	// label	
	strcpy(strg, MODIFIED_STRING);
	x=shadowRect.right - font.StringWidth(strg) - 10;
	y +=iTextOffset; 
	MovePenTo(x, y);
	SetHighColor(Red);
	DrawString(strg);

	// data
	x=shadowRect.right + 10.0;
	MovePenTo(x, y);
	iFile->ModifiedTmDesc(strg);
	SetHighColor(Black);
	DrawString(strg);

	// KIND
	
	// label	
	strcpy(strg, KIND_STRING);
	x=shadowRect.right - font.StringWidth(strg) - 10;
	y +=iTextOffset; 
	MovePenTo(x, y);
	SetHighColor(Red);
	DrawString(strg);

	// data
	x=shadowRect.right + 10.0;
	MovePenTo(x, y);
	SetHighColor(Black);
	if(iFile->IsVolume())
		DrawString("Volume");
	else
		DrawString(iFile->MimeDesc());

	// PATH
	
	// label	
	strcpy(strg, PATH_STRING);
	x=shadowRect.right - font.StringWidth(strg) - 10;
	y +=iTextOffset; 
	MovePenTo(x, y);
	SetHighColor(Red);
	DrawString(strg);

	// data
	x=shadowRect.right + 10.0;
	MovePenTo(x, y);
	SetHighColor(Black);
	DrawString(iFile->PathDesc());
}

SFileInfoWindow::SFileInfoWindow(SeekerWindow *mainWindow,const SFile *file,
	bool multiple,int32 offset)
 : BWindow(BRect(100, 100, 400, 300), "",
 	B_FLOATING_WINDOW,B_WILL_DRAW | B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
   iMainWindow(mainWindow),
   iFile(0),
   iFileInfoView(0)
{
	BRect rect;
	if(prefs->FindRect("mainframe",&rect)==B_OK)
	{
		rect.OffsetBy(offset,offset);
		MoveTo(rect.left,rect.top);
	}
	
	iFile=new SFile(file);
	
	// set window title
	char strg[B_FILE_NAME_LENGTH + 10];
	sprintf(strg, "Info: %s", iFile->Name());
	SetTitle(strg);

	// create view
	iFileInfoView=new SFileInfoView(this, iFile, Bounds(),
		"FileInfoWindow::FileInfoView",multiple);
	AddChild(iFileInfoView);
}

SFileInfoWindow::~SFileInfoWindow()
 {
	if(iFile) delete iFile;

	// remove from subset
	BMessage *message=new BMessage(MSG_PIONEER_REMOVE_FROM_SUBSET);
	message->AddPointer("window", this);
	iMainWindow->Looper()->PostMessage(message);
	delete message;
}
