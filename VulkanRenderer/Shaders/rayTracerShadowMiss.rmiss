#version 460

#extension GL_EXT_ray_tracing : require

layout (location = 1) rayPayloadInEXT float shadowVisibility;

void main() 
{
	shadowVisibility = 1.0;//when shadow ray misses geometry, that mean surface isnt obstructed by anything thus no shadow is present
}