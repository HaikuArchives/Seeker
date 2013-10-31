#include <Directory.h>
#include <string.h>
#include <Alert.h>
#include <stdio.h>
#include <fs_attr.h>
#include <malloc.h>
#include <String.h>
#include "PrintDebugTools.h"
#include "SCopyWindow.h"
#include "FSUtils.h"

SCopyWindow::SCopyWindow(SeekerWindow *mainWindow, BList *fileList, 
						  const SFile *destFolder, int copyOp, int pasteOp)
 : BWindow(BRect(50, 50, 350, 100), "Progress", B_FLOATING_WINDOW,B_WILL_DRAW | B_NOT_CLOSABLE |
 B_NOT_ZOOMABLE),
   iCopyThreadId(0),
   iCopyThreadActive(0),
   iCopyThreadTerminate(0),
   iMainWindow(mainWindow),
   iView(0),
   iStatusBar(0),
   iCancelButton(0),
   iFileList(0),
   iDestFolder(0),
   iCopyOp(copyOp),
   iPasteOp(pasteOp)
{
	iCopyThreadActive=create_sem(1, "CopyWindow::CopyThreadActive");
	iCopyThreadTerminate=create_sem(1, "CopyWindow::CopyThreadTerminate");

	iDestFolder=new SFile(destFolder);

	iFileList=new BList(fileList->CountItems());

	copy_record *copyrec;
	SFile *file;
	for (int32 i=0; i<fileList->CountItems(); i++)
	{
		file=(SFile *)fileList->ItemAt(i);
		if(!file)
			continue;
			
		copyrec=new copy_record;
		copyrec->from=*file->Ref();
		copyrec->to=*iDestFolder->Ref();
		iFileList->AddItem(copyrec);
	}
	

	iView=new BView(Bounds(), "CopyWindow::View", B_FOLLOW_ALL, B_WILL_DRAW);
	iView->SetViewColor(BeBackgroundGrey);
	AddChild(iView);

	BRect statusBarBounds(10, 10, Bounds().right-10, 35);

	iStatusBar=new BStatusBar(statusBarBounds, "CopyWindow::StatusBar");
	iView->AddChild(iStatusBar);
	iStatusBar->SetMaxValue(iFileList->CountItems());

	BRect buttonBounds(211, 10, 0, 0);	
	iCancelButton=new BButton(buttonBounds, "CopyWindow::CancelButton",
			"Cancel",  new BMessage(MSG_COPY_WINDOW_CANCEL));
	iCancelButton->ResizeToPreferred();
	buttonBounds=iCancelButton->Frame();
	iCancelButton->MoveTo(Bounds().right-(buttonBounds.Width()+10),
		iStatusBar->Frame().bottom+10);
	iView->AddChild(iCancelButton);
	
	ResizeTo(iCancelButton->Frame().right+10,iCancelButton->Frame().bottom+10);
}

SCopyWindow::~SCopyWindow()
{
	// make sure we stop the copy thread before we go and delete ourselves
	RequestCopyTerminate();

	// remove from subset
	BMessage *message=new BMessage(MSG_PIONEER_REMOVE_FROM_SUBSET);
	message->AddPointer("window", this);
	iMainWindow->Looper()->PostMessage(message);
	delete message;

	if(iCopyThreadActive) delete_sem(iCopyThreadActive);
	if(iCopyThreadTerminate) delete_sem(iCopyThreadTerminate);	

	if(iFileList)
	{
		copy_record *copyrec;
		for (int32 i=0; i<iFileList->CountItems(); i++)
		{
			copyrec=(copy_record*)iFileList->ItemAt(i);
			delete copyrec;
		}	
		iFileList->MakeEmpty();
		
		delete iFileList;
	}
	delete iDestFolder;
}

void SCopyWindow::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case MSG_COPY_WINDOW_CANCEL:
			RequestCopyTerminate();
			Quit();
			break;

		default:
			BWindow::MessageReceived(message);
	}
}

void SCopyWindow::Show(void)
{
	BWindow::Show();

	// start the copy thread
	iCopyThreadId=spawn_thread(CopyThread, "CopyWindow::CopyThread", 10, (void *) this);	
	resume_thread(iCopyThreadId);
}

