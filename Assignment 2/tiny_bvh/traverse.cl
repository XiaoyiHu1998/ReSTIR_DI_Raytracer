// gpu-side code for ray traversal

// #define CWBVH_COMPRESSED_TRIS // sync with tiny_bvh.h
// #define BVH4_GPU_COMPRESSED_TRIS // sync with tiny_bvh.h

struct Ray
{
	// data is defined here as 16-byte values to encourage the compilers
	// to fetch 16 bytes at a time: 12 (so, 8 + 4) will be slower.
	float4 O, D, rD; // 48 byte
	float4 hit; // 16 byte
};

struct BVHNodeAlt
{
	float4 lmin; // unsigned left in w
	float4 lmax; // unsigned right in w
	float4 rmin; // unsigned triCount in w
	float4 rmax; // unsigned firstTri in w
};

inline uint __activemask() // OpenCL alternative for CUDA's native __activemask
{
	uint mask;
#ifdef ISNVIDIA
	// this obviously only works on NVIDIA hardware.
	asm( "activemask.b32 %0;" : "=r"(mask) );
#endif
	return mask;
}

inline uint __bfind( const uint v ) // OpenCL alternative for CUDA's native __bfind
{
	// see https://docs.nvidia.com/cuda/parallel-thread-execution/#integer-arithmetic-instructions-bfind
#ifdef ISNVIDIA
	uint b;
	asm volatile("bfind.u32 %0, %1; " : "=r"(b) : "r"(v));
	return b;
#else
	return 31 - clz( v ); // only correct if v cannot be zero, which is the case in traverse_cwbvh.
#endif
}

inline uint __popc( const uint v ) // OpenCL alternative for CUDA's native __bfind
{
	// CUDA documentation: "Count the number of bits that are set to 1 in an integer."
#ifdef ISNVIDIA
	int p;
	asm volatile("popc.b32 %0, %1; " : "=r"(p) : "r"(v));
	return p; // note: identical performance to OpenCL popcount?
#else
	return popcount( v );
#endif
}

inline float _native_fma( const float a, const float b, const float c )
{
#ifdef ISNVIDIA
	float d;
	asm volatile("fma.rz.f32 %0, %1, %2, %3;" : "=f"(d) : "f"(a), "f"(b), "f"(c));
	return d;
#else
#ifdef FP_FAST_FMAF // https://registry.khronos.org/OpenCL/specs/3.0-unified/html/OpenCL_C.html
	return fma( a, b, c );
#else
	return a * b + c;
#endif
#endif
}

inline float fmin_fmin( const float a, const float b, const float c )
{
#if defined( ISNVIDIA ) && defined( ISPASCAL )
	// not a win on Turing
	return as_float( min_min( as_int( a ), as_int( b ), as_int( c ) ) );
#else
	return fmin( fmin( a, b ), c );
#endif
}

inline float fmax_fmax( const float a, const float b, const float c )
{
#if defined( ISNVIDIA ) && defined( ISPASCAL )
	return as_float( max_max( as_int( a ), as_int( b ), as_int( c ) ) );
#else
	return fmax( fmax( a, b ), c );
#endif
}

// ============================================================================
//
//        T R A V E R S E _ A I L A L A I N E
// 
// ============================================================================

float4 traverse_ailalaine( global struct BVHNodeAlt* altNode, global unsigned* idx, global float4* verts, const float3 O, const float3 D, const float3 rD, const float tmax )
{
	// traverse BVH
	float4 hit;
	hit.x = tmax;
	unsigned node = 0, stack[64], stackPtr = 0;
	while (1)
	{
		// fetch the node
		const float4 lmin = altNode[node].lmin, lmax = altNode[node].lmax;
		const float4 rmin = altNode[node].rmin, rmax = altNode[node].rmax;
		const unsigned triCount = as_uint( rmin.w );
		if (triCount > 0)
		{
			// process leaf node
			const unsigned firstTri = as_uint( rmax.w );
			for (unsigned i = 0; i < triCount; i++)
			{
				const unsigned triIdx = idx[firstTri + i];
				const float4* tri = verts + 3 * triIdx;
				// triangle intersection - Möller-Trumbore
				const float4 edge1 = tri[1] - tri[0], edge2 = tri[2] - tri[0];
				const float3 h = cross( D, edge2.xyz );
				const float a = dot( edge1.xyz, h );
				if (fabs( a ) < 0.0000001f) continue;
				const float f = 1 / a;
				const float3 s = O - tri[0].xyz;
				const float u = f * dot( s, h );
				if (u < 0 || u > 1) continue;
				const float3 q = cross( s, edge1.xyz );
				const float v = f * dot( D, q );
				if (v < 0 || u + v > 1) continue;
				const float d = f * dot( edge2.xyz, q );
				if (d > 0.0f && d < hit.x) hit = (float4)(d, u, v, as_float( triIdx ));
			}
			if (stackPtr == 0) break;
			node = stack[--stackPtr];
			continue;
		}
		unsigned left = as_uint( lmin.w ), right = as_uint( lmax.w );
		// child AABB intersection tests
		const float3 t1a = (lmin.xyz - O) * rD, t2a = (lmax.xyz - O) * rD;
		const float3 t1b = (rmin.xyz - O) * rD, t2b = (rmax.xyz - O) * rD;
		const float3 minta = fmin( t1a, t2a ), maxta = fmax( t1a, t2a );
		const float3 mintb = fmin( t1b, t2b ), maxtb = fmax( t1b, t2b );
		const float tmina = fmax( fmax( fmax( minta.x, minta.y ), minta.z ), 0 );
		const float tminb = fmax( fmax( fmax( mintb.x, mintb.y ), mintb.z ), 0 );
		const float tmaxa = fmin( fmin( fmin( maxta.x, maxta.y ), maxta.z ), hit.x );
		const float tmaxb = fmin( fmin( fmin( maxtb.x, maxtb.y ), maxtb.z ), hit.x );
		float dist1 = tmina > tmaxa ? 1e30f : tmina;
		float dist2 = tminb > tmaxb ? 1e30f : tminb;
		// traverse nearest child first
		if (dist1 > dist2)
		{
			float h = dist1; dist1 = dist2; dist2 = h;
			unsigned t = left; left = right; right = t;
		}
		if (dist1 == 1e30f)
		{
			if (stackPtr == 0) break; else node = stack[--stackPtr];
		}
		else
		{
			node = left;
			if (dist2 != 1e30f) stack[stackPtr++] = right;
		}
	}
	// write back intersection result
	return hit;
}

