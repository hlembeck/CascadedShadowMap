#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> // For CommandLineToArgvW

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgidebug.h>
#include <directxcollision.h>

#pragma comment( lib, "user32" )          // link against the win32 library
#pragma comment( lib, "d3d12.lib" )       // direct3D library
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler
#pragma comment(lib, "dxguid.lib")

// D3D12 extension library.
#include "d3dx12.h"

// STL Headers
#include <algorithm>
#include <cassert>
#include <chrono>

//Miscellaneous
#include <stdio.h>
#include <exception>
#include <chrono>
#include <fstream>
#include <stdlib.h>
#include <cstdlib>
#include <random>
#include <cmath>

constexpr UINT MAXINSTANCES = 1024;
constexpr float FOVY = DirectX::XM_PIDIV4;
constexpr UINT NMATERIALS = 1; //Corresponds to number of pipeline states used for materials (each "material" is a different pipeline state, as different lighting methods are required).
constexpr UINT NFRUSTA = 4;
constexpr DirectX::XMVECTOR LIGHTDIR = { 1.0f, -0.5f, 1.0f, 0.0f };
//Bias to frustum of shadow maps, to avoid having view frustum being too tightly fitted by the shadow map volume.
constexpr float SHADOWBIAS = 0.01f;
//63*2^6 < 2^12 = 4096. We generate terrain on the most detailed scale of 1m grid spacing, and so the terrain is rendered on 4032x4032 "meter" chunks. Then we can have 6 mip levels, where 0 is the raw generated terrain, and 5 is the coarsest terrain (spanned by 63x63 grid). Thus the 5th mipmap should be stored whenever the chunk is visible, and this fits in a single tile of the virtual texture.
constexpr UINT NTERRAINLEVELS = 1;
constexpr float TERRAINHEIGHTMAX = 1000.0f;

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        printf("%x\n", hr);
        throw std::exception();
    }
}

constexpr WCHAR wndClassName[] = L"Window Class";
constexpr WCHAR geomFileName[] = L"Geometries.geom";

inline UINT GetStringByteSize(const char* string) {
    UINT ret = 0;
    while (*(string++))
        ret++;
    return ret;
}

using namespace DirectX;

struct CameraShaderConstants {
    XMMATRIX cameraMatrix;
    XMFLOAT4 viewDirection;
    XMFLOAT4 viewPosition;
};

struct PointLight {
    XMFLOAT4 position;
    XMFLOAT4 color;
};

constexpr UINT MAXVERTEXINPUTS = 3;

struct SimpleVertex {
    XMFLOAT4 position;
    XMFLOAT4 normal;
};

struct TexturedVertex { //Simple vertex, plus an extra float2 that acts as texture UV coordinates
    XMFLOAT4 position;
    XMFLOAT4 normal;
    XMFLOAT2 texCoords;
};

class RandomGenerator {
    std::mt19937 m_gen;
    std::uniform_real_distribution<float> m_dist;
public:
    float Get() { return m_dist(m_gen); };
    RandomGenerator() : m_gen(std::random_device()()), m_dist(-1.0f, 1.0f) {};
};

struct VertexView {
    D3D12_VERTEX_BUFFER_VIEW views[2];
    UINT nVertices;
};

enum PIPELINESTATE {
    HEIGHTMAP_TERRAIN,
    LIGHT_PERSPECTIVE_DEPTHMAP,
    LAYERED_DEPTHMAP_DIRECTIONAL,
    LAYERED_DEPTHMAP_CUBICAL,
    CASCADED_SHADOW_MAP,
    SIMPLE
};

struct AspectRatio {
    float m_aspectRatio;
};

struct CameraView : public virtual AspectRatio {
    XMMATRIX m_viewMatrix = XMMatrixIdentity();
    float m_nearZ = 0.0f;
    float m_farZ = 0.0f;
    float m_fovY = 0.0f;
    XMVECTOR m_direction = {};
};

//Used to calculate screen space error for a tile in the terrain renderer.
struct TerrainLODViewParams {
    XMFLOAT4 position;
    float tanFOVH; //Actually is tan(fovH/2), as used in Ch. 19 of Real-Time Rendering.
    float screenWidth;
    BoundingFrustum frustum;
    XMMATRIX viewMatrix;
};

struct TileParams {
    XMMATRIX worldMatrix;
    XMUINT2 texCoords;
};

struct Time {
    std::chrono::time_point<std::chrono::high_resolution_clock> m_time;
};

//Count of BASICGEOMETRY to equal NOBJECTS
enum BASICGEOMETRY {
    CUBE
};

class RenderableObject {
protected:
    ComPtr<ID3D12Resource> m_vertices; //Vertex positions and normals
    ComPtr<ID3D12Resource> m_texCoords; //Texture coordinates for vertices
public:
    BOOL m_textured;
    VertexView m_vertexView = {};
    virtual ~RenderableObject() {};
    virtual void Load(ID3D12Device* device) = 0;
};

struct ObjectInstances {
    RenderableObject* pObject;
    ComPtr<ID3D12Resource> texture;
    ComPtr<ID3D12Resource> instanceData;
    D3D12_GPU_DESCRIPTOR_HANDLE m_cbvHandle;
    UINT nInstances;
};

struct Material {
    const PIPELINESTATE pipelineState; //All objects share the same pipeline state
    std::vector<ObjectInstances> objects;
};

inline UINT Pad256(UINT size) { return ((size+255) & (~255)); };

struct FrustumRays {
    XMVECTOR o;
    XMVECTOR v1;
    XMVECTOR v2;
    XMVECTOR v3;
    XMVECTOR v4;
    float d;
};