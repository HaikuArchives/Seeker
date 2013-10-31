#ifdef __MWERKS__
#include <alloca.h>
#else
#include <alloc.h>
#endif

#include <stdio.h>
#include <string.h>
#include <Application.h>
#include <Window.h>
#include <Region.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Directory.h>
#include <OutlineListView.h>
#include <String.h>
#include <ScrollView.h>
#include <ScrollBar.h>
#include <Path.h>
#include <Messenger.h>

#include <Alert.h>

#include "Colors.h"
#include "CLVColumn.h"

#include "SeekerWindow.h"
#include "STreeView.h"
#include "FSUtils.h"
#include "Preferences.h"

extern Preferences *prefs;

STreeItem::STreeItem(STreeView *treeView, SFile *file, bool expand)
 : SeekerItem(file, 0, true, expand, 20.0),
   iScanThreadActive(0),
   iScanThreadTerminate(0),
   iTreeView(treeView),
   iTextOffset(0),
   iTreeScan(false),
   iFileScan(false)   		   
{
	labelwidth=treeView->StringWidth(Name());
	labelstart=0;

	// create semaphores
	iScanThreadActive=create_sem(1, "TreeItem::ScanThreadActive");
	iScanThreadTerminate=create_sem(1, "TreeItem::ScanThreadTerminate");

	strcpy(iAutoSelectDir, "");
	
	// set level
	int32 level;

	if(iFile->IsVolume())
	{ 
		// always under desktop
		level=1;
	}
	else
	{
		// this new way is slightly faster
/*		int32 strlength=strlen(iFile->PathDesc());
		level=-3;

		char *str=new char[strlength+1];
		char *strindex=str;
		strcpy(str,iFile->PathDesc());
		while(*strindex>0)
		{
			if(*strindex=='/')
				level++;
			strindex++;
		}
		delete str;
*/
		level=0;
	}

	SetOutlineLevel(level);
	
	// add to node monitor watch list
	BNode node(iFile->Ref());
	node_ref nodeRef;
	node.GetNodeRef(&nodeRef);
	
	BMessage *message=new BMessage(MSG_NODEMONITOR_ADD_WATCH);
	message->AddInt32("device", nodeRef.device);
	message->AddInt64("node", nodeRef.node);
	iTreeView->Looper()->PostMessage(message);
	delete message;
}

STreeItem::~STreeItem()
{
	// remove from watch list
	BNode node(iFile->Ref());
	node_ref nodeRef;
	node.GetNodeRef(&nodeRef);

	BMessage *message=new BMessage(MSG_NODEMONITOR_REMOVE_WATCH);
	message->AddInt32("device", nodeRef.device);
	message->AddInt64("node", nodeRef.node);
	iTreeView->Looper()->PostMessage(message);
	delete message;

	// make sure we stop the scan thread before we go and delete ourselves
	RequestScanTerminate();

	if(iScanThreadActive) delete_sem(iScanThreadActive);
	if(iScanThreadTerminate) delete_sem(iScanThreadTerminate);	

//	if(iFile) delete iFile;
}

