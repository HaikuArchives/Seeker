#include <fs_volume.h>
#include <stdio.h>
#include <stdlib.h>
#include <Application.h>
#include <Menu.h>
#include <MenuItem.h>
#include <NodeMonitor.h>
#include <VolumeRoster.h>
#include <Volume.h>
#include <Entry.h>
#include <Path.h>
#include <String.h>
#include <Directory.h>
#include <Messenger.h>
#include <NodeInfo.h>
#include <Mime.h>
#include <Node.h>
#include <Roster.h>
#include <TranslationUtils.h>
#include <unistd.h>

#include "SeekerApp.h"
#include "SeekerWindow.h"
#include "IconMenuItem.h"

#include "SFileInfo.h"
#include "SplitterView.h"

#define WINDOW_TITLE	"Seeker"

void ProcessTrackerAddons(const char *path, BMenu *menu);

void NewItem(BMenu *menu, BMenuItem **item, char *label, uint16 id, uint16 shortcut=0)
{
	if(shortcut) {
		*item=new BMenuItem(label, new BMessage(id), (char) shortcut);
	} else {
		*item=new BMenuItem(label, new BMessage(id));
	}
	menu->AddItem(*item);
} 

SeekerWindow::SeekerWindow(SeekerApp *app, BRect frame)
 : BWindow(frame, WINDOW_TITLE, B_DOCUMENT_WINDOW, B_WILL_DRAW),
   iSplitterView(0),
   iTreeView(0),
   iFileView(0),
   iNodeMonitor(0),
   iFileWorker(0)
{
	this->iApp=app;

	// create menus
	BuildMenuBar();
	
	BRect bounds=this->Bounds();

	// modify bounds to accomodate menu and scrollbars
	bounds.top +=iMenuBar->Bounds().Height() + 1.0;
	iSplitterView=new CSplitterView(bounds, "SeekerWindow::SplitterView");
	AddChild(iSplitterView);	

	float splitpoint;
	if(prefs->FindFloat("splitpoint",&splitpoint)!=B_OK)
		splitpoint=0.30;
	iSplitterView->SetSplitPoint(splitpoint);

	// create tree-view
	BuildTreeView();
	
	// create file-view
	BuildFileView();

	// add node monitor handler to our looper list
	iNodeMonitor=new SNodeMonitor(iTreeView, iFileView);
	Looper()->AddHandler(iNodeMonitor); 
	
	// add file worker
	iFileWorker=new SFileWorker(this);
	
	// initial load of directories/files
	iTreeView->Refresh();

	BMessenger mountmsgr(this);
	watch_node(NULL,B_WATCH_MOUNT,mountmsgr);
}

SeekerWindow::~SeekerWindow()
{
	// save the window position and size
	prefs->RemoveData("mainframe");
	prefs->AddRect("mainframe",Frame());

	// save the placement of the divider
	prefs->RemoveData("splitpoint");
	prefs->AddFloat("splitpoint",iSplitterView->SplitPoint());
	
	// save the view colors
	rgb_color treecolor=iTreeView->ViewColor();;
	rgb_color filecolor=iFileView->ViewColor();;
	
	prefs->RemoveData("treecolor");
	prefs->AddData("treecolor",B_RGB_COLOR_TYPE,(const void *)&treecolor,sizeof(rgb_color));
	prefs->RemoveData("filecolor");
	prefs->AddData("filecolor",B_RGB_COLOR_TYPE,(const void *)&filecolor,sizeof(rgb_color));

	thread_id addon=find_thread("Seeker Tracker Addon");
	while(addon!=B_NAME_NOT_FOUND)
	{
		kill_thread(addon);
		addon=find_thread("Seeker Tracker Addon");
	}
	if(iNodeMonitor) delete iNodeMonitor;
	if(iFileWorker) delete iFileWorker;
}

