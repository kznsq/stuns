/************************************************************************************
 STUNS v0.1
 (c) Andrii Frolov aka kznsq, 2004
 falinc@ukr.net
************************************************************************************/

#ifndef FPKWAREDECODER_H
#define FPKWAREDECODER_H

#include "../FBaseDecoder.h"

class FPKWareDecoder : public FBaseDecoder {
	static unsigned inf(void *parent, unsigned char **buf)
	{
		*buf = ((FBaseDecoder*)parent)->InData + ((FBaseDecoder*)parent)->InPos;
		return ((FBaseDecoder*)parent)->InLen - ((FBaseDecoder*)parent)->InPos;
	}
	static int outf(void *parent, unsigned char *buf, unsigned len)
	{
		// output does not fit: blast() stops with an error
		if(len > MAX_OUT_SIZE - ((FBaseDecoder*)parent)->OutPos)
			return 1;
		memcpy(((FBaseDecoder*)parent)->OutData + ((FBaseDecoder*)parent)->OutPos, buf, len);
		((FBaseDecoder*)parent)->OutPos += len;
		return 0;
	}
public:
	bool Decompress(void);

	FPKWareDecoder(void) : FBaseDecoder()
	{
		strcpy(ClassName, "PKWare");
	}

	~FPKWareDecoder(void)
	{
	}
};

#endif