bool isoccluded_ailalaine( global struct BVHNodeAlt* altNode, global unsigned* idx, global float4* verts, const float3 O, const float3 D, const float3 rD, const float tmax )
{
	// traverse BVH
	unsigned node = 0, stack[64], stackPtr = 0;
	while (1)
	{
		// fetch the node
		const float4 lmin = altNode[node].lmin, lmax = altNode[node].lmax;
		const float4 rmin = altNode[node].rmin, rmax = altNode[node].rmax;
		const unsigned triCount = as_uint( rmin.w );
		if (triCount > 0)
		{
			// process leaf node
			const unsigned firstTri = as_uint( rmax.w );
			for (unsigned i = 0; i < triCount; i++)
			{
				const unsigned triIdx = idx[firstTri + i];
				const float4* tri = verts + 3 * triIdx;
				// triangle intersection - Möller-Trumbore
				const float4 edge1 = tri[1] - tri[0], edge2 = tri[2] - tri[0];
				const float3 h = cross( D, edge2.xyz );
				const float a = dot( edge1.xyz, h );
				if (fabs( a ) < 0.0000001f) continue;
				const float f = 1 / a;
				const float3 s = O - tri[0].xyz;
				const float u = f * dot( s, h );
				if (u < 0 || u > 1) continue;
				const float3 q = cross( s, edge1.xyz );
				const float v = f * dot( D, q );
				if (v < 0 || u + v > 1) continue;
				const float d = f * dot( edge2.xyz, q );
				if (d > 0.0f && d < tmax) return true;
			}
			if (stackPtr == 0) break;
			node = stack[--stackPtr];
			continue;
		}
		unsigned left = as_uint( lmin.w ), right = as_uint( lmax.w );
		// child AABB intersection tests
		const float3 t1a = (lmin.xyz - O) * rD, t2a = (lmax.xyz - O) * rD;
		const float3 t1b = (rmin.xyz - O) * rD, t2b = (rmax.xyz - O) * rD;
		const float3 minta = fmin( t1a, t2a ), maxta = fmax( t1a, t2a );
		const float3 mintb = fmin( t1b, t2b ), maxtb = fmax( t1b, t2b );
		const float tmina = fmax( fmax( fmax( minta.x, minta.y ), minta.z ), 0 );
		const float tminb = fmax( fmax( fmax( mintb.x, mintb.y ), mintb.z ), 0 );
		const float tmaxa = fmin( fmin( fmin( maxta.x, maxta.y ), maxta.z ), tmax );
		const float tmaxb = fmin( fmin( fmin( maxtb.x, maxtb.y ), maxtb.z ), tmax );
		float dist1 = tmina > tmaxa ? 1e30f : tmina;
		float dist2 = tminb > tmaxb ? 1e30f : tminb;
		// traverse nearest child first
		if (dist1 > dist2)
		{
			float h = dist1; dist1 = dist2; dist2 = h;
			unsigned t = left; left = right; right = t;
		}
		if (dist1 == 1e30f)
		{
			if (stackPtr == 0) break; else node = stack[--stackPtr];
		}
		else
		{
			node = left;
			if (dist2 != 1e30f) stack[stackPtr++] = right;
		}
	}
	// no hit found
	return false;
}

void kernel batch_ailalaine( global struct BVHNodeAlt* altNode, global unsigned* idx, global float4* verts, global struct Ray* rayData )
{
	// fetch ray
	const unsigned threadId = get_global_id( 0 );
	const float3 O = rayData[threadId].O.xyz;
	const float3 D = rayData[threadId].D.xyz;
	const float3 rD = rayData[threadId].rD.xyz;
	float4 hit = traverse_ailalaine( altNode, idx, verts, O, D, rD, 1e30f );
	rayData[threadId].hit = hit;
}

// ============================================================================
//
//        T R A V E R S E _ G P U 4 W A Y
// 
// ============================================================================

#ifdef BVH4_GPU_COMPRESSED_TRIS
#define STRIDE 4
#else
#define STRIDE 3
#endif

void IntersectTri( const unsigned vertIdx, const float3* O, const float3* D, float4* hit, const global float4* alt4Node ) 
{
#ifdef BVH4_GPU_COMPRESSED_TRIS
	const float4 T2 = alt4Node[vertIdx + 2];
	const float transS = T2.x * O->x + T2.y * O->y + T2.z * O->z + T2.w;
	const float transD = T2.x * D->x + T2.y * D->y + T2.z * D->z, d = -transS / transD;
	if (d <= 0 || d >= hit->x) return;
	const float4 T0 = alt4Node[vertIdx + 0], T1 = alt4Node[vertIdx + 1];
	const float3 I = *O + d * *D;
	const float u = T0.x * I.x + T0.y * I.y + T0.z * I.z + T0.w;
	const float v = T1.x * I.x + T1.y * I.y + T1.z * I.z + T1.w;
	const bool trihit = u >= 0 && v >= 0 && u + v < 1;
	if (trihit) *hit = (float4)(d, u, v, as_uint( alt4Node[vertIdx + 3].w ) );
#else
	const float4 edge2 = alt4Node[vertIdx + 2];
	const float4 edge1 = alt4Node[vertIdx + 1];
	const float4 v0 = alt4Node[vertIdx];
	const float3 h = cross( *D, edge2.xyz );
	const float a = dot( edge1.xyz, h );
	if (fabs( a ) < 0.0000001f) return;
	const float f = native_recip( a );
	const float3 s = *O - v0.xyz;
	const float u = f * dot( s, h );
	if (u < 0 || u > 1) return;
	const float3 q = cross( s, edge1.xyz );
	const float v = f * dot( *D, q );
	if (v < 0 || u + v > 1) return;
	const float d = f * dot( edge2.xyz, q );
	if (d > 0.0f && d < hit->x) *hit = (float4)(d, u, v, v0.w);
#endif
}

