#pragma once

#ifndef __COLORSPACE_H__
#define __COLORSPACE_H__

/*
================================================================================================
Contains the ColorSpace conversion declarations.
================================================================================================
*/

namespace idColorSpace {
	void	ConvertRGBToYCoCg(byte* dst, const byte* src, int width, int height);
	void	ConvertYCoCgToRGB(byte* dst, const byte* src, int width, int height);

	void	ConvertRGBToCoCg_Y(byte* dst, const byte* src, int width, int height);
	void	ConvertCoCg_YToRGB(byte* dst, const byte* src, int width, int height);
	void	ConvertCoCgSYToRGB(byte* dst, const byte* src, int width, int height);

	void	ConvertRGBToYCoCg420(byte* dst, const byte* src, int width, int height);
	void	ConvertYCoCg420ToRGB(byte* dst, const byte* src, int width, int height);

	void	ConvertRGBToYCbCr(byte* dst, const byte* src, int width, int height);
	void	ConvertYCbCrToRGB(byte* dst, const byte* src, int width, int height);

	void	ConvertRGBToCbCr_Y(byte* dst, const byte* src, int width, int height);
	void	ConvertCbCr_YToRGB(byte* dst, const byte* src, int width, int height);

	void	ConvertRGBToYCbCr420(byte* dst, const byte* src, int width, int height);
	void	ConvertYCbCr420ToRGB(byte* dst, const byte* src, int width, int height);

	void	ConvertNormalMapToStereographicHeightMap(byte* heightMap, const byte* normalMap, int width, int height, float& scale);
	void	ConvertStereographicHeightMapToNormalMap(byte* normalMap, const byte* heightMap, int width, int height, float scale);

	void	ConvertRGBToMonochrome(byte* mono, const byte* rgb, int width, int height);
	void	ConvertMonochromeToRGB(byte* rgb, const byte* mono, int width, int height);
};

#endif // !__COLORSPACE_H__