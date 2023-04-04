# CascadedShadowMap Readme

## Virtual Tiled Terrain
Suppose the camera is centered at the point $(x,0,z)$, where 0 is the "sea level" of our terrain. To make sure that all terrain in the frustum is visible, and to allow fast rotation without updating the virtual texture, we require that a sufficiently large grid of tiles is resident on the GPU. This grid is aligned to 62x62 worldspace tiles, and is centered about the camera (as best as possible).

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
    - `XMMATRIX worldMatrix` World matrix used to map [0,62]x[0,62] in worldspace to the correct bounds for the tile. For a tile of highest spatial extent, this matrix is just a translation. Otherwise, it is a translation composed with a scaling factor of $2^k$, for some integer $k$.
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
 
 