bool TriOccluded( const unsigned vertIdx, const float3* O, const float3* D, float tmax, const global float4* alt4Node ) 
{
#ifdef BVH4_GPU_COMPRESSED_TRIS
	const float4 T2 = alt4Node[vertIdx + 2];
	const float transS = T2.x * O->x + T2.y * O->y + T2.z * O->z + T2.w;
	const float transD = T2.x * D->x + T2.y * D->y + T2.z * D->z, d = -transS / transD;
	if (d <= 0 || d >= tmax) return false;
	const float4 T0 = alt4Node[vertIdx + 0], T1 = alt4Node[vertIdx + 1];
	const float3 I = *O + d * *D;
	const float u = T0.x * I.x + T0.y * I.y + T0.z * I.z + T0.w;
	const float v = T1.x * I.x + T1.y * I.y + T1.z * I.z + T1.w;
	return u >= 0 && v >= 0 && u + v < 1;
#else
	const float4 edge2 = alt4Node[vertIdx + 2];
	const float4 edge1 = alt4Node[vertIdx + 1];
	const float4 v0 = alt4Node[vertIdx];
	const float3 h = cross( *D, edge2.xyz );
	const float a = dot( edge1.xyz, h );
	if (fabs( a ) < 0.0000001f) return false;
	const float f = native_recip( a );
	const float3 s = *O - v0.xyz;
	const float u = f * dot( s, h );
	if (u < 0 || u > 1) return false;
	const float3 q = cross( s, edge1.xyz );
	const float v = f * dot( *D, q );
	if (v < 0 || u + v > 1) return false;
	const float d = f * dot( edge2.xyz, q );
	return d > 0.0f && d < tmax;
#endif
}

float4 traverse_gpu4way( const global float4* alt4Node, const float3 O, const float3 D, const float3 rD, const float tmax )
{
	float4 hit;
	hit.x = tmax;
	// traverse the BVH
	const float4 zero4 = (float4)(0);
	unsigned offset = 0, stack[64], stackPtr = 0;
	const unsigned smBase = get_local_id( 0 ) * 4;
	while (1)
	{
		// vectorized 4-wide quantized aabb intersection
		const float4 data0 = alt4Node[offset];
		const float4 data1 = alt4Node[offset + 1];
		const float4 data2 = alt4Node[offset + 2];
		const float4 cminx4 = convert_float4( as_uchar4( data0.w ) );
		const float4 cmaxx4 = convert_float4( as_uchar4( data1.w ) );
		const float4 cminy4 = convert_float4( as_uchar4( data2.x ) );
		const float3 bminO = (O - data0.xyz) * rD, rDe = rD * data1.xyz;
		const float4 cmaxy4 = convert_float4( as_uchar4( data2.y ) );
		const float4 cminz4 = convert_float4( as_uchar4( data2.z ) );
		const float4 cmaxz4 = convert_float4( as_uchar4( data2.w ) );
		const float4 t1x4 = cminx4 * rDe.xxxx - bminO.xxxx, t2x4 = cmaxx4 * rDe.xxxx - bminO.xxxx;
		const float4 t1y4 = cminy4 * rDe.yyyy - bminO.yyyy, t2y4 = cmaxy4 * rDe.yyyy - bminO.yyyy;
		const float4 t1z4 = cminz4 * rDe.zzzz - bminO.zzzz, t2z4 = cmaxz4 * rDe.zzzz - bminO.zzzz;
		uint4 data3 = as_uint4( alt4Node[offset + 3] );
		const float4 mintx4 = fmin( t1x4, t2x4 ), maxtx4 = fmax( t1x4, t2x4 );
		const float4 minty4 = fmin( t1y4, t2y4 ), maxty4 = fmax( t1y4, t2y4 );
		const float4 mintz4 = fmin( t1z4, t2z4 ), maxtz4 = fmax( t1z4, t2z4 );
		const float4 maxxy4 = select( mintx4, minty4, isless( mintx4, minty4 ) );
		const float4 maxyz4 = select( maxxy4, mintz4, isless( maxxy4, mintz4 ) );
		float4 dst4 = select( maxyz4, zero4, isless( maxyz4, zero4 ) );
		const float4 minxy4 = select( maxtx4, maxty4, isgreater( maxtx4, maxty4 ) );
		const float4 minyz4 = select( minxy4, maxtz4, isgreater( minxy4, maxtz4 ) );
		const float4 tmax4 = select( minyz4, hit.xxxx, isgreater( minyz4, hit.xxxx ) );
		dst4 = select( dst4, (float4)(1e30f), isgreater( dst4, tmax4 ) );
		// sort intersection distances - TODO: handle single-intersection case separately.
		if (dst4.x < dst4.z) dst4 = dst4.zyxw, data3 = data3.zyxw; // bertdobbelaere.github.io/sorting_networks.html
		if (dst4.y < dst4.w) dst4 = dst4.xwzy, data3 = data3.xwzy;
		if (dst4.x < dst4.y) dst4 = dst4.yxzw, data3 = data3.yxzw;
		if (dst4.z < dst4.w) dst4 = dst4.xywz, data3 = data3.xywz;
		if (dst4.y < dst4.z) dst4 = dst4.xzyw, data3 = data3.xzyw;
		// process results, starting with farthest child, so nearest ends on top of stack
		unsigned nextNode = 0;
		if (dst4.x < 1e30f) if ((data3.x >> 31) == 0) nextNode = data3.x; else
		{
			const unsigned triCount = (data3.x >> 16) & 0x7fff;
			for( int i = 0; i < triCount; i++ ) IntersectTri( (data3.x & 0xffff) + offset + i * STRIDE, &O, &D, &hit, alt4Node );
		}
		if (dst4.y < 1e30f) if (data3.y >> 31)
		{
			const unsigned triCount = (data3.y >> 16) & 0x7fff;
			for( int i = 0; i < triCount; i++ ) IntersectTri( (data3.y & 0xffff) + offset + i * STRIDE, &O, &D, &hit, alt4Node );
		}
		else
		{
			if (nextNode) stack[stackPtr++] = nextNode;
			nextNode = data3.y;
		}
		if (dst4.z < 1e30f) if (data3.z >> 31) 
		{
			const unsigned triCount = (data3.z >> 16) & 0x7fff;
			for( int i = 0; i < triCount; i++ ) IntersectTri( (data3.z & 0xffff) + offset + i * STRIDE, &O, &D, &hit, alt4Node );
		}
		else
		{
			if (nextNode) stack[stackPtr++] = nextNode;
			nextNode = data3.z;
		}
		if (dst4.w < 1e30f) if (data3.w >> 31) 
		{
			const unsigned triCount = (data3.w >> 16) & 0x7fff;
			for( int i = 0; i < triCount; i++ ) IntersectTri( (data3.w & 0xffff) + offset + i * STRIDE, &O, &D, &hit, alt4Node );
		}
		else
		{
			if (nextNode) stack[stackPtr++] = nextNode;
			nextNode = data3.w;
		}
		// continue with nearest node or first node on the stack
		if (nextNode) offset = nextNode; else
		{
			if (!stackPtr) break;
			offset = stack[--stackPtr];
		}
	}
	return hit;
}

