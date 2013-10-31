#ifndef _PFILEVIEW_H_
#define _PFILEVIEW_H_

#include <Bitmap.h>
#include <Entry.h>
#include <Path.h>
#include <OS.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "CLVListItem.h"
#include "ColumnListView.h"
#include "SeekerItem.h"

class SFileView;

class SFileItem : public SeekerItem
{
	public:
		SFileItem(SFileView *FileView, const SFile *file);
		~SFileItem();
		
		virtual void DrawItemColumn(BView *owner, BRect itemColumnRect, 
									 int32 columnIndex, bool complete);
		
		virtual void Update(BView *owner, const BFont *font);
		float LabelWidth() { return labelwidth; }
		BRect LabelRect(void) const { return labelrect; }
	private:

		SFileView *iFileView;
		
		float iTextOffset;
		font_height iFontAttr;
		float labelwidth;
		BRect labelrect;
};

class SFileView : public ColumnListView
{
	public:
	
		SFileView(BRect bounds, char *name);
		~SFileView();
		
		/* implemented MouseDown handler */
		virtual void MouseDown(BPoint point);
		virtual void MouseMoved(BPoint pt, uint32 transit, const BMessage *msg);
		virtual void MouseUp(BPoint point);
		virtual void KeyDown(const char *bytes,int32 numbytes);
//		virtual bool InitiateDrag(BPoint pt, int32 index, bool selected);
		virtual bool StartDrag(BPoint pt, int32 index, bool selected);
		virtual void MessageReceived(BMessage *msg);
		virtual void Draw(BRect update);

		// return container view
		CLVContainerView* View() { return iContainerView; }

		// set columns
		void SetColumns(bool useName, bool useSize, bool useModified,
			bool useCreated, bool useKind, bool usePath);

		// refresh filelist
		void Refresh(BMessage *message);
		void Refresh(void);
		
		// clear all items
		void Clear() { Refresh(NULL); }
		
		// current file selections
		void CurrentSelections(SFileList *fileList);	
		
		// open file or directory
		void Open(BMessage *message=NULL);
		
		void SelectAll();
		void InvertSelection(int32 start=0, int32 end=-1);
		void SelectRange(int32 start, int32 end, bool selected=true, bool extend=false);

		// node monitor: scan for created
		void ScanForCreated(const SFile *file);
		
		// node monitor: scan for stat change
		void ScanForStat(const SFile *file);
		
		// node monitor: scan for moved
		void ScanForMoved(const SFile *fromFile, const SFile *toFile);

		// node monitor: scan for removed
		void ScanAllForRemoved();
		void SpawnTracker();
		const char *Path();
		SFile *PathAsFile();

		void StartTracking(BRect rect, uint32 how);
		void EndTracking(void);
		bool IsTracking(void) const { return istracking; }
	protected:
	
		// build columns
		void BuildColumns();
		
		// Adjust columns based on saved preferences
		void LoadColumnSettings();
		void SaveColumnSettings();
		
		int32 FindNextAlphabetical(char c, int32 index=0);

		// sorting function
		static int Compare(const CLVListItem* aItem1, const CLVListItem* aItem2, int32 sortKey);

		// shared thread information
		thread_id			iScanThreadId;
		sem_id				iScanThreadActive;
		sem_id				iScanThreadTerminate;
		entry_ref			iScanEntryRef;

		// threaded file scan function
		static int32 ScanThread(void *dataPtr);

		// request scan termination and wait for it to stop
		void RequestScanTerminate();

	private:
	
		CLVContainerView*	iContainerView;
		CLVListItem *iTreeItem;
				
		bool				iUseName,
							iUseSize,
							iUseModified,
							iUseCreated,
							iUseKind,
							iUsePath;

		CLVColumn			*iColIcon,
							*iColName,
							*iColSize,
							*iColModified,
							*iColCreated,
							*iColKind,
							*iColPath;

		bool iFileScan;
		bool istracking;
		uint32 mousebuttons;
		BPoint mdownpt,currentpt;
		SFileItem *mdownitem;
		bool isdragging,isselecting;
		bool folderbitmap;
		int32 currentitem,lastitem;
		int32 mdownindex,olddropselect;
		bool oldselected;
		bool dblclick;
		bigtime_t lastclick;
};

#endif
