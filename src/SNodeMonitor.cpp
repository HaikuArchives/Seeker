#include <stdio.h>

#include <Alert.h>
#include <Node.h>
#include <NodeMonitor.h>

#include "SeekerWindow.h"
#include "STreeView.h"
#include "SFileView.h"

#include "SNodeMonitor.h"

SNodeMonitor::SNodeMonitor(STreeView *treeView, SFileView *fileView)
 : BHandler("SNodeMonitor"),
   iTreeView(treeView),
   iFileView(fileView)
{
}

SNodeMonitor::~SNodeMonitor()
{
}

// add a node to the watch list
void SNodeMonitor::AddWatch(BMessage *message)
{
	if(message==NULL)
		return;

	node_ref nodeRef;
	if(message->FindInt32("device", &nodeRef.device) !=B_OK)
		return;

	if(message->FindInt64("node", &nodeRef.node) !=B_OK)
		return;

	// add watch
	if(watch_node(&nodeRef, B_WATCH_MOUNT | B_WATCH_NAME | B_WATCH_ATTR | B_STAT_CHANGED | B_WATCH_DIRECTORY, this)==B_OK)
		iWatchCount +=1;
}
		
// remove a node from the watch list
void SNodeMonitor::RemoveWatch(BMessage *message)
{
	if(message==NULL) {
		return;
	}

	node_ref nodeRef;
	if(message->FindInt32("device", &nodeRef.device) !=B_OK) {
		return;
	}
	if(message->FindInt64("node", &nodeRef.node) !=B_OK) {
		return;
	}

	if(watch_node(&nodeRef, B_STOP_WATCHING, this)==B_OK) {
		iWatchCount -=1;
	}
}

// message handler
void SNodeMonitor::MessageReceived(BMessage *message)
{
	if(message==NULL)
		return;

	// this should really be a BMessageFilter
	if(message->what!=B_NODE_MONITOR)
		return;

	int32 op;
	entry_ref toEntryRef, fromEntryRef; 
	const char *name;
	
	message->FindInt32("opcode", &op);
	message->FindInt32("device", &fromEntryRef.device);

	
	if(op==B_ENTRY_MOVED)
	{
		message->FindInt64("from directory", &fromEntryRef.directory); 
		message->FindString("name", &name);
		fromEntryRef.set_name(name);

		message->FindInt32("device", &toEntryRef.device);
		message->FindInt64("to directory", &toEntryRef.directory); 
		toEntryRef.set_name(name);
	}
	else
	{
		message->FindInt64("directory", &fromEntryRef.directory); 
		message->FindString("name", &name);
		fromEntryRef.set_name(name);
	}

	if(op==B_DEVICE_MOUNTED)
	{
		message->FindInt64("directory", &fromEntryRef.directory); 
		message->FindInt32("new device", &fromEntryRef.device);
		BVolume volume(fromEntryRef.device);
	   	char volname[B_FILE_NAME_LENGTH];
	   	volume.GetName(volname);
	   	
		SFile volfile(&volume);
		iTreeView->ScanForCreated(&volfile);
		return;
	}
	
	if(op==B_STAT_CHANGED)
		message->FindInt64("node", &fromEntryRef.directory);

	SFile fromFile(&fromEntryRef);
	SFile toFile(&toEntryRef);

	// update views
	switch(op)
	{
		case B_STAT_CHANGED:
		{
			iFileView->ScanForStat(&fromFile);
			break;
		}
		case B_ENTRY_CREATED:
		{
			iTreeView->ScanForCreated(&fromFile);
			iFileView->ScanForCreated(&fromFile);
			break;
		}
		case B_ENTRY_REMOVED:
		{
			iTreeView->ScanAllForRemoved(); 
			iFileView->ScanAllForRemoved();
			iTreeView->ScanUpdateTrashIcon();
			break;
		}
		case B_ENTRY_MOVED:
		{
			iTreeView->ScanForMoved(&fromFile, &toFile);
			iFileView->ScanForMoved(&fromFile, &toFile);
			iTreeView->ScanUpdateTrashIcon();
			break;
		}
		default:
			break;
	}
}