void SeekerWindow::BuildMenuBar()
{	
	
	iMenuBar=new BMenuBar(this->Bounds(), "SeekerWindow::MenuBar");

	iFileMenu=new BMenu("File");
		NewItem(iFileMenu, &iFileNewFolderItem, "New Folder", MID_FILE_NEW_FOLDER, 'N');
		iFileMenu->AddSeparatorItem();

		NewItem(iFileMenu, &iFileOpenItem, "Open", MID_FILE_OPEN, 'O');
		NewItem(iFileMenu, &iFileGetInfoItem, "Get info", MID_FILE_GET_INFO, 'I');
		NewItem(iFileMenu, &iFileEditNameItem, "Rename", MID_FILE_EDIT_NAME, 'E');
		NewItem(iFileMenu, &iFileMoveToTrashItem, "Send to Trash", MID_FILE_MOVE_TO_TRASH, 'T');
		NewItem(iFileMenu, &iFileEmptyTrashItem, "Empty trash", MID_FILE_EMPTY_TRASH);
		iFileMenu->AddSeparatorItem();
		
		NewItem(iFileMenu, &iFileQuitItem, "Quit", MID_FILE_QUIT, 'Q');
	
	iMenuBar->AddItem(iFileMenu);

	iEditMenu=new BMenu("Edit");
		NewItem(iEditMenu, &iEditCutItem, "Cut", MID_EDIT_CUT, 'X');
		NewItem(iEditMenu, &iEditCopyItem, "Copy", MID_EDIT_COPY, 'C');
		NewItem(iEditMenu, &iEditPasteItem, "Paste", MID_EDIT_PASTE, 'V');
		NewItem(iEditMenu, &iEditPasteLinkItem, "Paste link", MID_EDIT_PASTE_LINK);
		iEditMenu->AddSeparatorItem();
		
		NewItem(iEditMenu, &iEditSelectAllItem, "Select All", MID_EDIT_SELECT_ALL, 'A');
		NewItem(iEditMenu, &iEditSelectInvertItem, "Invert Selection", MID_EDIT_SELECT_INVERT);
	
	iMenuBar->AddItem(iEditMenu);
	
	iViewMenu=new BMenu("View");
		NewItem(iViewMenu, &iViewRefreshItem, "Refresh", MID_VIEW_REFRESH, 'R');	
		iViewMenu->AddSeparatorItem();
		NewItem(iViewMenu, &iViewColumnsNameItem, "Name", MID_VIEW_COLUMNS_NAME);
		NewItem(iViewMenu, &iViewColumnsSizeItem, "Size", MID_VIEW_COLUMNS_SIZE);
		NewItem(iViewMenu, &iViewColumnsModifiedItem, "Modified", MID_VIEW_COLUMNS_MODIFIED);
		NewItem(iViewMenu, &iViewColumnsCreatedItem, "Created", MID_VIEW_COLUMNS_CREATED);
		NewItem(iViewMenu, &iViewColumnsKindItem, "Kind", MID_VIEW_COLUMNS_KIND);
		
	iMenuBar->AddItem(iViewMenu);

	// set initial selections
	bool tempbool;
	if(prefs->FindBool("usename",&tempbool)==B_OK && tempbool)
		iViewColumnsNameItem->SetMarked(true);
	if(prefs->FindBool("usesize",&tempbool)==B_OK && tempbool)
		iViewColumnsSizeItem->SetMarked(true);
	if(prefs->FindBool("usemodified",&tempbool)==B_OK && tempbool)
		iViewColumnsModifiedItem->SetMarked(true);
	if(prefs->FindBool("usecreated",&tempbool)==B_OK && tempbool)
		iViewColumnsCreatedItem->SetMarked(true);
	if(prefs->FindBool("usekind",&tempbool)==B_OK && tempbool)
		iViewColumnsKindItem->SetMarked(true);
	
	iTrackerAddonMenu=new BMenu("Addons");
	iMenuBar->AddItem(iTrackerAddonMenu);

	iHelpMenu=new BMenu("Help");
		NewItem(iHelpMenu, &iHelpAboutItem, "About...", MID_HELP_ABOUT);	
	iMenuBar->AddItem(iHelpMenu);
	AddChild(iMenuBar);

	ProcessTrackerAddons("/boot/beos/system/add-ons/Tracker",iTrackerAddonMenu);
	ProcessTrackerAddons("/boot/home/config/add-ons/Tracker",iTrackerAddonMenu);

	
	// build popup menus
	iVolumePopUpMenu=new BPopUpMenu("Volume", false, false);
		iVolumePopUpMenu->AddSeparatorItem();
		NewItem(iVolumePopUpMenu, &iVolumePopUpOpenItem, "Open", MID_FILE_OPEN, 'O');
		NewItem(iVolumePopUpMenu, &iVolumePopUpTrackerHereItem, "Open in Tracker", MID_TRACKER_HERE);
		iVolumePopUpMenu->AddSeparatorItem();
		NewItem(iVolumePopUpMenu, &iVolumePopUpNewFolderItem, "New Folder", MID_FILE_NEW_FOLDER, 'N');
		iVolumePopUpMenu->AddSeparatorItem();
		NewItem(iVolumePopUpMenu, &iVolumePopUpEditNameItem, "Rename", MID_FILE_EDIT_NAME, 'E');
		iVolumePopUpMenu->AddSeparatorItem();
//		NewItem(iVolumePopUpMenu, &iVolumePopUpUnmountItem, "Unmount",0);
//		iVolumePopUpMenu->AddSeparatorItem();
		NewItem(iVolumePopUpMenu, &iVolumePopUpGetInfoItem, "Get Info", MID_FILE_GET_INFO, 'I');

	iFolderPopUpMenu=new BPopUpMenu("File", false, false);
		iFolderPopUpMenu->AddSeparatorItem();
		NewItem(iFolderPopUpMenu, &iFolderPopUpOpenItem, "Open", MID_FILE_OPEN, 'O');
		NewItem(iFolderPopUpMenu, &iFolderPopUpTrackerHereItem, "Open in Tracker", MID_TRACKER_HERE);
		iFolderPopUpMenu->AddSeparatorItem();
		NewItem(iFolderPopUpMenu, &iFolderPopUpNewFolderItem, "New Folder", MID_FILE_NEW_FOLDER, 'N');
		iFolderPopUpMenu->AddSeparatorItem();
		NewItem(iFolderPopUpMenu, &iFolderPopUpCutItem, "Cut", MID_EDIT_CUT, 'X');
		NewItem(iFolderPopUpMenu, &iFolderPopUpCopyItem, "Copy", MID_EDIT_COPY, 'C');
		NewItem(iFolderPopUpMenu, &iFolderPopUpPasteItem, "Paste", MID_EDIT_PASTE, 'V');
		NewItem(iFolderPopUpMenu, &iFolderPopUpPasteLinkItem, "Paste Link", MID_EDIT_PASTE_LINK);
		iFolderPopUpMenu->AddSeparatorItem();
		NewItem(iFolderPopUpMenu, &iFolderPopUpEditNameItem, "Rename", MID_FILE_EDIT_NAME, 'E');
		NewItem(iFolderPopUpMenu, &iFolderPopUpMoveToTrashItem, "Send to Trash", MID_FILE_MOVE_TO_TRASH, 'T');
		iFolderPopUpMenu->AddSeparatorItem();
		NewItem(iFolderPopUpMenu, &iFolderPopUpGetInfoItem, "Get Info", MID_FILE_GET_INFO, 'I');

	iFilePopUpMenu=new BPopUpMenu("File", false, false);
		iFilePopUpMenu->AddSeparatorItem();
		NewItem(iFilePopUpMenu, &iFilePopUpOpenItem, "Open", MID_FILE_OPEN, 'O');
		iFilePopUpMenu->AddSeparatorItem();
		NewItem(iFilePopUpMenu, &iFilePopUpCutItem, "Cut", MID_EDIT_CUT, 'X');
		NewItem(iFilePopUpMenu, &iFilePopUpCopyItem, "Copy", MID_EDIT_COPY, 'C');
		iFilePopUpMenu->AddSeparatorItem();
		NewItem(iFilePopUpMenu, &iFilePopUpEditNameItem, "Rename", MID_FILE_EDIT_NAME, 'E');
		NewItem(iFilePopUpMenu, &iFilePopUpMoveToTrashItem, "Send to Trash", MID_FILE_MOVE_TO_TRASH, 'T');
		iFilePopUpMenu->AddSeparatorItem();
		NewItem(iFilePopUpMenu, &iFilePopUpGetInfoItem, "Get Info", MID_FILE_GET_INFO, 'I');

	iTrashPopUpMenu=new BPopUpMenu("File", false, false);
		NewItem(iTrashPopUpMenu, &iTrashPopUpEmptyTrashItem, "Empty Trash", MID_FILE_EMPTY_TRASH);
		iTrashPopUpMenu->AddSeparatorItem();
		NewItem(iTrashPopUpMenu, &iTrashPopUpOpenItem, "Open", MID_FILE_OPEN, 'O');
		NewItem(iTrashPopUpMenu, &iTrashPopUpGetInfoItem, "Get Info", MID_FILE_GET_INFO, 'I');

	iDropPopUpMenu=new BPopUpMenu("Drop", false, false);
		iDropPopUpMenu->AddSeparatorItem();
		NewItem(iDropPopUpMenu, &iDropPopUpCreateLinkItem, "Create Link", MID_DROP_LINK_HERE);
		NewItem(iDropPopUpMenu, &iDropPopUpMoveItem, "Move Here", MID_DROP_MOVE_HERE);
		NewItem(iDropPopUpMenu, &iDropPopUpCopyItem, "Copy Here", MID_DROP_COPY_HERE);
		iDropPopUpMenu->AddSeparatorItem();
		NewItem(iDropPopUpMenu, &iDropPopUpCancelItem, "Cancel", B_CANCEL);
}

