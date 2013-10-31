#ifndef _PFILEINFO_H_
#define _PFILEINFO_H_

#include <View.h>
#include <Bitmap.h>
#include <Window.h>
#include <Font.h>

class SeekerWindow;
class SFileInfoWindow;

class SFileInfoView : public BView
{
	public:
		SFileInfoView(SFileInfoWindow *infoWindow, const SFile *file,
			BRect frame, const char *name=NULL,bool multiple=false,
			uint32 resizingMode=B_FOLLOW_ALL, uint32 flags=B_WILL_DRAW);
		~SFileInfoView();

		// implemented Draw function
		virtual void Draw(BRect rect);

	private:
		SFileInfoWindow*	iInfoWindow;
		SFile*				iFile;
		float 				iTextOffset;
		BRect				shadowRect, bitmapRect;
		BFont				font;
};

class SFileInfoWindow : public BWindow
{
	public:
		SFileInfoWindow(SeekerWindow *mainWindow,const SFile *file,
			bool multiple=false,int32 offset=0);
		~SFileInfoWindow();
		
	protected:
//		static const BRect windowFrame () {
//			return BRect(100, 100, 400, 300);
//		}
	
	private:
		SeekerWindow*	iMainWindow;
		SFile*			iFile;

		BView*			iFileInfoView;	
};

#endif