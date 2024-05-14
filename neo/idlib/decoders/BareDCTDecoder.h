#pragma once

void IDCT_Init(void);
void IDCT_AP922_float(const char* coeff, const char* quant, __int16* dest);

class idBareDCTHuffmanTable {
public:
	unsigned __int8 bits[17];
	unsigned __int8 symbols[256];
	unsigned int code[256];
	char size[256];
	int minCode[17];
	int maxCode[18];
	int symOffset[17];
	unsigned __int8 look_nbits[256];
	unsigned __int8 look_sym[256];
	int test_nbits[16];

	void Init(const unsigned __int8* bits, const unsigned __int8* symbols);
};

class idBareDctBase {
public:
	int luminanceQuality;
	int chrominanceQuality;
	int alphaQuality;
	unsigned __int16* quantTableY;
	unsigned __int16* quantTableCoCg;
	unsigned __int16* quantTableA;
	idBareDCTHuffmanTable huffTableYDC;
	idBareDCTHuffmanTable huffTableYAC;
	idBareDCTHuffmanTable huffTableCoCgDC;
	idBareDCTHuffmanTable huffTableCoCgAC;
	idBareDCTHuffmanTable huffTableADC;
	idBareDCTHuffmanTable huffTableAAC;

	void InitHuffmanTable(void);
	void InitQuantTable(void);
	int QuantizationScaleFromQuality(int quality);
	void ScaleQuantTable(unsigned __int16* quantTable, const unsigned __int16* srcTable, int scale);
	void ScaleQuantTable_MMX(unsigned __int16* quantTable, const unsigned __int16* srcTable, int scale);
	void ScaleQuantTable_SSE2(unsigned __int16* quantTable, const unsigned __int16* srcTable, int scale);
	void SetQuality_Generic(int luminance, int chrominance, int alpha);
	void SetQuality_MMX(int luminance, int chrominance, int alpha);
	void SetQuality_SSE2(int luminance, int chrominance, int alpha);
	void SetQuality_Xenon(int luminance, int chrominance, int alpha);

	idBareDctBase(void);
	~idBareDctBase(void);
};

class idBareDctDecoder : public idBareDctBase {
public:
	int imageWidth;
	int imageHeight;
	int dcY;
	int dcCo;
	int dcCg;
	int dcA;
	int getBits;
	int getBuff;
	int dataBytes;
	const unsigned __int8* data;
	unsigned __int8 rangeLimitTable[1408];

	idBareDctDecoder(void);
	~idBareDctDecoder(void);

	void HuffmanDecode(__int16* coef, const idBareDCTHuffmanTable* dctbl, const idBareDCTHuffmanTable* actbl, int* lastDC);

	int DecodeLong_MMX(const idBareDCTHuffmanTable* htbl);
	void DecompressImageRGBA_Generic(const unsigned __int8* data, unsigned __int8* dest, int width, int height, int stride);
	void DecompressImageRGB_Generic(const unsigned __int8* data, unsigned __int8* dest, int width, int height, int stride);
	void DecompressImageYCoCg_Generic(const unsigned __int8* data, unsigned __int8* dest, int width, int height, int stride);
	void DecompressImageYCoCg_Xenon(const unsigned __int8* data, unsigned __int8* dest, int width, int height, int stride);
	void DecompressLuminanceEnhancement_Generic(const unsigned __int8* data, unsigned __int8* dest, int width, int height, int stride);
	void DecompressOneTileLuminance(unsigned __int8* dest, int stride);
	void DecompressOneTileRGB(unsigned __int8* dest, int stride);
	void DecompressOneTileRGBA(unsigned __int8* dest, int stride);
	void DecompressOneTileYCoCg(unsigned __int8* dest, int stride);
	void SetRangeTable(void);
	void StoreYCoCg(const short* block, unsigned __int8* dest, int stride);
	void YCoCgAToRGBA(const short* block, unsigned __int8* dest, int stride);
	void YCoCgToRGB(const short* block, unsigned __int8* dest, int stride);
};