// create tree-view
void SeekerWindow::BuildTreeView()
{
	BRect bounds=this->Bounds();
	
	// a bug in the ColumnListView stuff means that we have to
	// pre-calculate the bounds of the window before creating,
	// otherwise the scrollbars don't show up
	bounds=iSplitterView->PaneFrame(C_LEFT);
	bounds.bottom -=B_H_SCROLL_BAR_HEIGHT + 4; // 4=FANCY BORDER WIDTH
	bounds.right -=B_V_SCROLL_BAR_WIDTH + 4;
	
	iTreeView=new STreeView(bounds, "SeekerWindow::TreeView");
	
	ssize_t size;
	rgb_color *color;
	if(prefs->FindData("treecolor",B_RGB_COLOR_TYPE,(const void**)&color,&size)==B_OK)
		iTreeView->SetViewColor(*color);
	iSplitterView->AddChild(iTreeView->View(), C_LEFT);
}

// create file-view
void SeekerWindow::BuildFileView()
{
	BRect bounds=this->Bounds();

	// a bug in the ColumnListView stuff means that we have to
	// pre-calculate the bounds of the window before creating,
	// otherwise the scrollbars don't show up
	bounds=iSplitterView->PaneFrame(C_RIGHT);
	bounds.bottom -=B_H_SCROLL_BAR_HEIGHT + 4; // 4=FANCY BORDER WIDTH
	bounds.right -=B_V_SCROLL_BAR_WIDTH + 4;

	iFileView=new SFileView(bounds, "SeekerWindow::FileView");

	ssize_t size;
	rgb_color *color;
	if(prefs->FindData("filecolor",B_RGB_COLOR_TYPE,(const void**)&color,&size)==B_OK)
		iFileView->SetViewColor(*color);
	iSplitterView->AddChild(iFileView->View(), C_RIGHT);
}

// toggle column selection and update file-view
void SeekerWindow::UpdateColumns(BMenuItem *item)
{
	// invert mark
	item->SetMarked(!item->IsMarked());

	// update view
	iFileView->SetColumns(iViewColumnsNameItem->IsMarked(),
						   iViewColumnsSizeItem->IsMarked(),
						   iViewColumnsModifiedItem->IsMarked(),
						   iViewColumnsCreatedItem->IsMarked(),
						   iViewColumnsKindItem->IsMarked(),
						   false);

}

// update path in window title
void SeekerWindow::UpdateTitle(BMessage *message)
{
	if(message==NULL) {
		return;
	}
	
	const char *path;
	if(message->FindString("path", &path) !=B_OK) {
		return;
	}
	
	char strg[B_PATH_NAME_LENGTH+30];
	sprintf(strg, "%s : %s", WINDOW_TITLE, path);

	SetTitle(strg);
}

// remove floating window from subset
void SeekerWindow::RemoveFromSubset(BMessage *message)
{
	if(message==NULL) {
		return;
	}
	
	BWindow *window;
	if(message->FindPointer("window", (void **) &window) !=B_OK) {
		return;
	}
	
	BWindow::RemoveFromSubset(window);
}

// current selections
void SeekerWindow::CurrentSelections(SFileList *fileList)
{
	if(iTreeView->IsFocus())
	{
		iTreeView->CurrentSelections(fileList);
	}
	else
	{
		iFileView->CurrentSelections(fileList);		
	
		if(fileList->CountItems()==0)
		{
			// use tree-view instead
			iTreeView->CurrentSelections(fileList);
		}
	}
}

// current folder
void SeekerWindow::CurrentFolder(SFile **file, bool useSuperItem)
{
	SFileList fileList;
	iTreeView->CurrentSelections(&fileList);
	
	if(fileList.CountItems() > 0)
	{	

		// theoretically, this list should contain one and always one item	
		*file=new SFile((SFile *) fileList.ItemAt(0));

		// return parent folder ?
		// cut and copy routines have trouble using directories selected from the
		// tree-view.  In addition, this provides protection from copying the Desktop
		// root
		if(iTreeView->IsFocus())
		{
		
			if((*file)->IsDesktop())
			{
				delete *file;
				*file=0;
				return;
			}
			**file=*((*file)->Parent());
		} 
		
	}
	else
		*file=0;
}