// threaded file scan function
int32 SCopyWindow::CopyThread(void *dataPtr)
{
	SCopyWindow *window=(SCopyWindow *) dataPtr;

	// acquire lock on 'active' semaphore
	acquire_sem(window->iCopyThreadActive);

	sem_info semInfo;
	bool terminated=false;
	bool cancelled=false;

	// get item count
	int32 nItems=0;
	while (window->LockWithTimeout(10000)==B_TIMED_OUT)
	{
		get_sem_info(window->iCopyThreadTerminate, &semInfo);
		if(semInfo.count<=0)
		{
			terminated=true;
			break;
		}
	}
	if(terminated)
	{
		release_sem(window->iCopyThreadActive);
		return -1;
	}

	nItems=window->iFileList->CountItems();
	window->Unlock();	

	for (int32 sourceIndx=0; (sourceIndx<nItems) && !cancelled; sourceIndx++)
	{
		// if RequestCopyTerminate is waiting for us to quit, it is
		// taking up a lock.  Hello deadlock.
		while (window->LockWithTimeout(10000)==B_TIMED_OUT)
		{
			get_sem_info(window->iCopyThreadTerminate, &semInfo);
			if(semInfo.count<=0)
			{
				terminated=true;
				break;
			}
		}
		if(terminated)
			break;

		copy_record *copyrec=(copy_record *)window->iFileList->ItemAt(sourceIndx);
		SFile *file=new SFile(&copyrec->from);

		// Massive amounts of setup necessary. Ick.
		BEntry srcDirEntry, srcEntry(&copyrec->from);
		BEntry destDirEntry(&copyrec->to),destEntry;
		BDirectory destDir(&copyrec->to);
		BPath destpath;
		char srcnamestr[B_FILE_NAME_LENGTH];
		char destpathstr[B_PATH_NAME_LENGTH];
		status_t status=0;

		srcEntry.GetParent(&srcDirEntry);
		destDirEntry.GetPath(&destpath);
		srcEntry.GetName(srcnamestr);
		sprintf(destpathstr,"%s/%s",destpath.Path(),srcnamestr);

		if(window->iCopyOp==P_OP_CUT)
		{
			if(window->iPasteOp==P_OP_PASTE)
			{
				bool clobber=false;
				do
				{
					destEntry.SetTo(destpathstr);
					
	  				// test if destination exists first
					if(destEntry.Exists())
					{	
						// automagically change the name if we're pasting to the same directory
						// or moving to trash
						if((destDirEntry==srcDirEntry) || window->iDestFolder->IsTrash())
							strcpy(destpathstr,GetValidName(&destEntry));
					}
							
//					status=srcEntry.MoveTo(&destDir, destpathstr, clobber);
					MoveFile(&srcEntry,&destEntry,clobber);
					clobber=false;					

					// handle errors
					if(status!=B_OK)
					{
						char errMsg[256];
						BAlert *alert;
						
						switch(status)
						{
							case B_ENTRY_NOT_FOUND:
							{
								sprintf(errMsg, "%s was moved or deleted during this operation. "
												 "Do you wish to continue?", file->Name()); 
								alert=new BAlert("Error", errMsg, "Continue", "Stop");
								status=alert->Go();
								if(status==1)
									cancelled=true;
								break;
							}	
							case B_FILE_EXISTS:
							{
								sprintf(errMsg, "%s already exists in this folder. "
												  "Would you like to replace it with the "
												  "file you are moving?", file->Name()); 
								alert=new BAlert("Error", errMsg, "Replace File", "Skip File", "Stop");
								status=alert->Go();
								if(status==0)
									clobber=true;
								else
								if(status==2)
									cancelled=true;
								break;							
							}
							case B_NO_INIT:
							{
								printf("B_NO_INIT\n");
								break;							
							}
							case B_BUSY:
							{
								printf("B_BUSY\n");
								break;							
							}
							case B_CROSS_DEVICE_LINK:
							{
								printf("B_CROSS_DEVICE_LINK\n");
								break;							
							}
							default:
								sprintf(errMsg, "%s was not moved because "
												 "an unexpected error occurred.(error code=0x%lx)",
												 file->Name(), status);
								PrintErrorCode(status);
								alert=new BAlert("Error", errMsg, "Continue", "Stop");
								status=alert->Go();
								if(status==1)
									cancelled=true;
						}

					} 
		
				} while (clobber);
					
			} // end if(paste operation == P_OP_PASTE)
			else
				if(window->iPasteOp==P_OP_REMOVE)
				{
					// used for emptying the trash
					srcEntry.Remove();
				}
			
		} // end if(copy operation==P_OP_CUT) 
		else
		if(window->iCopyOp==P_OP_COPY)
		{
			if(window->iPasteOp==P_OP_PASTE)
			{
				// test if destination exists first
				destEntry.SetTo(destpathstr);
				
				if(destEntry.Exists())
				{	
					// automagically change the name if we're pasting to the same directory
					// or moving to trash
					if((destDirEntry==srcDirEntry) || window->iDestFolder->IsTrash())
						strcpy(destpathstr,GetValidName(&destEntry));
					else
					{
						char errMsg[256];
						BAlert *alert;
	
						sprintf(errMsg, "%s already exists in this folder. "
										 "Would you like to replace it with the "
										 "file you are copying?", file->Name()); 
	
						alert=new BAlert("Error", errMsg, "Replace File", "Skip File", "Stop");
						status=alert->Go();
						if(status==0)
						{
							// delete existing file
							BEntry delEntry(destpathstr);
							delEntry.Remove();
						}
						else
							if(status==2)
								cancelled=true;
					}
				}
				
				if(file->IsDirectory())
				{
					// add children to bottom of list
					BDirectory directory(file->Ref());
					BEntry entry;
					entry_ref ref;

					// create new directory instead of copying
					destDir.CreateDirectory(destpathstr, 0);
					entry.SetTo(destpathstr);
					entry.GetRef(&ref);

					while (directory.GetNextEntry(&entry, false)==B_OK)
					{
						copy_record *cr=new copy_record;
						entry.GetRef(&cr->from);
						cr->to=ref;
						window->iFileList->AddItem(cr);
						nItems++;
					}
				}
				else
					CopyFile(&srcEntry,&destEntry,true);
			} 
			else
			if(window->iPasteOp==P_OP_PASTE_LINK)
			{
		
				BEntry srcEntry(file->Ref());
				BPath srcPath(&srcEntry);
				const char *srcPathName=srcPath.Path();
				char srcLeafName[B_FILE_NAME_LENGTH];
				srcEntry.GetName(srcLeafName);

				BDirectory destDir(window->iDestFolder->Ref()); 

				BPath destPath(&destDir, srcPath.Leaf());
				const char *destPathName=destPath.Path();
								
				bool clobber=false;
				do
				{
					if(clobber)
					{
						// delete existing file
						BEntry delEntry(destPathName);
						delEntry.Remove();
						clobber=false;					
					}
							
					status=destDir.CreateSymLink(destPathName, srcPathName, NULL);
			
					// handle errors
					if(status!=B_OK)
					{
						char errMsg[256];
						BAlert *alert;
						
						switch (status)
						{
							case B_ENTRY_NOT_FOUND:
								sprintf(errMsg, "The file or folder %s was moved or deleted  "
												 "during this operation.  Do you wish to "
												 "continue?", file->Name()); 
								alert=new BAlert("Error", errMsg, "Continue", "Stop");
								status=alert->Go();
								if(status==1)
									cancelled=true;
								break;
								
							case B_FILE_EXISTS:
								sprintf(errMsg, "%s already exists in this folder. "
												 "Would you like to replace it with the "
												 "symbolic link you are creating?", file->Name()); 
								alert=new BAlert("Error", errMsg, "Replace File", "Skip File", "Stop");
								status=alert->Go();
								if(status==0)
									clobber=true;
								else
									if(status==2)
										cancelled=true;

								break;							
								
							default:
								sprintf(errMsg, "%s was not copied because "
												"an unexpected error occurred. (error code=%lx)", file->Name(), status);
								PrintErrorCode(status);
								alert=new BAlert("Error", errMsg, "Continue", "Stop");
								status=alert->Go();
								if(status==1)
									cancelled=true;
						}

					} // end if return code not B_OK

				} while (clobber);	// end paste_link loop
				
			}	// end if paste op is PASTE_LINK
		} // end if copy op == COPY


		delete file;
		
		// update status bar
		char text[B_PATH_NAME_LENGTH],trailingtext[B_PATH_NAME_LENGTH];

		if(window->iPasteOp==P_OP_REMOVE)
			sprintf(text, "Emptying Trash: %s",srcnamestr);
		else
			sprintf(text, "Copying: %s", srcnamestr);

		sprintf(trailingtext, "%ld items remaining", nItems-sourceIndx-1);
		window->iStatusBar->Update(1, text, trailingtext);

		window->Unlock();

	} // end for(int32 sourceIndx=0; (sourceIndx<nItems) && !cancelled; sourceIndx++)

	// close window
	while (window->LockWithTimeout(10000)==B_TIMED_OUT)
	{
		get_sem_info(window->iCopyThreadTerminate, &semInfo);
		if(semInfo.count<=0)
		{
			terminated=true;
			break;
		}
	}
	if(!terminated)
	{
		// Why did I ever post this message?
//		window->iMainWindow->PostMessage(MSG_FILEVIEW_REFRESH_CURRENT);
		window->Looper()->PostMessage(B_QUIT_REQUESTED);
		window->Unlock();
	}

	release_sem(window->iCopyThreadActive);
	return (terminated)?-1:0;
}

// request copy termination and wait for it to stop
void SCopyWindow::RequestCopyTerminate()
{
	acquire_sem(iCopyThreadTerminate);
		
	// sit and wait for the thread to end
	acquire_sem(iCopyThreadActive);
	release_sem(iCopyThreadActive);
	
	release_sem(iCopyThreadTerminate);
}