// custom paint
void STreeItem::DrawItemColumn(BView *owner, BRect itemColumnRect, 
								int32 columnIndex, bool complete)
{
	rgb_color color;
	bool selected=IsSelected();
	
	color=(selected)?Black:White;
	
	owner->SetLowColor(color);
	owner->SetDrawingMode(B_OP_COPY);
	if((selected || complete) && (columnIndex==2))
	{
		owner->SetHighColor(color);

		BRect rect=itemColumnRect;
		rect.left -=1.0;
		rect.top +=iTextOffset - ceil(iFontAttr.ascent);
		rect.bottom -=((iTextOffset - ceil(iFontAttr.ascent)) / 2.0) + 2.0;
		rect.right=rect.left + owner->StringWidth(iFile->Name()) + 3.0;

		owner->FillRect(rect);
	}
	else
		if(complete && (columnIndex==0))
		{
			// redraw expansion arrow
			owner->SetHighColor(White);
			owner->FillRect(itemColumnRect);
		}
	
	BRegion Region;
	Region.Include(itemColumnRect);
	owner->ConstrainClippingRegion(&Region);
	owner->SetHighColor(Black);

	// icon column
	if(columnIndex==1)
	{
	
		itemColumnRect.left +=2.0;
		itemColumnRect.right=itemColumnRect.left + 15.0;
		itemColumnRect.top +=2.0;
		if(Height() > 20.0)
			itemColumnRect.top +=ceil((Height()-20.0) / 2.0);

		itemColumnRect.bottom=itemColumnRect.top + 15.0;
		owner->SetDrawingMode(B_OP_OVER);
		if(iFile->SmallIcon())
			owner->DrawBitmap(iFile->SmallIcon(), BRect(0.0, 0.0, 15.0, 15.0), itemColumnRect);

		owner->SetDrawingMode(B_OP_COPY);
	
	}
	// label column
	else
		if(columnIndex==2)
		{
			BPoint point=BPoint(itemColumnRect.left + 2.0, 
								   itemColumnRect.top + iTextOffset	);
			labelstart=point.x;
			if(selected)
				owner->SetHighColor(White);
			owner->SetDrawingMode(B_OP_OVER);
			owner->DrawString(iFile->Name(), point);
			owner->SetDrawingMode(B_OP_COPY);
itemColumnRect.PrintToStream();
		}
	
	owner->ConstrainClippingRegion(NULL);
	
	// use this hook function to capture changes in expansion
	if(IsExpanded() && !iTreeScan)
	{
		iTreeScan=true;

		// if the scan thread is running, wait for it to stop 
		// (BTW, this shouldn't happen since iTreeScan should guarantee it is only run once)
		RequestScanTerminate();

		// start thread
		thread_id threadId=spawn_thread(ScanThread, "TreeItem::ScanThread", 10, (void *) this);	
		resume_thread(threadId);
	}

/*	// update file view
	if(selected && !iFileScan)
	{
		iFileScan=true;
		BMessage *message=new BMessage(MSG_FILEVIEW_REFRESH);
		message->AddPointer("tree_item", (void *) this);
		message->AddRef("root", iFile->Ref());
		iTreeView->Looper()->PostMessage(message);
		delete message;
	
	}
	else
		if(!selected)
			iFileScan=false;
*/
}
		
// custom update
void STreeItem::Update(BView *owner, const BFont *font)
{
	CLVListItem::Update(owner, font);
	
	be_plain_font->GetHeight(&iFontAttr);
	
	float FontHeight=ceil(iFontAttr.ascent) + ceil(iFontAttr.descent);
	iTextOffset=ceil(iFontAttr.ascent) + ((Height()-FontHeight) / 2.0);
}

// update icon displayed (specifically for 'Trash')
void STreeItem::ResetIcon()
{
	iFile->ResetIcons();
	iTreeView->Invalidate();
}

// does the item have an active scan thread ?
bool STreeItem::IsScanning()
{
	sem_info semInfo;
	
	get_sem_info(iScanThreadActive, &semInfo);

	return (semInfo.count <=0)?true:false;
}	
		
// has the item been previously expanded ?
bool STreeItem::IsScanned()
{
	return iTreeScan;
}

// auto-select when scanning
void STreeItem::SetAutoSelectOnScanDir(const char *name)
{
	strcpy(iAutoSelectDir, name);
}

// expand the item and select a specific directory
void STreeItem::ExpandAndSelect(SFile *file)
{
	// wait for the scanning thread to stop
	acquire_sem(iScanThreadActive);
	release_sem(iScanThreadActive);	

	// has been expanded before ?
	bool wasScanned=IsScanned();
	if(!wasScanned)
	{
		// instruct thread to select the entry when it adds it to the list
		SetAutoSelectOnScanDir(file->Name());
	}
	
	// recursive expand
	BList *expandList=new BList();
	STreeItem *expandItem=this;
	
	while (true)
	{
		expandItem=(STreeItem *) iTreeView->Superitem(expandItem);

		// made it to the root?
		if(expandItem==0)
			break;
		
		if(!expandItem->IsExpanded())
			expandList->AddItem(expandItem);	
	}

	// go backwards through the list, expandin' all the way
	for (int32 i=expandList->CountItems(); i > 0; i--)
	{
		expandItem=(STreeItem *) expandList->ItemAt(i-1);	
		iTreeView->Expand(expandItem);
	}

	// finally, expand ourself
	iTreeView->Expand(this);
	
	// the item should already exist in the list
	if(wasScanned)
	{
		// scan all subitems
		for (int32 i=0; i < iTreeView->FullListNumberOfSubitems(this); i++)
		{
			int32 itemIndex=iTreeView->FullListIndexOf(this) +i +1;
			STreeItem *subItem=(STreeItem *) iTreeView->FullListItemAt(itemIndex);
			
			const entry_ref *subItemEntryRef=subItem->iFile->Ref();
	
			BNode primaryNode(file->Ref());
			BNode subItemNode(subItemEntryRef);

			if(primaryNode==subItemNode)
			{
				int32 visualIndex=iTreeView->IndexOf(subItem);
				iTreeView->Select(visualIndex);
				break;
			}
		}
	}
}