void SeekerWindow::VolumePopUp(BMessage *message)
{
	if(message==NULL)
		return;

	BPoint point;
	if(message->FindPoint("point", &point) !=B_OK)
		return;

	STreeItem *item;
	if(message->FindPointer("item", (void **) &item) !=B_OK)
		return;

	if(point.x>5)
		point.x-=5;
	if(point.y>5)
		point.y-=5;
		
	// Add a submenu showing the selected volume to provide extra feedback
	IconMenuItem *iconitem;

	iconitem=new IconMenuItem(item->File()->Name(),NULL, new BBitmap(item->File()->SmallIcon()));
	iVolumePopUpMenu->AddItem(iconitem,0);

	if(item->File() && (strcasecmp(item->File()->PathDesc(), "/boot")==0))
	{
		iVolumePopUpEditNameItem->SetEnabled(false);
//		iVolumePopUpUnmountItem->SetEnabled(false);
	}
	BMenuItem *result=iVolumePopUpMenu->Go(point,false,true);
	
	iVolumePopUpMenu->RemoveItem(iconitem);
	delete iconitem;

	uint32 msgId=0;	

	if(result==iVolumePopUpOpenItem)
		msgId=MID_FILE_OPEN;
	else if(result==iVolumePopUpNewFolderItem)
		msgId=MID_FILE_NEW_FOLDER;
	else if(result==iVolumePopUpGetInfoItem)
		msgId=MID_FILE_GET_INFO;
	else if(result==iVolumePopUpEditNameItem)
		msgId=MID_FILE_EDIT_NAME;
	else if(result==iVolumePopUpTrackerHereItem)
		msgId=MID_TRACKER_HERE;
	else if(result==iVolumePopUpUnmountItem)
	{
		item->RequestScanTerminate();
		fs_unmount_volume(item->File()->PathDesc(), 0);
	}

	if(!iVolumePopUpEditNameItem->IsEnabled())
	{
		iVolumePopUpEditNameItem->SetEnabled(true);
//		iVolumePopUpUnmountItem->SetEnabled(true);
	}

	if(msgId)
	{
		BMessage message(msgId);
		message.AddInt8("fileview",0);
		Looper()->PostMessage(&message);
	}

}

void SeekerWindow::FolderPopUp(BMessage *message)
{
	if(message==NULL)
	{
		printf("FolderPopUp(NULL)\n");
		return;
	}

	BPoint point;
	if(message->FindPoint("point", &point)!=B_OK)
	{
		printf("FolderPopUp: Couldn't find point\n");
		return;
	}

	int8 fileview;
	if(message->FindInt8("fileview", &fileview)!=B_OK)
	{
		printf("FolderPopUp: Couldn't find fileview flag\n");
		fileview=0;
	}

	SeekerItem *item;
	if(message->FindPointer("item", (void **) &item)!=B_OK)
	{
		printf("FolderPopUp: Couldn't find item\n");
		return;
	}

	if(point.x>5)
		point.x-=5;
	if(point.y>5)
		point.y-=5;

	// Get the file list	
	SFileList *filelist=new SFileList;
	CurrentSelections(filelist);
	
	// Add a submenu showing all the selected files to provide extra feedback
	BMenu *menu=NULL;
	IconMenuItem *iconitem=NULL;
	
	if(filelist->CountItems()==0)
	{
		delete filelist;
		return;
	}
	
	// Just create 1 menu item if there is only one file selected
	if(filelist->CountItems()==1)
	{
		SFile *f=(SFile*)filelist->ItemAt(0);
		iconitem=new IconMenuItem(f->Name(),NULL, new BBitmap(f->SmallIcon()));
		iFolderPopUpMenu->AddItem(iconitem,0);
	}
	else
	{
		// Function adds multiple selections to the menu
		menu=GenerateIconMenu(filelist);
		iFolderPopUpMenu->AddItem(menu,0);
	}
	
	// Disable certain things you just don't do to certain items, such as /boot,
	// the Trash, or the Desktop
	bool disabled_items=false;
	for(int32 i=0;i<filelist->CountItems();i++)
	{
		SFile *f=(SFile*)filelist->ItemAt(i);	
		if(f && f->IsSystem())
		{
			iFolderPopUpEditNameItem->SetEnabled(false);
			iFolderPopUpMoveToTrashItem->SetEnabled(false);
			iFolderPopUpCutItem->SetEnabled(false);
			disabled_items=true;
			break;
		}
	}

	BMenuItem *result=iFolderPopUpMenu->Go(point,true,true);

	if(disabled_items)
	{
		iFolderPopUpEditNameItem->SetEnabled(true);
		iFolderPopUpMoveToTrashItem->SetEnabled(true);
		iFolderPopUpCutItem->SetEnabled(true);
	}

	// Remove all the iconized selection-related menu items from the popup menu
	if(filelist->CountItems()==1)
	{
		iFolderPopUpMenu->RemoveItem(iconitem);
		delete iconitem;
	}
	else
	{
		iFolderPopUpMenu->RemoveItem(menu);
		delete menu;
	}
	

	uint32 msgId=0;	

	if(result==iFolderPopUpOpenItem) {
		msgId=MID_FILE_OPEN;
	} else if(result==iFolderPopUpNewFolderItem) {
		msgId=MID_FILE_NEW_FOLDER;
	} else if(result==iFolderPopUpGetInfoItem) {
		msgId=MID_FILE_GET_INFO;
	} else if(result==iFolderPopUpEditNameItem) {
		msgId=MID_FILE_EDIT_NAME;
	} else if(result==iFolderPopUpMoveToTrashItem) {
		msgId=MID_FILE_MOVE_TO_TRASH;
	} else if(result==iFolderPopUpCutItem) {
		msgId=MID_EDIT_CUT;
	} else if(result==iFolderPopUpCopyItem) {
		msgId=MID_EDIT_COPY;
	} else if(result==iFolderPopUpPasteItem) {
		msgId=MID_EDIT_PASTE;
	} else if(result==iFolderPopUpPasteLinkItem) {
		msgId=MID_EDIT_PASTE_LINK;
	} else if(result==iFolderPopUpTrackerHereItem)
		msgId=MID_TRACKER_HERE;

	if(msgId)
	{
		BMessage message(msgId);
		message.AddInt8("fileview",fileview);
		Looper()->PostMessage(&message);
	}

	filelist->MakeEmpty();
	delete filelist;
}

