#include "SeekerItem.h"

SeekerItem::SeekerItem(const SFile *file,uint32 level, bool superitem, 
	bool expanded, float minheight)
 : CLVListItem(level,superitem,expanded,minheight)
{
	iFile=new SFile(file);
}

SeekerItem::~SeekerItem(void)
{
	delete iFile;
}