// threaded tree scan function
int32 STreeItem::ScanThread(void *dataPtr)
{
	STreeItem *item=(STreeItem *) dataPtr;

	// acquire lock on 'active' semaphore
	acquire_sem(item->iScanThreadActive);

	// if we didn't complete the scan the last time, remove the items
	sem_info semInfo;
	bool terminated=false;

	while (item->iTreeView->Window()->LockWithTimeout(10000)==B_TIMED_OUT) 
	{
		get_sem_info(item->iScanThreadTerminate, &semInfo);
		if(semInfo.count <=0) 
		{
			terminated=true;
			break;
		}
	}
	if(!terminated) 
	{
		
		// for some reason or another, if we delete old items from the 'Desktop'
		// item then the first volume will not show up...
		if(item->OutlineLevel() > 0) 
		{
	
			for (int32 i=0; i < item->iTreeView->FullListNumberOfSubitems(item); i++) 
			{
				int32 itemIndex=item->iTreeView->FullListIndexOf(item) + i + 1;
				STreeItem *subItem=(STreeItem *) item->iTreeView->FullListItemAt(itemIndex);
			
				item->iTreeView->RemoveItem(subItem);
				delete subItem;
			}
		}
		item->iTreeView->Window()->Unlock();

	}
	else
	{
		// lower flag
		release_sem(item->iScanThreadActive);
	
		return -1;
	}
	
	BDirectory directory;
	BEntry entry;
	int32 entry_count;

	// set directory to the entry represented by this item
	directory.SetTo(item->iFile->Ref());
	
	entry_count=directory.CountEntries();
	
	// scan entries
//	while (directory.GetNextEntry(&entry, true)==B_OK) 
	while (directory.GetNextEntry(&entry)==B_OK) 
	{

		// owner thread requested we abort ?
		get_sem_info(item->iScanThreadTerminate, &semInfo);
		if(semInfo.count <=0)
		{
			terminated=true;
			break;
		}

		// skip non-directories
		if(!entry.IsDirectory())
		{
			if(!entry.IsSymLink())
				continue;

			// don't skip symlinks pointed at directories
			entry_ref linkref;
			entry.GetRef(&linkref);
			entry.SetTo(&linkref,true);

			if(!entry.IsDirectory())
				continue;
			entry.SetTo(&linkref);
			
		}

		// if RequestScanTerminate is waiting for us to quit, it is
		// taking up a lock.  Hello deadlock.
		while (item->iTreeView->Window()->LockWithTimeout(10000)==B_TIMED_OUT)
		{
			get_sem_info(item->iScanThreadTerminate, &semInfo);
			if(semInfo.count <=0)
			{
				terminated=true;
				break;
			}
		}
		if(terminated) 
			break;

		// add item to tree under current directory
		SFile file(&entry);
		if(file.IsDesktop()) 
		{
			item->iTreeView->Window()->Unlock();
			continue;
		}
		
		STreeItem *subItem=new STreeItem(item->iTreeView, &file);																

		item->iTreeView->AddUnder(subItem, item);

		// this is so we can double-click on a directory in the file-view and have it
		// automagically be expanded & selected in the tree-view
		if(strlen(item->iAutoSelectDir)) 
		{
			if(strcasecmp(item->iAutoSelectDir, file.Name())==0) 
			{
				int32 itemIndex=item->iTreeView->IndexOf(subItem);
				item->iTreeView->Select(itemIndex);
				
				strcpy(item->iAutoSelectDir, "");
			}
		}
			
		item->iTreeView->Window()->Unlock();
	}

	directory.Unset();

	// we don't need this anymore
	strcpy(item->iAutoSelectDir, "");

	// try locking the window one more time so we can sort the new entries
	terminated=false;
	while (item->iTreeView->Window()->LockWithTimeout(10000)==B_TIMED_OUT)
	{
		get_sem_info(item->iScanThreadTerminate, &semInfo);
		if(semInfo.count <=0)
		{
			terminated=true;
			break;
		}
	}
	if(!terminated)
	{
		// sorting throws away the current selection, so...
		int32 selectIndx=item->iTreeView->FullListCurrentSelection();
		STreeItem *selectItem=(STreeItem *) item->iTreeView->FullListItemAt(selectIndx);	
	
		item->iTreeView->SortItems();
		
		int32 visualIndex=item->iTreeView->IndexOf(selectItem);
		item->iTreeView->Select(visualIndex);
		
		if(item->iTreeView->Window()->IsLocked())
			item->iTreeView->Window()->Unlock();
	}
	
	if(terminated)
	{
		// reset scan flag
		item->iTreeScan=false;
	}
	
	// lower flag
	release_sem(item->iScanThreadActive);
	
	return (terminated)?-1:0;
		
}

