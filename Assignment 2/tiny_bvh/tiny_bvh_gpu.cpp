// This example shows how to build a basic GPU path tracer using
// tinybvh. TinyOCL is used to render on the GPU using OpenCL.

#define FENSTER_APP_IMPLEMENTATION
#define SCRWIDTH 1280
#define SCRHEIGHT 800
#include "external/fenster.h" // https://github.com/zserge/fenster

// This application uses tinybvh - And this file will include the implementation.
#define TINYBVH_IMPLEMENTATION
#include "tiny_bvh.h"
using namespace tinybvh;

// This application uses tinyocl - And this file will include the implementation.
#define TINY_OCL_IMPLEMENTATION
#include "tiny_ocl.h"

// Other includes
#include <fstream>

// Application variables

static BVH4_GPU bvh;
static bvhvec4* tris = 0;
static int triCount = 0, frameIdx = 0, spp = 0;
static Kernel* init, * generate, * extend, * shade, * traceShadows, * finalize;
static Buffer* pixels, * accumulator, * raysIn, * raysOut, * connections;

// View pyramid for a pinhole camera
struct RenderData
{
	bvhvec4 eye = bvhvec4( 0, 30, 0, 0 );
	bvhvec4 view = bvhvec4( -1, 0, 0, 0 );
	bvhvec4 C, p0, p1, p2;
	uint32_t frameIdx, dummy1, dummy2, dummy3;
} rd;

// Scene management - Append a file, with optional position, scale and color override, tinyfied
void AddMesh( const char* file, float scale = 1, bvhvec3 pos = {}, int c = 0, int N = 0 )
{
	std::fstream s{ file, s.binary | s.in }; s.read( (char*)&N, 4 );
	bvhvec4* data = (bvhvec4*)tinybvh::malloc64( (N + triCount) * 48 );
	if (tris) memcpy( data, tris, triCount * 48 ), tinybvh::free64( tris );
	tris = data, s.read( (char*)tris + triCount * 48, N * 48 ), triCount += N;
	for (int* b = (int*)tris + (triCount - N) * 12, i = 0; i < N * 3; i++)
		*(bvhvec3*)b = *(bvhvec3*)b * scale + pos, b[3] = c ? c : b[3], b += 4;
}

// Application init
void Init()
{
	// create OpenCL kernels
	init = new Kernel( "wavefront.cl", "SetRenderData" );
	generate = new Kernel( "wavefront.cl", "Generate" );
	extend = new Kernel( "wavefront.cl", "Extend" );
	shade = new Kernel( "wavefront.cl", "Shade" );
	traceShadows = new Kernel( "wavefront.cl", "Connect" );
	finalize = new Kernel( "wavefront.cl", "Finalize" );
	// create OpenCL buffers
	int N = SCRWIDTH * SCRHEIGHT;
	pixels = new Buffer( N * sizeof( uint32_t ) );
	accumulator = new Buffer( N * sizeof( bvhvec4 ) );
	raysIn = new Buffer( N * sizeof( bvhvec4 ) * 4 );
	raysOut = new Buffer( N * sizeof( bvhvec4 ) * 4 );
	connections = new Buffer( N * sizeof( bvhvec4 ) * 3 );
	// set kernel arguments
	init->SetArguments( N, rd.eye, rd.p0, rd.p1, rd.p2, 0, 0, 0, 0 );
	// load raw vertex data
	AddMesh( "./testdata/cryteksponza.bin", 1, bvhvec3( 0 ), 0xffffff );
	AddMesh( "./testdata/lucy.bin", 1.1f, bvhvec3( -2, 4.1f, -3 ), 0xaaaaff );
	// build bvh (here: 'compressed wide bvh', for efficient GPU rendering)
	bvh.Build( tris, triCount );
	// load camera position / direction from file
	std::fstream t = std::fstream{ "camera_gpu.bin", t.binary | t.in };
	if (!t.is_open()) return;
	t.read( (char*)&rd, sizeof( rd ) );
}

// Keyboard handling
bool UpdateCamera( float delta_time_s, fenster& f )
{
	bvhvec3 right = normalize( cross( bvhvec3( 0, 1, 0 ), rd.view ) ), up = 0.8f * cross( rd.view, right );
	// get camera controls.
	bool moved = false;
	if (f.keys['A']) rd.eye += right * -1.0f * delta_time_s * 10, moved = true;
	if (f.keys['D']) rd.eye += right * delta_time_s * 10, moved = true;
	if (f.keys['W']) rd.eye += rd.view * delta_time_s * 10, moved = true;
	if (f.keys['S']) rd.eye += rd.view * -1.0f * delta_time_s * 10, moved = true;
	if (f.keys['R']) rd.eye += up * delta_time_s * 20, moved = true;
	if (f.keys['F']) rd.eye += up * -1.0f * delta_time_s * 20, moved = true;
	if (f.keys[20]) rd.view = normalize( rd.view + right * -1.0f * delta_time_s ), moved = true;
	if (f.keys[19]) rd.view = normalize( rd.view + right * delta_time_s ), moved = true;
	if (f.keys[17]) rd.view = normalize( rd.view + up * -1.0f * delta_time_s ), moved = true;
	if (f.keys[18]) rd.view = normalize( rd.view + up * delta_time_s ), moved = true;
	// recalculate right, up
	right = normalize( cross( bvhvec3( 0, 1, 0 ), rd.view ) ), up = 0.8f * cross( rd.view, right );
	bvhvec3 C = rd.eye + 1.2f * rd.view;
	rd.p0 = C - right + up, rd.p1 = C + right + up, rd.p2 = C - right - up;
	return moved;
}

// Application Tick
void Tick( float delta_time_s, fenster& f, uint32_t* buf )
{
	// handle user input and update camera
	if (UpdateCamera( delta_time_s, f ) || frameIdx++ == 0)
	{
		// memset( accumulator, 0, SCRWIDTH * SCRHEIGHT * sizeof( bvhvec3 ) );
		spp = 1;
	}
	// render on the GPU
	// ..
	// print frame time / rate in window title
	char title[50];
	sprintf( title, "tiny_bvh %.2f s %.2f Hz", delta_time_s, 1.0f / delta_time_s );
	fenster_update_title( &f, title );
}

// Application Shutdown
void Shutdown()
{
	// save camera position / direction to file
	std::fstream s = std::fstream{ "camera.bin", s.binary | s.out };
	s.write( (char*)&rd, sizeof( rd ) );
}