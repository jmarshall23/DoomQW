// MegaTexture.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "tr_local.h"

idCVar r_megaStreamFromDVD("r_megaStreamFromDVD", "0", CVAR_RENDERER | CVAR_INTEGER, "streams the megatexture from the cd drive");

/*
===================
idMegaTextureTile::Purge
===================
*/
void idMegaTextureTile::Purge()
{
	globalX = -99999;
	globalY = -99999;
	tileData = 0;
	dirty = 0;
	loaded = 0;
	Mem_Free16(compressedTileData);
	compressedTileData = 0;

	for (int i = 0; i < 4; i++)
	{
		Mem_Free16(childCompressedTileData[i]);
		childCompressedTileData[i] = nullptr;
	}
}

/*
===================
idMegaTextureTile::Upload
===================
*/
void idMegaTextureTile::Upload(idMegaTexture* megaTexture) {
	if (!dirty) return;

	//if (megaTexture->r_skipMegaTextureUpload.internalVar->integerValue) {
	//	dirty = false;
	//	return;
	//}

	unsigned char* data = (unsigned char* )(tileData ? (unsigned char*)tileData->pic : megaTexture->nullTileData);
	int size = 128;
	int level = 0;
	int offset = 0;

	if (megaTexture->imageCompressionFormat == IMAGE_COMPRESSION_NONE) {
		while (size >= 4) {
			qglTexSubImage2D(0xDE1, level, size * localX, size * localY, size, size, 0x1908, 0x1401, &data[offset]);
			offset += 4 * size * size;
			size >>= 1;
			++level;
		}
	}
	else if (megaTexture->imageCompressionFormat == IMAGE_COMPRESSION_DXT1) {
		while (size >= 4) {
			qglCompressedTexSubImage2DARB(0xDE1, level, size * localX, size * localY, size, size, 0x83F0, 8 * ((size + 3) / 4) * ((size + 3) / 4), &data[offset]);
			offset += 8 * ((size + 3) / 4) * ((size + 3) / 4);
			size >>= 1;
			++level;
		}
	}
	else if (megaTexture->imageCompressionFormat == IMAGE_COMPRESSION_DXT5) {
		while (size >= 4) {
			qglCompressedTexSubImage2DARB(0xDE1, level, size * localX, size * localY, size, size, 0x83F3, 16 * ((size + 3) / 4) * ((size + 3) / 4), &data[offset]);
			offset += 16 * ((size + 3) / 4) * ((size + 3) / 4);
			size >>= 1;
			++level;
		}
	}

	dirty = false;
}

/*
===================
idMegaTextureTile::IsLoaded
===================
*/
bool idMegaTextureTile::IsLoaded()
{
	if (level->alwaysCached)
		return 1;
	if (level->isInterleaved)
		return level->megaTexture->levels[level->levelNum + 1].tiles[(this->globalX >> 1) & 0xF][(this->globalY >> 1) & 0xF].IsLoaded();
	return loaded;
}

/*
===================
idMegaTextureTile::GetCompressedTileData
===================
*/
unsigned __int8* idMegaTextureTile::GetCompressedTileData()
{
	idMegaTextureLevel* level;
	unsigned __int8* result; 

	level = this->level;
	if (this->level->isInterleaved)
		return (&level->megaTexture->levels[level->levelNum + 1].tiles[(this->globalX >> 1) & 0xF][(this->globalY >> 1) & 0xF].childCompressedTileData[2 * (this->globalY & 1)])[this->globalX & 1];
	result = this->compressedTileData;
	if (!result)
		return (&level->compressedTiles[this->globalX % level->compressedTilesPerAxis])[level->compressedTilesPerAxis
		* (this->globalY
			% level->compressedTilesPerAxis)];
	return result;
}

/*
===================
idMegaTextureTile::PostInit
===================
*/
void idMegaTextureTile::PostInit() {
	int memoryUsed = 0;
	int previousLevelIndex = level->levelNum - 1;

	if (previousLevelIndex >= 0) {
		idMegaTextureLevel& previousLevel = level->megaTexture->levels[previousLevelIndex];

		if (previousLevel.isInterleaved) {
			size_t maxCompressedTileSize = previousLevel.maxCompressedTileSize;
			int numChildren = 4;

			for (int i = 0; i < numChildren; ++i) {
				childCompressedTileData[i] = reinterpret_cast<unsigned char*>(Mem_Alloc16(maxCompressedTileSize + 3));
				memoryUsed += maxCompressedTileSize + 3;
			}
		}
	}

	level->usedMemory += memoryUsed;
}