// request scan termination and wait for it to stop
void STreeItem::RequestScanTerminate()
{
	acquire_sem(iScanThreadTerminate);
		
	// sit and wait for the thread to end
	acquire_sem(iScanThreadActive);
	release_sem(iScanThreadActive);
		
	release_sem(iScanThreadTerminate);
}

STreeView::STreeView(BRect bounds, char *name)
 : ColumnListView(	bounds, 
 					&iContainerView, 
 					name,
 					B_FOLLOW_ALL,				// resizing mode
					B_WILL_DRAW | 	
					B_FRAME_EVENTS |			// flags		 
					B_NAVIGABLE,
					B_SINGLE_SELECTION_LIST,	// view type
					true,						// hierarchical	*/
					true,						// horizontal scrollbars
					true,						// vertical scrollbars
					true, 						// scroll-view corner
					B_FANCY_BORDER) 			// border style
{
	// build columns
	this->BuildColumns();
	
	// set sorting function
	SetSortFunction(Compare);

	// sort based on folder name
	SetSortKey(2);
	
	volumeRoster=new BVolumeRoster();
	previtem=-1;
	lastclick=-1;
	dblclick=false;
	olddropselect=-1;
	oldselected=false;	
}				

STreeView::~STreeView()
{
	delete volumeRoster;
	prefs->RemoveData("foldercol width");
	prefs->AddFloat("foldercol width",foldercolumn->Width());
}

void STreeView::MouseDown(BPoint point)
{
	if(!IsFocus())
		MakeFocus();
	
	BPoint tPoint;
	uint32 buttons;

	int32 itemIndex=IndexOf(point);

	// get mouse buttons
	GetMouse(&tPoint, &buttons, false);
	int32 clicks=1;
	bigtime_t thisclick=system_time();
	if( ((thisclick-lastclick) < 500000) && !dblclick)
	{
		clicks++;
		dblclick=true;
	}
	else
		dblclick=false;
		
	lastclick=thisclick;
	STreeItem *item=(STreeItem *) ItemAt(itemIndex);

	if(!item)
		return;
		
	if(item->ExpanderRectContains(point))
	{
		if(item->IsExpanded())
			Collapse(item);
		else
		{
			ScrollToItem(item);
			item->ExpandAndSelect(item->File());
		}
	}
	else
	{
		if(clicks==2  && item->IsSelected() && (buttons & B_PRIMARY_MOUSE_BUTTON))
		{
			// an item is selected if it has been clicked on once. If it isn't, then
			// the user has clicked on one and then another item. In this case,
			// we don't do doublelclick code and instead treat it as a quick
			// single click
			if( (itemIndex>=0))
			{
				if(item)
				{
					if(IsExpanded(itemIndex))
						Collapse(item);
					else
						item->ExpandAndSelect(item->File());
				}
			}
		}
		else
		{
			if(itemIndex>=0)
			{
				// select the item
				Select(itemIndex);
				ScrollToItem(item);
				if(buttons & B_SECONDARY_MOUSE_BUTTON)
				{
				
					// let the main window handle the menus
					BMessage *message;
					if(item->File()->IsTrash()) {
						message=new BMessage(MSG_PIONEER_TRASH_POPUP_MENU);
					} else if(item->IsVolume()) {
						message=new BMessage(MSG_PIONEER_VOLUME_POPUP_MENU);
					} else {
						message=new BMessage(MSG_PIONEER_FOLDER_POPUP_MENU);
					}
					message->AddPoint("point", ConvertToScreen(point));
					message->AddPointer("item", item);
					message->AddInt8("fileview",0);
					
					Window()->Looper()->PostMessage(message);
					delete message;
				}
			}
		}
	}
}

