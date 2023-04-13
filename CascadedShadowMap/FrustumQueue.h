#pragma once
#include "Shared.h"

//For simplicity of calculations, which increases speed, a chunk is defined by the circumscribed sphere around the future tile's bounding box (chunks of this type are not stored in the frustum terrain, but will be ad
//struct Chunk {
//	float x;
//	float y;
//	float dist; //Distance to frustum
//
//	void UpdateDistance(float radius, XMFLOAT3 o, XMFLOAT3 p1,);
//};
//
//class ChunkQueue {
//	float m_chunkWidth;
//};