/*
===================
idMegaTextureTile::PostInit
===================
*/
bool idMegaTextureTile::SetCachedTileData(idMegaTexture* megaTexture, int tileBase, int tilesPerAxis) {
	if (!dirty) {
		return 1;
	}

	if (globalX >= tilesPerAxis || globalX < 0 || globalY >= tilesPerAxis || globalY < 0) {
		tileData = nullptr;
		return 0;
	}

	tileData = level->FindCachedTile(tileBase, globalX, globalY);
	if (tileData) {
		return 1;
	}

	if (level->isInterleaved) {
		idMegaTextureLevel* nextLevel = &level->megaTexture->levels[level->levelNum + 1];
		idMegaTextureTile* neighborTile = &nextLevel->tiles[(globalX >> 1) & 0xF][(globalY >> 1) & 0xF];

		if (!neighborTile->IsLoaded()) {
			neighborTile->ReleaseTileData();

			if (neighborTile->dirtyNode.head == &neighborTile->dirtyNode) {
				nextLevel->AddDirtyTile(neighborTile);
			}
		}
	}

	level->AddDirtyTile(this);
	return 0;
}

/*
===================
idMegaTextureTile::ReleaseTileData
===================
*/
void idMegaTextureTile::ReleaseTileData()
{
	tileData_t* tileData;

	tileData = tileData;
	if (tileData)
	{
		level->ReleaseTile(tileData);
		tileData = 0;
	}
	dirtyNode.prev->next = dirtyNode.next;
	dirtyNode.next->prev = dirtyNode.prev;
	dirtyNode.next = &dirtyNode;
	dirtyNode.prev = &dirtyNode;
	dirtyNode.head = &dirtyNode;
}

/*
===================
idMegaTextureLevel::Init
===================
*/
void idMegaTextureLevel::Init(idMegaTexture* megaTexture, int levelNum, int tileBase, int tilesPerAxis, bool activateImage, megaCompressionFormat_t megaCompressionFormat, int maxCompressedTileSize) {
	compressedData = nullptr;
	compressedTiles = nullptr;
	image = nullptr;
	imageValid = false;
	dirty = false;
	usedMemory = 16540;

	Mem_Free(compressedData);
	Mem_Free(compressedTiles);
	ShutdownTileCache();

	this->megaTexture = megaTexture;
	this->levelNum = levelNum;
	this->tileBase = tileBase;
	this->tilesPerAxis = tilesPerAxis;
	this->maxCompressedTileSize = maxCompressedTileSize;
	this->megaCompressionFormat = megaCompressionFormat;
	this->isInterleaved = (levelNum == 0 && megaTexture->numLevels > 4);

	idStr imageName;
	imageName = "_megaLevel_";
	imageName += va("%d", levelNum);
	image = globalImages->GetImage(imageName.c_str());

	if (activateImage) {
		if (image) {
			image->generatorFunction = nullptr;
			image = globalImages->ImageFromFunction(imageName.c_str(), this);
			image->Reload(image, false, true);
		}
		else {
			image = globalImages->ImageFromFunction(imageName.c_str(), this);
		}
	}

	if (!image) {
		common->FatalError("idMegaTextureLevel::Init : NULL level image");
	}

	parms[0] = -1.0f;
	parms[1] = 0.0f;
	parms[2] = 0.0f;
	parms[3] = static_cast<float>(tilesPerAxis) * 0.0625f;

	compressedTiles = static_cast<unsigned __int8**>(Mem_Alloc(4 * tilesPerAxis * tilesPerAxis));
	memset(compressedTiles, 0, 4 * tilesPerAxis * tilesPerAxis);
	usedMemory += 4 * tilesPerAxis * tilesPerAxis;

	if (levelNum >= 2) {
		unsigned int totalSize = 0;
		for (int y = 0; y < tilesPerAxis; ++y) {
			for (int x = 0; x < tilesPerAxis; ++x) {
				totalSize += megaTexture->tileIndexedDataSizes[tileBase + y * tilesPerAxis + x] + 3;
			}
		}

		compressedData = static_cast<unsigned __int8*>(Mem_Alloc(totalSize));
		usedMemory += totalSize;
		unsigned __int8* currentDataPtr = compressedData;

		for (int y = 0; y < tilesPerAxis; ++y) {
			for (int x = 0; x < tilesPerAxis; ++x) {
				int tileIndex = tileBase + y * tilesPerAxis + x;
				megaTexture->SeekToTile(tileIndex);
				megaTexture->file->Read(currentDataPtr, megaTexture->tileIndexedDataSizes[tileIndex] + 3);
				compressedTiles[y * tilesPerAxis + x] = currentDataPtr;
				currentDataPtr += megaTexture->tileIndexedDataSizes[tileIndex] + 3;
			}
		}

		alwaysCached = true;
		compressedTilesPerAxis = tilesPerAxis;
	}

	InitTileCache();

	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			//int* tile = &tiles[y][x].compressedTileData[0];
			tiles[y][x].level = this;
			tiles[y][x].globalX = -99999;
			tiles[y][x].globalY = -99999;
			for (int i = 0; i < 4; ++i) {
				tiles[y][x].childCompressedTileData[i] = nullptr;
			}

			tiles[y][x].localX = x;
			tiles[y][x].localY = y;

			if (!alwaysCached && !isInterleaved)
			{
				tiles[y][x].compressedTileData = (unsigned __int8* )Mem_Alloc16(this->maxCompressedTileSize + 3);
			}
		}
	}

	imageName.FreeData();
}

