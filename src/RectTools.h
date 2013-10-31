#ifndef BRECT_TOOLS_H
#define BRECT_TOOLS_H

#include <Rect.h>

void Normalize(BRect *r);
bool Intersects(const BRect &r1, const BRect &r2);
bool ContainsPoint(const BRect &r, const BPoint &pt);
bool ContainsRect(const BRect &r1, const BRect &r2);

#endif