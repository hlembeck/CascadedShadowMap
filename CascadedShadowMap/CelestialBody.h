#pragma once
#include "DualContour.h"

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

//Contains information describing which part of the planet is visible.
struct VisiblePortion {
	XMFLOAT4 back; //portion of back face visible.
	XMFLOAT4 front;
	XMFLOAT4 left;
	XMFLOAT4 right;
	XMFLOAT4 bottom;
	XMFLOAT4 top;
};


class CelestialBody {
protected:
	TreeNode* m_root;

	ComPtr<ID3D12Resource> m_randTexture;
	D3D12_GPU_DESCRIPTOR_HANDLE m_randTextureSRV;

	//Tile mappings
	ComPtr<ID3D12Resource> m_tileMap;
	D3D12_GPU_DESCRIPTOR_HANDLE m_tileMapSRV;

	//Vertex buffer for instancing when rendering to screen. Any class of celestial body (e.g planet) that instantiates this class is to use only a single vertex buffer i.e only a single geometry base for the object. For a planet, where gravity has removed much of the overhanging terrain, the vertex buffer is triangles partitioning a square. For some other objects, dual contouring may be used, in whcih case the veretx buffer is a set of vertices with point "topology".
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	//Bounding spheres
	BoundingSphere m_innerSphere;
	BoundingSphere m_outerSphere;

	virtual void CreateRandTexture(CommandListAllocatorPair* pCmdListAllocPair) = 0;
	virtual void CreateTileMap(ID3D12Device* device) = 0;
	void GetTileMapTiling(ID3D12Device* device);

	virtual void CreateVertexBuffer(CommandListAllocatorPair* pCmdListAllocPair) = 0;
public:
	virtual ~CelestialBody() {};
	TreeNode* GetRoot();
	BOOL Intersects(BoundingBox aabb);
	virtual void FillRenderCmdList(XMVECTOR camPos, ID3D12GraphicsCommandList* commandList) = 0;
};