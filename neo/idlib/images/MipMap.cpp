// MipMap.cpp
//

#include "precompiled.h"

void idMipMap::CreateMips(unsigned __int8* data, const unsigned __int8 power) {
	unsigned __int8* currentMip = data;
	int mipSize = 1 << power;
	int numMips = power - 1;
	const unsigned __int8* sourceMip = data;
	unsigned __int8* nextMip = &data[4 * mipSize * mipSize];
	unsigned __int8* destinationMip = nextMip;

	for (int currentMipLevel = numMips; currentMipLevel >= 2; --currentMipLevel) {
		mipSize >>= 1;
		unsigned __int8* halfMip = &currentMip[8 * mipSize];
		const unsigned __int8* inPic = halfMip;
		int currentMipSize = mipSize;
		int halfMipSize = (mipSize - 1) / 2 + 1;
		int mipBytes = 16 * mipSize;

		for (int i = mipSize; i > 0; --i) {
			unsigned __int8* out = nextMip + 2;
			const unsigned __int8* in1 = halfMip + 1;
			const unsigned __int8* in2 = currentMip + 4;
			int offset = halfMip - currentMip;

			for (unsigned int j = halfMipSize; j > 0; --j) {
				*(out - 2) = (*in2 + *(in1 - 1) + *(in2 - 4) + in2[offset]) >> 2;
				*(out - 1) = (*in1 + in1[4] + in2[1] + *(in2 - 3)) >> 2;
				*out = (in1[1] + in1[5] + in2[2] + *(in2 - 2)) >> 2;
				out[1] = (in1[2] + in1[6] + *(in2 - 1) + in2[3]) >> 2;
				out[2] = (in1[7] + in1[11] + in2[4] + in2[8]) >> 2;
				out[3] = (in1[8] + in1[12] + in2[5] + in2[9]) >> 2;
				out[4] = (in1[9] + in1[13] + in2[6] + in2[10]) >> 2;
				out[5] = (in1[10] + in1[14] + in2[7] + in2[11]) >> 2;

				in1 += 16;
				in2 += 16;
				out += 8;
			}

			currentMip = (unsigned __int8*)inPic + mipBytes;
			halfMip = (unsigned __int8*)halfMip + mipBytes;
			nextMip = destinationMip + 4 * currentMipSize;
			destinationMip = nextMip;
		}
	}
}