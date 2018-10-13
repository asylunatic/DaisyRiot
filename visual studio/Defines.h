#pragma once
// this is a very ugly class to define some globally used stuff in our program
struct MatrixIndex {
	int col;
	int row;
};

struct UV {
	float u;
	float v;
};

#define RAYS_PER_PATCH 100;