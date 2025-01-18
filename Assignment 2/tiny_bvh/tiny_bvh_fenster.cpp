#define FENSTER_APP_IMPLEMENTATION
#define SCRWIDTH 800
#define SCRHEIGHT 600
#include "external/fenster.h" // https://github.com/zserge/fenster

#define LOADSCENE

#define TINYBVH_IMPLEMENTATION
#include "tiny_bvh.h"
#include <fstream>

using namespace tinybvh;

BVH bvh;
int frameIdx = 0;
Ray* rays = 0;
int* depths = 0;

#ifdef LOADSCENE
bvhvec4* vertices = 0;
uint32_t* indices = 0;
const char scene[] = "cryteksponza.bin";
#else
ALIGNED( 16 ) bvhvec4 vertices[259 /* level 3 */ * 6 * 2 * 49 * 3]{};
ALIGNED( 16 ) uint32_t indices[259 /* level 3 */ * 6 * 2 * 49 * 3]{};
#endif
int verts = 0, inds = 0;

// setup view pyramid for a pinhole camera:
// eye, p1 (top-left), p2 (top-right) and p3 (bottom-left)
#ifdef LOADSCENE
static bvhvec3 eye( -15.24f, 21.5f, 2.54f ), p1, p2, p3;
static bvhvec3 view = normalize( bvhvec3( 0.826f, -0.438f, -0.356f ) );
#else
static bvhvec3 eye( -3.5f, -1.5f, -6.5f ), p1, p2, p3;
static bvhvec3 view = normalize( bvhvec3( 3, 1.5f, 5 ) );
#endif

void sphere_flake( float x, float y, float z, float s, int d = 0 )
{
	// procedural tesselated sphere flake object
#define P(F,a,b,c) p[i+F*64]={(float)a ,(float)b,(float)c}
	bvhvec3 p[384], pos( x, y, z ), ofs( 3.5 );
	for (int i = 0, u = 0; u < 8; u++) for (int v = 0; v < 8; v++, i++)
		P( 0, u, v, 0 ), P( 1, u, 0, v ), P( 2, 0, u, v ),
		P( 3, u, v, 7 ), P( 4, u, 7, v ), P( 5, 7, u, v );
	for (int i = 0; i < 384; i++) p[i] = normalize( p[i] - ofs ) * s + pos;
	for (int i = 0, side = 0; side < 6; side++, i += 8)
		for (int u = 0; u < 7; u++, i++) for (int v = 0; v < 7; v++, i++)
			vertices[verts++] = p[i], vertices[verts++] = p[i + 8],
			vertices[verts++] = p[i + 1], vertices[verts++] = p[i + 1],
			vertices[verts++] = p[i + 9], vertices[verts++] = p[i + 8];
	if (d < 3) sphere_flake( x + s * 1.55f, y, z, s * 0.5f, d + 1 );
	if (d < 3) sphere_flake( x - s * 1.5f, y, z, s * 0.5f, d + 1 );
	if (d < 3) sphere_flake( x, y + s * 1.5f, z, s * 0.5f, d + 1 );
	if (d < 3) sphere_flake( x, x - s * 1.5f, z, s * 0.5f, d + 1 );
	if (d < 3) sphere_flake( x, y, z + s * 1.5f, s * 0.5f, d + 1 );
	if (d < 3) sphere_flake( x, y, z - s * 1.5f, s * 0.5f, d + 1 );
}