void STreeView::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
{
	if(msg)
	{
		int32 index=IndexOf(pt);
		if(transit==B_EXITED_VIEW || index==-1)
		{
			if(!oldselected)
				Deselect(olddropselect);
			olddropselect=-1;
		}
		else
		{
			if(index>=0)
			{
				if(!oldselected)
					Deselect(olddropselect);
				olddropselect=index;
				
				BListItem *item=ItemAt(index);
				oldselected=(item)?item->IsSelected():true;

				Select(index,true);
			}
			else
			{
				BListItem *item=ItemAt(index);
				oldselected=(item)?item->IsSelected():true;

				olddropselect=index;
			}
		}
	}
}

void STreeView::MouseUp(BPoint point)
{
	if(oldselected)
		Deselect(olddropselect);

	oldselected=false;
	olddropselect=-1;
}

void STreeView::SelectionChanged(void)
{
	int32 index=CurrentSelection();
	if(index<0)
		return;

	// this allows us to show the drop target without refresh while doing drag & drop
	if(olddropselect>=0)
		return;

	if(previtem==index)
		return;
	
	STreeItem *item=(STreeItem *)ItemAt(index);
	if(!item)
		return;
	
	BMessage *message=new BMessage(MSG_FILEVIEW_REFRESH);
	message->AddPointer("tree_item", (void *) item);
	message->AddRef("root", item->File()->Ref());
	Looper()->PostMessage(message);
	delete message;

	previtem=index;
}

void STreeView::KeyDown(const char *bytes,int32 numbytes)
{
	if(numbytes==1)
	{
		if(IsFilenameChar(bytes[0]))
		{
//			STreeItem *item=(STreeItem*)ItemAt(CurrentSelection());
//			Select(FindNextAlphabetical(bytes[0],CurrentSelection()));
			
			// Jump to the next item beginning with the key that was struck and then expand it
			int32 newindex=FindNextAlphabetical(bytes[0],CurrentSelection());
			STreeItem *item=(STreeItem*)ItemAt(newindex);
			if(item)
			{
				// uncomment this line if you want to autoexpand the keyselected item
//				item->ExpandAndSelect(item->File());
				Select(newindex);
				ScrollToItem(item);
			}
			
		}
		else
		switch(bytes[0])
		{
			// contract item or move up to parent item if contracted
			case B_LEFT_ARROW:
			{
				STreeItem *item=(STreeItem*)ItemAt(CurrentSelection());
				if(!item)
					break;
					
				if(item->IsExpanded())
					Collapse(item);
				else
				{
					// This is a bit of a hack because SuperItem doesn't
					// seem to work for ColumnListViews
//					int32 index=FullListIndexOf(item);
//					Select(index-1);
					STreeItem *parent=(STreeItem*)Superitem(item);
					if(parent)
						Select(IndexOf(parent));

/*					// Autoscroll the treeview as necessary
					BRect labelframe(ItemFrame(IndexOf(parent)));
					labelframe.left=parent->LabelStart();
					labelframe.right=labelframe.left+parent->LabelWidth();
					if(parent->OutlineLevel()>0)
						ScrollTo(BPoint(labelframe.left/2,labelframe.top));
					else
						ScrollTo(BPoint(0,labelframe.top));
*/
					ScrollToItem(parent);
				}
				break;
			}
			// expand item or move to first child item if expanded and one exists
			case B_RIGHT_ARROW:
			{
				STreeItem *item=(STreeItem*)ItemAt(CurrentSelection());
				if(!item)
					break;
					
				if(item->IsExpanded())
				{

					// This is a bit of a hack because ItemUnderAt isn't defined
					// for ColumnListViews
					int32 index=FullListIndexOf(item);
					Select(index+1);
					
/*					// Autoscroll the treeview as necessary
					BRect labelframe(ItemFrame(CurrentSelection()));
					labelframe.left=item->LabelStart();
					labelframe.right=labelframe.left+item->LabelWidth();
					if(item->OutlineLevel()>0)
						ScrollTo(BPoint(labelframe.left/2,labelframe.top));
*/
					ScrollToItem(item);

				}
				else
					Expand(item);
				break;
			}
			case B_UP_ARROW:
			case B_DOWN_ARROW:
			case B_HOME:
			case B_END:
			{
				ColumnListView::KeyDown(bytes, numbytes);
				ScrollToItem((STreeItem*)ItemAt(CurrentSelection()));
				break;
			}
			case B_DELETE:
			{
				Window()->PostMessage(MID_FILE_MOVE_TO_TRASH);
				break;
			}
			
			// Toggle expansion on these keys
			case B_SPACE:
			case B_ENTER:
			{
				CLVListItem *item=(CLVListItem*)ItemAt(CurrentSelection());
				if(!item)
					break;
					
				if(item->IsExpanded())
					Collapse(item);
				else
					Expand(item);
				break;
			}
			default:
				ColumnListView::KeyDown(bytes,numbytes);
		}
	}
	else
	{
		// Call the inherited version for everything that we don't specifically support
		ColumnListView::KeyDown(bytes,numbytes);
	}
}

