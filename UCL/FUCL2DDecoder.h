/************************************************************************************
 STUNS v0.1
 (c) Andrii Frolov aka kznsq, 2004
 falinc@ukr.net
************************************************************************************/

#ifndef FUCL2DDECODER_H
#define FUCL2DDECODER_H

#include "..\\FBaseDecoder.h"

class FUCL2DDecoder : public FBaseDecoder {
public:
	bool Decompress(void);

	FUCL2DDecoder(void) : FBaseDecoder()
	{
		strcpy(ClassName, "UCL2D");
	}

	~FUCL2DDecoder(void)
	{
	}
};

#endif
