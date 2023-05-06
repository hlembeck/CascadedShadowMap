# Dual Contour Implementation ([Source](https://www.cs.rice.edu/~jwarren/papers/dualcontour.pdf))

## Algorithm Sketch

For each edge in a voxel grid that has a sign change with respect to the generating function, join vertices lying in adgacent voxels. Uses a virtual texture to store the vertex in each voxel, since computation of vertex locations may be avoided in this way. Since determining a sign change is as simple as sampling the generating function, there is no need to store the function values at voxel corners.

To render terrain, we grids of $k\times k\times k$ vertices, where each vertex represents the close-bottom-left of a voxel, and such that $k\times k\times k$ does not exceed the dimensions of a tile in the virtual texture above. For each vertex in the draw call, consider the edges eminating in a positive direction from it. For each of these edges that has a sign change from the generating function, generate a quad in the geometry shader, by sampling the virtual texture to obtain vertex locations. The main issue with this implementation is that (in most cases) most voxels do not intersect the surface, so many vertices are discarded.

An octree is used to help alleviate the above problem. Each node represents a block of world space that has its vertices set in the virtual texture. A texture with one texel per smallest node to be rendered is u