void SeekerWindow::FilePopUp(BMessage *message)
{
	if(message==NULL)
		return;

	BPoint point;
	if(message->FindPoint("point", &point) !=B_OK)
		return;

	STreeItem *item;
	if(message->FindPointer("item", (void **) &item) !=B_OK)
		return;

	if(point.x>5)
		point.x-=5;
	if(point.y>5) 
		point.y-=5;

	// Get the file list	
	SFileList *filelist=new SFileList;
	CurrentSelections(filelist);
	
	// Add a submenu showing all the selected files to provide extra feedback
	BMenu *menu=NULL;
	IconMenuItem *iconitem=NULL;
	
	if(filelist->CountItems()==0)
	{
		delete filelist;
		return;
	}
	
	// Just create 1 menu item if there is only one file selected
	if(filelist->CountItems()==1)
	{
		iconitem=new IconMenuItem(item->File()->Name(),NULL, new BBitmap(item->File()->SmallIcon()));
		iFilePopUpMenu->AddItem(iconitem,0);
	}
	else
	{
		// Function adds multiple selections to the menu
		menu=GenerateIconMenu(filelist);
		iFilePopUpMenu->AddItem(menu,0);
	}

	BMenuItem *result=iFilePopUpMenu->Go(point, true);

	// Remove all the iconized selection-related menu items from the popup menu
	if(filelist->CountItems()==1)
	{
		iFilePopUpMenu->RemoveItem(iconitem);
		delete iconitem;
	}
	else
	{
		iFilePopUpMenu->RemoveItem(menu);
		delete menu;
	}
	
	uint32 msgId=0;	

	if(result==iFilePopUpOpenItem) {
		msgId=MID_FILE_OPEN;
	} else if(result==iFilePopUpGetInfoItem) {
		msgId=MID_FILE_GET_INFO;
	} else if(result==iFilePopUpEditNameItem) {
		msgId=MID_FILE_EDIT_NAME;
	} else if(result==iFilePopUpMoveToTrashItem) {
		msgId=MID_FILE_MOVE_TO_TRASH;
	} else if(result==iFilePopUpCutItem) {
		msgId=MID_EDIT_CUT;
	} else if(result==iFilePopUpCopyItem) {
		msgId=MID_EDIT_COPY;
	}

	if(msgId)
	{
		BMessage message(msgId);
		Looper()->PostMessage(&message);
	}
}

void SeekerWindow::TrashPopUp(BMessage *message)
{
	if(message==NULL)
		return;

	BPoint point;
	if(message->FindPoint("point", &point) !=B_OK)
		return;

	STreeItem *item;
	if(message->FindPointer("item", (void **) &item) !=B_OK)
		return;

	if(point.x>5)
		point.x-=5;
	if(point.y>5)
		point.y-=5;
	BMenuItem *result=iTrashPopUpMenu->Go(point,false,true);
	
	uint32 msgId=0;	

	if(result==iTrashPopUpOpenItem) {
		msgId=MID_FILE_OPEN;
	} else if(result==iTrashPopUpGetInfoItem) {
		msgId=MID_FILE_GET_INFO;
	} else if(result==iTrashPopUpEmptyTrashItem) {
		msgId=MID_FILE_EMPTY_TRASH;
	}
	
	if(msgId)
	{
		BMessage message(msgId);
		Looper()->PostMessage(&message);
	}

}

