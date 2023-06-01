#pragma once
#include "DualContour.h"

//Incomplete list
enum CelestialBodyClass {
	Planet,
	Star,
	Moon, //Natural satellite (https://en.wikipedia.org/wiki/Natural_satellite)
};

struct NodeTile {
	BoundingBox aabb;
	float weight;
	XMUINT4 tileCoords;
	XMMATRIX GetWorldMatrix(); //Maps [-1,1]x[-1,1]x[-1,1] -> aabb. (0,0,0) maps to aabb.Center.
};

class CelestialBody;

struct TreeNode {
	CelestialBody* body;
	TreeNode* parent = nullptr;
	NodeTile tiles[8];
	UINT n = 0;

	float parentWeight = 0.0f; //nonpositive when no parent exists (node is root node).
	TreeNode(BoundingBox bounds, CelestialBody* pBody, TreeNode* par = nullptr, float parWeight = 0, XMFLOAT3 camPos = {});
	TreeNode(BoundingSphere bounds, CelestialBody* pBody, XMFLOAT3 camPos = {});

	void DecreaseCount(); //Stop tracking tile
	void IncreaseCount(BoundingBox bounds); //Add back to tracked tiles
};

class CelestialBody {
	TreeNode* m_root;

	CelestialBodyClass m_class;
	ComPtr<ID3D12Resource> m_randTexture;
	D3D12_GPU_DESCRIPTOR_HANDLE m_randTextureSRV;

	//Tile mappings
	ComPtr<ID3D12Resource> m_tileMap;
	D3D12_GPU_DESCRIPTOR_HANDLE m_tileMapSRV;

	BoundingSphere m_outerSphere;
	BoundingSphere m_innerSphere;
	//XMFLOAT3 m_axisLengths = {1.0f,1.0f,1.0f}; //For ellipsoidal base for the celestial body, since not all planets(for example) are spheres.

	float m_maxNoise = 0.0f; //Maximum extent of noise


	void CreateRandTexture(ID3D12Device* device);
	void CreateTileMap(ID3D12Device* device);
public:
	CelestialBody(CelestialBodyClass cbClass, float radius, XMFLOAT3 initialPos, ID3D12Device* device, XMFLOAT4 camPos = {});
	~CelestialBody();
	TreeNode* GetRoot();
	BOOL Intersects(BoundingBox aabb);
};