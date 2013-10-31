#ifndef COPY_FOLDER_WINDOW_H
#define COPY_FOLDER_WINDOW_H

#include <Window.h>
#include <List.h>
#include "SFile.h"
#include "SeekerWindow.h"

typedef struct
{
	entry_ref from;
	entry_ref to;
} copy_record;

class SCopyWindow : public BWindow
{
	public:
		
		SCopyWindow(SeekerWindow *mainWindow, BList *fileList,
				const SFile *destFolder, int copyOp, int pasteOp);
		~SCopyWindow();
		
		virtual void MessageReceived(BMessage* message);

		virtual void Show(void);
		
	protected:
	
//		static const BRect windowFrame () 
//		{
//			return BRect(50, 50, 350, 100);
//		}

		// shared thread information
		thread_id			iCopyThreadId;
		sem_id				iCopyThreadActive;
		sem_id				iCopyThreadTerminate;

		// threaded file opeation functions
		static int32 CopyThread(void *dataPtr);

		// request copy termination and wait for it to stop
		void RequestCopyTerminate();

		SeekerWindow*	iMainWindow;
	private:

		BView*			iView;

		BStatusBar*		iStatusBar;
		BButton*		iCancelButton;	

		BList*			iFileList;
		SFile*			iDestFolder;
		int				iCopyOp;
		int				iPasteOp;
	
};


#endif