void sphere_flake_indexed( float x, float y, float z, float s, int d = 0 )
{
	// procedural tesselated sphere flake object
#define P(F,a,b,c) p[i+F*64]={(float)a ,(float)b,(float)c}
	bvhvec3 p[384], pos( x, y, z ), ofs( 3.5 );
	for (int i = 0, u = 0; u < 8; u++) for (int v = 0; v < 8; v++, i++)
		P( 0, u, v, 0 ), P( 1, u, 0, v ), P( 2, 0, u, v ),
		P( 3, u, v, 7 ), P( 4, u, 7, v ), P( 5, 7, u, v );
	for (int i = 0; i < 384; i++)
		p[i] = vertices[verts + i] = normalize( p[i] - ofs ) * s + pos;
	for (int i = verts, side = 0; side < 6; side++, i += 8, verts += 64)
		for (int u = 0; u < 7; u++, i++) for (int v = 0; v < 7; v++, i++)
			indices[inds++] = i, indices[inds++] = i + 8,
			indices[inds++] = i + 1, indices[inds++] = i + 1,
			indices[inds++] = i + 9, indices[inds++] = i + 8;
	if (d < 3) sphere_flake_indexed( x + s * 1.55f, y, z, s * 0.5f, d + 1 );
	if (d < 3) sphere_flake_indexed( x - s * 1.5f, y, z, s * 0.5f, d + 1 );
	if (d < 3) sphere_flake_indexed( x, y + s * 1.5f, z, s * 0.5f, d + 1 );
	if (d < 3) sphere_flake_indexed( x, x - s * 1.5f, z, s * 0.5f, d + 1 );
	if (d < 3) sphere_flake_indexed( x, y, z + s * 1.5f, s * 0.5f, d + 1 );
	if (d < 3) sphere_flake_indexed( x, y, z - s * 1.5f, s * 0.5f, d + 1 );
}

void Init()
{
#ifdef LOADSCENE
	// load raw vertex data for Crytek's Sponza
	std::string filename{ "./testdata/" };
	filename += scene;
	std::fstream s{ filename, s.binary | s.in };
	s.seekp( 0 );
	s.read( (char*)&verts, 4 );
	printf( "Loading triangle data (%i tris).\n", verts );
	verts *= 3, vertices = (bvhvec4*)malloc64( verts * 16 );
	s.read( (char*)vertices, verts * 16 );
	s.close();
#else
	// generate a sphere flake scene
	// sphere_flake( 0, 0, 0, 1.5f ); // regular sphere flake
	sphere_flake_indexed( 0, 0, 0, 1.5f ); // indexed sphere flake
#endif

	// build a BVH over the scene
	if (inds > 0)
		bvh.BuildHQ( vertices, indices, inds / 3 );
	else
		bvh.BuildHQ( vertices, verts / 3 );

	// load camera position / direction from file
	std::fstream t = std::fstream{ "camera.bin", t.binary | t.in };
	if (!t.is_open()) return;
	t.seekp( 0 );
	t.read( (char*)&eye, sizeof( eye ) );
	t.read( (char*)&view, sizeof( view ) );
	t.close();

	// allocate buffers
	rays = (Ray*)tinybvh::malloc64( SCRWIDTH * SCRHEIGHT * 16 * sizeof( Ray ) );
	depths = (int*)tinybvh::malloc64( SCRWIDTH * SCRHEIGHT * sizeof( int ) );
}

bool UpdateCamera( float delta_time_s, fenster& f )
{
	bvhvec3 right = normalize( cross( bvhvec3( 0, 1, 0 ), view ) );
	bvhvec3 up = 0.8f * cross( view, right );

	// get camera controls.
	bool moved = false;
	if (f.keys['A']) eye += right * -1.0f * delta_time_s * 10, moved = true;
	if (f.keys['D']) eye += right * delta_time_s * 10, moved = true;
	if (f.keys['W']) eye += view * delta_time_s * 10, moved = true;
	if (f.keys['S']) eye += view * -1.0f * delta_time_s * 10, moved = true;
	if (f.keys['R']) eye += up * delta_time_s * 10, moved = true;
	if (f.keys['F']) eye += up * -1.0f * delta_time_s * 10, moved = true;
	if (f.keys[20]) view = normalize( view + right * -1.0f * delta_time_s ), moved = true;
	if (f.keys[19]) view = normalize( view + right * delta_time_s ), moved = true;
	if (f.keys[17]) view = normalize( view + up * -1.0f * delta_time_s ), moved = true;
	if (f.keys[18]) view = normalize( view + up * delta_time_s ), moved = true;

	// recalculate right, up
	right = normalize( cross( bvhvec3( 0, 1, 0 ), view ) );
	up = 0.8f * cross( view, right );
	bvhvec3 C = eye + 2 * view;
	p1 = C - right + up, p2 = C + right + up, p3 = C - right - up;
	return moved;
}

