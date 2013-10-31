#ifndef _PTREEVIEW_H_
#define _PTREEVIEW_H_

#include <Bitmap.h>
#include <Entry.h>
#include <VolumeRoster.h>

#include "CLVListItem.h"
#include "ColumnListView.h"
#include "SeekerItem.h"
#include "SFile.h"

class STreeView;

//class STreeItem : public CLVListItem

class STreeItem : public SeekerItem

{
	public:

		STreeItem(STreeView *treeView, SFile *file, bool expand=false);
		~STreeItem();
		
		virtual void DrawItemColumn(BView *owner, BRect itemColumnRect, 
									 int32 columnIndex, bool complete);
		virtual void Update(BView *owner, const BFont *font);
		
		// expand the item and select a specific directory
		void ExpandAndSelect(SFile *file);

		bool IsScanning();
		bool IsScanned();
		void ResetIcon();
		const char *Name() { return (iFile)?iFile->Name():NULL; }
		float LabelWidth() { return labelwidth; }
		float LabelStart() { return labelstart; }
		bool IsVolume()
		{
			if (iFile) return iFile->IsVolume();
			else return false;
		}
//		SFile* File() {	return iFile; }
		void RequestScanTerminate();

	protected:

		sem_id				iScanThreadActive;
		sem_id				iScanThreadTerminate;

		static int32 ScanThread(void *dataPtr);
		void SetAutoSelectOnScanDir(const char *name);
	
	private:

		STreeView*  iTreeView;
//		SFile*		iFile;

		char 		iAutoSelectDir[B_FILE_NAME_LENGTH];
		float 		iTextOffset;
		font_height iFontAttr;
		bool		iTreeScan;
		bool		iFileScan;
		float		labelwidth, labelstart;
};

class STreeView : public ColumnListView
{
	public:
	
		STreeView(BRect bounds, char *name);
		~STreeView();

		virtual void MouseDown(BPoint point);
		virtual void MouseMoved(BPoint pt, uint32 transit, const BMessage *msg);
		virtual void MouseUp(BPoint point);
//		virtual bool InitiateDrag(BPoint pt, int32 index, bool selected);
		virtual bool StartDrag(BPoint pt, int32 index, bool selected);
		CLVContainerView* View() {	return iContainerView; }
		virtual void Expand(CLVListItem* item);

		void Refresh();
		void AbortScan();
		void CurrentSelections(SFileList *fileList);	
		void SpawnTracker();
		void HandleDrop(BMessage *msg, int8 fileop);
		
		void ExpandAndSelect(BMessage *message);
		void ExpandAndSelect(entry_ref ref);
		void Collapse(CLVListItem *item);
		bool HasItemUnder(STreeItem *item, STreeItem*subitem);

		void ScanForCreated(SFile *file);
		void ScanForMoved(SFile *fromFile, SFile *toFile);
		void ScanAllForRemoved();
		void ScanUpdateTrashIcon();
		virtual void KeyDown(const char *bytes,int32 numbytes);
		virtual void SelectionChanged(void);
		STreeItem *FindItem(const char *path, int32 offset=0);
		void ScrollToItem(STreeItem *item);
	protected:
		void BuildColumns();
		static int Compare(const CLVListItem* aItem1, const CLVListItem* aItem2, int32 sortKey);
		int32 FindNextAlphabetical(char c, int32 index=0);

	private:
		CLVContainerView*	iContainerView;
		BVolumeRoster *volumeRoster;
		uint32 mousebuttons;
		BPoint mdownpt;
		STreeItem *mdownitem;
		bool isdragging;
		int32 previtem;
		bigtime_t lastclick;
		bool dblclick;
		int32 olddropselect;
		bool oldselected;
		CLVColumn *foldercolumn;
};

#endif