// MegaTextureTileLoader.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "tr_local.h"

idMegaTextureTileLoader megaTextureTileLoader;

int tileLoadSeek[8192] = {};
int tileLoadData[8192] = {};
int tileLoadTimes[8192] = {};
int lastTileLoadTime;

int GetMegaTilesPerSecond() {
	int v0 = Sys_Milliseconds() - 1000;
	int v1 = 0;
	for (int i = 0; i < 32768; i += 4) {
		if (tileLoadTimes[i] >= v0) ++v1;
		if (tileLoadTimes[i + 1] >= v0) ++v1;
		if (tileLoadTimes[i + 2] >= v0) ++v1;
		if (tileLoadTimes[i + 3] >= v0) ++v1;
	}
	return v1;
}

int GetCompressedUsefulKiloBytesReadPerSecond() {
	int v0 = Sys_Milliseconds() - 1000;
	int v1 = 0;
	for (unsigned int v2 = 0; v2 < 0x8000; v2 += 16) {
		if (tileLoadTimes[v2 / 4] >= v0) v1 += tileLoadData[v2 / 4];
	}
	return v1 / 1024;
}

int GetCompressedSeekMBPerSecond() {
	int v0 = Sys_Milliseconds() - 1000;
	int v1 = 0;
	for (unsigned int v2 = 0; v2 < 0x8000; v2 += 16) {
		if (tileLoadTimes[v2 / 4] >= v0) v1 += tileLoadSeek[v2 / 4] / 1024;
	}
	return v1 / 1024;
}

int GetCompressedSeeksPerSecond() {
	int v0 = Sys_Milliseconds() - 1000;
	int result = 0;
	for (unsigned int v2 = 0; v2 < 0x8000; v2 += 16) {
		if (tileLoadTimes[v2 / 4] >= v0) result += (tileLoadSeek[v2 / 4] > 0);
	}
	return result;
}

double GetTilesPerSeek() {
	int seekCount = 0;
	int tileCount = 0;

	for (unsigned int i = 0; i < 0x8000; i += 16) {
		if (tileLoadTimes[i / 4] > 0) {
			seekCount += (tileLoadSeek[i / 4] > 0) ? 1 : 0;
			++tileCount;
		}
	}

	if (seekCount == 0) {
		return 0.0;
	}

	return static_cast<double>(tileCount) / seekCount;
}

idMegaTextureTileLoader::~idMegaTextureTileLoader() {
	thread = nullptr;
	// Clean up signals
}

void idMegaTextureTileLoader::StartThread() {
	thread = new sdThread(this);
	if (!thread->Start(0, 0)) {
		common->FatalError("idMegaTextureTileLoader::StartThread : failed to start thread\n");
	}
}

void idMegaTextureTileLoader::SetActiveMegaTexture(idMegaTexture* megaTexture) {
	idMegaTexture* currentActiveMegaTexture = this->activeMegaTexture;
	if (currentActiveMegaTexture != megaTexture) {
		if (currentActiveMegaTexture) {
			currentActiveMegaTexture->lock.Acquire(1);
		}

		this->activeMegaTexture = megaTexture;

		if (currentActiveMegaTexture) {
			currentActiveMegaTexture->lock.Release();
		}
		else {
			this->signal.Set();
		}
	}
}

void idMegaTextureTileLoader::ForceUpdate() {
	if (activeMegaTexture) {
		activeMegaTexture->ForceUpdate();
	}
}

void idMegaTextureTileLoader::Init() {
	StartThread();
}

// icecoldduke: this function had some fucked up disassembly. 
unsigned int idMegaTextureTileLoader::Run(void* parm) {
	while (!terminate) {
		if (!activeMegaTexture) {
			signal.Wait(-1);
			continue;
		}

		activeMegaTexture->lock.Acquire(1);
		idMegaTexture* texture = activeMegaTexture;
		int currentLevel = texture->numLevels - 1;

		while (currentLevel >= 0) {
			auto& level = texture->levels[currentLevel];
			if (!level.isInterleaved) {
				auto* tile = level.dirtyTiles.next ? level.dirtyTiles.next->owner : nullptr;

				while (tile && tile->IsLoaded()) {
					auto* next = tile->dirtyNode.next;
					if (!next || next == tile->dirtyNode.head) {
						tile = nullptr;
						break;
					}
					tile = next->owner;
				}

				if (tile) {
					int globalY = tile->globalY;
					int globalX = tile->globalX;
					int tileIndex = globalX + level.tileBase + globalY * level.tilesPerAxis;

					auto* compressedTileData = tile->GetCompressedTileData();
					std::vector<int> tileData = { globalX, globalY, (int)&level, tileIndex };
					int numSubTiles = 1;
					int subLevelIndex = level.levelNum - 1;

					if (subLevelIndex >= 0) {
						auto& subLevel = texture->levels[subLevelIndex];
						if (subLevel.isInterleaved) {
							std::vector<char> childTileData(40 * 4);
							int childY = 2 * globalY;
							int childX = 2 * globalX;
							for (int i = 0; i < 4; ++i) {
								childTileData[i * 10 + 8] = subLevel.tileBase + childX + (childY + i / 2) * subLevel.tilesPerAxis;
								childTileData[i * 10] = childX;
								childTileData[i * 10 + 4] = childY + i / 2;
								childTileData[i * 10 + 12] = (int)tile->GetChildCompressedTileData(i);
								childX += (i % 2 == 1) ? -1 : 1;
							}
							numSubTiles = 5;
						}
					}

					for (int i = 0; i < numSubTiles; ++i) {
						auto* dataPointer = (unsigned char*)(tileData[i * 5 + 4] + texture->tileIndexedDataSizes[tileData[i * 5 + 3]] + 3);
						*dataPointer = tileData[i * 5] - 1;
						dataPointer[1] = tileData[i * 5 + 1] - 1;
					}

					texture->lock.Release();

					for (int i = 0; i < numSubTiles; ++i) {
						int dataSize = texture->tileIndexedDataSizes[tileData[i * 5 + 3]] + 3;
						int seekPos = texture->SeekToTile(tileData[i * 5 + 3]);
						texture->file->Read((void*)compressedTileData, dataSize);

						int loadTimeIndex = lastTileLoadTime & 0x1FFF;
						tileLoadTimes[loadTimeIndex] = Sys_Milliseconds();
						tileLoadData[loadTimeIndex] = dataSize;
						tileLoadSeek[loadTimeIndex] = seekPos;
						++lastTileLoadTime;
					}

					texture->lock.Acquire(1);
					if (globalX == tile->globalX && globalY == tile->globalY) {
						tile->loaded = true;
						megaTextureTileDecompressor.signal.Set();
					}
					texture->lock.Release();

					++numProcessedTiles;
					continue;
				}
			}

			--currentLevel;
		}

		texture->lock.Release();
		signal.Wait(-1);
	}

	return 0;
}