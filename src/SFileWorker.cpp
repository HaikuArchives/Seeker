#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs_attr.h>

#include <Node.h>
#include <NodeInfo.h>
#include <Entry.h>
#include <Directory.h>
#include <Alert.h>
#include <String.h>

#include "Colors.h"

#include "SeekerWindow.h"
#include "SFileInfo.h"

#include "SCopyWindow.h"
#include "SFileWorker.h"
#include "PrintDebugTools.h"

SFileWorker::SFileWorker(SeekerWindow *mainWindow)
 : iWindow(mainWindow),
   iFileOp(P_OP_NONE),
   iFileList(0)
{
	// create new list
	iFileList=new BList();
}

SFileWorker::~SFileWorker()
{
	if(iFileList)
	{
		for (int32 i=0; i < iFileList->CountItems(); i++)
		{
			entry_ref *entryRef=(entry_ref *) iFileList->ItemAt(i);
			delete entryRef;
		}
		iFileList->MakeEmpty();
		delete iFileList;
	}
}

void SFileWorker::NewFolder(const SFile *parentFolder)
{
	// display modal dialog
	if(parentFolder)
	{
		SNewFolderWindow *newFolderWindow=new SNewFolderWindow(iWindow, parentFolder);
		newFolderWindow->Show();
	}
}
/*
void SFileWorker::GetInfo(const SFile *file, int32 offset)
{
	if(file)
	{
		SFileInfoWindow *infoWindow=new SFileInfoWindow(iWindow, file,offset);
		iWindow->AddToSubset(infoWindow);
		infoWindow->Show();
	}
}
*/
void SFileWorker::GetInfo(const SFileList *filelist)
{
	if(!filelist)
		return;
	
	bool multiple=(filelist->CountItems()>1)?true:false;
	for(int32 i=0;i<filelist->CountItems();i++)
	{
		SFile *file=(SFile*)filelist->ItemAt(i);
		if(file && !file->IsSystem())
		{
			SFileInfoWindow *infoWindow=new SFileInfoWindow(iWindow, file, multiple, i*3);
			infoWindow->Show();
		}	
	}
}

void SFileWorker::EditName(const SFileList *filelist)
{
	if(!filelist)
		return;
	
	bool multiple=(filelist->CountItems()>1)?true:false;
	for(int32 i=0;i<filelist->CountItems();i++)
	{
		SFile *file=(SFile*)filelist->ItemAt(i);
		if(file && !file->IsSystem())
		{
			SRenameWindow *renameWindow=new SRenameWindow(iWindow, file, multiple, i*3);
			renameWindow->Show();
		}	
	}
}

void SFileWorker::MoveToTrash(const SFile *srcFolder, const SFileList *fileList)
{
	BEntry entry(TRASH_ENTRY);
	SFile trashFolder(&entry);

	ModifyRestoreAttribute(fileList,true);
	Cut(srcFolder, fileList);
	Paste(&trashFolder);
}

// Add or Remove OpenTracker's Restore attribute
void SFileWorker::ModifyRestoreAttribute(const SFileList *fileList, bool add)
{
	SFile *file;
	BNode node;
	
	for(int32 i=0; i<fileList->CountItems();i++)
	{
		file=(SFile*)fileList->ItemAt(i);
		if(!file)
			continue;

		if(node.SetTo(file->Ref())!=B_OK)
			continue;
		
		if(add)
		{
			node.WriteAttr("_trk/original_path",B_STRING_TYPE,0,
				file->PathDesc(),strlen(file->PathDesc())+1);
		}
		else
			node.RemoveAttr("_trk/original_path");
	}
}

// used for emptying trash (deleting in reverse recursive order),
// sorts by the directory depth
static int CompareByLevel(const void *item1, const void *item2)
{
	const SFile *file1=(const SFile *) item1;
	const SFile *file2=(const SFile *) item2;
	
	return file1->Level() < file2->Level() ? -1 : 1;		
}

