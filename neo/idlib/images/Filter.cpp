#include "precompiled.h"

void GetCubicWeights(float s, float A, float* bvals) {
	double sA = s * A;
	double sAs = s * sA;
	double sAss = s * sAs;

	bvals[0] = sA - 2 * A * s * s + sAss;
	bvals[1] = 1.0f - (A + 3.0f) * s * s + (A + 2.0f) * s * s * s;
	bvals[2] = s * (2 * A + 3.0f) * s - sA - (A + 2.0f) * s * s * s;
	bvals[3] = sAs - sAss;
}

void idFilter::UpScale2xBicubic_Generic(
	const unsigned __int8* src,
	int width,
	int height,
	int stride,
	unsigned __int8* dst,
	idFilter::bicubicFilter_t type)
{
	float A = -0.75f;
	float bvals[4];
	float weights[4];
	float table[4][16] = { 0 };

	if (type == BICUBIC_SHIFTED) {
		GetCubicWeights(0.8f, A, bvals);
		GetCubicWeights(0.2f, A, weights);
	}
	else {
		GetCubicWeights(0.75f, A, bvals);
		GetCubicWeights(0.25f, A, weights);
	}

	for (int i = 0; i < 2; ++i) {
		int index = i & 1;
		for (int j = 0; j < 2; ++j) {
			int offset = j & 1;
			float* sourceWeights = &bvals[4 * offset];
			float* tablePtr = &table[offset | (2 * index)][2];
			for (int k = 0; k < 4; ++k) {
				tablePtr[0] = sourceWeights[0] * weights[k];
				tablePtr[1] = sourceWeights[1] * weights[k];
				tablePtr[2] = sourceWeights[2] * weights[k];
				tablePtr[3] = sourceWeights[3] * weights[k];
				tablePtr += 4;
			}
		}
	}

	for (int y = 0; y < 2 * height; ++y) {
		for (int x = 0; x < 2 * width; ++x) {
			int srcY = (y >> 1) - 1;
			int srcX = (x >> 1) - 1;

			float accum[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			for (int ky = 0; ky < 4; ++ky) {
				int yy = srcY + ky;
				if (yy < 0) yy = 0;
				if (yy >= height) yy = height - 1;

				for (int kx = 0; kx < 4; ++kx) {
					int xx = srcX + kx;
					if (xx < 0) xx = 0;
					if (xx >= width) xx = width - 1;

					for (int c = 0; c < 4; ++c) {
						accum[c] += src[4 * (yy * stride + xx) + c] * table[ky][kx * 4 + c];
					}
				}
			}

			for (int c = 0; c < 4; ++c) {
				dst[4 * (y * 2 * width + x) + c] = static_cast<unsigned __int8>(fminf(fmaxf(accum[c], 0.0f), 255.0f));
			}
		}
	}
}
