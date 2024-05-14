
/*
================================================================================================
Contains the DxtEncoder and DxtDecoder declarations.
================================================================================================
*/

/*
================================================
idDxtEncoder encodes Images in a number of DXT formats. Raw input Images are assumed to be in
4-byte RGBA format. Raw input NormalMaps are assumed to be in 4-byte tangent-space NxNyNz format.

The supported formats are:
	* DXT1 = colors in 4x4 block approximated by equidistant points on a line through 3D space
	* DXT2 = DXT3 + colors are pre-multiplied by alpha
	* DXT3 = DXT1 + explicit 4-bit alpha
	* DXT4 = DXT5 + colors are pre-multiplied by alpha
	* DXT5 = DXT1 + alpha values in 4x4 block approximated by equidistant points on line through alpha space
	* CTX1 = colors in a 4x4 block approximated by equidistant points on a line through 2D space
	* DXN1 = one DXT5 alpha block (aka DXT5A, or ATI1N)
	* DXN2 = two DXT5 alpha blocks (aka 3Dc, or ATI2N)
================================================
*/
class idDxtEncoder {
public:
			idDxtEncoder() { srcPadding = dstPadding = 0; }
			~idDxtEncoder() {}

	void	SetSrcPadding( int pad ) { srcPadding = pad; }
	void	SetDstPadding( int pad ) { dstPadding = pad; }

	// high quality DXT1 compression (no alpha), uses exhaustive search to find a line through color space and is very slow
	void	CompressImageDXT1HQ( const byte *inBuf, byte *outBuf, int width, int height );
	