void SFileWorker::EmptyTrash(void)
{
	BEntry trashEntry(TRASH_ENTRY);
	SFile dest_trash(&trashEntry);
	for (int32 i=0; i < iFileList->CountItems(); i++) 
	{
		SFile *file=(SFile *) iFileList->ItemAt(i);
		delete file;
	}
	iFileList->MakeEmpty();

	// set directory to the entry represented by this item
	BEntry entry;
	BDirectory directory(TRASH_ENTRY);

	while ((directory.GetNextEntry(&entry, false)==B_OK))
	{
		SFile *file=new SFile(&entry);
		iFileList->AddItem(file);
	}

	// deep-scan
	int32 nItems=iFileList->CountItems();
	for (int32 indx=0; indx < nItems; indx++)
	{
		SFile *file=(SFile *) iFileList->ItemAt(indx);
		
		if(file->IsDirectory())
		{
			directory.SetTo(file->Ref());
			while (directory.GetNextEntry(&entry, false)==B_OK)
			{
				SFile *newFile=new SFile(&entry);				
				iFileList->AddItem(newFile);
				nItems +=1;				
			}
		}
	}

	iFileList->SortItems(CompareByLevel);

	iFileOp=P_OP_CUT;

	if(iFileList->CountItems()==0)
		return;

	SCopyWindow *copyWindow=new SCopyWindow(iWindow, iFileList, &dest_trash,iFileOp, P_OP_REMOVE);
	iWindow->AddToSubset(copyWindow);
	copyWindow->Show();
	
	// clear old list
	for (int32 i=0; i < iFileList->CountItems(); i++)
	{
		SFile *file=(SFile *) iFileList->ItemAt(i);
		delete file;
	}
	iFileList->MakeEmpty();
}

void SFileWorker::Copy(const SFile *srcFolder, const SFileList *fileList)
{
	// clear old list
	for (int32 i=0; i < iFileList->CountItems(); i++)
	{
		SFile *file=(SFile *) iFileList->ItemAt(i);
		delete file;
	}
	iFileList->MakeEmpty();

	// copy new list	
	for (int32 j=0; j < fileList->CountItems(); j++)
	{
		SFile *file=new SFile((SFile *) fileList->ItemAt(j));
		iFileList->AddItem(file);
	}
	
	// set operation
	iFileOp=P_OP_COPY;
}

void SFileWorker::Cut(const SFile *srcFolder, const SFileList *fileList)
{
	// same as copy but we'll override the file operation flag
	Copy(srcFolder, fileList);
	iFileOp=P_OP_CUT;
}

void SFileWorker::Paste(const SFile *folder)
{
	if(iFileList->CountItems()==0)
		return;

	if(iFileOp==P_OP_CUT)
		MoveFiles(iFileList,folder);
	else
		CopyFiles(iFileList,folder);

	// clear old list
	for (int32 i=0; i < iFileList->CountItems(); i++)
	{
		SFile *file=(SFile *) iFileList->ItemAt(i);
		delete file;
	}
	iFileList->MakeEmpty();
}

void SFileWorker::PasteLink(const SFile *folder)
{
	if(iFileList->CountItems()==0)
		return;
	
	MakeLinks(iFileList,folder);
	
	// clear old list
	for (int32 i=0; i < iFileList->CountItems(); i++)
	{
		SFile *file=(SFile *) iFileList->ItemAt(i);
		delete file;
	}
	iFileList->MakeEmpty();
}