/*
===================
idMegaTexture::InitTileCache
===================
*/
void idMegaTextureLevel::InitTileCache() {
	int imageCompressionFormat = this->megaTexture->imageCompressionFormat;
	int tileSize = 128;
	int compressionMultiplier = 1;
	int mipLevels = 1;

	if (imageCompressionFormat == IMAGE_COMPRESSION_NONE) {
		compressionMultiplier = 1;
	}
	else if (imageCompressionFormat <= IMAGE_COMPRESSION_DXT5 && imageCompressionFormat >= IMAGE_COMPRESSION_DXT1) {
		compressionMultiplier = (imageCompressionFormat == IMAGE_COMPRESSION_DXT1) ? 2 : 4;
	}
	else {
		common->FatalError("Unsupported compression format");
	}

	while (tileSize > compressionMultiplier) {
		++mipLevels;
		tileSize >>= 1;
	}

	unsigned int totalSize = 0;
	for (int i = 0; i < mipLevels; ++i) {
		if (imageCompressionFormat > IMAGE_COMPRESSION_DXT1) {
			totalSize += 16 * ((tileSize + 3) / 4) * ((tileSize + 3) / 4);
		}
		else if (imageCompressionFormat >= IMAGE_COMPRESSION_DXT1) {
			totalSize += 8 * ((tileSize + 3) / 4) * ((tileSize + 3) / 4);
		}
		else if (imageCompressionFormat == IMAGE_COMPRESSION_NONE) {
			totalSize += 4 * tileSize * tileSize;
		}
		else {
			common->FatalError("Unsupported compression format");
		}
		tileSize >>= 1;
	}

	this->tileCacheSize = 288;
	this->usedMemory = 32 * this->tileCacheSize;

	tileData_t* tileCache = new tileData_t[this->tileCacheSize];
	this->tileCache = tileCache;

	for (int i = 0; i < this->tileCacheSize; ++i) {
		tileCache[i].pic = (char *)static_cast<unsigned __int8*>(Mem_Alloc16(totalSize));
		this->availableTiles.AddToEnd(tileCache[i].node);
		this->usedMemory += totalSize;
	}
}

