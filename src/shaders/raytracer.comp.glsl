#version 460
#extension GL_EXT_scalar_block_layout : require


layout(local_size_x = 16, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, set = 0, scalar) buffer storageBuffer
{
	vec3 imageData[];
};

void main()
{
	const uvec2 resolution = uvec2(800, 600);
	const uvec2 pixel = gl_GlobalInvocationID.xy;

	if (pixel.x >= resolution.x || pixel.y >= resolution.y)
	{
		return;
	}

	const vec3 color = vec3(float(pixel.x) / resolution.x,
							float(pixel.y) / resolution.y,
							0.0);
	
	//should be row by row first, then the column
	uint index = pixel.y * resolution.x + pixel.x;

	imageData[index].x = color.r;
	imageData[index].y = color.g;
	imageData[index].z = color.b;
}