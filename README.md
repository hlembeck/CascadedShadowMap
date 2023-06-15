# Cascaded Shadow Map (CSM)
The implementation of CSM is standard, but is not linked with Celestial Body implementation yet (no shadows for celestial bodies). This portion will be updated when the two are linked.

# Celestial Body 

### Planet
A planet is defined in part by its roughly spherical shape, although in reality planets form an ellipsoid. For simplicity, the spherical base is used for all planets.

The surface of any planet can be bound between two spheres, namely the **_inner sphere_** and the **_outer sphere_**. The inner sphere has radius equal to the minimum altitude of the planet's surface, and the outer sphere has radius equal to the maximum altitude of the planet's surface.
The minimum and maximum values can be easily shown to exist by noting that the image of a compact space under a continuous map is compact (continuous map is $S^2\to\mathbb{R}$ taking point on sphere to its altitude; here we use that our terrain is a heightmap on the sphere).
One might also take the outer sphere to be defined by the planet's atmosphere, and in fact the implementation may use this radius in the future.

The planet can be roughly rendered with no LOD by passing a cube to the graphics pipeline, normalizing its points, and then applying noise and transforming to world coordinates.
Applying LOD using this scheme may be difficult if the input cube is not oriented to the camera position, so a transform is done to align the cube so that its close face is orthogonal to the vector from the camera position to the planet center.
Given this orientation, LOD can be applied by determining a rectangular region of each face to be rendered, and applying a sort of persistent grid to this region. The interested reader may see the code in **Planet.cpp** and **PlanetTiler.hlsl**.
