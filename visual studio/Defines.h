#pragma once

struct UV {
	float u;
	float v;
};

struct MatrixIndex {
	int col;
	int row;
	UV uv;
};


#define RAYS_PER_PATCH 50