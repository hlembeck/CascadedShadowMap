# CascadedShadowMap Readme

## Virtual Tiled Terrain
### Introduction:
The tiled terrain implemented in this repository uses a virtual texture to store the heightmap data that is to be rendered on screen. The heightfield format is a `float4`, where the first coordinate is the height at the point, and the other three coordinates are the normal at the point. With this format, tiles in the virtual texture are of size $64\times 64$, so for simplicity we choose a resolution of $63\times 63$ for the heightmap data.

### Definitions:
- A **Chunk** is a 

Suppose the camera is centered at the point $(x,0,z)$, where 0 is the "sea level" of our terrain. To make sure that all terrain in the frustum is visible, and to allow fast rotation without updating the virtual texture, we require that a sufficiently large grid of tiles is resident on the GPU. This grid is aligned to 62x62 worldspace tiles, and is centered about the camera (as best as possible).

A few notes regarding the desire to have a camera-centered grid.
- As the view frustum grows, the necessary number of tiles increases quadratically. Thus, if we want to maintain the assertion that we always have the entire grid renderable (that is, at least the lowest level tile is resident, for each chunk), we are forced to increase the tree depth as frustum size increases. Ignoring floating point precision issues, increasing tree depth will work for all frustum sizes (since the frustum length is itself bound by floats).
- It may also be worthwhile to forego the cented grid condition, and instead rely on the GPU to quickly render lowest-level terrain as the camera rotates, as is done when the camera moves. 

These initial tiles form the highest spatial extent terrain tiles in the terrain. We define the following data structures:

```
struct TileParams {
    XMMATRIX worldMatrix;
    XMUINT2 texCoords;
};

struct TileInformation {
    BoundingBox bounds;
    TileParams tileParams;
    INT isResident;
};
```
 - `struct TileParams` Tile parameters to be copied into a constant buffer for use in shaders.
    - `XMMATRIX worldMatrix` World matrix used to map [0,62]x[0,62] in worldspace to the correct bounds for the tile. In the vertex shader of the screen render stage, it is applied to the world position of a vertex after it has been scaled to the correct tile size (each level in the tree rooted at a max-spanning tile is half the span of the previous level).
    - `XMUINT2 texCoords` Texture coordinates, in texels, of the start of the tile. Since the loading of the texture is done manually at this time, UV coordinates are not used; we index directly into the resource.
 - `BoundingBox bounds` Spatial bounds of the terrain tile. Used to test for intersection with view frustum, using `BoundingBox::Intersects(BoundingFrustum)`.
 - `INT isResident` Integer value in $\{0,1\}$ that determines whether the tile is resident on the GPU. A value of $0$ is not resident, while a value of $1$ indicates that the tile is resident. INT is used instead of BOOL to ensure that the TRUE value is 1.
 
 `TileInformation` instances are members of the following class:
 
 ```
 class Tile {
    TileInformation m_tileInfo;
    Tile m_children[4];
 public:
    Tile();
    Tile(TileInformation);
  
    BOOL IsVisible(BoundingFrustum, XMMATRIX);
    BOOL NeedsUpdate(BoundingFrustum, XMMATRIX);
    XMMATRIX Create(float x, float y, UINT level, XMUINT2 texCoords);
    void Create(float x, float y);
 };
 ```
 
 