//bool STreeView::InitiateDrag(BPoint pt, int32 index, bool selected)
bool STreeView::StartDrag(BPoint pt, int32 index, bool selected)
{
	BMessage *refmsg=new BMessage(B_SIMPLE_DATA);
	
	STreeItem *treeitem=(STreeItem*)ItemAt(CurrentSelection());
	SFile *file=treeitem->File();
	if(!file->Ref())
	{
		delete refmsg;
		return false;
	}
	refmsg->AddRef("refs",file->Ref());

	BRect r(ItemFrame(index));
	DragMessage(refmsg,r);
	return true;
}

void STreeView::ScrollToItem(STreeItem *item)
{
	// Scroll the item to the center of the view for maximum visibility

	// Get the visible region's size
	BRect bounds=Parent()->Bounds();
	BRect labelframe(ItemFrame(IndexOf(item)));
	labelframe.left=((item->OutlineLevel()+3)*21);
	labelframe.right=labelframe.left+item->LabelWidth();
	float scrollx,scrolly;

	// calculate the horizontal scroll value
	scrollx=labelframe.left-65;
	scrolly=labelframe.top-((bounds.Height()-labelframe.Height())/2);

	ScrollTo(BPoint(scrollx,scrolly));
}

void STreeView::Expand(CLVListItem* item)
{
	ColumnListView::Expand(item);
	STreeItem *i=(STreeItem*)item;
	float w=i->LabelStart()+i->LabelWidth();
	
	CLVColumn *col=ColumnAt(2);	// folder column

	if(col && w>col->Width())
		col->SetWidth(w);
}

void STreeView::Collapse(CLVListItem* item)
{
	if(HasItemUnder((STreeItem*)item,(STreeItem*)ItemAt(CurrentSelection())))
		Select(IndexOf((BListItem*)item));
	ColumnListView::Collapse(item);
}

void STreeView::BuildColumns() 
{
	// create expander column
	AddColumn(new CLVColumn(NULL,						// label 
							  20.0,						// width
							  CLV_EXPANDER | 			
							  CLV_LOCK_AT_BEGINNING | 	// flags
							  CLV_NOT_MOVABLE));
	
	// create icon column						  
	AddColumn(new CLVColumn(NULL,
							  20.0,
							  CLV_LOCK_AT_BEGINNING |
							  CLV_NOT_MOVABLE | 
							  CLV_NOT_RESIZABLE | 
							  CLV_PUSH_PASS | 
							  CLV_MERGE_WITH_RIGHT));
	// create folder name column
	foldercolumn=new CLVColumn("Folder",
							  200.0,
					  	  	  CLV_SORT_KEYABLE,
							  50.0);
	AddColumn(foldercolumn);
	float ftemp=0;
	if(prefs->FindFloat("foldercol width",&ftemp)==B_OK)
		foldercolumn->SetWidth(ftemp);
}