SNewFolderWindow::SNewFolderWindow(SeekerWindow *mainWindow,
			 	 					const SFile *parentFile)
 : BWindow(BRect(50, 50, 0, 0), "New Folder", 
 			B_FLOATING_WINDOW,
			B_WILL_DRAW | B_NOT_ZOOMABLE | 
			B_NOT_MINIMIZABLE | B_NOT_RESIZABLE),
   iMainWindow(mainWindow),
   iView(0),
   iParentTextControl(0),
   iNameTextControl(0),
   iOkButton(0),
   iCancelButton(0)
{
	char *parentLabel="Parent";
	char *nameLabel="New Folder";

	iParentFile=new SFile(parentFile);

	// set window feel
	SetFeel(B_MODAL_APP_WINDOW_FEEL);

	// create view
	iView=new BView(Bounds(), "NewFolderWindow::View", B_FOLLOW_ALL, 
					   B_WILL_DRAW);
	iView->SetViewColor(BeBackgroundGrey);
	AddChild(iView);

	// parent label/text
	BRect rect;
	rect.left=10;
	rect.top=10;
	iParentTextControl=new BTextControl(rect, "NewFolderWindow::ParentTextControl",
										   parentLabel, iParentFile->Name(), NULL);
	iParentTextControl->ResizeToPreferred();
	iView->AddChild(iParentTextControl);

	// new folder name label/text
	rect.top +=10 + iParentTextControl->Bounds().Height();
	iNameTextControl=new BTextControl(rect, "NewFolderWindow::NameTextControl",
										 nameLabel, "", NULL);
	iNameTextControl->ResizeToPreferred();
	iView->AddChild(iNameTextControl);

	// align fields
	float parentStringWidth=be_plain_font->StringWidth(parentLabel);
	float nameStringWidth=be_plain_font->StringWidth(nameLabel);
	float maxWidth=MAX(parentStringWidth, nameStringWidth) + 10;
	
	iNameTextControl->SetDivider(maxWidth);
	iNameTextControl->ResizeTo(MAX(iNameTextControl->Bounds().Width(), 200),
								iNameTextControl->Bounds().Height());

	iParentTextControl->SetDivider(maxWidth);
	iParentTextControl->ResizeTo(iNameTextControl->Bounds().Width(), 
								  iParentTextControl->Bounds().Height()); 

	// cancel button
	iCancelButton=new BButton(BRect(), "NewFolderWindow::CancelButton",
								 "Cancel",  new BMessage(MSG_NEWFOLDER_WINDOW_CANCEL));
	iCancelButton->ResizeToPreferred();
	iCancelButton->MoveTo(iNameTextControl->Frame().right - iCancelButton->Bounds().Width(),
						   iNameTextControl->Frame().bottom + 10);			
	
	// ok button
	iOkButton=new BButton(BRect(), "NewFolderWindow::OkButton",
							 "OK",  new BMessage(MSG_NEWFOLDER_WINDOW_OK));
	iOkButton->ResizeToPreferred();
	iOkButton->MoveTo(iCancelButton->Frame().left - 10 - iOkButton->Bounds().Width(),
					   iCancelButton->Frame().top);	
	iOkButton->MakeDefault(true);
	
	// add window in tab-order
	iView->AddChild(iOkButton);
	iView->AddChild(iCancelButton);

	// re-size window
	rect.right=iCancelButton->Frame().right + 10;
	rect.bottom=iCancelButton->Frame().bottom + 10;
	ResizeTo(rect.right, rect.bottom);
	
	// disable editing of parent name
	iParentTextControl->TextView()->MakeEditable(false);
	iParentTextControl->TextView()->MakeSelectable(false);
	iParentTextControl->TextView()->SetViewColor(BeBackgroundGrey);
	iParentTextControl->SetViewColor(BeBackgroundGrey);

	// Prevent goofs by changing the folder location
	iNameTextControl->TextView()->DisallowChar('/');
	
	// set focus to name field
	iNameTextControl->MakeFocus();
}			 	  

SNewFolderWindow::~SNewFolderWindow()
{
	if(iParentFile) delete iParentFile;
}

void SNewFolderWindow::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case MSG_NEWFOLDER_WINDOW_OK:
			CreateFolder();
			break;
	
		case MSG_NEWFOLDER_WINDOW_CANCEL:
			Quit();
			break;

		default:
			BWindow::MessageReceived(message);
	}

}

