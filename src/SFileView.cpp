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
#include <fs_attr.h>
#include <NodeInfo.h>
#include <Alert.h>
#include <Font.h>
#include <Directory.h>
#include <File.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <TranslationUtils.h>
#include <View.h>

#include "Colors.h"
#include "CLVColumn.h"
#include "SafeTime.h"

#include "SeekerWindow.h"
#include "StatusWindow.h"
#include "STreeView.h"
#include "SeekerApp.h"
#include "RectTools.h"
#include "FSUtils.h"

#include "SFileView.h"

SFileItem::SFileItem(SFileView *fileView, const SFile *file)
 : SeekerItem(file,0, false, false, 20.0),	// 20.0 is minimum row height
   iFileView(fileView),
   iTextOffset(0.0)
{
	// copy entry
	labelwidth=-1;
	labelrect.Set(0,0,0,0);
	if(labelwidth<0)
	{
		BFont font;
		iFileView->GetFont(&font);
		labelwidth=font.StringWidth(iFile->Name());
	}
}

SFileItem::~SFileItem()
{
}

void SFileItem::DrawItemColumn(BView *owner, BRect itemColumnRect, 
								int32 columnIndex, bool complete)
{
	rgb_color color;
	bool selected=IsSelected();
	bool drawlink=false;
	
	color=(selected)?Black:White;
	
	owner->SetLowColor(color);
	owner->SetDrawingMode(B_OP_COPY);

	if((selected || complete) && (columnIndex==1))
	{
		owner->SetHighColor(color);

		BRect rect=itemColumnRect;
		rect.left -=1.0;
		rect.top +=iTextOffset - ceil(iFontAttr.ascent);
		rect.bottom -=((iTextOffset - ceil(iFontAttr.ascent)) / 2.0) + 2.0;
		rect.right=rect.left + owner->StringWidth(iFile->Name()) + 3.0;
		
		owner->FillRect(rect);
		owner->StrokeRect(rect,B_MIXED_COLORS);
	}

	
	BRegion Region;
	Region.Include(itemColumnRect);

	owner->ConstrainClippingRegion(&Region);
	owner->SetHighColor(Black);

	// icon column
	if(columnIndex==0)
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
	else
	{
		owner->SetLowColor(White);
		owner->SetHighColor(Black);

		char strg[400];
		strcpy(strg, "");
		
		uint32 align=B_ALIGN_LEFT;
		switch (columnIndex)
		{
		
			// name column
			case 1 :
			{
				if(iFile->IsLink())
					drawlink=true;
				if(selected)
				{
					owner->SetLowColor(Black);
					owner->SetHighColor(White);
				}
				strcpy(strg, iFile->Name());

//				BFont font;
//				iFileView->GetFont(&font);
//				labelwidth=font.StringWidth(iFile->Name());
				labelrect=itemColumnRect;
				labelrect.right=labelrect.left+labelwidth+5;
				break;
			}	
			// size column
			case 2 :
				align=B_ALIGN_RIGHT;
				iFile->SizeDesc(strg);
				break;
				
			// modified column
			case 3 :
				iFile->ModifiedTmDesc(strg);
				break;
				
			// created column
			case 4 :
				iFile->CreatedTmDesc(strg);
				break;
				
			// kind column
			case 5 :
				strcpy(strg, iFile->MimeDesc());
				break;
				
			// path column
			case 6 :
				strcpy(strg, iFile->PathDesc());
				break;	
		}
		
		BPoint point;
		if(align==B_ALIGN_LEFT)
		{
			point=BPoint(itemColumnRect.left + 2.0, itemColumnRect.top + iTextOffset);
		}
		else
		if(align==B_ALIGN_RIGHT)
		{
			BFont font;
			iFileView->GetFont(&font);
			float stringWidth=font.StringWidth(strg);
			float offset=itemColumnRect.right - stringWidth - 2.0;
			offset=(offset>0)?offset:0;
			point=BPoint(offset,itemColumnRect.top + iTextOffset);
		}
		
		if(strg)
		{
			if(drawlink)
			{
				owner->DrawString(strg, point);
				owner->StrokeLine(BPoint(point.x, point.y+2),
					BPoint(point.x+LabelWidth(),point.y+2), B_MIXED_COLORS);
			}
			else
				owner->DrawString(strg, point);

		}
	}
	
	owner->ConstrainClippingRegion(NULL);
}
		
void SFileItem::Update(BView *owner, const BFont *font)
{
	CLVListItem::Update(owner, font);
	
	be_plain_font->GetHeight(&iFontAttr);
	
	float FontHeight=ceil(iFontAttr.ascent) + ceil(iFontAttr.descent);
	iTextOffset=ceil(iFontAttr.ascent) + ((Height()-FontHeight) / 2.0);
}