void SeekerWindow::DropPopUp(BMessage *message)
{
	if(message==NULL)
	{
		printf("NULL Drop message\n");
		return;
	}

	BPoint point,screenpoint, menupoint;
	if(message->FindPoint("_drop_point_", &screenpoint) !=B_OK)
		return;

	point=ConvertFromScreen(screenpoint);

	// figure out which pane received the drop and get the appropriate list item
	int8 treeview=0;	// 0=neither pane, 1=treeview, 2=fileview
	
	if(iTreeView->Parent()->Frame().Contains(point))
		treeview=1;
	else
		if(iFileView->Parent()->Frame().Contains(point))
			treeview=2;

	if(treeview==0)
		return;

	// Add color drop support to the app
	rgb_color *dropcolor;
	ssize_t size;
	if (message->FindData("RGBColor", 'RGBC',(const void**)&dropcolor, &size) == B_OK)
	{
		if(treeview==1)
			iTreeView->SetViewColor(*dropcolor);
		else
			iFileView->SetViewColor(*dropcolor);
	}

	// Get the target item in the tree. This can be any kind of item - if there is
	// one at all. It is possible to drop it in the right pane on nothing	
	SFile *dropfile=NULL;
	SeekerItem *dropitem;
	int32 itemindex=0;
	if(treeview==1)
	{
		itemindex=iTreeView->IndexOf(iTreeView->ConvertFromScreen(screenpoint));
		if(itemindex>=0)
		{
			dropitem=(SeekerItem*)iTreeView->ItemAt(itemindex);
			dropfile=dropitem->File();
		}
		else
		{
			// well, it was just plain dropped in a blank area. See if there
			// is a selected folder. If so, copy to that folder. If not,
			// use the fileview's current path. If there isn't one, there
			// are *serious* problems going on here and we need to just
			// do nothing.
			STreeItem *dropitem=(STreeItem*)iTreeView->ItemAt(iTreeView->CurrentSelection());
			if(dropitem)
				dropfile=dropitem->File();
			else
			{
				dropfile=iFileView->PathAsFile();
				if(!dropfile)
					return;
			}
		}
	}
	else
	{
		itemindex=iFileView->IndexOf(iFileView->ConvertFromScreen(screenpoint));
		if(itemindex>=0)
		{
			dropitem=(SeekerItem*)iFileView->ItemAt(itemindex);
			dropfile=dropitem->File();
		}
		else
		{
			dropfile=iFileView->PathAsFile();
		}
	}

	// Determine whether we need to pop up a menu. If a drop is made on a file,
	// we need to launch the targeted application if an application or a symlink
	// to one, or we do nothing and if otherwise.

	// treeview == 2 when dropped on the iFileView
	if(treeview==2 && dropfile->TargetType()==TYPE_FILE)
	{
		// MimeDesc() is passed True, it returns the type of the target 
		// if it's a link or its own type if not a link
		BString mimestr(dropfile->MimeDesc(true));
	
		if(mimestr.Compare("application/x-vnd.Be-elfexecutable")==0 ||
			mimestr.Compare("Be Application")==0)
		{
			// an executable, if launched without argv/argc expects a B_REFS_RECEIVED
			// message. Seeing how it only has the attachment "refs", we can get away
			// with changing the command constant and sending it to Launch()
			message->what=B_REFS_RECEIVED;
			be_roster->Launch(dropfile->Ref(),message);
			return;
		}
		return;
	}	

	// we got this far, so the drop target is evidently a folder or a link to one.
	// Check to see if it's the Trash. If it is, automatically do a MoveToTrash.
	// Otherwise, pop up the menu and get the appropriate command

	// Set up the file list for the file operations	
	SFileList *filelist=new SFileList(0);
	entry_ref ref;
	int32 refindex=0;
	SFile *fileitem;
	
	while(message->FindRef("refs",refindex,&ref)==B_OK)
	{
		refindex++;
		fileitem=new SFile(&ref);
		filelist->AddItem(fileitem);
	}

	bool usetrash=(dropfile->IsTrash())?true:false;
	BMenuItem *result=NULL;
	
	if(usetrash==false)
	{
		// Add a submenu showing all the selected files to provide extra feedback
		BMenu *menu=NULL;
		IconMenuItem *iconitem=NULL;

		// Just create 1 menu item if there is only one file selected
		if(filelist->CountItems()==1)
		{
			SFile *f=(SFile*)filelist->ItemAt(0);
			iconitem=new IconMenuItem(f->Name(),NULL, new BBitmap(f->SmallIcon()));
			iDropPopUpMenu->AddItem(iconitem,0);
		}
		else
		{
			// Function adds multiple selections to the menu
			menu=GenerateIconMenu(filelist);
			if(menu)
				iDropPopUpMenu->AddItem(menu,0);
		}

		// Disable certain things you just don't do to certain items, such as /boot,
		// the Trash, or the Desktop
		bool disabled_items=false;
		for(int32 i=0;i<filelist->CountItems();i++)
		{
			SFile *f=(SFile*)filelist->ItemAt(i);	
			if(f && f->IsSystem())
			{
				iDropPopUpMoveItem->SetEnabled(false);
				disabled_items=true;
				break;
			}
		}
	
		menupoint=screenpoint;
		if(menupoint.x>5)
			menupoint.x-=5;
		if(menupoint.y>5)
			menupoint.y-=5;
		result=iDropPopUpMenu->Go(menupoint,false,true);
		
		if(disabled_items)
			iDropPopUpMoveItem->SetEnabled(false);

		// Remove all the iconized selection-related menu items from the popup menu
		if(filelist->CountItems()==1)
		{
			iDropPopUpMenu->RemoveItem(iconitem);
			delete iconitem;
		}
		else
		{
			if(menu)
			{
				iDropPopUpMenu->RemoveItem(menu);
				delete menu;
			}
		}
	}

	if(usetrash)
		iFileWorker->MoveFiles(filelist,dropfile);
	else	
		if(result==iDropPopUpCreateLinkItem)
			iFileWorker->MakeLinks(filelist,dropfile);
		else
			if(result==iDropPopUpMoveItem)
				iFileWorker->MoveFiles(filelist,dropfile);
			else
				if(result==iDropPopUpCopyItem)
					iFileWorker->CopyFiles(filelist,dropfile);
				else
					return;


	// Clean up our mess
	SFile *removedfile;
	for(int32 i=0;i<filelist->CountItems();i++)
	{
		removedfile=(SFile*)filelist->ItemAt(i);
		delete removedfile;
	}
	filelist->MakeEmpty();
	delete filelist;
}