	// fast DXT1 compression (no alpha), for real-time use at the cost of a little quality
	void	CompressImageDXT1Fast( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressImageDXT1Fast_Generic( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressImageDXT1Fast_SSE2( const byte *inBuf, byte *outBuf, int width, int height );

	// high quality DXT1 compression (with alpha), uses exhaustive search to find a line through color space and is very slow
	void	CompressImageDXT1AlphaHQ( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }

	// fast DXT1 compression (with alpha), for real-time use at the cost of a little quality
	void	CompressImageDXT1AlphaFast( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressImageDXT1AlphaFast_Generic( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressImageDXT1AlphaFast_SSE2( const byte *inBuf, byte *outBuf, int width, int height );

	// high quality DXT5 compression, uses exhaustive search to find a line through color space and is generally
	// too slow to actually use for anything
	void	CompressImageDXT5HQ( const byte *inBuf, byte *outBuf, int width, int height );

	// fast DXT5 compression for real-time use at the cost of a little quality
	void	CompressImageDXT5Fast( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressImageDXT5Fast_Generic( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressImageDXT5Fast_SSE2( const byte *inBuf, byte *outBuf, int width, int height );

	// high quality CTX1 compression, uses exhaustive search to find a line through 2D space and is very slow
	void	CompressImageCTX1HQ( const byte *inBuf, byte *outBuf, int width, int height );

	// fast CTX1 compression for real-time use
	void	CompressImageCTX1Fast( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }
	void	CompressImageCTX1Fast_Generic( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }
	void	CompressImageCTX1Fast_SSE2( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }

	// high quality DXN1 (aka DXT5A or ATI1N) compression, uses exhaustive search to find a line through color space and is very slow
	void	CompressImageDXN1HQ( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }
	
	// fast single channel compression into, DXN1 (aka DXT5A or ATI1N) format, for real-time use
	void	CompressImageDXN1Fast( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressImageDXN1Fast_Generic( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressImageDXN1Fast_SSE2( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }

	// high quality YCoCg DXT5 compression, uses exhaustive search to find a line through color space and is very slow
	void	CompressYCoCgDXT5HQ( const byte *inBuf, byte *outBuf, int width, int height );
	
	// fast YCoCg DXT5 compression for real-time use (the input is expected to be in CoCg_Y format)
	void	CompressYCoCgDXT5Fast( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressYCoCgDXT5Fast_Generic( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressYCoCgDXT5Fast_SSE2( const byte *inBuf, byte *outBuf, int width, int height );

	// fast YCoCg-Alpha DXT5 compression for real-time use (the input is expected to be in CoCgAY format)
	void	CompressYCoCgAlphaDXT5Fast( const byte *inBuf, byte *outBuf, int width, int height );

	// high quality YCoCg CTX1 + DXT5A compression, uses exhaustive search to find a line through 2D space and is very slow
	void	CompressYCoCgCTX1DXT5AHQ( const byte *inBuf, byte *outBuf, int width, int height );

	// fast YCoCg CTX1 + DXT5A compression for real-time use (the input is expected to be in CoCg_Y format)
	void	CompressYCoCgCTX1DXT5AFast( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressYCoCgCTX1DXT5AFast_Generic( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressYCoCgCTX1DXT5AFast_SSE2( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }

	// high quality tangent space NxNyNz normal map compression into DXT1 format (Nz is not used)
	void	CompressNormalMapDXT1HQ( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressNormalMapDXT1RenormalizeHQ( const byte *inBuf, byte *outBuf, int width, int height );

	// fast tangent space NxNyNz normal map compression into DXT1 format (Nz is not used), for real-time use
	void	CompressNormalMapDXT1Fast( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }
	void	CompressNormalMapDXT1Fast_Generic( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }
	void	CompressNormalMapDXT1Fast_SSE2( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }

	// high quality tangent space _Ny_Nx normal map compression into DXT5 format
	void	CompressNormalMapDXT5HQ( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressNormalMapDXT5RenormalizeHQ( const byte *inBuf, byte *outBuf, int width, int height );

	// fast tangent space _Ny_Nx normal map compression into DXT5 format, for real-time use
	void	CompressNormalMapDXT5Fast( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressNormalMapDXT5Fast_Generic( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressNormalMapDXT5Fast_SSE2( const byte *inBuf, byte *outBuf, int width, int height );

	// high quality tangent space NxNy_ normal map compression into DXN2 (3Dc, ATI2N) format
	void	CompressNormalMapDXN2HQ( const byte *inBuf, byte *outBuf, int width, int height );
	
	// fast tangent space NxNy_ normal map compression into DXN2 (3Dc, ATI2N) format, for real-time use
	void	CompressNormalMapDXN2Fast( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressNormalMapDXN2Fast_Generic( const byte *inBuf, byte *outBuf, int width, int height );
	void	CompressNormalMapDXN2Fast_SSE2( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }

	// fast single channel conversion from DXN1 (aka DXT5A or ATI1N) to DXT1, reasonably fast (also works in-place)
	void	ConvertImageDXN1_DXT1( const byte *inBuf, byte *outBuf, int width, int height );
	
	// fast single channel conversion from DXT1 to DXN1 (aka DXT5A or ATI1N), reasonably fast (also works in-place)
	void	ConvertImageDXT1_DXN1( const byte *inBuf, byte *outBuf, int width, int height ) { /* not implemented */ assert( 0 ); }

	// fast tangent space NxNyNz normal map conversion from DXN (3Dc, ATI2N) to DXT5, reasonably fast (also works in-place)
	void	ConvertNormalMapDXN2_DXT5( const byte *inBuf, byte *outBuf, int width, int height );

	// fast tangent space NxNyNz normal map conversion DXT5 to DXN (3Dc, ATI2N), reasonably fast (also works in-place)
	void	ConvertNormalMapDXT5_DXN2( const byte *inBuf, byte *outBuf, int width, int height );

private:
	int					width;
	int					height;
	byte *				outData;
	int					srcPadding;
	int					dstPadding;

	void				EmitByte( byte b );
	void				EmitUShort( unsigned short s );
	void				EmitUInt( unsigned int i );
	unsigned int		AlphaDistance( const byte a1, const byte a2 ) const;
	unsigned int		ColorDistance( const byte *c1, const byte *c2 ) const;
	unsigned int		ColorDistanceWeighted( const byte *c1, const byte *c2 ) const;
	unsigned int		CTX1Distance( const byte *c1, const byte *c2 ) const;
	unsigned short		ColorTo565( const byte *color ) const;
	unsigned short		ColorTo565( byte r, byte g, byte b ) const;
	void				ColorFrom565( unsigned short c565, byte *color ) const;
	byte				GreenFrom565( unsigned short c565 ) const;

	void				NV4XHardwareBugFix( byte *minColor, byte *maxColor ) const;

	bool				HasConstantValuePer4x4Block( const byte *inBuf, int width, int height, int channel ) const;
	void				WriteTinyColorDXT1( const byte *inBuf, int width, int height );
	void				WriteTinyColorDXT5( const byte *inBuf, int width, int height );
	void				WriteTinyColorCTX1DXT5A( const byte *inBuf, int width, int height );
	void				WriteTinyNormalMapDXT5( const byte *NxNy, int width, int height );
	void				WriteTinyNormalMapDXN( const byte *NxNy, int width, int height );
	void				WriteTinyDXT5A( const byte *NxNy, int width, int height );

	void				GetMinMaxColorsMaxDist( const byte *colorBlock, byte *minColor, byte *maxColor ) const;
	void				GetMinMaxColorsLuminance( const byte *colorBlock, byte *minColor, byte *maxColor ) const;
	int					GetSquareAlphaError( const byte *colorBlock, const int alphaOffset, const byte minAlpha, const byte maxAlpha, int lastError ) const;
	int					GetMinMaxAlphaHQ( const byte *colorBlock, const int alphaOffset, byte *minColor, byte *maxColor ) const;
	int					GetSquareColorsError( const byte *colorBlock, const unsigned short color0, const unsigned short color1, int lastError ) const;
	int					GetMinMaxColorsHQ( const byte *colorBlock, byte *minColor, byte *maxColor, bool noBlack ) const;
	int					GetSquareCTX1Error( const byte *colorBlock, const byte *color0, const byte *color1, int lastError ) const;
	int					GetMinMaxCTX1HQ( const byte *colorBlock, byte *minColor, byte *maxColor ) const;
	int					GetSquareNormalYError( const byte *colorBlock, const unsigned short color0, const unsigned short color1, int lastError, int scale ) const;
	int					GetMinMaxNormalYHQ( const byte *colorBlock, byte *minColor, byte *maxColor, bool noBlack, int scale ) const;
	int					GetSquareNormalsDXT1Error( const int *colorBlock, const unsigned short color0, const unsigned short color1, int lastError, unsigned int &colorIndices ) const;
	int					GetMinMaxNormalsDXT1HQ( const byte *colorBlock, byte *minColor, byte *maxColor, unsigned int &colorIndices, bool noBlack ) const;
	int					GetSquareNormalsDXT5Error( const int *normalBlock, const byte *minNormal, const byte *maxNormal, int lastError, unsigned int &colorIndices, byte *alphaIndices ) const;
	int					GetMinMaxNormalsDXT5HQ( const byte *normalBlock, byte *minColor, byte *maxColor, unsigned int &colorIndices, byte *alphaIndices ) const;
	int					GetMinMaxNormalsDXT5HQFast( const byte *normalBlock, byte *minColor, byte *maxColor, unsigned int &colorIndices, byte *alphaIndices ) const;
	void				ScaleYCoCg( byte *colorBlock ) const;
	void				BiasScaleNormalY( byte *colorBlock ) const;
	void				RotateNormalsDXT1( byte *block ) const;
	void				RotateNormalsDXT5( byte *block ) const;
	int					FindColorIndices( const byte *colorBlock, const unsigned short color0, const unsigned short color1, unsigned int &result ) const;
	int					FindAlphaIndices( const byte *colorBlock, const int alphaOffset, const byte alpha0, const byte alpha1, byte *indexes ) const;
	int					FindCTX1Indices( const byte *colorBlock, const byte *color0, const byte *color1, unsigned int &result ) const;

	void				ExtractBlock( const byte *inPtr, int width, byte *colorBlock ) const;
	void				GetMinMaxBBox( const byte *colorBlock, byte *minColor, byte *maxColor ) const;
	void				InsetColorsBBox( byte *minColor, byte *maxColor ) const;
	void				SelectColorsDiagonal( const byte *colorBlock, byte *minColor, byte *maxColor ) const;
	void				ScaleYCoCg( byte *colorBlock, byte *minColor, byte *maxColor ) const;
	void				InsetYCoCgAlpaBBox( byte *minColor, byte *maxColor ) const;
	void				InsetYCoCgBBox( byte *minColor, byte *maxColor ) const;
	void				SelectYCoCgDiagonal( const byte *colorBlock, byte *minColor, byte *maxColor ) const;
	void				InsetNormalsBBoxDXT5( byte *minNormal, byte *maxNormal ) const;
	void				InsetNormalsBBox3Dc( byte *minNormal, byte *maxNormal ) const;
	void				EmitColorIndices( const byte *colorBlock, const byte *minColor, const byte *maxColor );
	void				EmitColorAlphaIndices( const byte *colorBlock, const byte *minColor, const byte *maxColor );
	void				EmitCTX1Indices( const byte *colorBlock, const byte *minColor, const byte *maxColor );
	void				EmitAlphaIndices( const byte *colorBlock, const int channel, const byte minAlpha, const byte maxAlpha );
	void				EmitGreenIndices( const byte *block, const int channel, const byte minGreen, const byte maxGreen );

	// Keeping the ASM versions to keep the performance of 32-bit debug builds reasonable.
	// The implementation using intrinsics is very slow in debug builds because registers are continuously spilled to memory.
	void				ExtractBlock_SSE2( const byte *inPtr, int width, byte *colorBlock ) const;
	void				GetMinMaxBBox_SSE2( const byte *colorBlock, byte *minColor, byte *maxColor ) const;
	void				InsetColorsBBox_SSE2( byte *minColor, byte *maxColor ) const;
	void				InsetNormalsBBoxDXT5_SSE2( byte *minNormal, byte *maxNormal ) const;
	void				EmitColorIndices_SSE2( const byte *colorBlock, const byte *minColor, const byte *maxColor );
	void				EmitColorAlphaIndices_SSE2( const byte *colorBlock, const byte *minColor, const byte *maxColor );
	void				EmitCoCgIndices_SSE2( const byte *colorBlock, const byte *minColor, const byte *maxColor );
	void				EmitAlphaIndices_SSE2( const byte *colorBlock, const int minAlpha, const int maxAlpha );
	void				EmitAlphaIndices_SSE2( const byte *colorBlock, const int channelBitOffset, const int minAlpha, const int maxAlpha );
	void				EmitGreenIndices_SSE2( const byte *block, const int channelBitOffset, const int minGreen, const int maxGreen );
	void				ScaleYCoCg_SSE2( byte *colorBlock, byte *minColor, byte *maxColor ) const;
	void				InsetYCoCgBBox_SSE2( byte *minColor, byte *maxColor ) const;
	void				SelectYCoCgDiagonal_SSE2( const byte *colorBlock, byte *minColor, byte *maxColor ) const;



	void				EmitNormalYIndices( const byte *normalBlock, const int offset, const byte minNormalY, const byte maxNormalY );
	void				EmitNormalYIndices_SSE2( const byte *normalBlock, const int offset, const byte minNormalY, const byte maxNormalY );

	void				DecodeDXNAlphaValues( const byte *inBuf, byte *values );
	void				EncodeDXNAlphaValues( byte *outBuf, const byte min, const byte max, const byte *values );

	void				DecodeNormalYValues( const byte *inBuf, byte &min, byte &max, byte *values );
	void				EncodeNormalRGBIndices( byte *outBuf, const byte min, const byte max, const byte *values );
};

/*
========================
idDxtEncoder::CompressImageDXT1Fast
========================
*/
ID_INLINE void idDxtEncoder::CompressImageDXT1Fast( const byte *inBuf, byte *outBuf, int width, int height ) {
#ifdef ID_WIN_X86_SSE2_INTRIN
	CompressImageDXT1Fast_SSE2( inBuf, outBuf, width, height );
#else
	CompressImageDXT1Fast_Generic( inBuf, outBuf, width, height );
#endif
}

/*
========================
idDxtEncoder::CompressImageDXT1AlphaFast
========================
*/
ID_INLINE void idDxtEncoder::CompressImageDXT1AlphaFast( const byte *inBuf, byte *outBuf, int width, int height ) {
#ifdef ID_WIN_X86_SSE2_INTRIN
	CompressImageDXT1AlphaFast_SSE2( inBuf, outBuf, width, height );
#else
	CompressImageDXT1AlphaFast_Generic( inBuf, outBuf, width, height );
#endif
}

/*
========================
idDxtEncoder::CompressImageDXT5Fast
========================
*/
ID_INLINE void idDxtEncoder::CompressImageDXT5Fast( const byte *inBuf, byte *outBuf, int width, int height ) {
#ifdef ID_WIN_X86_SSE2_INTRIN
	CompressImageDXT5Fast_SSE2( inBuf, outBuf, width, height );
#else
	CompressImageDXT5Fast_Generic( inBuf, outBuf, width, height );
#endif
}

/*
========================
idDxtEncoder::CompressImageDXN1Fast
========================
*/
ID_INLINE void idDxtEncoder::CompressImageDXN1Fast( const byte *inBuf, byte *outBuf, int width, int height ) {
	CompressImageDXN1Fast_Generic( inBuf, outBuf, width, height );
}

/*
========================
idDxtEncoder::CompressYCoCgDXT5Fast
========================
*/
ID_INLINE void idDxtEncoder::CompressYCoCgDXT5Fast( const byte *inBuf, byte *outBuf, int width, int height ) {
#ifdef ID_WIN_X86_SSE2_INTRIN
	CompressYCoCgDXT5Fast_SSE2( inBuf, outBuf, width, height );
#else
	CompressYCoCgDXT5Fast_Generic( inBuf, outBuf, width, height );
#endif
}

/*
========================
idDxtEncoder::CompressYCoCgCTX1DXT5AFast
========================
*/
ID_INLINE void idDxtEncoder::CompressYCoCgCTX1DXT5AFast( const byte *inBuf, byte *outBuf, int width, int height ) {
	CompressYCoCgCTX1DXT5AFast_Generic( inBuf, outBuf, width, height );
}

/*
========================
idDxtEncoder::CompressNormalMapDXT5Fast
========================
*/
ID_INLINE void idDxtEncoder::CompressNormalMapDXT5Fast( const byte *inBuf, byte *outBuf, int width, int height ) {
#ifdef ID_WIN_X86_SSE2_INTRIN
	CompressNormalMapDXT5Fast_SSE2( inBuf, outBuf, width, height );
#else
	CompressNormalMapDXT5Fast_Generic( inBuf, outBuf, width, height );
#endif
}

/*
========================
idDxtEncoder::CompressNormalMapDXN2Fast
========================
*/
ID_INLINE void idDxtEncoder::CompressNormalMapDXN2Fast( const byte *inBuf, byte *outBuf, int width, int height ) {
	CompressNormalMapDXN2Fast_Generic( inBuf, outBuf, width, height );
}

/*
========================
idDxtEncoder::EmitByte
========================
*/
ID_INLINE void idDxtEncoder::EmitByte( byte b ) {
	*outData = b;
	outData += 1;
}

/*
========================
idDxtEncoder::EmitUShort
========================
*/
ID_INLINE void idDxtEncoder::EmitUShort( unsigned short s ) {
	*((unsigned short *)outData) = s;
	outData += 2;
}

/*
========================
idDxtEncoder::EmitUInt
========================
*/
ID_INLINE void idDxtEncoder::EmitUInt( unsigned int i ) {
	*((unsigned int *)outData) = i;
	outData += 4;
}

/*
========================
idDxtEncoder::AlphaDistance
========================
*/
ID_INLINE unsigned int idDxtEncoder::AlphaDistance( const byte a1, const byte a2 ) const {
	return ( a1 - a2 ) * ( a1 - a2 );
}

/*
========================
idDxtEncoder::ColorDistance
========================
*/
ID_INLINE unsigned int idDxtEncoder::ColorDistance( const byte *c1, const byte *c2 ) const {
	return ( ( c1[ 0 ] - c2[ 0 ] ) * ( c1[ 0 ] - c2[ 0 ] ) ) + ( ( c1[ 1 ] - c2[ 1 ] ) * ( c1[ 1 ] - c2[ 1 ] ) ) + ( ( c1[ 2 ] - c2[ 2 ] ) * ( c1[ 2 ] - c2[ 2 ] ) );
}

/*
========================
idDxtEncoder::ColorDistanceWeighted
========================
*/
ID_INLINE unsigned int idDxtEncoder::ColorDistanceWeighted( const byte *c1, const byte *c2 ) const {
	int r, g, b;
	int rmean;

	// http://www.compuphase.com/cmetric.htm
	rmean = ( (int)c1[0] + (int)c2[0] ) / 2;
	r = (int)c1[0] - (int)c2[0];
	g = (int)c1[1] - (int)c2[1];
	b = (int)c1[2] - (int)c2[2];
	return ( ( ( 512 + rmean ) * r * r ) >> 8 ) + 4 * g * g + ( ( ( 767 - rmean ) * b * b ) >> 8 );
}

/*
========================
idDxtEncoder::CTX1Distance
========================
*/
ID_INLINE unsigned int idDxtEncoder::CTX1Distance( const byte *c1, const byte *c2 ) const {
	return ( ( c1[ 0 ] - c2[ 0 ] ) * ( c1[ 0 ] - c2[ 0 ] ) ) + ( ( c1[ 1 ] - c2[ 1 ] ) * ( c1[ 1 ] - c2[ 1 ] ) );
}

/*
========================
idDxtEncoder::ColorTo565
========================
*/
ID_INLINE unsigned short idDxtEncoder::ColorTo565( const byte *color ) const {
	return ( ( color[ 0 ] >> 3 ) << 11 ) | ( ( color[ 1 ] >> 2 ) << 5 ) | ( color[ 2 ] >> 3 );
}

/*
========================
idDxtEncoder::ColorFrom565
========================
*/
ID_INLINE void idDxtEncoder::ColorFrom565( unsigned short c565, byte *color ) const {
	color[0] = byte( ( ( c565 >> 8 ) & ( ( ( 1 << ( 8 - 3 ) ) - 1 ) << 3 ) ) | ( ( c565 >> 13 ) & ((1<<3)-1) ) );
	color[1] = byte( ( ( c565 >> 3 ) & ( ( ( 1 << ( 8 - 2 ) ) - 1 ) << 2 ) ) | ( ( c565 >>  9 ) & ((1<<2)-1) ) );
	color[2] = byte( ( ( c565 << 3 ) & ( ( ( 1 << ( 8 - 3 ) ) - 1 ) << 3 ) ) | ( ( c565 >>  2 ) & ((1<<3)-1) ) );
}

/*
========================
idDxtEncoder::ColorTo565
========================
*/
ID_INLINE unsigned short idDxtEncoder::ColorTo565( byte r, byte g, byte b ) const {
	return ( ( r >> 3 ) << 11 ) | ( ( g >> 2 ) << 5 ) | ( b >> 3 );
}

/*
========================
idDxtEncoder::GreenFrom565
========================
*/
ID_INLINE byte idDxtEncoder::GreenFrom565( unsigned short c565 ) const {
	byte c = byte( ( c565 & ( ( ( 1 << 6 ) - 1 ) << 5 ) ) >> 3 );
	return ( c | ( c >> 6 ) );
}