/*
===================
idMegaTexture::EmptyLevelImage
===================
*/
void idMegaTextureLevel::EmptyLevelImage(idImage* image) {
	unsigned __int8* buffer = static_cast<unsigned __int8*>(Mem_Alloc(2048 * 2048 * 4));
	unsigned __int8* ptr = buffer;

	// Initialize buffer with empty tile data
	for (int i = 0; i < 0x400000; ++i) {
		*ptr++ = 0xFF;
		*ptr++ = 0x00;
		*ptr++ = 0x00;
		*ptr++ = 0xFF;
	}

	// Determine the number of mipmap levels
	imageCompressionFormat_t imageCompressionFormat = this->megaTexture->imageCompressionFormat;
	int width = 128;
	int height = 128;
	int mipLevels = 1;
	int blockSize = (imageCompressionFormat == IMAGE_COMPRESSION_NONE) ? 1 : 4;

	while (width > blockSize || height > blockSize) {
		++mipLevels;
		width >>= 1;
		height >>= 1;
	}

	// Generate the empty image with the buffer
	image->GenerateImage(buffer, 2048, 2048, TF_DEFAULT, 0, TR_REPEAT, TD_DEFAULT, imageCompressionFormat, mipLevels);

	// Free the allocated buffer
	Mem_Free(buffer);

	// Check if the generated image has the correct internal format
	if (image->internalFormat != imageCompressionFormat) {
		common->Error("idMegaTextureLevel::EmptyLevelImage: generated image has an incorrect internal format (0x%x expected 0x%x)",
			image->internalFormat, imageCompressionFormat);
	}

	// Mark the level as dirty if the image is valid
	if (this->imageValid) {
		this->dirty = 1;
		bool* tileDirty = &this->tiles[0][0].dirty;

		for (int y = 0; y < 16; ++y) {
			for (int x = 0; x < 16; ++x) {
				tileDirty[x * 64] = true;
			}
			tileDirty += 16 * 64;
		}

		megaTexture->ForceUpdate();
	}
}

/*
===================
idMegaTexture::ShutdownTileCache
===================
*/
bool idMegaTextureLevel::UpdateForCenter(const idVec2* center, bool force) {
	int globalTileCorner[2];
	int localTileOffset[2];
	idMegaTextureTile* dirtyTilesTable[256];
	float* newParms = &this->newParms[0];
	int numDirtyTiles = 0;

	if (this->tilesPerAxis > 16) {
		for (int i = 0; i < 2; ++i) {
			float offset = (center->ToFloatPtr()[i] * this->parms[3] - 0.5f) * 16.0f;
			int tileCorner = static_cast<int>(offset + 0.5f);
			globalTileCorner[i] = tileCorner;
			localTileOffset[i] = tileCorner & 0xF;
			newParms[i] = static_cast<float>(-tileCorner) * 0.0625f;
		}
	}
	else {
		globalTileCorner[0] = globalTileCorner[1] = 0;
		this->newParms[0] = this->newParms[1] = 0.25f;
		localTileOffset[0] = localTileOffset[1] = 0;
	}

	char result = 0;
	int tileY = globalTileCorner[1];
	int tileX = globalTileCorner[0];
	for (int i = 0; i < 16; ++i) {
		for (int j = 0; j < 16; ++j) {
			idMegaTextureTile& tile = this->tiles[(tileY + i) & 0xF][(tileX + j) & 0xF];
			if (tile.globalY != tileY + i || tile.globalX != tileX + j || force) {
				dirtyTilesTable[numDirtyTiles++] = &tile;
				tile.globalY = tileY + i;
				tile.globalX = tileX + j;
				tile.dirty = true;
				tile.cached = false;
				tile.loaded = false;

				// Add the tile to the dirty tiles table
				dirtyTilesTable[numDirtyTiles++] = &tile;

				// Release any existing cached tile data
				if (tile.tileData) {					
					ReleaseTile(tile.tileData);
					tile.tileData = nullptr;
				}
				this->dirty = true;
			}
		}
	}

	if (this->levelNum >= 0) {
		for (int i = 0; i < numDirtyTiles; ++i) {
			if (!dirtyTilesTable[i]->SetCachedTileData(this->megaTexture, this->tileBase, this->tilesPerAxis)) {
				result = 1;
			}
		}
	}

	return result;
}