SFileView::SFileView(BRect bounds, char *name)
 : ColumnListView(	bounds, 
 					&iContainerView, 
 					name,
 					B_FOLLOW_ALL,				// resizing mode
					B_WILL_DRAW | 	
					B_FRAME_EVENTS |			// flags		 
					B_NAVIGABLE,
					B_MULTIPLE_SELECTION_LIST,	// view type
					false,						// hierarchical
					true,						// horizontal scrollbars
					true,						// vertical scrollbars
					false,						// scroll-view corner
					B_FANCY_BORDER),			// border style
   iScanThreadId(0),
   iScanThreadActive(0),
   iScanThreadTerminate(0),
   iTreeItem(0), 
   iColIcon(0),
   iColName(0),
   iColSize(0),
   iColModified(0),
   iColCreated(0),
   iColKind(0),
   iColPath(0),
   iFileScan(false)
{
	// Get columns to be displayed
	if(prefs->FindBool("usename",&iUseName)!=B_OK)
		iUseName=true;
	if(prefs->FindBool("usesize",&iUseSize)!=B_OK)
		iUseSize=true;
	if(prefs->FindBool("usemodified",&iUseModified)!=B_OK)
		iUseModified=true;
	if(prefs->FindBool("usecreated",&iUseCreated)!=B_OK)
		iUseCreated=false;
	if(prefs->FindBool("usekind",&iUseKind)!=B_OK)
		iUseKind=false;

	// set sorting function
	SetSortFunction(Compare);

	// build columns
	BuildColumns();
	LoadColumnSettings();
	
	// create and set invoke message
	SetInvocationMessage(new BMessage(MSG_FILEVIEW_INVOKE));
		
	// create semaphores
	iScanThreadActive=create_sem(1, "FileView::ScanThreadActive");
	iScanThreadTerminate=create_sem(1, "FileView::ScanThreadTerminate");
	
	// Miscellaneous initializations
	mousebuttons=0;
	mdownpt.Set(0,0);
	mdownitem=NULL;
	mdownindex=-1;
	folderbitmap=false;
	isdragging=false;
	isselecting=false;
	istracking=false;
	currentitem=-1;
	olddropselect=-1;
	oldselected=false;
	lastclick=-1;
	dblclick=false;
}				

SFileView::~SFileView()
{
	prefs->RemoveData("usename");
	prefs->RemoveData("usesize");
	prefs->RemoveData("usemodified");
	prefs->RemoveData("usecreated");
	prefs->RemoveData("usekind");
	prefs->AddBool("usename",iUseName);
	prefs->AddBool("usesize",iUseSize);
	prefs->AddBool("usemodified",iUseModified);
	prefs->AddBool("usecreated",iUseCreated);
	prefs->AddBool("usekind",iUseKind);

	SaveColumnSettings();
	// make sure we stop the scan thread before we go and delete ourselves
	RequestScanTerminate();

	if(iScanThreadActive) delete_sem(iScanThreadActive);
	if(iScanThreadTerminate) delete_sem(iScanThreadTerminate);	
}

void SFileView::Draw(BRect update)
{
	float l,r,t,b;
	l=(mdownpt.x<currentpt.x)?mdownpt.x:currentpt.x;
	t=(mdownpt.y<currentpt.y)?mdownpt.y:currentpt.y;
	r=(mdownpt.x>currentpt.x)?mdownpt.x:currentpt.x;
	b=(mdownpt.y>currentpt.y)?mdownpt.y:currentpt.y;
	BRect rect(l,t,r,b);
	
	if(isselecting)
		FillRect(rect.InsetByCopy(1,1),B_SOLID_LOW);
	ColumnListView::Draw(update);
	if(isselecting)
		StrokeRect(rect,B_MIXED_COLORS);
}

void SFileView::MouseDown(BPoint point)
{
	if(!IsFocus())
		MakeFocus();

	BPoint tPoint;
	mdownpt=point;
	currentpt=point;
	mdownitem=(SFileItem*)ItemAt(IndexOf(point));
	mdownindex=IndexOf(mdownitem);

	// get mouse buttons
	GetMouse(&tPoint, &mousebuttons, false);
	int32 clicks=1;
	bigtime_t thisclick=system_time();
	if( ((thisclick-lastclick) < 500000) && (mdownindex==lastitem) && !dblclick)
	{
		clicks++;
		dblclick=true;
	}
	else
		dblclick=false;
		
	lastclick=thisclick;

	// It seems we've reinvented the wheel, but it is necessary to allow for
	// later choice of BeOS or Windows UI style.
	
	// As it stands:
	// Shift key - select all between the previous non-Shift click and the current
	// Option key - invert the selection of the clicked item (only)



	if(mdownitem)
	{
		if(dblclick)
		{
			MouseUp(point);
			Invalidate();
			Invoke();
			return;
		}
		else
		{
			BRect r=mdownitem->LabelRect();
			r.left=0;
			if(r.Contains(point))
			{
				if(mousebuttons & B_PRIMARY_MOUSE_BUTTON)
				{
					uint32 mod=modifiers();
				
					if(mod & B_OPTION_KEY)
					{		
						if(mdownitem->IsSelected())
							Deselect(mdownindex);
						else
						{
							Select(mdownindex,true);
							lastitem=mdownindex;
						}
					}
					else
					{
						if(mod & B_SHIFT_KEY)
						{
							if(lastitem!=-1)
							{
								int32 start=MIN(lastitem,mdownindex);
								int32 end=MAX(lastitem,mdownindex);
								SelectRange(start,end);
							}
							else
							{
								Select(mdownindex);
								lastitem=mdownindex;
							}
						}
						else
						{
							// no significant modifiers
							if(!mdownitem->IsSelected())
							{
								Select(mdownindex);
								lastitem=mdownindex;
							}
						}
					}
				}  // if B_PRIMARY_MOUSE_BUTTON
				else
				if(mousebuttons & B_SECONDARY_MOUSE_BUTTON)
				{
					Select(mdownindex,true);
					lastitem=mdownindex;
				}
			} // if labelrect contains point
			else
			{
				DeselectAll();
				lastitem=-1;
			}
		} // else not doubleclick
	}
	else
	{
		// didn't click an item, so deselect everything
		DeselectAll();
		lastitem=-1;
	}
//	SetMouseEventMask(B_POINTER_EVENTS,B_LOCK_WINDOW_FOCUS);
}