bool isoccluded_gpu4way( const global float4* alt4Node, const float3 O, const float3 D, const float3 rD, const float tmax )
{
	// traverse the BVH
	const float4 zero4 = (float4)(0), t4 = (float4)(tmax);
	unsigned offset = 0, stack[64], stackPtr = 0;
	const unsigned smBase = get_local_id( 0 ) * 4;
	while (1)
	{
		// vectorized 4-wide quantized aabb intersection
		const float4 data0 = alt4Node[offset];
		const float4 data1 = alt4Node[offset + 1];
		const float4 data2 = alt4Node[offset + 2];
		const float4 cminx4 = convert_float4( as_uchar4( data0.w ) );
		const float4 cmaxx4 = convert_float4( as_uchar4( data1.w ) );
		const float4 cminy4 = convert_float4( as_uchar4( data2.x ) );
		const float3 bminO = (O - data0.xyz) * rD, rDe = rD * data1.xyz;
		const float4 cmaxy4 = convert_float4( as_uchar4( data2.y ) );
		const float4 cminz4 = convert_float4( as_uchar4( data2.z ) );
		const float4 cmaxz4 = convert_float4( as_uchar4( data2.w ) );
		const float4 t1x4 = cminx4 * rDe.xxxx - bminO.xxxx, t2x4 = cmaxx4 * rDe.xxxx - bminO.xxxx;
		const float4 t1y4 = cminy4 * rDe.yyyy - bminO.yyyy, t2y4 = cmaxy4 * rDe.yyyy - bminO.yyyy;
		const float4 t1z4 = cminz4 * rDe.zzzz - bminO.zzzz, t2z4 = cmaxz4 * rDe.zzzz - bminO.zzzz;
		uint4 data3 = as_uint4( alt4Node[offset + 3] );
		const float4 mintx4 = fmin( t1x4, t2x4 ), maxtx4 = fmax( t1x4, t2x4 );
		const float4 minty4 = fmin( t1y4, t2y4 ), maxty4 = fmax( t1y4, t2y4 );
		const float4 mintz4 = fmin( t1z4, t2z4 ), maxtz4 = fmax( t1z4, t2z4 );
		const float4 maxxy4 = select( mintx4, minty4, isless( mintx4, minty4 ) );
		const float4 maxyz4 = select( maxxy4, mintz4, isless( maxxy4, mintz4 ) );
		float4 dst4 = select( maxyz4, zero4, isless( maxyz4, zero4 ) );
		const float4 minxy4 = select( maxtx4, maxty4, isgreater( maxtx4, maxty4 ) );
		const float4 minyz4 = select( minxy4, maxtz4, isgreater( minxy4, maxtz4 ) );
		const float4 tmax4 = select( minyz4, t4, isgreater( minyz4, t4 ) );
		dst4 = select( dst4, (float4)(1e30f), isgreater( dst4, tmax4 ) );
		// sort intersection distances - TODO: handle single-intersection case separately.
		if (dst4.x < dst4.z) dst4 = dst4.zyxw, data3 = data3.zyxw; // bertdobbelaere.github.io/sorting_networks.html
		if (dst4.y < dst4.w) dst4 = dst4.xwzy, data3 = data3.xwzy;
		if (dst4.x < dst4.y) dst4 = dst4.yxzw, data3 = data3.yxzw;
		if (dst4.z < dst4.w) dst4 = dst4.xywz, data3 = data3.xywz;
		if (dst4.y < dst4.z) dst4 = dst4.xzyw, data3 = data3.xzyw;
		// process results, starting with farthest child, so nearest ends on top of stack
		unsigned nextNode = 0;
		if (dst4.x < 1e30f) if ((data3.x >> 31) == 0) nextNode = data3.x; else
		{
			const unsigned triCount = (data3.x >> 16) & 0x7fff;
			for( int i = 0; i < triCount; i++ ) 
				if (TriOccluded( (data3.x & 0xffff) + offset + i * STRIDE, &O, &D, tmax, alt4Node )) return true;
		}
		if (dst4.y < 1e30f) if (data3.y >> 31)
		{
			const unsigned triCount = (data3.y >> 16) & 0x7fff;
			for( int i = 0; i < triCount; i++ ) 
				if (TriOccluded( (data3.y & 0xffff) + offset + i * STRIDE, &O, &D, tmax, alt4Node )) return true;
		}
		else
		{
			if (nextNode) stack[stackPtr++] = nextNode;
			nextNode = data3.y;
		}
		if (dst4.z < 1e30f) if (data3.z >> 31) 
		{
			const unsigned triCount = (data3.z >> 16) & 0x7fff;
			for( int i = 0; i < triCount; i++ ) 
				if (TriOccluded( (data3.z & 0xffff) + offset + i * STRIDE, &O, &D, tmax, alt4Node )) return true;
		}
		else
		{
			if (nextNode) stack[stackPtr++] = nextNode;
			nextNode = data3.z;
		}
		if (dst4.w < 1e30f) if (data3.w >> 31) 
		{
			const unsigned triCount = (data3.w >> 16) & 0x7fff;
			for( int i = 0; i < triCount; i++ ) 
				if (TriOccluded( (data3.w & 0xffff) + offset + i * STRIDE, &O, &D, tmax, alt4Node )) return true;
		}
		else
		{
			if (nextNode) stack[stackPtr++] = nextNode;
			nextNode = data3.w;
		}
		// continue with nearest node or first node on the stack
		if (nextNode) offset = nextNode; else
		{
			if (!stackPtr) break;
			offset = stack[--stackPtr];
		}
	}
	// no occlusion found.
	return false;
}

