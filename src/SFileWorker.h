#ifndef _PFILEWORKER_H_
#define _PFILEWORKER_H_

#include <List.h>
#include <Window.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TextControl.h>
#include <Button.h>

class SeekerWindow;

#define P_OP_NONE		0
#define P_OP_COPY		1
#define P_OP_CUT		2
#define P_OP_PASTE		3
#define P_OP_PASTE_LINK	4
#define P_OP_REMOVE		5

class SFileWorker
{
	public:
		SFileWorker(SeekerWindow *mainWindow);
		~SFileWorker();

		// single stage operations
		void NewFolder(const SFile *parentFolder);
//		void GetInfo(const SFile *file, int32 offset=0);
		void GetInfo(const SFileList *filelist);
		void EditName(const SFileList *filelist);
		void MoveToTrash(const SFile *srcFolder, const SFileList *fileList);
		void EmptyTrash(void);

		// add files to the copy list
		void Copy(const SFile* srcFolder, const SFileList *fileList);
		void Cut(const SFile* srcFolder, const SFileList *fileList);

		// start a file operation
		void Paste(const SFile *folder);
		void PasteLink(const SFile *folder);
		void ModifyRestoreAttribute(const SFileList *fileList, bool add);
		
		void CopyFiles(SFileList *fileList, const SFile *folder);
		void MoveFiles(SFileList *fileList, const SFile *folder);
		void MakeLinks(SFileList *fileList, const SFile *folder);
		
	private:
		SeekerWindow*	iWindow;

		int				iFileOp;
		BList*			iFileList;		
};

class SNewFolderWindow : public BWindow
{
	public:
	
		SNewFolderWindow(SeekerWindow *mainWindow,
					 	  const SFile *parentFile);
		~SNewFolderWindow();
		
		virtual void MessageReceived(BMessage* message);
		
	protected:
	
//		static const BRect windowFrame () {
//			return BRect(50, 50, 350, 200);
//		}
		
		void CreateFolder();

	private:
	
		SeekerWindow*	iMainWindow;
		SFile*			iParentFile;

		BView*			iView;

		BTextControl*	iParentTextControl;
		BTextControl*	iNameTextControl;
		
		BButton*		iOkButton;
		BButton*		iCancelButton;
};

class SRenameWindow : public BWindow
{
	public:
		SRenameWindow(SeekerWindow *mainWindow,const SFile *file,bool multiple, int32 offset=0);
		~SRenameWindow();
		
		virtual void MessageReceived(BMessage* message);
		
	protected:
//		static const BRect windowFrame () {
//			return BRect(50, 50, 350, 200);
//		}
		void RenameFile();

	private:
		SeekerWindow*	iMainWindow;
		SFile*			iFile;

		BView*			iView;

		BTextControl*	iOldNameTextControl;
		BTextControl*	iNewNameTextControl;
		
		BButton*		iOkButton;
		BButton*		iCancelAllButton;
		BButton*		iCancelButton;
};

#endif