void SFileView::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
{
	float dx=pt.x-mdownpt.x, dy=pt.y-mdownpt.y;
	currentpt=pt;

	// Hack to work around problems with SetMouseEventMask
	if(isselecting && transit==B_EXITED_VIEW)
	{
		pt.ConstrainTo(Bounds());
		MouseUp(pt);
		Invalidate();
		return;
	}

	// drag and drop code
	if(msg)
	{
//		SetEventMask(B_POINTER_EVENTS,0);
		int32 index=IndexOf(pt);

		// This code is for drop target hightlighting
		if( olddropselect>=0 && ( (transit==B_EXITED_VIEW || transit==B_OUTSIDE_VIEW) ||
			(index==-1) ) )
		{
			if(!oldselected)
				Deselect(olddropselect);
			olddropselect=-1;
		}
		else
		{
			if((transit==B_ENTERED_VIEW || transit==B_INSIDE_VIEW))
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
					olddropselect=index;
					BListItem *item=ItemAt(index);
					oldselected=(item)?item->IsSelected():true;
				}
			}
		}
	}

	// Code to initiate dragging or drag selections
	if(mousebuttons && (dx<-4 || dx>4 || dy<-4 || dy>4) && !isdragging)
	{
		if(mdownitem && mdownitem->LabelRect().Contains(pt))
		{
//			SetEventMask(B_POINTER_EVENTS,0);
			StartDrag(pt,IndexOf(mdownitem),true);
		}
		else
		{
//			StartTracking(BRect(mdownpt,pt),B_TRACK_RECT_CORNER);
			isselecting=true;
			lastitem=(mdownindex<0)?CountItems()-1:mdownindex;
			Draw(Bounds());
		}
		isdragging=true;
	}
	
	// Continues a drag selection
	if(isselecting)
	{
		int32 index=IndexOf(pt);
		if(index!=-1)
		{
			if(currentitem!=lastitem)
				Deselect(currentitem);
			currentitem=index;

			int32 start=MIN(lastitem, currentitem);
			int32 end=MAX(lastitem, currentitem);

			Select(start,end);
		}
		Draw(Bounds());
	}
}

void SFileView::MouseUp(BPoint point)
{
	currentpt=point;
	// right mouse button (only) ?
	if( (mousebuttons & B_SECONDARY_MOUSE_BUTTON) && !isdragging)
	{

		BMessage *message;

		if(mdownitem==NULL)
		{
			message=new BMessage(MSG_PIONEER_FOLDER_POPUP_MENU);
			message->AddPoint("point", ConvertToScreen(point));
			message->AddPointer("item", iTreeItem);
			message->AddInt8("fileview",0);
			
			Window()->Looper()->PostMessage(message);
			delete message;
			mousebuttons=0;
			return;
		}
		// grab focus
		MakeFocus();

		SFileItem *item=mdownitem;
	
		// select the item if it is part of a selected group
		if(!item->IsSelected())
			Select(IndexOf(mdownitem));

		bool folderOnly=false;
		
		SFileList fileList;
		CurrentSelections(&fileList);
		if(fileList.CountItems()==1)
		{
			SFile *file=(SFile *) fileList.ItemAt(0);
			folderOnly=file->IsDirectory();
		}

		bool hvTrash=false;
		
		for (int32 i=0; i < fileList.CountItems(); i++)
		{
			SFile *file=(SFile *) fileList.ItemAt(i);
			if(file->IsTrash())
			{
				hvTrash=true;
				break;
			}
		}		

		// let the main window handle the menus
		STreeItem *currentFolder=(STreeItem *) iTreeItem;
		SFileItem *fileitem=(SFileItem*)ItemAt(CurrentSelection());

		if(currentFolder->File()->IsTrash() || hvTrash)
			message=new BMessage(MSG_PIONEER_TRASH_POPUP_MENU);			
		else if(fileitem->File()->TargetType()==TYPE_DIRECTORY)
			message=new BMessage(MSG_PIONEER_FOLDER_POPUP_MENU);
		else
			message=new BMessage(MSG_PIONEER_FILE_POPUP_MENU);
		message->AddPoint("point", ConvertToScreen(point));
		message->AddPointer("item", item);
		message->AddInt8("fileview",1);
		
		Window()->Looper()->PostMessage(message);
		delete message;
	}

//	SetEventMask(B_POINTER_EVENTS,0);
	if(isdragging)
	{
//		Deselect(olddropselect);
		isdragging=false;
	}

	mousebuttons=0;
	mdownitem=NULL;
	mdownindex=-1;
	if(isselecting)
	{
		isselecting=false;
//		EndTracking();
		currentitem=-1;
		float l,r,t,b;
		l=(mdownpt.x<currentpt.x)?mdownpt.x:currentpt.x;
		t=(mdownpt.y<currentpt.y)?mdownpt.y:currentpt.y;
		r=(mdownpt.x>currentpt.x)?mdownpt.x:currentpt.x;
		b=(mdownpt.y>currentpt.y)?mdownpt.y:currentpt.y;
		BRect rect(l,t,r,b);
		FillRect(rect,B_SOLID_LOW);
		Draw(rect);
	}
}

