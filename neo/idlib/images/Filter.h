// Filter.h
//

#pragma once

class idFilter {
public:
	enum bicubicFilter_t {
		BICUBIC_DEFAULT,
		BICUBIC_SHIFTED
	};

	// Static method to upscale using bicubic filtering
	static void UpScale2xBicubic_Generic(
		const unsigned __int8* src,
		int width,
		int height,
		int stride,
		unsigned __int8* dst,
		bicubicFilter_t type);

};