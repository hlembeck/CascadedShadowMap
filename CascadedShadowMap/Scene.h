#pragma once
#include "DXBase.h"

class Scene : public virtual DXBase {
	std::vector<RenderableObject*> m_objects;
	Material m_materials[NMATERIALS] = {};
public:
	Scene();
	~Scene();
	void Load(ID3D12GraphicsCommandList* commandList);

	void Draw(ID3D12GraphicsCommandList* commandList);
	void DrawBarebones(ID3D12GraphicsCommandList* commandList);
};