/*
===================
idMegaTexture::ShutdownTileCache
===================
*/
void idMegaTextureLevel::ShutdownTileCache() {
	// Clear the available and active tiles lists
	this->availableTiles.Clear();
	this->activeTiles.Clear();

	// Free the memory for each tile in the cache
	for (int i = 0; i < this->tileCacheSize; ++i) {
		Mem_Free16(this->tileCache[i].pic);
	}

	// Delete the tile cache if it exists
	if (this->tileCache) {
		// Call the destructor for each tileData_t object in the cache
		for (int i = 0; i < this->tileCacheSize; ++i) {
			this->tileCache[i].~tileData_t();
		}
		// Free the allocated memory for the tile cache
		operator delete[](this->tileCache);
		this->tileCache = nullptr;
	}

	// Reset the tile cache size to 0
	this->tileCacheSize = 0;
}

/*
===================
idMegaTexture::OpenFile
===================
*/
bool idMegaTexture::OpenFile() {
	idStr fileName = "megatextures/" + name;
	fileName.StripFileExtension();

	fileName += ".mega";

	if (r_megaStreamFromDVD.GetInteger()) {
		fileName = fileSystem->RelativePathToOSPath(fileName.c_str(), "fs_cdpath");
	}
	else {
		file = fileSystem->OpenFileRead(fileName.c_str(), true, false);
		if (!file) {
			return false;
		}
		fileName = file->GetName();
		fileSystem->CloseFile(file);
		file = nullptr;
	}

	winFile = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (winFile == INVALID_HANDLE_VALUE) {
		DWORD lastError = GetLastError();
		LPVOID errorMsg;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errorMsg, 0, NULL);
		common->FatalError("idMegaTexture::OpenFile : failed to open '%s' (%d : %s)", fileName, lastError, (char*)errorMsg);
		LocalFree(errorMsg);
		return false;
	}

	if (!winFileScratch) {
		winFileScratch = Mem_Alloc16(0x100000);
		if (file) {
			delete file;
		}
		file = new idFile_Memory("winFileScratch", (const char *)winFileScratch, 0x100000);
	}

	OVERLAPPED overlapped = { 0 };
	DWORD bytesRead;
	if (!ReadFile(winFile, winFileScratch, 0x100000, &bytesRead, &overlapped)) {
		common->FatalError("idMegaTexture::OpenFile : failed to read '%s'", fileName);
		return false;
	}

	winFileBlockOffset = 0;
	winFileNumBlocks = 32;

	int fileId;
	file->ReadInt(fileId);
	if (fileId != 1095189837) {
		common->FatalError("idMegaTexture::OpenFile : unknown fileid on '%s'", fileName);
		return false;
	}

	file->ReadInt(version);
	if (version != 9 && version != 8) {
		common->FatalError("idMegaTexture::OpenFile : wrong version on '%s' (%d should be 9)", fileName, version);
		return false;
	}

	//cmdSystem->AddCommand("megaTestStreamingPerformance", idMegaTexture::MegaTestStreamingPerformance_f, CMD_FL_SYSTEM, "Tests streaming performance without seeking.", nullptr);
	//cmdSystem->AddCommand("megaShowMemoryUsage", idMegaTexture::MegaShowMemoryUsage_f, CMD_FL_SYSTEM, "Show memory usage of active mega texture.", nullptr);

	return true;
}

/*
====================
idMegaTexture::InitFromFile
====================
*/
bool idMegaTexture::InitFromFile(const char* fileBase)
{
	name = fileBase;

	if (!idMegaTexture::OpenFile())
	{
		idMegaTexture::CloseFile();
		return false;
	}

	idMegaTexture::CloseFile();
	return true;
}

/*
====================
idMegaTexture::InitFromFile
====================
*/
bool idMegaTexture::UploadTiles(int time) {
	for (int i = 0; i < numLevels; ++i) {
		levels[i].UploadTiles(time);
	}

	return true;
}