void SFileView::KeyDown(const char *bytes,int32 numbytes)
{
	if(numbytes==1)
	{
		if(IsFilenameChar(bytes[0]))
		{
			Select(FindNextAlphabetical(bytes[0],CurrentSelection()));
			ScrollToSelection();
		}
		switch(bytes[0])
		{
			// contract item or move up to parent item if contracted
			case B_DELETE:
			{
				if(PathAsFile()->IsTrash())
				{
					// user is in the Trash and wishes to remove some files
					for(int32 i=0;i<CountItems();i++)
					{
						SFileItem *item=(SFileItem*)ItemAt(i);
						if(item && item->IsSelected())
						{
							BEntry entry(item->File()->PathDesc());
							entry.Remove();
						}
					}
					ScanAllForRemoved();
				}
				Window()->PostMessage(MID_FILE_MOVE_TO_TRASH);
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


//bool SFileView::InitiateDrag(BPoint pt, int32 index, bool selected)
bool SFileView::StartDrag(BPoint pt, int32 index, bool selected)
{
	// We have our own drag hook because we don't want to use the default
	// drag behavior
	BMessage *refmsg=new BMessage(B_SIMPLE_DATA);
	
	SFileList *list=new SFileList(0);
	CurrentSelections(list);
	SFile*file;

	for(int32 i=0; i<list->CountItems();i++)
	{
		file=(SFile*)list->ItemAt(i);
		if(file)
			refmsg->AddRef("refs",file->Ref());
	}
	list->MakeEmpty();			
	delete list;

	BRect r;
	SFileItem *item=(SFileItem*)ItemAt(index);
	if(item)
		r=item->LabelRect();
	else
		r=ItemFrame(index);
	DragMessage(refmsg,r);

	// we need to return false because we will call this when we need to,
	// but we don't want left-click dragging.
	return true;
}

void SFileView::MessageReceived(BMessage *msg)
{
	if(msg->what==B_SIMPLE_DATA)
		Window()->PostMessage(msg);
	else
		ColumnListView::MessageReceived(msg);
}

// build columns
void SFileView::BuildColumns() 
{

	// create icon column
	if(!iColIcon)
	{						  
		iColIcon=new CLVColumn(	NULL, 20.0,
			CLV_LOCK_AT_BEGINNING |	CLV_NOT_MOVABLE | 
			CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT);
		this->AddColumn(iColIcon);
	}
	
	// name column
	if(!iColName)
	{
		iColName=new CLVColumn("Name",110.0, CLV_NOT_MOVABLE | CLV_SORT_KEYABLE,50.0);
		this->AddColumn(iColName);
	}
	iColName->SetShown(iUseName);
	
	// size column
	if(!iColSize)
	{
		iColSize=new CLVColumn("Size",80.0,CLV_SORT_KEYABLE | CLV_RIGHT_JUSTIFIED,
			50.0);
		this->AddColumn(iColSize);
	
	}
	iColSize->SetShown(iUseSize);
		
	// modified column
	if(!iColModified)
	{
		iColModified=new CLVColumn("Modified",140.0,CLV_SORT_KEYABLE,50.0);
		this->AddColumn(iColModified);
	}
	iColModified->SetShown(iUseModified);

	// created column
	if(!iColCreated)
	{
		iColCreated=new CLVColumn("Created",140.0,CLV_SORT_KEYABLE,50.0);
		this->AddColumn(iColCreated);
	}
	iColCreated->SetShown(iUseCreated);
	
	// kind column
	if(!iColKind)
	{
		iColKind=new CLVColumn("Kind",80.0,CLV_SORT_KEYABLE,50.0);
		this->AddColumn(iColKind);
	}
	iColKind->SetShown(iUseKind);
		
	// path column
	if(!iColPath)
	{
		iColPath=new CLVColumn("Path",120.0,CLV_SORT_KEYABLE,50.0);
		this->AddColumn(iColPath);
	}
	iColPath->SetShown(iUsePath);
	
}

void SFileView::LoadColumnSettings()
{
	// Adjust columns based on what settings are in the saved preferences
	CLVColumn *col;
	float ftemp;
	int8 temp8;
	int32 columns[7],temp32,i;
	char colstr[20];
	CLVSortMode sortmodes[8];

	// Get and set the column display order

	// We either get *all* of the display order or none of it	
	for(i=0;i<8;i++)
	{
		sprintf(colstr,"col%ld order",i);
		if(prefs->FindInt32(colstr,&temp32)!=B_OK)
			break;
		columns[i]=temp32;
	}
	if(i==7)
		SetDisplayOrder(columns);

	// Get and set sort data all at once - loading preferences takes forever
	// if done otherwise
	for(i=0;i<8;i++)
	{
		col=ColumnAt(i);
		if(!col)
			continue;
		
		sprintf(colstr,"col%ld sorting",i);
		if(prefs->FindInt8(colstr,&temp8)==B_OK)
		{
			if(temp8==1)
				sortmodes[i]=Ascending;
			else
			if(temp8==2)
				sortmodes[i]=Descending;
			else
				sortmodes[i]=NoSort;

			// All this crazy stuff here sets the sorting for each column
			// while only doing the sort the very last time and reversing
			// the sort order if sort is set to Descending
			if(temp8>0)
			{
				if(i==0)
					SetSortKey(0,false);
				else
				{
					if(i==7)
						AddSortKey(i,(temp8==2)?false:true);
					else
						AddSortKey(i,false);
				}

				if(temp8==2)
					ReverseSortMode(i,false || i==7);
			}
		}

	}
	
	// Get the other column data separately from the display order
	for(i=0;i<8;i++)
	{
		col=ColumnAt(i);
		if(!col)
			continue;
		
		sprintf(colstr,"col%ld width",i);
		if(prefs->FindFloat(colstr,&ftemp)==B_OK)
			col->SetWidth(ftemp);
	}
}

void SFileView::SaveColumnSettings()
{
	// Get however the columns are set and save stuff about them

	// Adjust columns based on what settings are in the saved preferences
	CLVColumn *col;
	int32 columns[8],i;
	CLVSortMode sortmode;
	char colstr[20];

	GetDisplayOrder(columns);

	for(i=0;i<8;i++)
	{
		col=ColumnAt(i);
		if(!col)
			continue;
		
		// Save the width of each display column
		sprintf(colstr,"col%ld width",i);
		prefs->RemoveData(colstr);
		prefs->AddFloat(colstr,col->Width());

		// Get and save the column display order
		sprintf(colstr,"col%ld order",i);
		prefs->RemoveData(colstr);
		prefs->AddInt32(colstr,columns[i]);
		
		// Get and save the column's sorting mode
		sortmode=col->SortMode();
		
		sprintf(colstr,"col%ld sorting",i);
		prefs->RemoveData(colstr);
		
		if(sortmode==Ascending)
			prefs->AddInt8(colstr,1);
		else
		if(sortmode==Descending)
			prefs->AddInt8(colstr,2);
		else
			prefs->AddInt8(colstr,0);
	}
	
}

// set columns
void SFileView::SetColumns(bool useName,bool useSize,bool useModified,
	bool useCreated,bool useKind,bool usePath)
{
	iUseName=useName;
	iUseSize=useSize;
	iUseModified=useModified;
	iUseCreated=useCreated;
	iUseKind=useKind;
	iUsePath=usePath;

	BuildColumns();
}

int SFileView::Compare(const CLVListItem* aItem1, const CLVListItem* aItem2, int32 sortKey)
{
	SFileItem *item1=(SFileItem *) aItem1;
	SFileItem *item2=(SFileItem *) aItem2;
	char kind1[B_MIME_TYPE_LENGTH], kind2[B_MIME_TYPE_LENGTH];
	char path1[B_PATH_NAME_LENGTH], path2[B_PATH_NAME_LENGTH];

	// sort names
	switch (sortKey)
	{
	
		case 1 :	// sort on name
		{
			// Sort folders first
			if(item1->File()->TargetType()==TYPE_DIRECTORY && item2->File()->TargetType()==TYPE_DIRECTORY)
				// sort directories by name
				return strcasecmp(item1->File()->Name(), item2->File()->Name());

			if(item1->File()->TargetType()==TYPE_DIRECTORY)
				return -1;
			if(item2->File()->TargetType()==TYPE_DIRECTORY)
				return 1;

			return strcasecmp(item1->File()->Name(), item2->File()->Name());
		}			
		case 2 :	// sort on size
		{
			if(item1->File()->TargetType()==TYPE_DIRECTORY && item2->File()->TargetType()==TYPE_DIRECTORY)
				// sort directories by name
				return strcasecmp(item1->File()->Name(), item2->File()->Name());

			if(item1->File()->TargetType()==TYPE_DIRECTORY)
				return -1;
			if(item2->File()->TargetType()==TYPE_DIRECTORY)
				return 1;
			
				return item1->File()->Size() < item2->File()->Size() ? -1 : 1;				
		}
		case 3 :	// sort on modified date
		{
			return item1->File()->ModifiedTime() < item2->File()->ModifiedTime() ? -1 : 1;
		}			
		case 4 : 	// sort on created date
		{
			return item1->File()->CreatedTime() < item2->File()->CreatedTime() ? -1 : 1;
		}
		case 5 : 	// file type, sort folders first, subsort all alphabetically
		{
			if(item1->File()->TargetType()==TYPE_DIRECTORY && item2->File()->TargetType()==TYPE_DIRECTORY)
				// sort directories by name
				return strcasecmp(item1->File()->Name(), item2->File()->Name());

			if(item1->File()->TargetType()==TYPE_DIRECTORY)
				return -1;
			if(item2->File()->TargetType()==TYPE_DIRECTORY)
				return 1;

			strcpy(kind1, item1->File()->MimeDesc());
			strcpy(kind2, item2->File()->MimeDesc());
			int8 cmpval=strcasecmp(kind1, kind2);
			return (cmpval==0)?strcasecmp(item1->File()->Name(), item2->File()->Name()):cmpval;
		}
		case 6 :	// path
		{
			strcpy(path1, item1->File()->PathDesc());
			strcpy(path2, item2->File()->PathDesc());
			return strcasecmp(path1, path2);
		}
	}
	
	return 0;
}

// threaded file scan function
int32 SFileView::ScanThread(void *dataPtr)
{
	SFileView *view=(SFileView *) dataPtr;

	// acquire lock on 'active' semaphore
	acquire_sem(view->iScanThreadActive);

	// scan contents
	BDirectory directory;
	BEntry entry;
	StatusWindow *statuswin=NULL;
	bool terminated=false;
	sem_info semInfo;
	BMessage *statmsg=NULL;

	// set directory to the entry represented by this item
	directory.SetTo(&view->iScanEntryRef);
	int32 entry_count=directory.CountEntries();

	if(entry_count>100)
	{
		char countstring[30];
		while (view->Window()->LockWithTimeout(10000)==B_TIMED_OUT)
		{
			get_sem_info(view->iScanThreadTerminate, &semInfo);
			if(semInfo.count<=0)
			{
				terminated=true;
				break;
			}
		}
		BRect statrect(view->ConvertToScreen(view->Frame()));
		statrect.bottom=statrect.top+100;
		view->Window()->Unlock();

		sprintf(countstring,"Total: %ld entries",entry_count);
		statuswin=new StatusWindow(statrect, "Reading Entries",
			B_FLOATING_WINDOW,B_ASYNCHRONOUS_CONTROLS);
	
		statmsg=new BMessage(SW_SET_TEXT);
		statmsg->AddString("text",countstring);
		statuswin->PostMessage(statmsg);

		statmsg=new BMessage(SW_SET_MAX_VALUE);
		statmsg->AddFloat("value",(float)entry_count);
		statuswin->PostMessage(statmsg);
		statuswin->Show();
		terminated=false;
	}
	
	// scan entries

	while ((directory.GetNextEntry(&entry)==B_OK))
	{
		// owner thread requested we abort ?
		get_sem_info(view->iScanThreadTerminate, &semInfo);
		if(semInfo.count <=0)
		{
			terminated=true;
			break;
		}
			
		// if RequestScanTerminate is waiting for us to quit, it is
		// taking up a lock.  Hello deadlock.
		while (view->Window()->LockWithTimeout(10000)==B_TIMED_OUT)
		{
			get_sem_info(view->iScanThreadTerminate, &semInfo);
			if(semInfo.count <=0)
			{
				terminated=true;
				break;
			}
		}
		if(terminated)
			break;

		// add item to list	
		SFile file(&entry);
		if(!file.IsDesktop())
		{
			SFileItem *item=new SFileItem(view, &file);
			view->AddItem(item);
		}
		view->Window()->Unlock();

		if(statuswin)
		{
			statmsg=new BMessage(SW_ADD_VALUE);
			statmsg->AddFloat("value",1.0);
			statuswin->PostMessage(statmsg);
		}
	}

	directory.Unset();

	// try locking the window one more time so we can sort the new entries
	terminated=false;
	while (view->Window()->LockWithTimeout(10000)==B_TIMED_OUT)
	{
		get_sem_info(view->iScanThreadTerminate, &semInfo);
		if(semInfo.count <=0)
		{
			terminated=true;
			break;
		}
	}
	if(!terminated)
	{
		view->SortItems();
		if(view->Window()->IsLocked())
			view->Window()->Unlock();
	}

	if(statuswin)
	{
		statuswin->PostMessage(B_QUIT_REQUESTED);
	}
	// lower flag
	release_sem(view->iScanThreadActive);

	return (terminated)?-1:0;
}

// request scan termination and wait for it to stop
void SFileView::RequestScanTerminate()
{
	acquire_sem(iScanThreadTerminate);
	
	// sit and wait for the thread to end
	acquire_sem(iScanThreadActive);
	release_sem(iScanThreadActive);
	
	release_sem(iScanThreadTerminate);
}

// refresh tree
void SFileView::Refresh(BMessage *message)
{
	// if the scan thread is running, wait for it to stop
	RequestScanTerminate();

	// delete old items
	SFileItem *oldItem;
	for (uint32 i=0; (oldItem=(SFileItem *) ItemAt(i)); i++)
		delete oldItem;

	// clear list
	MakeEmpty();

	if(message==NULL)
		return;

	entry_ref entryRef;
	if(message->FindRef("root", &entryRef) !=B_OK)
		return;
	
	if(message->FindPointer("tree_item", (void **) &iTreeItem) !=B_OK)
		return;

	// update title on main window
	BMessage *titleMessage=new BMessage(MSG_PIONEER_UPDATE_TITLE);

	BEntry entry(&entryRef);
	BPath path(&entry);
	titleMessage->AddString("path", path.Path());
	Window()->Looper()->PostMessage(titleMessage);
	delete titleMessage;	 
	
	iScanEntryRef=entryRef;

	// start thread
	iScanThreadId=spawn_thread(ScanThread, "FileView::ScanThread", 10, (void *) this);	
	resume_thread(iScanThreadId);
}

void SFileView::Refresh(void)
{
	// Do a refresh using the current tree item and path
	// if the scan thread is running, wait for it to stop
	RequestScanTerminate();

	// delete old items
	SFileItem *oldItem;
	for (uint32 i=0; (oldItem=(SFileItem *) ItemAt(i)); i++)
		delete oldItem;

	// clear list
	MakeEmpty();

	// start thread
	iScanThreadId=spawn_thread(ScanThread, "FileView::ScanThread", 10, (void *) this);	
	resume_thread(iScanThreadId);
}

// current selections
void SFileView::CurrentSelections(SFileList *fileList)
{
	if(!fileList)
		return;

	for (int32 i=0; i < CountItems(); i++) 
	{
		SFileItem *item=(SFileItem *) ItemAt(i); 
		if(item->IsSelected()) 
			fileList->AddItem((void *) item->File());
	}
}

void SFileView::Open(BMessage *message)
{
	SFileItem *item;
	
	// we will open only 1 folder
	bool opened_folder=false;
	
	for(int32 i=0;i<CountItems();i++)
	{
		item=(SFileItem *) ItemAt(i);
		if(!item)
			continue;
		
		if(!item->IsSelected())
			continue;
		
		// is a directory ?
		if(item->File()->TargetType()==TYPE_DIRECTORY && opened_folder==false)
		{
			// we don't have to ensure that we are dealing with a folder
			// and not a symlink to a folder because the treeview also handles
			// folder symlinks
			BEntry entry(item->File()->Ref(),false);
			entry_ref ref;
			entry.GetRef(&ref);
			
			// sync with tree-view
			BMessage *message=new BMessage(MSG_TREEVIEW_EXPAND);
			message->AddPointer("tree_item", iTreeItem);
			message->AddRef("dir",&ref);
			Looper()->PostMessage(message);
			delete message;
			opened_folder=true;
		}
		else
		{
			team_id team;
			if(be_roster->Launch((entry_ref *) item->File()->Ref(),
					(BMessage*)NULL, &team)==B_OK)
				be_roster->ActivateApp(team);
		}
	}
}

// select all entries
void SFileView::SelectAll()
{
	// This is locking the window, and the scan thread won't continue until the lock is lifted.
	sem_info semInfo;
	get_sem_info(iScanThreadActive, &semInfo);
	if(semInfo.count <=0)
		return;

	for (int32 i=0; i < CountItems(); i++)
	{
		SFileItem *item=(SFileItem *) ItemAt(i); 
		if(!item->IsSelected())
			Select(i, true);
	}
}
		
// invert selection
void SFileView::InvertSelection(int32 start, int32 end)
//void SFileView::InvertSelection(int32 start=0, int32 end=-1)
{
	// This is locking the window, and the scan thread won't continue until the lock is lifted.
	sem_info semInfo;
	get_sem_info(iScanThreadActive, &semInfo);
	if(semInfo.count <=0)
		return;

	int32 countend=(end==-1 || end<start)?CountItems():end;
	for (int32 i=start;i<=countend; i++)
	{
		SFileItem *item=(SFileItem *) ItemAt(i); 
		if(!item)
			break;
		
		if(item->IsSelected()) 
			Deselect(i);
		else
			Select(i, true);
	}
}

void SFileView::SelectRange(int32 start, int32 end, bool selected, bool extend)
//void SFileView::SelectRange(int32 start, int32 end, bool selected=true, bool extend=false)
{
	// This is locking the window, and the scan thread won't continue until the lock is lifted.
	sem_info semInfo;
	get_sem_info(iScanThreadActive, &semInfo);
	if(semInfo.count <=0)
		return;

	int32 countend=(end==-1 || end<start)?CountItems():end;

	int32 i=start;
	SFileItem *item=(SFileItem *) ItemAt(i); 
	if(!item)
		return;
	
	if(selected)
		Select(i, extend);
	else
		Deselect(i);
	
	for (i=start+1;i<=countend; i++)
	{
		item=(SFileItem *) ItemAt(i); 
		if(!item)
			break;
		
		if(selected)
			Select(i, true);
		else
			Deselect(i);
	}
}

void SFileView::ScanForStat(const SFile *file)
{
	entry_ref entryRef=*file->Ref();
	SFileItem *existItem;
	bool found=false;
	
	for (uint32 i=0; (existItem=(SFileItem *) ItemAt(i)); i++)
	{
		if(strcasecmp(existItem->File()->Name(), entryRef.name)==0)
		{
			found=true;
			break;
		}
	}

	if(!found)
		return;
	existItem->File()->ResetSize();
}

// node monitor: scan for created
void SFileView::ScanForCreated(const SFile *file)
{
	entry_ref entryRef=*file->Ref();

	// find the parent of the normal entry
	BEntry childEntry(&entryRef);
	BEntry parentEntry;
	childEntry.GetParent(&parentEntry);
	
	entry_ref parentEntryRef;
	parentEntry.GetRef(&parentEntryRef);
	
	// the file activity applies to this list ?
	if(parentEntryRef !=iScanEntryRef)
		return;

	// check to see if we've already added it to the list
	// this prevents duplicate entries from being created
	// when we rename a folder
	SFileItem *existItem;
	bool found=false;
	for (uint32 i=0; (existItem=(SFileItem *) ItemAt(i)); i++)
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

	// Sorting removes the selection, so we have to work around this
	BList selectlist(0);
	BListItem *item;
	for(int32 i=0;i<CountItems();i++)
	{
		item=ItemAt(i);
		if(item && item->IsSelected())
			selectlist.AddItem(item);
	}

	// add the entry
	SFileItem *newItem=new SFileItem(this, file);																
	AddItem(newItem);
 
	SortItems();

	for(int32 i=0;i<selectlist.CountItems();i++)
	{
		item=(BListItem*)selectlist.ItemAt(i);
		if(item)
			Select(IndexOf(item),true);
	}
	
}

// node monitor: scan for moved
void SFileView::ScanForMoved(const SFile *fromFile, const SFile *toFile)
{
	BEntry entry(toFile->Ref());
	BEntry parent;
	entry.GetParent(&parent);
	entry_ref ref;
	parent.GetRef(&ref);

	// Scan only when necessary
	if(ref==iScanEntryRef)
	{
		// use the standard creation routine
		ScanForCreated(toFile);
	}
	
	entry.SetTo(fromFile->Ref());
	entry.GetParent(&parent);
	parent.GetRef(&ref);
	
	if(ref==iScanEntryRef)
	{
		// since we don't get a stupid parent node, we can't tell whether this
		// should be refreshed...STUPID STUPID STUPID
		ScanAllForRemoved();
	}
}

// node monitor: scan for removed
void SFileView::ScanAllForRemoved()
{
	SFileItem *item;
	int32 selection=-1;
	for (uint32 i=0; (item=(SFileItem *) ItemAt(i)); i++)
	{
		if(!item->File()->Exists()) 
		{
			if(item->IsSelected() && selection==-1)
				selection=i;

			// remove it !
			RemoveItem(item);
			delete item;
		}	
	}
	if(selection>0)
		selection--;
	if(selection!=-1)
		Select(selection);
}

void SFileView::SpawnTracker()
{
	for(int32 i=0;i<CountItems();i++)
	{
		SFileItem *item=(SFileItem*)ItemAt(i);
	
		if(!item)
			return;
		
		if(item->IsSelected())
		{
			
			BString string("/boot/beos/system/Tracker '");
			string+=item->File()->PathDesc();
			string+="' &";
			system(string.String());
		}
	}
}

const char *SFileView::Path()
{
	if(iTreeItem)
	{
		STreeItem *sti=(STreeItem*)iTreeItem;
		if(sti->File())
			return sti->File()->PathDesc();
	}
	return NULL;
}

SFile *SFileView::PathAsFile()
{
	if(iTreeItem)
	{
		STreeItem *sti=(STreeItem*)iTreeItem;
		return sti->File();
	}
	return NULL;
}

void SFileView::StartTracking(BRect r, uint32 how)
{
	BeginRectTracking(r,how);
	istracking=true;
}

void SFileView::EndTracking(void)
{
	EndRectTracking();
	istracking=false;
}

int32 SFileView::FindNextAlphabetical(char c, int32 index)
{
	SFileItem *fileitem;
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
		fileitem=(SFileItem*)ItemAt(i);
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
