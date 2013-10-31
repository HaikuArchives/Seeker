#ifndef _PIONEERWINDOW_H_
#define _PIONEERWINDOW_H_

#include <Window.h>
#include <MenuBar.h>
#include <PopUpMenu.h>
#include <OS.h>

#include "SplitterView.h"
#include "STreeView.h"
#include "SFileView.h"
#include "SNodeMonitor.h"
#include "SFileWorker.h"

#define MID_FILE						100
#define MID_FILE_NEW_FOLDER				110
#define MID_FILE_OPEN					120
#define MID_FILE_GET_INFO				130
#define MID_FILE_EDIT_NAME				140
#define MID_FILE_MOVE_TO_TRASH			150
#define MID_FILE_EMPTY_TRASH			160
#define MID_FILE_QUIT					170

#define MID_EDIT						200
#define MID_EDIT_CUT					210
#define MID_EDIT_COPY					220
#define MID_EDIT_PASTE					230
#define MID_EDIT_PASTE_LINK				240
#define MID_EDIT_SELECT_ALL				250
#define MID_EDIT_SELECT_INVERT			260

#define MID_VIEW						300
#define MID_VIEW_COLUMNS				310
#define MID_VIEW_COLUMNS_NAME			311
#define MID_VIEW_COLUMNS_SIZE			312
#define MID_VIEW_COLUMNS_MODIFIED		313
#define MID_VIEW_COLUMNS_CREATED		314
#define MID_VIEW_COLUMNS_KIND			315
#define MID_VIEW_REFRESH				320

#define MID_TOOLS						400
#define MID_TOOLS_FIND					410
#define MID_TRACKER_HERE				420
#define MID_TRACKER_ADDON				430
#define MID_REFRESH_TRACKER_ADDONS		440

#define MID_HELP						500
#define MID_HELP_ABOUT					510

#define MID_DROP_LINK_HERE				600
#define MID_DROP_MOVE_HERE				610
#define MID_DROP_COPY_HERE				620

#define MSG_PIONEER_UPDATE_TITLE		1000
#define MSG_PIONEER_REMOVE_FROM_SUBSET	1001
#define MSG_PIONEER_VOLUME_POPUP_MENU	1002
#define MSG_PIONEER_FOLDER_POPUP_MENU	1003
#define MSG_PIONEER_FILE_POPUP_MENU		1004
#define MSG_PIONEER_TRASH_POPUP_MENU	1005

#define MSG_TREEVIEW_REFRESH			2000
#define MSG_TREEVIEW_EXPAND				2001

#define MSG_FILEVIEW_REFRESH			3000
#define MSG_FILEVIEW_INVOKE				3001
#define MSG_FILEVIEW_CLEAR				3002
#define MSG_FILEVIEW_REFRESH_CURRENT	3003

#define MSG_NODEMONITOR_ADD_WATCH		4000
#define MSG_NODEMONITOR_REMOVE_WATCH	4001

#define MSG_NEWFOLDER_WINDOW_OK			5000
#define MSG_NEWFOLDER_WINDOW_CANCEL		5001

#define MSG_RENAME_WINDOW_OK			6000
#define MSG_RENAME_WINDOW_CANCEL		6001
#define MSG_RENAME_WINDOW_CANCEL_ALL	6002

#define MSG_COPY_WINDOW_CANCEL			7000

#define MSG_INFO_WINDOW_CLOSE_ALL		8000

class SeekerApp;

class SeekerWindow : public BWindow
{
	public:
		
