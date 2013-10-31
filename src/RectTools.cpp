#include "RectTools.h"

#ifndef MIN
#define MIN(a,b) (a<b)?a:b
#endif

#ifndef MAX
#define MAX(a,b) (a>b)?a:b
#endif

void Normalize(BRect *rect)
{
	float 	l=rect->left,
			r=rect->right,
			t=rect->top,
			b=rect->bottom;
	rect->Set( MIN(l,r), MIN(t,b), MAX(l,r), MAX(t,b) );
}

bool Intersects(const BRect &r1, const BRect &r2)
{
	return ( ContainsPoint(r1,r2.LeftTop()) || ContainsPoint(r1,r2.RightTop()) ||
		ContainsPoint(r1,r2.LeftBottom()) || ContainsPoint(r1,r2.RightBottom()) ||
		ContainsPoint(r2,r1.LeftTop()) || ContainsPoint(r2,r1.RightTop()) ||
		ContainsPoint(r2,r1.LeftBottom()) || ContainsPoint(r2,r1.RightBottom()) );
}

bool ContainsPoint(const BRect &r, const BPoint &pt)
{
	return ( (pt.x>=r.left) && (pt.x<=r.right) && (pt.y>=r.top) && 
		(pt.y<=r.bottom) );
}

bool ContainsRect(const BRect &r1, const BRect &r2)
{
	return ( r1.Contains(r2) || r1==r2);
}
