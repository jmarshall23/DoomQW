#pragma once


/*
================================================
idDxtDecoder decodes DXT-compressed Images. Raw output Images are in
4-byte RGBA format. Raw output NormalMaps are in 4-byte tangent-space NxNyNz format.
================================================
*/
class idDxtDecoder {
public:

	// DXT1 decompression (no alpha)
	void	DecompressImageDXT1(const byte* inBuf, byte* outBuf, int width, int height);

	// DXT5 decompression
	void	DecompressImageDXT5(const byte* inBuf, byte* outBuf, int width, int height);

	// DXT5 decompression with nVidia 7x hardware bug
	void	DecompressImageDXT5_nVidia7x(const byte* inBuf, byte* outBuf, int width, int height);

	// CTX1
	void	DecompressImageCTX1(const byte* inBuf, byte* outBuf, int width, int height) { /* not implemented */ assert(0); }

	// DXN1
	void	DecompressImageDXN1(const byte* inBuf, byte* outBuf, int width, int height) { /* not implemented */ assert(0); }

	// YCoCg DXT5 (the output is in CoCg_Y format)
	void	DecompressYCoCgDXT5(const byte* inBuf, byte* outBuf, int width, int height);

	// YCoCg CTX1 + DXT5A (the output is in CoCg_Y format)
	void	DecompressYCoCgCTX1DXT5A(const byte* inBuf, byte* outBuf, int width, int height);

	// tangent space normal map decompression from DXT1 format
	void	DecompressNormalMapDXT1(const byte* inBuf, byte* outBuf, int width, int height);
	void	DecompressNormalMapDXT1Renormalize(const byte* inBuf, byte* outBuf, int width, int height);

	// tangent space normal map decompression from DXT5 format
	void	DecompressNormalMapDXT5(const byte* inBuf, byte* outBuf, int width, int height);
	void	DecompressNormalMapDXT5Renormalize(const byte* inBuf, byte* outBuf, int width, int height);

	// tangent space normal map decompression from DXN2 format
	void	DecompressNormalMapDXN2(const byte* inBuf, byte* outBuf, int width, int height);

	// decompose a DXT image into indices and two images with colors
	void	DecomposeImageDXT1(const byte* inBuf, byte* colorIndices, byte* pic1, byte* pic2, int width, int height);
	void	DecomposeImageDXT5(const byte* inBuf, byte* colorIndices, byte* alphaIndices, byte* pic1, byte* pic2, int width, int height);

private:
	int					width;
	int					height;
	const byte* inData;

	byte				ReadByte();
	unsigned short		ReadUShort();
	unsigned int		ReadUInt();
	unsigned short		ColorTo565(const byte* color) const;
	void				ColorFrom565(unsigned short c565, byte* color) const;
	unsigned short		NormalYTo565(byte y) const;
	byte				NormalYFrom565(unsigned short c565) const;
	byte				NormalScaleFrom565(unsigned short c565) const;
	byte				NormalBiasFrom565(unsigned short c565) const;

	void				EmitBlock(byte* outPtr, int x, int y, const byte* colorBlock);
	void				DecodeAlphaValues(byte* colorBlock, const int offset);
	void				DecodeColorValues(byte* colorBlock, bool noBlack, bool writeAlpha);
	void				DecodeCTX1Values(byte* colorBlock);

	void				DecomposeColorBlock(byte colors[2][4], byte colorIndices[16], bool noBlack);
	void				DecomposeAlphaBlock(byte colors[2][4], byte alphaIndices[16]);

	void				DecodeNormalYValues(byte* normalBlock, const int offsetY, byte& bias, byte& scale);
	void				DeriveNormalZValues(byte* normalBlock);
};

/*
========================
idDxtDecoder::ReadByte
========================
*/
ID_INLINE byte idDxtDecoder::ReadByte() {
	byte b = *inData;
	inData += 1;
	return b;
}

/*
========================
idDxtDecoder::ReadUShort
========================
*/
ID_INLINE unsigned short idDxtDecoder::ReadUShort() {
	unsigned short s = *((unsigned short*)inData);
	inData += 2;
	return s;
}

/*
========================
idDxtDecoder::ReadUInt
========================
*/
ID_INLINE unsigned int idDxtDecoder::ReadUInt() {
	unsigned int i = *((unsigned int*)inData);
	inData += 4;
	return i;
}

/*
========================
idDxtDecoder::ColorTo565
========================
*/
ID_INLINE unsigned short idDxtDecoder::ColorTo565(const byte* color) const {
	return ((color[0] >> 3) << 11) | ((color[1] >> 2) << 5) | (color[2] >> 3);
}

/*
========================
idDxtDecoder::ColorFrom565
========================
*/
ID_INLINE void idDxtDecoder::ColorFrom565(unsigned short c565, byte* color) const {
	color[0] = byte(((c565 >> 8) & (((1 << (8 - 3)) - 1) << 3)) | ((c565 >> 13) & ((1 << 3) - 1)));
	color[1] = byte(((c565 >> 3) & (((1 << (8 - 2)) - 1) << 2)) | ((c565 >> 9) & ((1 << 2) - 1)));
	color[2] = byte(((c565 << 3) & (((1 << (8 - 3)) - 1) << 3)) | ((c565 >> 2) & ((1 << 3) - 1)));
}

/*
========================
idDxtDecoder::NormalYTo565
========================
*/
ID_INLINE unsigned short idDxtDecoder::NormalYTo565(byte y) const {
	return ((y >> 2) << 5);
}

/*
========================
idDxtDecoder::NormalYFrom565
========================
*/
ID_INLINE byte idDxtDecoder::NormalYFrom565(unsigned short c565) const {
	byte c = byte((c565 & (((1 << 6) - 1) << 5)) >> 3);
	return (c | (c >> 6));
}

/*
========================
idDxtDecoder::NormalBiasFrom565
========================
*/
ID_INLINE byte idDxtDecoder::NormalBiasFrom565(unsigned short c565) const {
	byte c = byte((c565 & (((1 << 5) - 1) << 11)) >> 8);
	return (c | (c >> 5));
}

/*
========================
idDxtDecoder::NormalScaleFrom565
========================
*/
ID_INLINE byte idDxtDecoder::NormalScaleFrom565(unsigned short c565) const {
	byte c = byte((c565 & (((1 << 5) - 1) << 0)) << 3);
	return (c | (c >> 5));
}