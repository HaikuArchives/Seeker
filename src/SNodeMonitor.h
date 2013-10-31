#ifndef _PNODEMONITOR_H_
#define _PNODEMONITOR_H_

#include <Handler.h>
#include <Entry.h>

class STreeView;
class SFileView;

class SNodeMonitor : public BHandler
{
	public:
		SNodeMonitor(STreeView *treeView, SFileView *fileView);
		~SNodeMonitor();

		/* add a node to the watch list */
		void AddWatch(BMessage *message);
		
		/* remove a node from the watch list */
		void RemoveWatch(BMessage *message);
		
		/* get # of actives watches */
		int32 WatchCount() {
			return iWatchCount;
		}

	protected:
	
		/* message handler */
		virtual void MessageReceived(BMessage *message=NULL);		
	
	private:
	
		STreeView*	iTreeView;
		SFileView*	iFileView;
		
		int32		iWatchCount;		
};

#endif