void Tick( float delta_time_s, fenster& f, uint32_t* buf )
{
	// handle user input and update camera
	bool moved = UpdateCamera( delta_time_s, f ) || frameIdx++ == 0;

	// handle user input and update camera
	UpdateCamera( delta_time_s, f );

	// clear the screen with a debug-friendly color
	for (int i = 0; i < SCRWIDTH * SCRHEIGHT; i++) buf[i] = 0xff00ff;

	// generate primary rays in a cacheline-aligned buffer - and, for data locality:
	// organized in 4x4 pixel tiles, 16 samples per pixel, so 256 rays per tile.
	int N = 0;
	Ray* rays = (Ray*)tinybvh::malloc64( SCRWIDTH * SCRHEIGHT * 16 * sizeof( Ray ) );
	int* depths = (int*)tinybvh::malloc64( SCRWIDTH * SCRHEIGHT * sizeof( int ) );
	for (int ty = 0; ty < SCRHEIGHT; ty += 4) for (int tx = 0; tx < SCRWIDTH; tx += 4)
	{
		for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++)
		{
			float u = (float)(tx + x) / SCRWIDTH, v = (float)(ty + y) / SCRHEIGHT;
			bvhvec3 D = normalize( p1 + u * (p2 - p1) + v * (p3 - p1) - eye );
			rays[N++] = Ray( eye, D, 1e30f );
		}
	}

	// trace primary rays
	for (int i = 0; i < N; i++) depths[i] = bvh.Intersect( rays[i] );

	// visualize result
	const bvhvec3 L = normalize( bvhvec3( 1, 2, 3 ) );
	for (int i = 0, ty = 0; ty < SCRHEIGHT / 4; ty++) for (int tx = 0; tx < SCRWIDTH / 4; tx++)
	{
		for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++, i++) if (rays[i].hit.t < 10000)
		{
			int pixel_x = tx * 4 + x, pixel_y = ty * 4 + y, primIdx = rays[i].hit.prim;
			int v0idx = primIdx * 3, v1idx = v0idx + 1, v2idx = v0idx + 2;
			if (inds) v0idx = indices[v0idx], v1idx = indices[v1idx], v2idx = indices[v2idx];
			bvhvec3 v0 = vertices[v0idx];
			bvhvec3 v1 = vertices[v1idx];
			bvhvec3 v2 = vertices[v2idx];
			bvhvec3 N = normalize( cross( v1 - v0, v2 - v0 ) );
			int c = (int)(255.9f * fabs( dot( N, L ) ));
			buf[pixel_x + pixel_y * SCRWIDTH] = c + (c << 8) + (c << 16);
			// buf[pixel_x + pixel_y * SCRWIDTH] = (primIdx * 0xdeece66d + 0xb) & 0xFFFFFF; // color is hashed primitive index
			// buf[pixel_x + pixel_y * SCRWIDTH] = depths[i] << 17; // render depth as red
		}
	}
}

void Shutdown()
{
	// save camera position / direction to file
	std::fstream s = std::fstream{ "camera.bin", s.binary | s.out };
	s.seekp( 0 );
	s.write( (char*)&eye, sizeof( eye ) );
	s.write( (char*)&view, sizeof( view ) );
	s.close();

	// delete allocated buffers
	tinybvh::free64( rays );
	tinybvh::free64( depths );
}