// sorting function
int STreeView::Compare(const CLVListItem* aItem1, const CLVListItem* aItem2, int32 sortKey)
{
	STreeItem *item1=(STreeItem *) aItem1;
	STreeItem *item2=(STreeItem *) aItem2;

	// Sort volumes first
	if(item1->File()->IsVolume() && item2->IsVolume())
		// sort volumes by name
		return strcasecmp(item1->File()->Name(), item2->File()->Name());

	if(item1->File()->IsVolume())
		return -1;
	if(item2->File()->IsVolume())
		return 1;

	return strcasecmp(item1->File()->Name(), item2->File()->Name());
}

// refresh tree
void STreeView::Refresh()
{
	// save entries that are expanded
	// ...
	
	// delete old items
	STreeItem *oldItem;
	uint32 i=0;
	oldItem=(STreeItem *) FullListItemAt(i++);

	// Necessary to prevent a crash -> startup + refresh = *CRASH*
	DeselectAll();

	while(oldItem)
	{
		delete oldItem;
		oldItem=(STreeItem *) FullListItemAt(i++);
	}

	// clear list
	MakeEmpty();

	// add Desktop root
	BEntry entry(DESKTOP_ENTRY);
	SFile file(&entry); 
	STreeItem *desktopItem=new STreeItem(this, &file, false);
    AddItem(desktopItem);
    entry.Unset();
 
    // add volumes
	BVolume volume;
   	char volName[B_FILE_NAME_LENGTH];

	volumeRoster->Rewind();
    while(volumeRoster->GetNextVolume(&volume)==B_NO_ERROR)
    {
    	// skipped unmounted volumes
    	volume.GetName(volName);
    	if(strlen(volName)==0)
    		continue;
    	SFile volRoot(&volume);

    	STreeItem *volumeItem=new STreeItem(this, &volRoot);
    	AddUnder(volumeItem, desktopItem);
    }
    
    // select and expand desktop entry on refresh
	MakeFocus();
	Select(0);
	Expand(desktopItem);

    // re-expand previous selections
    // ...
}

void STreeView::AbortScan()
{
	BEntry entry(DESKTOP_ENTRY);
	SFile file(&entry); 
	STreeItem *desktopItem=new STreeItem(this, &file, false);
    entry.Unset();

	desktopItem->RequestScanTerminate();
	delete desktopItem;
}

// current selections
void STreeView::CurrentSelections(SFileList *fileList)
{
	if(!fileList)
		return;

	for (int32 i=0; i < CountItems(); i++)
	{
		STreeItem *item=(STreeItem *) ItemAt(i); 
		if(item->IsSelected())
			fileList->AddItem((void *) item->File());
	}
}

// expand and select
void STreeView::ExpandAndSelect(BMessage *message)
{
	if(!message)
		return;

	entry_ref entryRef;
	if(message->FindRef("dir", 0, &entryRef) !=B_OK)
		return;

	STreeItem* treeItem;
	if(message->FindPointer("tree_item", 0, (void **) &treeItem) !=B_OK)
		return;

	SFile file(&entryRef);
	if(treeItem)
		treeItem->ExpandAndSelect(&file);
}

void STreeView::ExpandAndSelect(entry_ref ref)
{
	// This function finds the volume on which the ref exists and expands to that
	// folder.
}