void kernel batch_gpu4way( const global float4* alt4Node, global struct Ray* rayData )
{
	// fetch ray
	const unsigned threadId = get_global_id( 0 );
	const float3 O = rayData[threadId].O.xyz;
	const float3 D = rayData[threadId].D.xyz;
	const float3 rD = rayData[threadId].rD.xyz;
	float4 hit = traverse_gpu4way( alt4Node, O, D, rD, 1e30f );
	rayData[threadId].hit = hit;
}

// ============================================================================
//
//        T R A V E R S E _ C W B V H
// 
// ============================================================================

// preliminaries

#define STACK_SIZE 32
#ifdef ISINTEL // Iris Xe, Arc, ..
// #define USE_VLOAD_VSTORE
#define SIMD_AABBTEST
#elif defined ISNVIDIA // 2080, 3080, 4080, ..
#define USE_VLOAD_VSTORE
// #define SIMD_AABBTEST
#elif defined ISAMD
#define USE_VLOAD_VSTORE
// #define SIMD_AABBTEST
#else // unkown GPU
// #define USE_VLOAD_VSTORE
#define SIMD_AABBTEST
#endif

#ifdef USE_VLOAD_VSTORE
#define STACK_POP(X) { unsigned* a = &stack[--stackPtr]; X = vload2( 0, a ); }
#define STACK_PUSH(X) { unsigned* a = &stack[stackPtr++]; vstore2( X, 0, a ); }
#else
#define STACK_POP(X) { X = stack[--stackPtr]; }
#define STACK_PUSH(X) { stack[stackPtr++] = X; }
#endif
inline unsigned sign_extend_s8x4( const unsigned i )
{
#ifdef ISNVIDIA
	// inline ptx as suggested by AlanWBFT
	uint v;
	asm( "prmt.b32 %0, %1, 0x0, 0x0000BA98;" : "=r"(v) : "r"(i) ); // BA98: 1011`1010`1001`1000
	return v;
#else
	// docs: "with the given parameters, prmt will extend the sign to all bits in a byte."
	const unsigned b0 = (i & 0b10000000000000000000000000000000) ? 0xff000000 : 0;
	const unsigned b1 = (i & 0b00000000100000000000000000000000) ? 0x00ff0000 : 0;
	const unsigned b2 = (i & 0b00000000000000001000000000000000) ? 0x0000ff00 : 0;
	const unsigned b3 = (i & 0b00000000000000000000000010000000) ? 0x000000ff : 0;
	return b0 + b1 + b2 + b3; // probably can do better than this.
#endif
}
#ifdef ISNVIDIA
#define UPDATE_HITMASK asm( "vshl.u32.u32.u32.wrap.add %0,%1.b0, %2.b0, %3;" : "=r"(hitmask) : "r"(child_bits4), "r"(bit_index4), "r"(hitmask) );
#define UPDATE_HITMASK0 asm( "vshl.u32.u32.u32.wrap.add %0,%1.b0, %2.b0, %3;" : "=r"(hitmask) : "r"(child_bits4), "r"(bit_index4), "r"(hitmask) );
#define UPDATE_HITMASK1 asm( "vshl.u32.u32.u32.wrap.add %0,%1.b1, %2.b1, %3;" : "=r"(hitmask) : "r"(child_bits4), "r"(bit_index4), "r"(hitmask) );
#define UPDATE_HITMASK2 asm( "vshl.u32.u32.u32.wrap.add %0,%1.b2, %2.b2, %3;" : "=r"(hitmask) : "r"(child_bits4), "r"(bit_index4), "r"(hitmask) );
#define UPDATE_HITMASK3 asm( "vshl.u32.u32.u32.wrap.add %0,%1.b3, %2.b3, %3;" : "=r"(hitmask) : "r"(child_bits4), "r"(bit_index4), "r"(hitmask) );
#else
#define UPDATE_HITMASK hitmask = (child_bits4 & 255) << (bit_index4 & 31)
#define UPDATE_HITMASK0 hitmask |= (child_bits4 & 255) << (bit_index4 & 31)
#define UPDATE_HITMASK1 hitmask |= ((child_bits4 >> 8) & 255) << ((bit_index4 >> 8) & 31);
#define UPDATE_HITMASK2 hitmask |= ((child_bits4 >> 16) & 255) << ((bit_index4 >> 16) & 31);
#define UPDATE_HITMASK3 hitmask |= (child_bits4 >> 24) << (bit_index4 >> 24);
#endif

// kernel
// based on CUDA code by AlanWBFT https://github.com/AlanIWBFT