/*
====================
idMegaTexture::GetPureServerChecksum
====================
*/
int idMegaTexture::GetPureServerChecksum(unsigned int offset) {
	idStr fileName;

	fileName = "megatextures/";
	fileName += name;
	fileName += ".mega";

	if (r_megaStreamFromDVD.GetInteger()) {
		const char* osPath = fileSystem->RelativePathToOSPath(fileName.c_str(), "fs_cdpath");
		fileName = osPath;
	}
	else {
		idFile* file = fileSystem->OpenFileRead(fileName.c_str(), true, 0);
		if (!file) {
			fileName.FreeData();
			return 0;
		}
		fileName = file->GetFullPath();
		fileSystem->CloseFile(file);
	}

	idFile* file = fileSystem->OpenExplicitFileRead(fileName.c_str());
	if (!file) {
		fileName.FreeData();
		return 0;
	}

	int checksum = 0;
	for (int i = 0; i < 16; ++i) {
		int tempChecksum = 0;
		for (int j = 0; j < 32; j += 8) {
			int fileLength = file->Length();
			int seekOffset = (fileLength > 1) ? ((69069 * offset + 1) & 0x7FFF) % (fileLength - 1) : 0;
			file->Seek(seekOffset, FS_SEEK_SET);
			tempChecksum |= static_cast<unsigned int>(file->ReadUnsignedChar()) << j;
		}
		checksum ^= tempChecksum;
	}

	fileSystem->CloseFile(file);
	fileName.FreeData();
	return checksum;
}

/*
====================
idMegaTexture::UpdateForViewOrigin
====================
*/
void idMegaTexture::UpdateForViewOrigin(const idVec3* viewOrigin, int time) {
	//if (lastUsedFrame < backEnd.frameCount) {
	//	UploadTiles(time);
	//	SetViewOrigin(viewOrigin);
	//	lastUsedFrame = backEnd.frameCount;
	//}
	//
	//for (int i = 0; i < numLevels; ++i) {
	//	UpdateLevelForViewOrigin(&levels[numLevels - i - 1], i, time);
	//}
	//
	//if (r_megaUpscale.internalVar->integerValue) {
	//	if (upscaleLevel) {
	//		UpdateLevelForViewOrigin(upscaleLevel, numLevels, time);
	//	}
	//}
	//else {
	//	if (upscaleLevel) {
	//		float timea = upscaleLevel->parms[3];
	//		auto* maskParams = rbinds->megaMaskParams[numLevels];
	//		maskParams->data.vector[0] = -1.0f;
	//		maskParams->data.vector[1] = 0.0f;
	//		maskParams->data.vector[2] = 0.0f;
	//		maskParams->data.vector[3] = timea;
	//
	//		int numLevelsShifted = 1 << (numLevels + 1);
	//		auto* textureParams = rbinds->megaTextureParams[numLevels];
	//		float halfShifted = static_cast<float>(numLevelsShifted) * 0.5f;
	//		textureParams->data.vector[0] = halfShifted;
	//		textureParams->data.vector[1] = halfShifted;
	//		textureParams->data.vector[2] = halfShifted;
	//		textureParams->data.vector[3] = halfShifted;
	//	}
	//
	//	unsigned int levelIndex = numLevels - 1;
	//	if (levelIndex <= 3) {
	//		rbinds->megaTextureOpacity15->data.vector[levelIndex] = 0.0f;
	//	}
	//	rbinds->megaTextureLevel[numLevels]->data.attrib = reinterpret_cast<int>(idImage::blackImage);
	//}
	//
	//idImage* grayImage = detailTexture->defaulted || !detailTexture->IsLoaded(detailTexture)
	//	? idImage::grayImage : detailTexture;
	//idImage* detailMaskImage = detailTextureMask->defaulted || !detailTextureMask->IsLoaded(detailTextureMask)
	//	? idImage::defaultDetailMaskImage : detailTextureMask;
	//
	//rbinds->megaDetailTexture->data.attrib = reinterpret_cast<int>(grayImage);
	//rbinds->megaDetailTextureMask->data.attrib = reinterpret_cast<int>(detailMaskImage);
	//
	//auto* detailParams = rbinds->megaDetailTextureParams;
	//float ratio = (static_cast<float>(tilesPerAxis) * 128.0f) / grayImage->uploadWidth;
	//ratio *= r_detailRatio.internalVar->floatValue;
	//detailParams->data.vector[0] = ratio;
	//detailParams->data.vector[1] = 1.0f;
	//detailParams->data.vector[2] = 1.0f;
	//detailParams->data.vector[3] = 1.0f;
}