bool SeekerWindow::QuitRequested()
{
	iTreeView->AbortScan();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void SeekerWindow::MessageReceived(BMessage* message)
{
	SFile *currentFolder=0;
	SFileList fileList;

	switch(message->what)
	{
		case MID_FILE_NEW_FOLDER:
		{
			CurrentSelections(&fileList);
			if(fileList.CountItems()==1)
			{
				SFile *file=(SFile *)fileList.ItemAt(0);
				if(file && file->TargetType()==TYPE_DIRECTORY)
					iFileWorker->NewFolder(file);
			}
			else
			if(fileList.CountItems()>1)
			{
				// we have multiple files, so get the parent folder from
				// one of the entries
				SFile *file=(SFile *)fileList.ItemAt(0);
				if(file)
					iFileWorker->NewFolder(file->Parent());
			}
			break;
		}
  		case MID_FILE_OPEN:
  		{
			if(iFileView->IsFocus())
				iFileView->Open();

			if(iTreeView->IsFocus())
			{	
				STreeItem *item=(STreeItem *) iTreeView->ItemAt(iTreeView->CurrentSelection()); 
				if(item)
					iTreeView->Expand(item);				
			}
  			break;
  		}	
  		case MID_FILE_GET_INFO:
  		{
			CurrentSelections(&fileList);
			iFileWorker->GetInfo(&fileList);
			break;
		}	
		case MID_FILE_EDIT_NAME:
		{
			CurrentSelections(&fileList);
			iFileWorker->EditName(&fileList);
			break;
		}
		case MID_FILE_MOVE_TO_TRASH:
		{
			CurrentSelections(&fileList);
			CurrentFolder(&currentFolder, true);
			if(currentFolder)
				iFileWorker->MoveToTrash(currentFolder, &fileList);
			break;
		}	
		case MID_FILE_EMPTY_TRASH:
		{
			iFileWorker->EmptyTrash();
			break;
		}
 		case MID_FILE_QUIT:
 		{
			Quit();
  			break;
		}
		case MID_EDIT_CUT:
		{
			CurrentSelections(&fileList);
			CurrentFolder(&currentFolder, true);
			if(currentFolder)
				iFileWorker->Cut(currentFolder, &fileList);
			break;
		}
		case MID_EDIT_COPY:
		{
			CurrentSelections(&fileList);
			CurrentFolder(&currentFolder, true);
			if(currentFolder)
				iFileWorker->Copy(currentFolder, &fileList);
			break;
		}
		case MID_EDIT_PASTE:
		{
			CurrentSelections(&fileList);
			if(fileList.CountItems() > 0)
				iFileWorker->Paste((SFile *) fileList.ItemAt(fileList.CountItems()-1));
			break;
		}
		case MID_EDIT_PASTE_LINK:
		{
			CurrentSelections(&fileList);
			if(fileList.CountItems() > 0)
				iFileWorker->PasteLink((SFile *) fileList.ItemAt(fileList.CountItems()-1));
			break;
		}	  			
		case MID_EDIT_SELECT_ALL:
		{
			iFileView->SelectAll(); 
			break;
		}	  			
		case MID_EDIT_SELECT_INVERT:
		{
			iFileView->InvertSelection();
			break;
		}	  			
  		case MID_VIEW_COLUMNS_NAME :
		{
			UpdateColumns(iViewColumnsNameItem);
			break;
		}	  			
  		case MID_VIEW_COLUMNS_SIZE :
		{
			UpdateColumns(iViewColumnsSizeItem);
			break;
		}	  			
		case MID_VIEW_COLUMNS_MODIFIED:
		{
			UpdateColumns(iViewColumnsModifiedItem);
			break;
		}	  			
		case MID_VIEW_COLUMNS_CREATED:
		{
			UpdateColumns(iViewColumnsCreatedItem);
			break;
		}	  			
		case MID_VIEW_COLUMNS_KIND:
		{
			UpdateColumns(iViewColumnsKindItem);
			break;
		}	  			
  		case MID_VIEW_REFRESH:
		{
  			iTreeView->Refresh();
  			break;
		}	  			
		case MID_TRACKER_HERE:
		{
			int8 fileview;
			if(message->FindInt8("fileview",&fileview)==B_OK)
			{
				if(fileview)
					iFileView->SpawnTracker();
				else
					iTreeView->SpawnTracker();
			}
			break;
		}
		case MID_TRACKER_ADDON:
		{
			image_id id;
			if(message->FindInt32("addon",&id)==B_OK)
				RunTrackerAddon(id);
			break;
		}
  		case MSG_PIONEER_UPDATE_TITLE:
		{
  			UpdateTitle(message);
  			break;
		}
  		case MSG_PIONEER_REMOVE_FROM_SUBSET :
		{
  			RemoveFromSubset(message); 
  			break;
		}
  		case MSG_PIONEER_VOLUME_POPUP_MENU :
		{
  			VolumePopUp(message);
  			break;
		}
		case MSG_PIONEER_FOLDER_POPUP_MENU:
		{
			FolderPopUp(message);
			break;
		}
  		case MSG_PIONEER_FILE_POPUP_MENU:
		{
  			FilePopUp(message);
  			break;
		}
  		case MSG_PIONEER_TRASH_POPUP_MENU:
		{
  			TrashPopUp(message);
  			break;
		}
  		case MSG_TREEVIEW_EXPAND:
		{
  			iTreeView->ExpandAndSelect(message);
  			break;
		}
  		case MSG_FILEVIEW_REFRESH:
		{
 			iFileView->Refresh(message);
  			break;
		}
  		case MSG_FILEVIEW_REFRESH_CURRENT:
		{
 			iFileView->Refresh();
  			break;
		}
  		case MSG_FILEVIEW_INVOKE:
		{
  			iFileView->Open();
  			break;
		}
  		case MSG_FILEVIEW_CLEAR:
		{
  			iFileView->Clear();
  			break;
		}
  		case MSG_NODEMONITOR_ADD_WATCH:
		{
  			iNodeMonitor->AddWatch(message);
  			break;
		}
  		case MSG_NODEMONITOR_REMOVE_WATCH :
		{
  			iNodeMonitor->RemoveWatch(message);
  			break;
		}
		case B_PASTE:
		{
			if(message->WasDropped())
			{
				// Add color drop support to the app
				rgb_color *dropcolor;
				ssize_t size;
				if(message->FindData("RGBColor", 'RGBC',(const void**)&dropcolor, &size)!=B_OK)
					break;

				BPoint screenpoint;
				if(message->FindPoint("_drop_point_", &screenpoint) !=B_OK)
					break;
			
				ConvertFromScreen(&screenpoint);
			
				if(iTreeView->Parent()->Frame().Contains(screenpoint))
				{
					iTreeView->SetViewColor(*dropcolor);
					iTreeView->Invalidate();
				}
				else
					if(iFileView->Parent()->Frame().Contains(screenpoint))
					{
						iFileView->SetViewColor(*dropcolor);
						iFileView->Invalidate();
					}
			}
			break;
		}
		case B_SIMPLE_DATA:
		{
			if(message->WasDropped())
				DropPopUp(message);
			break;
		}
		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			if(message->FindRef("refs",0,&ref)==B_OK)
			{
				BEntry entry(&ref);
				if(!entry.IsDirectory())
				{
					BEntry parent;
					entry.GetParent(&parent);
					parent.GetRef(&ref);
				}
				iTreeView->ExpandAndSelect(ref);
			}
			break;
		}
		case MID_HELP_ABOUT:
			be_app->PostMessage(new BMessage(B_ABOUT_REQUESTED));
			break;
		default:
			BWindow::MessageReceived(message);
	}
}

void SeekerWindow::RefreshTrackerAddons(void)
{
	// empty tracker addon menu
	BMenuItem *mitem=iTrackerAddonMenu->RemoveItem(0L);
	while(mitem)
	{
		delete mitem;
		mitem=iTrackerAddonMenu->RemoveItem(0L);
	}
	
	// iterate through /boot/beos/system/addons/Tracker
	// and ~config/addons/Tracker, adding only legit addons
}

