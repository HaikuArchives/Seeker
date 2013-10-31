#ifndef SEEKER_ITEM_H_
#define SEEKER_ITEM_H_

#include "CLVListItem.h"
#include "SFile.h"

class SeekerItem : public CLVListItem
{
public:
	SeekerItem(const SFile *file,uint32 level=0, bool superitem=false, 
		bool expanded=false, float minheight=0.0);
	virtual ~SeekerItem(void);
	SFile *File() { return iFile; }

protected:
	SFile *iFile;
};

#endif
