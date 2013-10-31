#ifndef ERROR_TRANSLATOR_H
#define ERROR_TRANSLATOR_H

#include <SupportDefs.h>
#include <GraphicsDefs.h>

void PrintSourceKit(status_t error);
void PrintErrorCode(status_t error);
void PrintColorSpace(color_space value);
void PrintMessageCode(int32 code);

#endif