void ProcessTrackerAddons(const char *path, BMenu *menu)
{
	if(!path || !menu)
		return;
	
	BDirectory *dir=new BDirectory(path);
	BEntry entry;
	BNode node;
	BNodeInfo nodeinfo;
	BPath epath;
	BString string;
	BMenuItem *item;
	addon_process_refs *processfunc=NULL;
	status_t stat;
	image_id addon;
	BMessage *addonmsg;
	
	while(dir->GetNextEntry(&entry)==B_OK)
	{
		entry.GetPath(&epath);
		addon=load_add_on(epath.Path());
		stat=get_image_symbol(addon, "process_refs", B_SYMBOL_TYPE_TEXT, (void**)&processfunc);

		if(stat!=B_OK)
		{
			printf("ERROR: file %s is not a Tracker Addon\n", epath.Path());
			unload_add_on(addon);
			continue;
		}
		
		// if we got this far, it must be a tracker addon, so add the thing to the
		// menu
		
		string.SetTo(epath.Path());

		// obtain just the file's leaf
		string.Remove(0,string.FindLast('/')+1);

		// remove the trailing shortcut from the name string, if there is one.
		int32 dashpos=string.FindLast('-');
		uint8 shortcut=(uint8)string.ByteAt(string.CountChars()-1);
		
		addonmsg=new BMessage(MID_TRACKER_ADDON);
		addonmsg->AddInt32("addon",addon);

		// Set up to get the icon
		node.SetTo(&entry);
		nodeinfo.SetTo(&node);

		// We have to jump through a lot of hoops to determine the keyboard
		// shortcut because the kind of format is specific, but we don't want
		// to truncate the name of any addon which uses a dash and doesn't fit
		// the shortcut format
		if(dashpos!=B_ERROR && dashpos==string.CountChars()-2 && 
			( (shortcut>64 && shortcut<91) || (shortcut>96 && shortcut<123)) )
		{
			string.Truncate(dashpos);

			// This sucker will have the same exact keyboard shortcut as Tracker. :)		
			item=new BMenuItem(string.String(),addonmsg,(char)shortcut,
				B_COMMAND_KEY | B_OPTION_KEY);

			// Need a little more work before I can import the OpenTracker IconMenuItem stuff
//			item=(BMenuItem*)new IconMenuItem(string.String(),addonmsg,&nodeinfo,
//				B_MINI_ICON);
		}
		else
		{
			item=new BMenuItem(string.String(),addonmsg);
		}
		menu->AddItem(item);
	}

	delete dir;
}

void SeekerWindow::RunTrackerAddon(image_id addon)
{
	// get the working directory item
	STreeItem *treeitem=(STreeItem*)iTreeView->ItemAt(iTreeView->CurrentSelection());
	if(!treeitem)
		return;
	
	// Create the ref message based on which pane is active
	BMessage *refmsg=new BMessage(B_REFS_RECEIVED);
	if(iTreeView->IsFocus())
		refmsg->AddRef("refs",treeitem->File()->Ref());
	else
	{
		if(iFileView->IsFocus())
		{
			SFileList *list=new SFileList(0);
			iFileView->CurrentSelections(list);
			SFile*file;

			for(int32 i=0; i<list->CountItems();i++)
			{
				file=(SFile*)list->ItemAt(i);
				if(file)
					refmsg->AddRef("refs",file->Ref());
			}
			list->MakeEmpty();			
			delete list;
			
		}
		else
		{
			delete refmsg;
			return;
		}
	}
	refmsg->AddRef("directory",treeitem->File()->Ref());
	refmsg->AddInt32("addon",addon);
	
	thread_id addon_thread=spawn_thread(TrackerAddonThread,"Seeker Tracker Addon",
		B_NORMAL_PRIORITY,refmsg);

	if(addon_thread!=B_NO_MORE_THREADS && addon_thread!=B_NO_MEMORY)
		resume_thread(addon_thread);
	else
		delete refmsg;

	// Don't need to delete the thing because the addon thread will do it
	//delete refmsg;
}

int32 SeekerWindow::TrackerAddonThread(void *data)
{
	BMessage *msg=(BMessage*)data;
	image_id addon;
	entry_ref ref;
	
	if(msg->FindInt32("addon",&addon)!=B_OK)
	{
		delete msg;
		return -1;
	}

	if(msg->FindRef("directory",&ref)!=B_OK)
	{
		delete msg;
		return -1;
	}

	// Initial setup
	addon_process_refs *processfunc=NULL;

	status_t status;
	status=get_image_symbol(addon, "process_refs", B_SYMBOL_TYPE_TEXT, (void**)&processfunc);

	if(status!=B_OK)
	{
		delete msg;
		return -1;
	}
	
	// we make a dummy pointer in case some addon would choke if we just passed NULL	
	int8 *ptr=NULL;

	processfunc(ref,msg,ptr);
	
	return 0;
}

BMenu *GenerateIconMenu(const SFileList *filelist)
{
	int32 filecount=filelist->CountItems();
	if(filecount==0)
		return NULL;

	IconMenuItem *iconitem;
	SFile *file;

	BMenu *menu=new BMenu("Files");
	
	for(int32 i=0; i<filecount;i++)
	{
		file=(SFile*)filelist->ItemAt(i);
		if(file)
		{
			iconitem=new IconMenuItem(file->Name(),NULL,new BBitmap(file->SmallIcon()));
			menu->AddItem(iconitem);
		}
	}
	return menu;
}

void SeekerWindow::WindowActivated(bool active)
{
	if(!active)
	{
		if(iFileView->IsTracking())
			iFileView->EndTracking();
	}
}

void SeekerWindow::FrameMoved(BPoint origin)
{
	prefs->RemoveData("mainframe");
	prefs->AddRect("mainframe",Frame());
}

void SeekerWindow::FrameResized(float width, float height)
{
	prefs->RemoveData("mainframe");
	prefs->AddRect("mainframe",Frame());
}
