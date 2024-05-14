// MegaTextureTileDecompressor.cpp
//

#include "precompiled.h"
#include "tr_local.h"

idMegaTextureTileDecompressor megaTileDecompressor;

// Destructor
idMegaTextureTileDecompressor::~idMegaTextureTileDecompressor() {

}

// Function to decompress luminance
void idMegaTextureTileDecompressor::DecompressLuminance(char* destination) {
	if (compressedData.parentCachedLevelNum != compressedData.parentLevelNum
		|| compressedData.parentCachedGlobalX != compressedData.parentGlobalX
		|| compressedData.parentCachedGlobalY != compressedData.parentGlobalY) {

		unsigned char* v3 = compressedData.parentData;
		dctDecoder->SetQuality_Generic((unsigned __int8)*v3, (unsigned __int8)v3[1], (unsigned __int8)v3[2]);
		dctDecoder->DecompressImageYCoCg_Generic(v3 + 3, compressedData.parentCachedData, 128, 128, compressedData.parentSize);
		compressedData.parentCachedLevelNum = compressedData.parentLevelNum;
		compressedData.parentCachedGlobalX = compressedData.parentGlobalX;
	}
	idFilter::UpScale2xBicubic_Generic(&compressedData.parentCachedData[256 * ((compressedData.globalX & 1) + ((compressedData.globalY & 1) << 7))], 64, 64, 128, (unsigned char *)destination, idFilter::BICUBIC_SHIFTED);
	dctDecoder->SetQuality_Generic((unsigned __int8)*compressedData.data, (unsigned __int8)compressedData.data[1], (unsigned __int8)compressedData.data[2]);
	dctDecoder->DecompressLuminanceEnhancement_Generic(compressedData.data + 3, (unsigned char *)destination, 128, 128, compressedData.size);
	idColorSpace::ConvertYCoCgToRGB((byte *)destination, (byte*)destination, 128, 128);
}

// Function to decompress a tile
void idMegaTextureTileDecompressor::DecompressTile(megaCompressionFormat_t format, char* destination) {
	switch (format) {
	case 1:
		dctDecoder->SetQuality_Generic((unsigned __int8)*compressedData.data, (unsigned __int8)compressedData.data[1], (unsigned __int8)compressedData.data[2]);
		dctDecoder->DecompressImageRGB_Generic(compressedData.data + 3, (unsigned char *)destination, 128, 128, compressedData.size);
		break;
	case 2:
		dctDecoder->SetQuality_Generic( (unsigned __int8)*compressedData.data, (unsigned __int8)compressedData.data[1], (unsigned __int8)compressedData.data[2]);
		dctDecoder->DecompressImageRGBA_Generic(compressedData.data + 3, (unsigned char *)destination, 128, 128, compressedData.size);
		break;
	case 3:
		DecompressLuminance(destination);
		break;
	}
}

// Function to recompress a tile
void idMegaTextureTileDecompressor::RecompressTile(imageCompressionFormat_t format, char* source, char* tileData) {
	idMipMap::CreateMips((unsigned char *)source, 7);
	if (format != 32856) {
		int v7 = 128;
		do {
			if (format == 33776)
				dxtEncoder->CompressImageDXT1Fast_Generic((const byte *)source, (byte *)tileData, v7, v7);
			else
				dxtEncoder->CompressImageDXT5Fast_Generic((const byte*)source, (byte*)tileData, v7, v7);
			tileData += (unsigned int)source;
			source += 4 * v7 * v7;
			v7 >>= 1;
		} while (v7 >= 4);
	}
}