float4 traverse_cwbvh( global const float4* cwbvhNodes, global const float4* cwbvhTris, const float3 O, const float3 D, const float3 rD, const float t )
{
	// initialize ray
	const unsigned threadId = get_global_id( 0 );
#ifdef SIMD_AABBTEST
	const float4 O4 = (float4)( O, 1 ); // rayData[threadId].O;
	const float4 D4 = (float4)( D, 0 ); // rayData[threadId].D;
	const float4 rD4 = (float4)( rD, 1 ); // rayData[threadId].rD;
#endif // otherwise, we'll use the float3 input directly.
	float4 hit;
	hit.x = t; // not fetching this from ray data to avoid one memory operation.
	// prepare traversal
	uint2 stack[STACK_SIZE];
	uint hitAddr, stackPtr = 0;
	float2 uv;
	float tmax = t;
#ifdef SIMD_AABBTEST
	const uint octinv4 = (7 - ((D4.x < 0 ? 4 : 0) | (D4.y < 0 ? 2 : 0) | (D4.z < 0 ? 1 : 0))) * 0x1010101;
#else
	const uint octinv4 = (7 - ((D.x < 0 ? 4 : 0) | (D.y < 0 ? 2 : 0) | (D.z < 0 ? 1 : 0))) * 0x1010101;
#endif
	uint2 ngroup = (uint2)(0, 0b10000000000000000000000000000000), tgroup = (uint2)(0);
	do
	{
		if (ngroup.y > 0x00FFFFFF)
		{
			const unsigned hits = ngroup.y, imask = ngroup.y;
			const unsigned child_bit_index = __bfind( hits );
			const unsigned child_node_base_index = ngroup.x;
			ngroup.y &= ~(1 << child_bit_index);
			if (ngroup.y > 0x00FFFFFF) { STACK_PUSH( ngroup ); }
			{
				const unsigned slot_index = (child_bit_index - 24) ^ (octinv4 & 255);
				const unsigned relative_index = __popc( imask & ~(0xFFFFFFFF << slot_index) );
				const unsigned child_node_index = child_node_base_index + relative_index;
			#ifdef USE_VLOAD_VSTORE
				const float* p = (float*)&cwbvhNodes[child_node_index * 5 + 0];
				float4 n0 = vload4( 0, p ), n1 = vload4( 1, p ), n2 = vload4( 2, p );
				float4 n3 = vload4( 3, p ), n4 = vload4( 4, p );
			#else
				float4 n0 = cwbvhNodes[child_node_index * 5 + 0], n1 = cwbvhNodes[child_node_index * 5 + 1];
				float4 n2 = cwbvhNodes[child_node_index * 5 + 2], n3 = cwbvhNodes[child_node_index * 5 + 3];
				float4 n4 = cwbvhNodes[child_node_index * 5 + 4];
			#endif
				const char4 e = as_char4( n0.w );
				ngroup.x = as_uint( n1.x ), tgroup = (uint2)(as_uint( n1.y ), 0);
				unsigned hitmask = 0;
			#ifdef SIMD_AABBTEST
				const float4 idir4 = (float4)(
					as_float( (e.x + 127) << 23 ) * rD4.x, as_float( (e.y + 127) << 23 ) * rD4.y,
					as_float( (e.z + 127) << 23 ) * rD4.z, 1
				);
				const float4 orig4 = (n0 - O4) * rD4;
			#else
				const float idirx = as_float( (e.x + 127) << 23 ) * rD.x;
				const float idiry = as_float( (e.y + 127) << 23 ) * rD.y;
				const float idirz = as_float( (e.z + 127) << 23 ) * rD.z;
				const float origx = (n0.x - O.x) * rD.x;
				const float origy = (n0.y - O.y) * rD.y;
				const float origz = (n0.z - O.z) * rD.z;
			#endif
				{	// first 4
					const unsigned meta4 = as_uint( n1.z ), is_inner4 = (meta4 & (meta4 << 1)) & 0x10101010;
					const unsigned inner_mask4 = sign_extend_s8x4( is_inner4 << 3 );
					const unsigned bit_index4 = (meta4 ^ (octinv4 & inner_mask4)) & 0x1F1F1F1F;
					const unsigned child_bits4 = (meta4 >> 5) & 0x07070707;
					const float4 lox4 = convert_float4( as_uchar4( rD.x < 0 ? n3.z : n2.x ) ), hix4 = convert_float4( as_uchar4( rD.x < 0 ? n2.x : n3.z ) );
					const float4 loy4 = convert_float4( as_uchar4( rD.y < 0 ? n4.x : n2.z ) ), hiy4 = convert_float4( as_uchar4( rD.y < 0 ? n2.z : n4.x ) );
					const float4 loz4 = convert_float4( as_uchar4( rD.z < 0 ? n4.z : n3.x ) ), hiz4 = convert_float4( as_uchar4( rD.z < 0 ? n3.x : n4.z ) );
					{
					#ifdef SIMD_AABBTEST
						const float4 tminx4 = lox4 * idir4.xxxx + orig4.xxxx, tmaxx4 = hix4 * idir4.xxxx + orig4.xxxx;
						const float4 tminy4 = loy4 * idir4.yyyy + orig4.yyyy, tmaxy4 = hiy4 * idir4.yyyy + orig4.yyyy;
						const float4 tminz4 = loz4 * idir4.zzzz + orig4.zzzz, tmaxz4 = hiz4 * idir4.zzzz + orig4.zzzz;
						const float cmina = fmax( fmax( fmax( tminx4.x, tminy4.x ), tminz4.x ), 0 );
						const float cmaxa = fmin( fmin( fmin( tmaxx4.x, tmaxy4.x ), tmaxz4.x ), tmax );
						const float cminb = fmax( fmax( fmax( tminx4.y, tminy4.y ), tminz4.y ), 0 );
						const float cmaxb = fmin( fmin( fmin( tmaxx4.y, tmaxy4.y ), tmaxz4.y ), tmax );
						if (cmina <= cmaxa) UPDATE_HITMASK;
						if (cminb <= cmaxb) UPDATE_HITMASK1;
					#else
						float tminx0 = _native_fma( lox4.x, idirx, origx ), tminx1 = _native_fma( lox4.y, idirx, origx );
						float tminy0 = _native_fma( loy4.x, idiry, origy ), tminy1 = _native_fma( loy4.y, idiry, origy );
						float tminz0 = _native_fma( loz4.x, idirz, origz ), tminz1 = _native_fma( loz4.y, idirz, origz );
						float tmaxx0 = _native_fma( hix4.x, idirx, origx ), tmaxx1 = _native_fma( hix4.y, idirx, origx );
						float tmaxy0 = _native_fma( hiy4.x, idiry, origy ), tmaxy1 = _native_fma( hiy4.y, idiry, origy );
						float tmaxz0 = _native_fma( hiz4.x, idirz, origz ), tmaxz1 = _native_fma( hiz4.y, idirz, origz );
						n0.x = fmax( fmax_fmax( tminx0, tminy0, tminz0 ), 0 );
						n0.y = fmin( fmin_fmin( tmaxx0, tmaxy0, tmaxz0 ), tmax );
						n1.x = fmax( fmax_fmax( tminx1, tminy1, tminz1 ), 0 );
						n1.y = fmin( fmin_fmin( tmaxx1, tmaxy1, tmaxz1 ), tmax );
						if (n0.x <= n0.y) UPDATE_HITMASK;
						if (n1.x <= n1.y) UPDATE_HITMASK1;
					#endif
					#ifdef SIMD_AABBTEST
						const float cminc = fmax( fmax( fmax( tminx4.z, tminy4.z ), tminz4.z ), 0 );
						const float cmaxc = fmin( fmin( fmin( tmaxx4.z, tmaxy4.z ), tmaxz4.z ), tmax );
						const float cmind = fmax( fmax( fmax( tminx4.w, tminy4.w ), tminz4.w ), 0 );
						const float cmaxd = fmin( fmin( fmin( tmaxx4.w, tmaxy4.w ), tmaxz4.w ), tmax );
						if (cminc <= cmaxc) UPDATE_HITMASK2;
						if (cmind <= cmaxd) UPDATE_HITMASK3;
					#else
						tminx0 = _native_fma( lox4.z, idirx, origx ), tminx1 = _native_fma( lox4.w, idirx, origx );
						tminy0 = _native_fma( loy4.z, idiry, origy ), tminy1 = _native_fma( loy4.w, idiry, origy );
						tminz0 = _native_fma( loz4.z, idirz, origz ), tminz1 = _native_fma( loz4.w, idirz, origz );
						tmaxx0 = _native_fma( hix4.z, idirx, origx ), tmaxx1 = _native_fma( hix4.w, idirx, origx );
						tmaxy0 = _native_fma( hiy4.z, idiry, origy ), tmaxy1 = _native_fma( hiy4.w, idiry, origy );
						tmaxz0 = _native_fma( hiz4.z, idirz, origz ), tmaxz1 = _native_fma( hiz4.w, idirz, origz );
						n0.x = fmax( fmax_fmax( tminx0, tminy0, tminz0 ), 0 );
						n0.y = fmin( fmin_fmin( tmaxx0, tmaxy0, tmaxz0 ), tmax );
						n1.x = fmax( fmax_fmax( tminx1, tminy1, tminz1 ), 0 );
						n1.y = fmin( fmin_fmin( tmaxx1, tmaxy1, tmaxz1 ), tmax );
						if (n0.x <= n0.y) UPDATE_HITMASK2;
						if (n1.x <= n1.y) UPDATE_HITMASK3;
					#endif
					}
				}
				{	// second 4
					const unsigned meta4 = as_uint( n1.w ), is_inner4 = (meta4 & (meta4 << 1)) & 0x10101010;
					const unsigned inner_mask4 = sign_extend_s8x4( is_inner4 << 3 );
					const unsigned bit_index4 = (meta4 ^ (octinv4 & inner_mask4)) & 0x1F1F1F1F;
					const unsigned child_bits4 = (meta4 >> 5) & 0x07070707;
					const float4 lox4 = convert_float4( as_uchar4( rD.x < 0 ? n3.w : n2.y ) ), hix4 = convert_float4( as_uchar4( rD.x < 0 ? n2.y : n3.w ) );
					const float4 loy4 = convert_float4( as_uchar4( rD.y < 0 ? n4.y : n2.w ) ), hiy4 = convert_float4( as_uchar4( rD.y < 0 ? n2.w : n4.y ) );
					const float4 loz4 = convert_float4( as_uchar4( rD.z < 0 ? n4.w : n3.y ) ), hiz4 = convert_float4( as_uchar4( rD.z < 0 ? n3.y : n4.w ) );
					{
					#ifdef SIMD_AABBTEST
						const float4 tminx4 = lox4 * idir4.xxxx + orig4.xxxx, tmaxx4 = hix4 * idir4.xxxx + orig4.xxxx;
						const float4 tminy4 = loy4 * idir4.yyyy + orig4.yyyy, tmaxy4 = hiy4 * idir4.yyyy + orig4.yyyy;
						const float4 tminz4 = loz4 * idir4.zzzz + orig4.zzzz, tmaxz4 = hiz4 * idir4.zzzz + orig4.zzzz;
						const float cmina = fmax( fmax( fmax( tminx4.x, tminy4.x ), tminz4.x ), 0 );
						const float cmaxa = fmin( fmin( fmin( tmaxx4.x, tmaxy4.x ), tmaxz4.x ), tmax );
						const float cminb = fmax( fmax( fmax( tminx4.y, tminy4.y ), tminz4.y ), 0 );
						const float cmaxb = fmin( fmin( fmin( tmaxx4.y, tmaxy4.y ), tmaxz4.y ), tmax );
						if (cmina <= cmaxa) UPDATE_HITMASK0;
						if (cminb <= cmaxb) UPDATE_HITMASK1;
					#else
						float tminx0 = _native_fma( lox4.x, idirx, origx ), tminx1 = _native_fma( lox4.y, idirx, origx );
						float tminy0 = _native_fma( loy4.x, idiry, origy ), tminy1 = _native_fma( loy4.y, idiry, origy );
						float tminz0 = _native_fma( loz4.x, idirz, origz ), tminz1 = _native_fma( loz4.y, idirz, origz );
						float tmaxx0 = _native_fma( hix4.x, idirx, origx ), tmaxx1 = _native_fma( hix4.y, idirx, origx );
						float tmaxy0 = _native_fma( hiy4.x, idiry, origy ), tmaxy1 = _native_fma( hiy4.y, idiry, origy );
						float tmaxz0 = _native_fma( hiz4.x, idirz, origz ), tmaxz1 = _native_fma( hiz4.y, idirz, origz );
						n0.x = fmax( fmax_fmax( tminx0, tminy0, tminz0 ), 0 );
						n0.y = fmin( fmin_fmin( tmaxx0, tmaxy0, tmaxz0 ), tmax );
						n1.x = fmax( fmax_fmax( tminx1, tminy1, tminz1 ), 0 );
						n1.y = fmin( fmin_fmin( tmaxx1, tmaxy1, tmaxz1 ), tmax );
						if (n0.x <= n0.y) UPDATE_HITMASK0;
						if (n1.x <= n1.y) UPDATE_HITMASK1;
					#endif
					#ifdef SIMD_AABBTEST
						const float cminc = fmax( fmax( fmax( tminx4.z, tminy4.z ), tminz4.z ), 0 );
						const float cmaxc = fmin( fmin( fmin( tmaxx4.z, tmaxy4.z ), tmaxz4.z ), tmax );
						const float cmind = fmax( fmax( fmax( tminx4.w, tminy4.w ), tminz4.w ), 0 );
						const float cmaxd = fmin( fmin( fmin( tmaxx4.w, tmaxy4.w ), tmaxz4.w ), tmax );
						if (cminc <= cmaxc) UPDATE_HITMASK2;
						if (cmind <= cmaxd) UPDATE_HITMASK3;
					#else
						tminx0 = _native_fma( lox4.z, idirx, origx ), tminx1 = _native_fma( lox4.w, idirx, origx );
						tminy0 = _native_fma( loy4.z, idiry, origy ), tminy1 = _native_fma( loy4.w, idiry, origy );
						tminz0 = _native_fma( loz4.z, idirz, origz ), tminz1 = _native_fma( loz4.w, idirz, origz );
						tmaxx0 = _native_fma( hix4.z, idirx, origx ), tmaxx1 = _native_fma( hix4.w, idirx, origx );
						tmaxy0 = _native_fma( hiy4.z, idiry, origy ), tmaxy1 = _native_fma( hiy4.w, idiry, origy );
						tmaxz0 = _native_fma( hiz4.z, idirz, origz ), tmaxz1 = _native_fma( hiz4.w, idirz, origz );
						n0.x = fmax( fmax_fmax( tminx0, tminy0, tminz0 ), 0 );
						n0.y = fmin( fmin_fmin( tmaxx0, tmaxy0, tmaxz0 ), tmax );
						n1.x = fmax( fmax_fmax( tminx1, tminy1, tminz1 ), 0 );
						n1.y = fmin( fmin_fmin( tmaxx1, tmaxy1, tmaxz1 ), tmax );
						if (n0.x <= n0.y) UPDATE_HITMASK2;
						if (n1.x <= n1.y) UPDATE_HITMASK3;
					#endif
					}
				}
				ngroup.y = (hitmask & 0xFF000000) | (as_uint( n0.w ) >> 24), tgroup.y = hitmask & 0x00FFFFFF;
			}
		}
		else tgroup = ngroup, ngroup = (uint2)(0);
		while (tgroup.y != 0)
		{
		#ifdef CWBVH_COMPRESSED_TRIS
			// Fast intersection of triangle data for the algorithm in:
			// "Fast Ray-Triangle Intersections by Coordinate Transformation"
			// Baldwin & Weber, 2016.
			const unsigned triangleIndex = __bfind( tgroup.y ), triAddr = tgroup.x + triangleIndex * 4;
			const float4 T2 = cwbvhTris[triAddr + 2];
			const float transS = T2.x * O.x + T2.y * O.y + T2.z * O.z + T2.w;
			const float transD = T2.x * D.x + T2.y * D.y + T2.z * D.z;
			const float d = -transS / transD;
			if (d > 0 && d < tmax)
			{
				const float4 T0 = cwbvhTris[triAddr + 0];
				const float4 T1 = cwbvhTris[triAddr + 1];
			#ifdef SIMD_AABBTEST
				const float4 I = O4 + d * D4;
			#else
				const float3 I = O + d * D;
			#endif
				const float u = T0.x * I.x + T0.y * I.y + T0.z * I.z + T0.w;
				const float v = T1.x * I.x + T1.y * I.y + T1.z * I.z + T1.w;
				const bool hit = u >= 0 && v >= 0 && u + v < 1;
				if (hit) uv = (float2)( u, v ), tmax = d, hitAddr = as_uint( cwbvhTris[triAddr + 3].w );
			}
		#else
			// Möller-Trumbore intersection; triangles are stored as 3x16 bytes,
			// with the original primitive index in the (otherwise unused) w 
			// component of vertex 0.
			const int triangleIndex = __bfind( tgroup.y ), triAddr = tgroup.x + triangleIndex * 3;
			const float3 v0 = cwbvhTris[triAddr].xyz;
			const float3 e1 = cwbvhTris[triAddr + 1].xyz - v0;
			const float3 e2 = cwbvhTris[triAddr + 2].xyz - v0;
		#ifdef SIMD_AABBTEST
			const float3 r = cross( D4.xyz, e2 );
		#else
			const float3 r = cross( D, e2 );
		#endif
			const float a = dot( e1, r );
			if (fabs( a ) > 0.0000001f)
			{
				const float f = 1 / a;
			#ifdef SIMD_AABBTEST
				const float3 s = O4.xyz - v0;
			#else
				const float3 s = O - v0;
			#endif
				const float u = f * dot( s, r );
				if (u >= 0 && u <= 1)
				{
					const float3 q = cross( s, e1 );
				#ifdef SIMD_AABBTEST
					const float v = f * dot( D4.xyz, q );
				#else
					const float v = f * dot( D, q );
				#endif
					if (v >= 0 && u + v <= 1)
					{
						const float d = f * dot( e2, q );
						if (d > 0.0f && d < tmax)
						{
							uv = (float2)(u, v), tmax = d;
							hitAddr = as_uint( cwbvhTris[triAddr].w );
						}
					}
				}
			}
		#endif
			tgroup.y -= 1 << triangleIndex;
		}
		if (ngroup.y <= 0x00FFFFFF)
		{
			if (stackPtr > 0) { STACK_POP( ngroup ); }
			else
			{
				hit = (float4)(tmax, uv.x, uv.y, as_float( hitAddr ));
				break;
			}
		}
	} while (true);
	return hit;
}

void kernel batch_cwbvh( global const float4* cwbvhNodes, global const float4* cwbvhTris, global struct Ray* rayData )
{
	// initialize ray
	const unsigned threadId = get_global_id( 0 );
	const float4 O4 = rayData[threadId].O;
	const float4 D4 = rayData[threadId].D;
	const float4 rD4 = rayData[threadId].rD;
	float4 hit = traverse_cwbvh( cwbvhNodes, cwbvhTris, O4.xyz, D4.xyz, rD4.xyz, 1e30f );
	rayData[threadId].hit = hit;
}