// MegaTexture.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "tr_local.h"

idCVar r_megaStreamFromDVD("r_megaStreamFromDVD", "0", CVAR_RENDERER | CVAR_INTEGER, "streams the megatexture from the cd drive");

int dataLoadSizes[2048];
int dataLoadTimes[2048];
int lastDataLoadTime = 0;

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

	emptyLevelImageFunctor.Init(this, &idMegaTextureLevel::EmptyLevelImage);

	idStr imageName;
	imageName = "_megaLevel_";
	imageName += va("%d", levelNum);
	image = globalImages->GetImage(imageName.c_str());

	if (activateImage) {
		if (image) {
			image->generatorFunction = nullptr;
			image = globalImages->ImageFromFunction(imageName.c_str(), &emptyLevelImageFunctor);
			image->Reload(false, true);
		}
		else {
			image = globalImages->ImageFromFunction(imageName.c_str(), &emptyLevelImageFunctor);
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

		compressedData = static_cast<char *>(Mem_Alloc(totalSize));
		usedMemory += totalSize;
		unsigned __int8* currentDataPtr = (unsigned __int8* )compressedData;

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
	image->GenerateImageEx(buffer, 2048, 2048, TF_DEFAULT, 0, TR_REPEAT, TD_DEFAULT, imageCompressionFormat, mipLevels);

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
idMegaTextureLevel::ShutdownTileCache
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
idMegaTextureLevel::FindCachedTile
===================
*/
tileData_t* idMegaTextureLevel::FindCachedTile(int tileBase, int globalX, int globalY)
{
	idLinkList<tileData_t>* next;
	tileData_t* result; 
	idLinkList<tileData_t>* v6;
	idLinkList<tileData_t>* head; 

	next = this->availableTiles.next;
	if (!next || next == this->availableTiles.head)
		return 0;
	result = next->owner;
	if (result)
	{
		while (result->tileBase != tileBase || result->x != globalX || result->y != globalY)
		{
			v6 = result->node.next;
			if (!v6 || v6 == result->node.head)
				return 0;
			result = v6->owner;
			if (!result)
				return result;
		}
		head = activeTiles.head;
		result->node.prev->next = result->node.next;
		result->node.next->prev = result->node.prev;
		result->node.prev = &result->node;
		result->node.head = &result->node;
		result->node.next = head;
		result->node.prev = head->prev;
		head->prev = &result->node;
		result->node.prev->next = &result->node;
		result->node.head = head->head;
	}
	return result;
}

/*
===================
idMegaTextureLevel::GetAvailableTile
===================
*/
tileData_t* idMegaTextureLevel::GetAvailableTile() {
	idLinkList<tileData_t> * next = availableTiles.next;
	tileData_t* owner;

	if (!next || next == availableTiles.head) {
		owner = nullptr;
		common->FatalError("idMegaTexture::GetFreeTile : no available cache tiles");
		return nullptr;
	}

	owner = next->owner;
	if (!owner) {
		common->FatalError("idMegaTexture::GetFreeTile : no available cache tiles");
		return nullptr;
	}

	owner->tileBase = -1;
	owner->x = -1;
	owner->y = -1;

	idLinkList<tileData_t>* head = activeTiles.head;
	owner->node.prev->next = owner->node.next;
	owner->node.next->prev = owner->node.prev;

	owner->node.prev = &owner->node;
	owner->node.head = &owner->node;
	owner->node.next = head;
	owner->node.prev = head->prev;
	head->prev = &owner->node;
	owner->node.prev->next = &owner->node;
	owner->node.head = head->head;

	return owner;
}

/*
===================
idMegaTexture::ReleaseTile
===================
*/
void idMegaTextureLevel::ReleaseTile(tileData_t* tileData)
{
	idLinkList<tileData_t>* v2; 
	idLinkList<tileData_t>* head; 

	if (tileData->x == -1 || tileData->y == -1 || tileData->tileBase == -1)
	{
		head = this->availableTiles.head;
		tileData->node.prev->next = tileData->node.next;
		tileData->node.next->prev = tileData->node.prev;
		tileData->node.next = &tileData->node;
		tileData->node.head = &tileData->node;
		tileData->node.prev = head;
		tileData->node.next = head->next;
		head->next = &tileData->node;
		tileData->node.next->prev = &tileData->node;
		tileData->node.head = head->head;
	}
	else
	{
		v2 = this->availableTiles.head;
		tileData->node.prev->next = tileData->node.next;
		tileData->node.next->prev = tileData->node.prev;
		tileData->node.prev = &tileData->node;
		tileData->node.head = &tileData->node;
		tileData->node.next = v2;
		tileData->node.prev = v2->prev;
		v2->prev = &tileData->node;
		tileData->node.prev->next = &tileData->node;
		tileData->node.head = v2->head;
	}
}

/*
===================
idMegaTexture::OpenFile
===================
*/
void idMegaTextureLevel::AddDirtyTile(idMegaTextureTile* tile)
{
	int* tileIndexMap;
	idLinkList<idMegaTextureTile>* head;
	idLinkList<idMegaTextureTile>* next;
	idMegaTextureTile* owner;
	idLinkList<idMegaTextureTile>* v6;

	tileIndexMap = this->megaTexture->tileIndexMap;
	head = this->dirtyTiles.head;
	next = head->next;
	if (next && next != head->head && (owner = next->owner) != 0)
	{
		while (tileIndexMap[tile->globalY + tile->level->tileBase + tile->globalX * tile->level->tilesPerAxis] >= tileIndexMap[owner->globalY + owner->level->tileBase + owner->globalX * owner->level->tilesPerAxis])
		{
			v6 = owner->dirtyNode.next;
			if (v6)
			{
				if (v6 != owner->dirtyNode.head)
				{
					owner = v6->owner;
					if (owner)
						continue;
				}
			}
			goto LABEL_8;
		}
		tile->dirtyNode.prev->next = tile->dirtyNode.next;
		tile->dirtyNode.next->prev = tile->dirtyNode.prev;
		tile->dirtyNode.prev = &tile->dirtyNode;
		tile->dirtyNode.head = &tile->dirtyNode;
		tile->dirtyNode.next = &owner->dirtyNode;
		tile->dirtyNode.prev = owner->dirtyNode.prev;
		owner->dirtyNode.prev = &tile->dirtyNode;
		tile->dirtyNode.prev->next = &tile->dirtyNode;
		tile->dirtyNode.head = owner->dirtyNode.head;
	}
	else
	{
	LABEL_8:
		tile->dirtyNode.prev->next = tile->dirtyNode.next;
		tile->dirtyNode.next->prev = tile->dirtyNode.prev;
		tile->dirtyNode.prev = &tile->dirtyNode;
		tile->dirtyNode.head = &tile->dirtyNode;
		tile->dirtyNode.next = head;
		tile->dirtyNode.prev = head->prev;
		head->prev = &tile->dirtyNode;
		tile->dirtyNode.prev->next = &tile->dirtyNode;
		tile->dirtyNode.head = head->head;
	}
	megaTextureTileLoader.signal.Set();
}

/*
===================
idMegaTexture::RemoveDirtyTile
===================
*/
void idMegaTextureLevel::RemoveDirtyTile(idMegaTextureTile* tile)
{
	tile->dirtyNode.prev->next = tile->dirtyNode.next;
	tile->dirtyNode.next->prev = tile->dirtyNode.prev;
	tile->dirtyNode.next = &tile->dirtyNode;
	tile->dirtyNode.prev = &tile->dirtyNode;
	tile->dirtyNode.head = &tile->dirtyNode;
}

/*
===================
idMegaTexture::UploadTiles
===================
*/
void idMegaTextureLevel::UploadTiles(int time) {
	if (dirty) {
		idLinkList<idMegaTextureTile> * next = dirtyTiles.next;
		if (next && next != dirtyTiles.head && next->owner) {
			return;
		}

		int cleanCount = 256;
		bool* p_dirty = &tiles[0][1].dirty;

		for (int i = 0; i < 16; ++i) {
			for (int j = 0; j < 4; ++j) {
				if (*(p_dirty - 64)) --cleanCount;
				if (*p_dirty) --cleanCount;
				if (p_dirty[64]) --cleanCount;
				if (p_dirty[128]) --cleanCount;
				p_dirty += 256;
			}
		}

		if (cleanCount < 128) {
			fadeTime = time;
		}

		//image->BindFragment(image); // jmarshall <-- fix
		parms[0] = newParms[0];
		parms[1] = newParms[1];

		for (int i = 0; i < 16; ++i) {
			for (int j = 0; j < 16; ++j) {
				tiles[i][j].Upload(megaTexture);
			}
		}

		imageValid = true;
		dirty = false;
	}
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
		winFileScratch = (char *)Mem_Alloc16(0x100000);
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
void idMegaTexture::ForceUpdate()
{
	int lastShaderQuality; // edi

	lastShaderQuality = this->lastShaderQuality;
	this->lastShaderQuality = 4; // r_shaderQuality.internalVar->integerValue;
	this->forcedUpdate = 1;
	while (!UploadTiles(0))
	{
		megaTextureTileLoader.signal.Set();
		megaTextureTileDecompressor.signal.Set();
	}
	this->lastShaderQuality = lastShaderQuality;
	this->forcedUpdate = 0;
}

/*
====================
idMegaTexture::Touch
====================
*/
void idMegaTexture::Touch()
{
	if (!purged)
		idMegaTexture::LoadDetailTexture();
}

/*
====================
idMegaTexture::Reset
====================
*/
void idMegaTexture::Reset()
{
	int v1;
	int v2; 
	idMegaTextureLevel* upscaleLevel; 

	if (!this->purged)
	{
		v1 = 0;
		if (this->numLevels > 0)
		{
			v2 = 0;
			do
			{
				this->levels[v2].fadeTime = 0;
				++v1;
				++v2;
			} while (v1 < this->numLevels);
		}
		upscaleLevel = this->upscaleLevel;
		if (upscaleLevel)
			upscaleLevel->fadeTime = 0;
	}
}

/*
====================
idMegaTexture::SeekToTile
====================
*/
unsigned int idMegaTexture::SeekToTile(int tileIndex) {
	__int64 tileInfo = tileIndexMap[tileIndex];
	int startBlock = ((tileInfo >> 32) & 0x7FFF) + (int)tileInfo;
	int dataSize = tileIndexedDataSizes[tileIndex];
	__int64 endBlock = (dataSize + ((tileInfo >> 32) & 0x7FFF) + 32770LL);
	int endBlockOffset = ((endBlock >> 32) & 0x7FFF) + (int)endBlock;

	int numStartBlocks = startBlock >> 15;
	int numEndBlocks = endBlockOffset >> 15;

	if (numStartBlocks < winFileBlockOffset || numStartBlocks + numEndBlocks > winFileBlockOffset + winFileNumBlocks) {
		// icecoldduke do we need this. 
		//if (numEndBlocks < idMegaTexture::r_megaStreamBlocks->integerValue) {
		//	numEndBlocks = idMegaTexture::r_megaStreamBlocks->integerValue;
		//}

		OVERLAPPED overlapped = { 0 };
		overlapped.Offset = numStartBlocks << 15;
		DWORD bytesRead = 0;

		ReadFile(winFile, winFileScratch, numEndBlocks << 15, &bytesRead, &overlapped);

		int blockDiff = numStartBlocks - winFileNumBlocks;
		winFileNumBlocks = numEndBlocks;
		int blockOffsetDiff = blockDiff - winFileBlockOffset;
		winFileBlockOffset = numStartBlocks;

		unsigned int loadDistance = abs(blockOffsetDiff << 15);
		int loadIndex = lastDataLoadTime & 0x7FF;
		int currentTime = Sys_Milliseconds();
		++lastDataLoadTime;
		dataLoadSizes[loadIndex] = numEndBlocks << 15;
		dataLoadTimes[loadIndex] = currentTime;

		file->Seek(startBlock & 0x7FFF, FS_SEEK_SET);

		return loadDistance;
	}
	else {
		file->Seek((startBlock & 0x7FFF) + ((numStartBlocks - winFileBlockOffset) << 15), FS_SEEK_SET);
		return 0;
	}
}

/*
====================
idMegaTexture::AllocRecompressionScratch
====================
*/
void idMegaTexture::AllocRecompressionScratch() {
	if (useImageCompression) {
		int sizeX = 128;
		int sizeY = 128;
		int initialSizeX = 128;
		int initialSizeY = 128;
		int numLevels = 1;

		while (initialSizeX > 1 || initialSizeY > 1) {
			++numLevels;
			initialSizeX >>= 1;
			initialSizeY >>= 1;
		}

		unsigned int totalSize = 0;
		for (int i = 0; i < numLevels; ++i) {
			totalSize += 4 * (sizeX * sizeY);
			sizeX >>= 1;
			sizeY >>= 1;
		}

		tileRecompressionScratch = static_cast<unsigned char*>(Mem_Alloc16(totalSize));
	}
	else {
		tileRecompressionScratch = nullptr;
	}
}

/*
====================
idMegaTexture::GenerateGridTileData
====================
*/
void idMegaTexture::GenerateGridTileData() {
	int width = 128;
	int height = 128;
	int levelWidth = 128;
	int levelHeight = 128;
	int minSize = 1;
	int compressionLevels = 1;

	if (imageCompressionFormat == IMAGE_COMPRESSION_NONE) {
		minSize = 1;
	}
	else if (imageCompressionFormat <= 33775 || imageCompressionFormat > IMAGE_COMPRESSION_DXT5) {
		minSize = 4;
	}
	else {
		minSize = 4;
	}

	while (levelWidth > minSize || levelHeight > minSize) {
		++compressionLevels;
		levelWidth >>= 1;
		levelHeight >>= 1;
	}

	unsigned int totalSize = 0;
	int remainingLevels = compressionLevels;
	while (remainingLevels > 0) {
		if (imageCompressionFormat > (IMAGE_COMPRESSION_DXT1 | 0x1)) {
			if (imageCompressionFormat > IMAGE_COMPRESSION_DXT5) {
				totalSize = -1;
				break;
			}
			totalSize += 16 * ((width + 3) / 4) * ((height + 3) / 4);
		}
		else if (imageCompressionFormat >= IMAGE_COMPRESSION_DXT1) {
			totalSize += 8 * ((width + 3) / 4) * ((height + 3) / 4);
		}
		else if (imageCompressionFormat == IMAGE_COMPRESSION_NONE) {
			totalSize += 4 * width * height;
		}
		else {
			totalSize = -1;
			break;
		}
		width >>= 1;
		height >>= 1;
		--remainingLevels;
	}

	unsigned char* allocatedMemory = static_cast<unsigned char*>(Mem_Alloc16(totalSize));
	gridTileData = allocatedMemory;

	unsigned char* recompressionScratch = useImageCompression ? tileRecompressionScratch : gridTileData;

	int rowIndex = 0;
	unsigned char* currentPixel = recompressionScratch + 2;
	do {
		for (int colIndex = 0; colIndex < 128; ++colIndex) {
			char value = -((colIndex ^ rowIndex) & 0x10);
			currentPixel[1] = value;
			currentPixel[0] = value;
			currentPixel[-1] = value;
			currentPixel[-2] = value;
			currentPixel += 4;
		}
		++rowIndex;
	} while (rowIndex < 128);

	megaTextureTileDecompressor.RecompressTile(imageCompressionFormat, (char *)recompressionScratch, (char*)gridTileData);
}

/*
====================
idMegaTexture::CloseFile
====================
*/
bool idMegaTexture::CloseFile()
{
	if (winFile != INVALID_HANDLE_VALUE) {
		CloseHandle(winFile);
		winFile = INVALID_HANDLE_VALUE;
	}

	if (winFileScratch) {
		Mem_Free16(winFileScratch);
		winFileScratch = nullptr;
	}

	if (file) {
		delete file;
		file = nullptr;
	}

	return 1;
}

/*
====================
idMegaTexture::GenerateNullTileData
====================
*/
void idMegaTexture::GenerateNullTileData() {
	int width = 128;
	int height = 128;
	int levelWidth = 128;
	int levelHeight = 128;
	int minSize = 1;
	int compressionLevels = 1;

	if (imageCompressionFormat == IMAGE_COMPRESSION_NONE) {
		minSize = 1;
	}
	else if (imageCompressionFormat <= 33775 || imageCompressionFormat > IMAGE_COMPRESSION_DXT5) {
		minSize = 4;
	}
	else {
		minSize = 4;
	}

	while (levelWidth > minSize || levelHeight > minSize) {
		++compressionLevels;
		levelWidth >>= 1;
		levelHeight >>= 1;
	}

	unsigned int totalSize = 0;
	int remainingLevels = compressionLevels;
	while (remainingLevels > 0) {
		if (imageCompressionFormat > (IMAGE_COMPRESSION_DXT1 | 0x1)) {
			if (imageCompressionFormat > IMAGE_COMPRESSION_DXT5) {
				totalSize = -1;
				break;
			}
			totalSize += 16 * ((width + 3) / 4) * ((height + 3) / 4);
		}
		else if (imageCompressionFormat >= IMAGE_COMPRESSION_DXT1) {
			totalSize += 8 * ((width + 3) / 4) * ((height + 3) / 4);
		}
		else if (imageCompressionFormat == IMAGE_COMPRESSION_NONE) {
			totalSize += 4 * width * height;
		}
		else {
			totalSize = -1;
			break;
		}
		width >>= 1;
		height >>= 1;
		--remainingLevels;
	}

	unsigned char* allocatedMemory = static_cast<unsigned char*>(Mem_Alloc16(totalSize));
	nullTileData = allocatedMemory;

	unsigned char* recompressionScratch = useImageCompression ? tileRecompressionScratch : nullTileData;
	std::memset(recompressionScratch, 0, 0x10000);

	megaTextureTileDecompressor.RecompressTile(imageCompressionFormat, (char *)recompressionScratch, (char*)nullTileData);
}

/*
====================
idMegaTexture::LoadDetailTexture
====================
*/
void idMegaTexture::LoadDetailTexture()
{
	// icecoldduke todo
}

/*
====================
idMegaTexture::UpdateForViewOrigin
====================
*/
void idMegaTexture::UpdateMapping(idRenderWorldLocal* world)
{
	int megaTextureSTGridWidth; // eax
	int megaTextureSTGridHeight; // ecx
	idVec2* megaTextureSTGrid; // edx
	double y; // st7

	if (world)
	{
		if (megaTextureTileLoader.activeMegaTexture != this)
			megaTextureTileLoader.SetActiveMegaTexture(this);
		if (megaTextureTileDecompressor.activeMegaTexture != this)
			megaTextureTileDecompressor.SetActiveMegaTexture(this);
// icecoldduke	
		//megaTextureSTGridWidth = world->megaTextureSTGridWidth;
		//megaTextureSTGridHeight = world->megaTextureSTGridHeight;
		//megaTextureSTGrid = world->megaTextureSTGrid;
		//if (world != this->currentWorld
		//	|| megaTextureSTGrid != this->stGrid
		//	|| megaTextureSTGridHeight != this->stGridHeight
		//	|| megaTextureSTGridWidth != this->stGridWidth)
		//{
		//	this->currentWorld = world;
		//	this->stGridBounds.bounds[0].x = world->megaTextureBounds.b[0].x;
		//	this->stGridBounds.bounds[0].y = world->megaTextureBounds.b[0].y;
		//	this->stGridBounds.bounds[1].x = world->megaTextureBounds.b[1].x;
		//	y = world->megaTextureBounds.b[1].y;
		//	this->stGridWidth = megaTextureSTGridWidth;
		//	this->stGridBounds.bounds[1].y = y;
		//	this->stGridHeight = megaTextureSTGridHeight;
		//	this->stGrid = megaTextureSTGrid;
		//}
	}
}

/*
====================
idMegaTexture::UpdateForViewOrigin
====================
*/
void idMegaTexture::UpdateLevelForViewOrigin(idMegaTextureLevel* level, int idx, int time)
{
	//const sdDeclRenderBinding* v5; // ecx
	//const sdDeclRenderBinding* v6; // ecx
	//int fadeTime; // ecx
	//float opacitya; // [esp+8h] [ebp+4h]
	//float opacity; // [esp+8h] [ebp+4h]
	//
	//if (level->imageValid)
	//{
	//	v5 = rbinds->megaMaskParams[idx];
	//	v5->data.vector[0] = level->parms[0];
	//	v5->data.vector[1] = level->parms[1];
	//	v5->data.vector[2] = level->parms[2];
	//	v5->data.vector[3] = level->parms[3];
	//	v6 = rbinds->megaTextureParams[idx];
	//	opacitya = (double)(1 << (idx + 1)) * 0.5;
	//	v6->data.vector[3] = opacitya;
	//	v6->data.vector[2] = opacitya;
	//	v6->data.vector[1] = opacitya;
	//	v6->data.vector[0] = opacitya;
	//}
	//opacity = 1.0;
	//fadeTime = level->fadeTime;
	//if (fadeTime > time - idMegaTexture::r_megaFadeTime.internalVar->integerValue)
	//	opacity = (double)(time - fadeTime) / (double)idMegaTexture::r_megaFadeTime.internalVar->integerValue;
	//if ((unsigned int)(idx - 1) <= 3)
	//	*((float*)&rbinds->megaTextureOpacity15->infrequent + idx) = opacity;
	//if (idMegaTexture::r_showMegaTextureLevels.internalVar->integerValue)
	//{
	//	if ((idx & 1) != 0)
	//		rbinds->megaTextureLevel[idx]->data.attrib = (int)globalImages->blackImage;
	//	else
	//		rbinds->megaTextureLevel[idx]->data.attrib = (int)globalImages->whiteImage;
	//}
	//else
	//{
	//	rbinds->megaTextureLevel[idx]->data.attrib = (int)level->image;
	//}
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