void SNewFolderWindow::CreateFolder()
{
	char newPath[B_PATH_NAME_LENGTH];

	BString text(iNameTextControl->Text());
	if(text.Compare(".")==0 || text.Compare("..")==0)
	{
		BAlert *alert=new BAlert("Error", "The names '.' and '..' refer to"
		" the current folder and its parent, respectively. Please choose"
		" another name.", "OK");
		alert->Go();
		return;
	}
	if(text.CountChars()==0)
		return;
		
	strcpy(newPath, iParentFile->PathDesc());
	strcat(newPath, "/");
	strcat(newPath, iNameTextControl->Text());

	BDirectory directory;
	status_t rc;
	
	rc=directory.CreateDirectory(newPath, 0);
	
	if(rc!=B_OK)
	{
		char errMsg[80];
		switch (rc)
		{
			case B_BAD_VALUE:
				strcpy(errMsg, "The folder cannot be created because an "
				"invalid folder name was given (possibly because of illegal characters)");
				break;
			case B_FILE_EXISTS:
				strcpy(errMsg, "The folder cannot be created because the name "
				"given is already in use");
				break;
				
			case B_NAME_TOO_LONG :		
				strcpy(errMsg, "The folder cannot be created because the name "
				"given is too long");
				break;
				
			case B_NOT_ALLOWED:
				strcpy(errMsg, "The folder cannot be created because the target "
				"volume is read-only");
				break;
				
			case B_PERMISSION_DENIED:
				strcpy(errMsg, "The folder cannot be created because you do not "
				"have sufficient privileges on the target folder");
				break;
				
			default	:
				sprintf(errMsg, "The folder cannot be created because an unexpected error "
				"occurred. (error code=0x%lx)", rc);
		}
		
	
		BAlert *alert=new BAlert("Error", errMsg, "OK");
		alert->Go();
		return;
		
	}
	else
		Quit();
}		

SRenameWindow::SRenameWindow(SeekerWindow *mainWindow, const SFile *file, bool multiple, int32 offset)
	: BWindow(BRect(50+offset, 50+offset, 0, 0), "Rename", B_FLOATING_WINDOW, B_WILL_DRAW | 
//	B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE),
	B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE),
	iMainWindow(mainWindow),
	iView(0),
	iOldNameTextControl(0),
	iNewNameTextControl(0),
	iOkButton(0),
	iCancelAllButton(0),
	iCancelButton(0)
{
	char *oldNameLabel="Old Name";
	char *newNameLabel="New Name";

	iFile=new SFile(file);

	// set window feel
	SetFeel(B_MODAL_APP_WINDOW_FEEL);

	// create view
	iView=new BView(Bounds(), "RenameWindow::View", B_FOLLOW_ALL, 
					   B_WILL_DRAW);
	iView->SetViewColor(BeBackgroundGrey);
	AddChild(iView);

	// old-name label/text
	BRect rect;
	rect.left=10;
	rect.top=10;
	iOldNameTextControl=new BTextControl(rect, "RenameWindow::OldNameTextControl",
										    oldNameLabel, iFile->Name(), NULL);
	iOldNameTextControl->ResizeToPreferred();
	iView->AddChild(iOldNameTextControl);

	// new folder name label/text
	rect.top +=10 + iOldNameTextControl->Bounds().Height();
	iNewNameTextControl=new BTextControl(rect, "RenameWindow::NewNameTextControl",
										 	newNameLabel, iFile->Name(), NULL);
	iNewNameTextControl->ResizeToPreferred();
	iView->AddChild(iNewNameTextControl);

	// align fields
	float oldNameStringWidth=be_plain_font->StringWidth(oldNameLabel);
	float newNameStringWidth=be_plain_font->StringWidth(newNameLabel);
	float maxWidth=MAX(oldNameStringWidth, newNameStringWidth) + 10;
	
	iNewNameTextControl->SetDivider(maxWidth);
	if(iNewNameTextControl->Bounds().Width()<200)
		iNewNameTextControl->ResizeBy(200-iNewNameTextControl->Bounds().Width(),0);

	iOldNameTextControl->SetDivider(maxWidth);
	iOldNameTextControl->ResizeTo(iNewNameTextControl->Bounds().Width(), 
								   iOldNameTextControl->Bounds().Height()); 


	// OK button
	iOkButton=new BButton(BRect(), "RenameWindow::OKButton",
			"OK", new BMessage(MSG_RENAME_WINDOW_OK));
	iOkButton->ResizeToPreferred();
	iOkButton->MoveTo(iNewNameTextControl->Frame().left,iNewNameTextControl->Frame().bottom+10);
	iView->AddChild(iOkButton);
	iOkButton->MakeDefault(true);

	// Cancel All button
	iCancelAllButton=new BButton(BRect(), "RenameWindow::CancelAllButton",
			"Cancel All", new BMessage(MSG_RENAME_WINDOW_CANCEL_ALL));
	iCancelAllButton->ResizeToPreferred();
	iCancelAllButton->MoveTo(iOkButton->Frame().right+10,iOkButton->Frame().top);
	iView->AddChild(iCancelAllButton);
	if(!multiple)
		iCancelAllButton->SetEnabled(false);

	// Cancel button
	iCancelButton=new BButton(BRect(), "RenameWindow::CancelButton",
			"Cancel", new BMessage(MSG_RENAME_WINDOW_CANCEL));
	iCancelButton->ResizeToPreferred();
	iCancelButton->MoveTo(iCancelAllButton->Frame().right+10,iCancelAllButton->Frame().top);
	iView->AddChild(iCancelButton);
	
	// add window in tab-order
	iCancelAllButton->SetTarget(be_app);
	
	// re-size window
	rect.right=iCancelButton->Frame().right + 10;
	rect.bottom=iCancelButton->Frame().bottom + 10;
	ResizeTo(rect.right, rect.bottom);
	
	// disable editing of old-name
	iOldNameTextControl->TextView()->MakeEditable(false);
	iOldNameTextControl->TextView()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// set focus to name field
	iNewNameTextControl->MakeFocus();
	iNewNameTextControl->TextView()->DisallowChar('/');
}			 	  

