/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

class idMegaTextureLevel;
class idRenderWorldLocal;

#define FLT_EPSILON 1.19209290E-07F

#include <vector>
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

// tileData_t Class Definition
class tileData_t {
public:
	// Constructor
	tileData_t();

	// Destructor
	~tileData_t();

	// Data Members
	int x;
	int y;
	int tileBase;
	char* pic;
	idLinkList<tileData_t> node;
};


enum imageCompressionFormat_t : __int32
{
	IMAGE_COMPRESSION_NONE = 0x8058,
	IMAGE_COMPRESSION_DXT1 = 0x83F0,
	IMAGE_COMPRESSION_DXT5 = 0x83F3,
};

enum megaCompressionFormat_t : __int32
{
	MEGA_COMPRESSION_NONE = 0x0,
	MEGA_COMPRESSION_RGB = 0x1,
	MEGA_COMPRESSION_RGBA = 0x2,
	MEGA_COMPRESSION_LUM = 0x3,
};

// idMegaTextureTile Class Definition
class idMegaTextureTile {
public:
	// Member Functions
	void Purge();
	void Upload(idMegaTexture* megaTexture);
	bool IsLoaded();
	unsigned __int8* GetCompressedTileData();
	unsigned __int8* GetChildCompressedTileData(int index)
	{
		return childCompressedTileData[index];
	}

	void PostInit();
	bool idMegaTextureTile::SetCachedTileData(idMegaTexture* megaTexture, int tileBase, int tilesPerAxis);
	void ReleaseTileData();

	// Data Members
	idMegaTextureLevel* level;
	idLinkList<idMegaTextureTile> dirtyNode;
	int localX;
	int localY;
	int globalX;
	int globalY;
	unsigned __int8* compressedTileData;
	unsigned __int8* childCompressedTileData[4];
	tileData_t* tileData;
	bool dirty;
	bool loaded;
	bool cached;
};

class idMegaTextureTileDecompressor : public sdThreadProcess {
public:
	virtual ~idMegaTextureTileDecompressor();
	void DecompressLuminance(char* destination);
	void DecompressTile(megaCompressionFormat_t format, char* destination);
	void RecompressTile(imageCompressionFormat_t format, char* source, char* tileData);	
	void GetCompressedTileData(idMegaTexture* mega, idMegaTextureLevel* level, idMegaTextureTile* tile);
	void Stop();
	void StartThread();
	void SetActiveMegaTexture(idMegaTexture* megaTexture);
	void Init();
	void Shutdown();
	unsigned int Run(void* parm);

	static idCVar r_megaTilesPerSecond;
	static idCVar r_megaShowGrid;
	static idCVar r_megaShowTileSize;

	struct compressedTileData_t
	{
		int globalX;
		int globalY;
		unsigned __int8* data;
		int size;
		int parentLevelNum;
		int parentGlobalX;
		int parentGlobalY;
		unsigned __int8* parentData;
		int parentSize;
		int parentCachedLevelNum;
		int parentCachedGlobalX;
		int parentCachedGlobalY;
		unsigned __int8* parentCachedData;
	};

	idBareDctDecoder* dctDecoder;
	idDxtEncoder* dxtEncoder;
	sdThread* thread;
	idMegaTexture* activeMegaTexture;
	cpuid_t cpuid;
	int terminate;
	sdSignal signal;
	sdSignal throttleSignal;
	compressedTileData_t compressedData;
	int numProcessedTiles;
	int numTilesThisMsec;
	int lastProcessedTime;
};

// idMegaTextureLevel Class Definition
class idMegaTextureLevel {
public:
	// Member Functions
	void Init(idMegaTexture* megaTexture, int levelNum, int tileBase, int tilesPerAxis, bool activateImage, megaCompressionFormat_t megaCompressionFormat, int maxCompressedTileSize);
	void InitTileCache();
	void ShutdownTileCache();
	bool UpdateForCenter(const idVec2* center, bool force);
	void EmptyLevelImage(idImage* image);
	tileData_t* FindCachedTile(const int tileBase, const int globalX, const int globalY);
	tileData_t* GetAvailableTile();
	void ReleaseTile(tileData_t* tileData);
	void AddDirtyTile(idMegaTextureTile* tile);
	void RemoveDirtyTile(idMegaTextureTile* tile);
	void UpdateLevelForViewOrigin(int idx, int time);
	void UploadTiles(int time);