// Function to get compressed tile data
void idMegaTextureTileDecompressor::GetCompressedTileData(idMegaTexture* mega, idMegaTextureLevel* level, idMegaTextureTile* tile) {
	compressedData.globalX = tile->globalX;
	compressedData.globalY = tile->globalY;
	int v6 = compressedData.globalX + level->tileBase + compressedData.globalY * level->tilesPerAxis;
	compressedData.data = (unsigned char *)tile->GetCompressedTileData();
	compressedData.size = mega->tileIndexedDataSizes[v6];
	if (level->megaCompressionFormat == 3) {
		int v7 = level->levelNum + 1;
		compressedData.parentLevelNum = v7;
		compressedData.parentGlobalX = tile->globalX >> 1;
		compressedData.parentGlobalY = tile->globalY >> 1;
		int v11 = compressedData.parentGlobalX + mega->levels[v7].tileBase + compressedData.parentGlobalY * mega->levels[v7].tilesPerAxis;
		compressedData.parentData = mega->levels[v7].GetCompressedTileData(compressedData.parentGlobalX, compressedData.parentGlobalY);
		compressedData.parentSize = mega->tileIndexedDataSizes[v11];
	}
}

// Function to stop the decompressor
void idMegaTextureTileDecompressor::Stop() {
	terminate = 1;
	signal.Set();
	throttleSignal.Set();
}

// Function to start a thread
void idMegaTextureTileDecompressor::StartThread() {
	thread = new sdThread(this);
	thread->SetName("MegaTextureTileDecompressor");
	if (!thread->Start( 0, 0))
		common->Warning("idMegaTextureTileDecompressor::StartThread : failed to start thread");
}

// Function to set active mega texture
void idMegaTextureTileDecompressor::SetActiveMegaTexture(idMegaTexture* megaTexture) {
	if (activeMegaTexture != megaTexture) {
		if (activeMegaTexture) activeMegaTexture->lock.Acquire(1);
		activeMegaTexture = megaTexture;
		if (activeMegaTexture) activeMegaTexture->lock.Release();
		else signal.Set();
	}
}

// Initialization function
void idMegaTextureTileDecompressor::Init() {
	cpuid = sys->GetProcessorId();
	dctDecoder = new idBareDctDecoder();
	dxtEncoder = new idDxtEncoder();
	memset(&compressedData, 0, sizeof(compressedData));
	compressedData.parentCachedLevelNum = -1;
	compressedData.parentCachedGlobalX = -1;
	compressedData.parentCachedGlobalY = -1;
	compressedData.parentCachedData = (unsigned char*)Mem_Alloc16(0x10000);
	StartThread();
}

// Shutdown function
void idMegaTextureTileDecompressor::Shutdown() {
	if (thread) {
		thread->Stop();
		thread->Join();
		thread->Destroy();
		thread = nullptr;
	}
	if (dctDecoder) {
		delete dctDecoder;
		dctDecoder = nullptr;
	}
	if (dxtEncoder) {
		delete dxtEncoder;
		dxtEncoder = nullptr;
	}
	if (compressedData.parentCachedData) {
		Mem_Free16(compressedData.parentCachedData);
		compressedData.parentCachedData = nullptr;
	}
}

// Run function
unsigned int idMegaTextureTileDecompressor::Run(void* parm) {
	idMegaTexture* mega;
	idMegaTextureTile* tile = nullptr;
	idMegaTextureTileDecompressor* v44 = this;
	if (terminate) return 0;

	do {
		if (!activeMegaTexture) {
			signal.Wait(-1);
			continue;
		}

		activeMegaTexture->lock.Acquire(1);
		mega = activeMegaTexture;
		int v7 = mega->numLevels - 1;

		if (v7 < 0) {
			mega->lock.Release();
			signal.Wait(-1);
			continue;
		}

		for (int v8 = v7; v8 >= 0; --v8) {
			idMegaTextureLevel* v9 = &mega->levels[v8];
			if (!v9->dirtyTiles.next || v9->dirtyTiles.next == v9->dirtyTiles.head) continue;

			tile = v9->dirtyTiles.next->owner;
			if (!tile) continue;

			if (tile->IsLoaded()) break;
			tile = nullptr;
		}

		if (!tile) {
			mega->lock.Release();
			signal.Wait(-1);
			continue;
		}

		// Process tile decompression/recompression based on CPUID and other conditions
		mega->lock.Release();
	} while (!terminate);

	return 0;
}