SRenameWindow::~SRenameWindow()
{
	if(iFile) delete iFile;
}

void SRenameWindow::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case MSG_RENAME_WINDOW_OK:
			RenameFile();
			break;
	
		case MSG_RENAME_WINDOW_CANCEL:
			Quit();
			break;

		default:
			BWindow::MessageReceived(message);
	}

}

void SRenameWindow::RenameFile()
{
	BEntry entry(iFile->Ref());
	
	status_t rc;
	
	rc=entry.Rename(iNewNameTextControl->Text(), false);
	
	if(rc==B_OK)
		Quit();

	char errMsg[80];
	switch (rc)
	{
		case B_FILE_EXISTS:
			strcpy(errMsg, "The name given is already in use.");
			break;
			
		default	:
			sprintf(errMsg, "The folder cannot be created because an unexpected error "
			"occurred. (error code=0x%lx)", rc);
	}
	
	BAlert *alert=new BAlert("Error", errMsg, "OK");
	alert->Go();
}

void SFileWorker::CopyFiles(SFileList *fileList, const SFile *folder)
{
	SCopyWindow *copyWindow=new SCopyWindow(iWindow, fileList, folder, P_OP_COPY, P_OP_PASTE);
	iWindow->AddToSubset(copyWindow);
	copyWindow->Show();
}

void SFileWorker::MoveFiles(SFileList *fileList, const SFile *folder)
{
	SCopyWindow *moveWindow=new SCopyWindow(iWindow, fileList, folder, P_OP_CUT, P_OP_PASTE);
	iWindow->AddToSubset(moveWindow);
	moveWindow->Show();
}

void SFileWorker::MakeLinks(SFileList *fileList, const SFile *folder)
{
	SCopyWindow *linkWindow=new SCopyWindow(iWindow, fileList, folder, P_OP_COPY, P_OP_PASTE_LINK);
	iWindow->AddToSubset(linkWindow);
	linkWindow->Show();
}