// node monitor: scan for created
void STreeView::ScanForCreated(SFile *file)
{
	entry_ref entryRef=*file->Ref();

	// find the parent of the entry
	BEntry childEntry(&entryRef);
	
	// make sure the is a directory (or symbolic link to a possible directory)
	if(!childEntry.IsDirectory() && !childEntry.IsSymLink())
		return;
	
	BEntry parentEntry;
	childEntry.GetParent(&parentEntry);
	
	entry_ref parentEntryRef;
	parentEntry.GetRef(&parentEntryRef);
	
	// find the parent in the tree by comparing entry_ref's
	STreeItem *item;
	bool found=false;
	
	for (uint32 i=0; (item=(STreeItem *) FullListItemAt(i)); i++)
	{
		// found it ?
		if(*item->File()->Ref()==parentEntryRef)
		{
			found=true;
			break;
		}
	}
		
	if(file->IsVolume())
	{
		entryRef.set_name(file->Name());
		item=(STreeItem*)ItemAt(0);
	}
	else
	{
		if(!found)
		{
			// there must be something wrong...
			printf("Scan for created - entry not found\n");
			return;
		}
		
		// if we haven't scanned it yet, why bother?
		if(!item->IsScanned())
		{
			printf("Scan for created - item not scanned\n");
			return;
		}
	}

	// check to see if we've already added it to the list
	// this prevents duplicate entries from being created
	// when we rename a folder
	STreeItem *existItem;
	found=false;
	
	for (uint32 i=0; (existItem=(STreeItem *) FullListItemAt(i)); i++)
	{
		// found it ?
		if(strcasecmp(existItem->File()->Name(), entryRef.name)==0)
		{
			found=true;
			break;
		}
	}

	if(found)
		return;

	// add the entry
	STreeItem *subItem=new STreeItem(this, file);																
	AddUnder(subItem, item);
	
	SortItems();
}

// node monitor: scan for moved
void STreeView::ScanForMoved(SFile *fromFile,SFile *toFile)
{
	// use the standard creation routine
	ScanForCreated(toFile);
	ScanAllForRemoved();
}

// node monitor: scan for removed
void STreeView::ScanAllForRemoved()
{
	STreeItem *item;

	for (uint32 i=0; (item=(STreeItem *) FullListItemAt(i)); i++)
	{
		if(!item->File()->Exists())
		{
			// refresh the file view if the item is selected
/*			if(item->IsSelected())
			{
				BMessage *message=new BMessage(MSG_FILEVIEW_CLEAR);
				Looper()->PostMessage(message);
				delete message;
			}
*/
			// if the item is selected, we are deleting a folder, so select the previous one in
			// the list.
			if(item->IsSelected())
				Select(CurrentSelection()-1);
			
			RemoveItem(item);
			delete item;
		}	
	}
}

// node monitor: update trash icon
void STreeView::ScanUpdateTrashIcon()
{
	// find the 'Trash' folder in the first level
	CLVListItem *desktopItem=FullListItemAt(0);
	for (int32 i=0; i<FullListNumberOfSubitems(desktopItem); i++)
	{
		STreeItem *subItem=(STreeItem *) FullListItemAt(i);
		if(subItem->File()->IsTrash())
			subItem->ResetIcon();
	}
}

void STreeView::SpawnTracker()
{
	STreeItem *item=(STreeItem*)ItemAt(CurrentSelection());

	if(!item)
		return;

	BString string("/boot/beos/system/Tracker '");
	string+=item->File()->PathDesc();
	string+="' &";
	system(string.String());
}

bool STreeView::HasItemUnder(STreeItem *item, STreeItem *subitem)
{
	if(!item || !subitem)
		return false;
	
	uint32 level=subitem->OutlineLevel();

	STreeItem *parent=(STreeItem*)subitem;
	
	while(level>item->OutlineLevel())
	{
		parent=(STreeItem*)Superitem(parent);
		if(parent==item)
			return true;
		level--;
	}
	return false;
}

STreeItem *STreeView::FindItem(const char *path, int32 offset)
//STreeItem *STreeView::FindItem(const char *path, int32 offset=0)
{
	int32 count=CountItems()-1;
	if(offset>count)
		return NULL;
	
	STreeItem *item;
	for(int32 i=offset; i<count; i++)
	{
		item=(STreeItem*)ItemAt(i);
		if(item && item->File())
		{
			if(strcmp(item->File()->PathDesc(), path)==0)
				return item;
		}
	}
	return NULL;
}

int32 STreeView::FindNextAlphabetical(char c, int32 index)
{
	STreeItem *fileitem;
	char name[255];
	if(index==-1)
		index=0;
	
	if(!IsFilenameChar(c))
		return -1;

	int32 i=index+1;
	while(i!=index)
	{
		if(i==CountItems())
			i=0;
		fileitem=(STreeItem*)ItemAt(i);
		if(fileitem && fileitem->File())
		{
			strcpy(name,fileitem->File()->Name());
			if(charncmp(name[0],c)==0)
				return i;
		}
		i++;
	}
	return -1;
}