	unsigned __int8* GetCompressedTileData(int globalX, int globalY)
	{
		return (&this->compressedTiles[globalX % this->compressedTilesPerAxis])[this->compressedTilesPerAxis
			* (globalY % this->compressedTilesPerAxis)];
	}

	// Data Members
	idImageGeneratorFunctor<idMegaTextureLevel> emptyLevelImageFunctor;
	idMegaTexture* megaTexture;
	int levelNum;
	int usedMemory;
	idImage* image;
	bool imageValid;
	int tileBase;
	int tilesPerAxis;
	megaCompressionFormat_t megaCompressionFormat;
	bool isInterleaved;
	int fadeTime;
	idMegaTextureTile tiles[16][16];
	bool alwaysCached;
	char* compressedData;
	unsigned char** compressedTiles;
	int compressedTilesPerAxis;
	tileData_t* tileCache;
	int tileCacheSize;
	int maxCompressedTileSize;
	float parms[4];
	float newParms[2];
	idLinkList<tileData_t> availableTiles;
	idLinkList<tileData_t> activeTiles;
	bool dirty;
	idLinkList<idMegaTextureTile> dirtyTiles;
};


class idMegaTexture {
public:
	void GenerateNullTileData();
	void GenerateGridTileData();
	void AllocRecompressionScratch();
	void LoadDetailTexture();
	void Reset();
	void UpdateMapping(idRenderWorldLocal* world);
	void UpdateLevelForViewOrigin(idMegaTextureLevel* level, int idx, int time);
	char __thiscall CloseFile();
	unsigned int SeekToTile(int tileIndex);
	int GetPureServerChecksum(unsigned int offset);
	bool OpenFile();
	void Touch();
	bool InitFromFile(const char* fileBase);
	bool UploadTiles(int time);
	void ForceUpdate();
	void UpdateForViewOrigin(const idVec3* viewOrigin, int time);
	static int __cdecl TotalStoredTileCount(const int resolution);

	idStr name;
	int version;
	int resolution;
	bool levelLoadReferenced;
	bool referencedOutsideLevelLoad;
	bool purged;
	idFile* file;
	int lastTileOffset;
	void* winFile;
	char* winFileScratch;
	int winFileBlockOffset;
	int winFileNumBlocks;
	imageCompressionFormat_t imageCompressionFormat;
	bool useImageCompression;
	bool forcedUpdate;
	idImage* detailTexture;
	idImage* detailTextureMask;
	int lastUsedFrame;
	const idRenderWorldLocal* currentWorld;
	idVec3 currentViewOrigin;
	int tilesPerAxis;
	int numLevels;
	idMegaTextureLevel* levels;
	idMegaTextureLevel* upscaleLevel;
	sdBounds2D stGridBounds;
	int stGridWidth;
	int stGridHeight;
	idVec2* stGrid;
	int* tileIndexMap;
	int* tileIndexedDataSizes;
	unsigned __int8* nullTileData;
	unsigned __int8* gridTileData;
	unsigned __int8* tileRecompressionScratch;
	int lastShaderQuality;
	sdLock lock;
};

class idMegaTextureTileLoader : sdThreadProcess {
public:
	idMegaTextureTileLoader();
	~idMegaTextureTileLoader();

	sdThread* thread;
	sdSignal signal;
	sdSignal throttleSignal;
	idMegaTexture* activeMegaTexture;
	int numProcessedTiles;

	struct TileInfo {
		int y;
		idMegaTextureLevel* level;
		int tileNum;
		uint8_t* compressedData;
		int x;
	};

	TileInfo tiles[5];

	void StartThread();
	void SetActiveMegaTexture(idMegaTexture* megaTexture);
	void ForceUpdate();
	void Init();
	unsigned int Run(void* parm);
};



extern int tileLoadSeek[8192];
extern int tileLoadData[8192];
extern int tileLoadTimes[8192];
extern int lastTileLoadTime;
extern idMegaTextureTileLoader megaTextureTileLoader;
extern idMegaTextureTileDecompressor megaTextureTileDecompressor;