		SeekerWindow(SeekerApp *app, BRect frame);
		~SeekerWindow();
		
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage* message);
		virtual void WindowActivated(bool active);
		virtual void FrameMoved(BPoint origin);
		virtual void FrameResized(float width, float height);
	protected:
		SeekerApp *iApp;
	
	private:
		// create menus
		void BuildMenuBar();
		
		// create tree-view
		void BuildTreeView();
		
		// create file-view
		void BuildFileView();
		
		// toggle column selection and update file-view
		void UpdateColumns(BMenuItem *item);
		
		// update path in window title
		void UpdateTitle(BMessage *message);
	
		// remove floating window from subset
		void RemoveFromSubset(BMessage *message);

		// current selections
		void CurrentSelections(SFileList *fileList);	

		// current folder
		void CurrentFolder(SFile **file, bool useSuperItem);

		void VolumePopUp(BMessage *message);
		void FolderPopUp(BMessage *message);
		void FilePopUp(BMessage *message);
		void TrashPopUp(BMessage *message);
		void DropPopUp(BMessage *message);
		
		void RefreshTrackerAddons(void);
		void RunTrackerAddon(image_id addon);
		static int32 TrackerAddonThread(void *data);
	
		// menus
		BMenuBar*       iMenuBar;

        BMenu*			iFileMenu;
			BMenuItem*		iFileNewFolderItem;
			BMenuItem*		iFileOpenItem;
			BMenuItem*		iFileGetInfoItem;
			BMenuItem*		iFileEditNameItem;
			BMenuItem*		iFileMoveToTrashItem;
			BMenuItem*		iFileEmptyTrashItem;
			BMenuItem*		iFileQuitItem;

		BMenu*			iEditMenu;
			BMenuItem*		iEditCutItem;
			BMenuItem*		iEditCopyItem;
			BMenuItem*		iEditPasteItem;
			BMenuItem*		iEditPasteLinkItem;
			BMenuItem*		iEditSelectAllItem;
			BMenuItem*		iEditSelectInvertItem;			
			
		BMenu*			iViewMenu;
			BMenuItem*		iViewColumnsNameItem;
			BMenuItem*		iViewColumnsSizeItem;
			BMenuItem*		iViewColumnsModifiedItem;
			BMenuItem*		iViewColumnsCreatedItem;
			BMenuItem*		iViewColumnsKindItem;
			BMenuItem*		iViewRefreshItem;
			
		BMenu*			iToolsMenu;
			BMenuItem*		iToolsFindItem;
			
		BMenu*			iHelpMenu;
			BMenuItem*		iHelpAboutItem;
		
		// pop-up menus
		BPopUpMenu*		iVolumePopUpMenu;
			BMenuItem*		iVolumePopUpOpenItem;
			BMenuItem*		iVolumePopUpNewFolderItem;
			BMenuItem*		iVolumePopUpGetInfoItem;
			BMenuItem*		iVolumePopUpEditNameItem;
			BMenuItem*		iVolumePopUpTrackerHereItem;
			BMenuItem*		iVolumePopUpUnmountItem;

		BPopUpMenu*		iFolderPopUpMenu;
			BMenuItem*		iFolderPopUpOpenItem;
			BMenuItem*		iFolderPopUpNewFolderItem;
			BMenuItem*		iFolderPopUpGetInfoItem;
			BMenuItem*		iFolderPopUpEditNameItem;
			BMenuItem*		iFolderPopUpMoveToTrashItem;
			BMenuItem*		iFolderPopUpCutItem;
			BMenuItem*		iFolderPopUpCopyItem;
			BMenuItem*		iFolderPopUpPasteItem;
			BMenuItem*		iFolderPopUpPasteLinkItem;
			BMenuItem*		iFolderPopUpTrackerHereItem;
					
		BPopUpMenu*		iFilePopUpMenu;
			BMenuItem*		iFilePopUpOpenItem;
			BMenuItem*		iFilePopUpGetInfoItem;
			BMenuItem*		iFilePopUpEditNameItem;
			BMenuItem*		iFilePopUpMoveToTrashItem;
			BMenuItem*		iFilePopUpCutItem;
			BMenuItem*		iFilePopUpCopyItem;
		
		BPopUpMenu*		iTrashPopUpMenu;
			BMenuItem*		iTrashPopUpEmptyTrashItem;
			BMenuItem*		iTrashPopUpOpenItem;
			BMenuItem*		iTrashPopUpGetInfoItem;

		BPopUpMenu*		iDropPopUpMenu;
			BMenuItem*		iDropPopUpCreateLinkItem;
			BMenuItem*		iDropPopUpMoveItem;
			BMenuItem*		iDropPopUpCopyItem;
			BMenuItem*		iDropPopUpCancelItem;

		BMenu*	iTrackerAddonMenu;		
		// splitter-view
		CSplitterView*	iSplitterView;
			
		// tree-view
		STreeView*		iTreeView;
		
		// file-view
		SFileView*		iFileView;									

		// node monitor
		SNodeMonitor*	iNodeMonitor;
		
		// file worker
		SFileWorker*	iFileWorker;
};

BMenu *GenerateIconMenu(const SFileList *filelist);

typedef void addon_process_refs(entry_ref dir_ref, BMessage *msg, void *reserved);

#endif
