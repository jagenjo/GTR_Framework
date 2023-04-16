#pragma once

#include "../core/core.h"
#include "../gfx/texture.h" //FloatImage

class Image;

namespace GFX {

	class Texture;
	class Mesh;

	#define GPU_FRAME_HISTORY_SIZE 256
	extern long gpu_frame_microseconds;
	extern long gpu_frame_microseconds_history[GPU_FRAME_HISTORY_SIZE];

	//check opengl errors
	bool checkGLErrors();

	void startGPULabel(const char* text);
	void endGPULabel();

	std::string getGPUStats();
	void drawGrid();
	bool drawText(float x, float y, std::string text, Vector4f c, float scale);
	bool drawText3D(Vector3f pos, std::string text, Vector4f c, float scale);

	void drawTexture2D(Texture* tex, vec4 pos);
	void drawPoints(std::vector<Vector3f> points, Vector4f color, int size);

	void displaceMesh(Mesh* mesh, ::Image* heightmap, float altitude);

	class GPUQuery
	{
	public:
		GLuint type;
		GLuint handler;
		GLuint64 value;
		bool waiting;
		GPUQuery(GLuint type);
		~GPUQuery();
		void start();
		void finish();
		bool isReady();
	};

};


//GPU state representation from BGFX

/*
//Color RGB/alpha/depth write. When it's not specified write will be disabled.

#define GFX_STATE_WRITE_R                        UINT64_C(0x0000000000000001) //!< Enable R write.
#define GFX_STATE_WRITE_G                        UINT64_C(0x0000000000000002) //!< Enable G write.
#define GFX_STATE_WRITE_B                        UINT64_C(0x0000000000000004) //!< Enable B write.
#define GFX_STATE_WRITE_A                        UINT64_C(0x0000000000000008) //!< Enable alpha write.
#define GFX_STATE_WRITE_Z                        UINT64_C(0x0000004000000000) //!< Enable depth write.
 /// Enable RGB write.
#define GFX_STATE_WRITE_RGB (0 \
	| GFX_STATE_WRITE_R \
	| GFX_STATE_WRITE_G \
	| GFX_STATE_WRITE_B \
	)

/// Write all channels mask.
#define GFX_STATE_WRITE_MASK (0 \
	| GFX_STATE_WRITE_RGB \
	| GFX_STATE_WRITE_A \
	| GFX_STATE_WRITE_Z \
	)


//Depth test state. When `GFX_STATE_DEPTH_` is not specified depth test will be disabled.
#define GFX_STATE_DEPTH_TEST_LESS                UINT64_C(0x0000000000000010) //!< Enable depth test, less.
#define GFX_STATE_DEPTH_TEST_LEQUAL              UINT64_C(0x0000000000000020) //!< Enable depth test, less or equal.
#define GFX_STATE_DEPTH_TEST_EQUAL               UINT64_C(0x0000000000000030) //!< Enable depth test, equal.
#define GFX_STATE_DEPTH_TEST_GEQUAL              UINT64_C(0x0000000000000040) //!< Enable depth test, greater or equal.
#define GFX_STATE_DEPTH_TEST_GREATER             UINT64_C(0x0000000000000050) //!< Enable depth test, greater.
#define GFX_STATE_DEPTH_TEST_NOTEQUAL            UINT64_C(0x0000000000000060) //!< Enable depth test, not equal.
#define GFX_STATE_DEPTH_TEST_NEVER               UINT64_C(0x0000000000000070) //!< Enable depth test, never.
#define GFX_STATE_DEPTH_TEST_ALWAYS              UINT64_C(0x0000000000000080) //!< Enable depth test, always.
#define GFX_STATE_DEPTH_TEST_SHIFT               4                            //!< Depth test state bit shift
#define GFX_STATE_DEPTH_TEST_MASK                UINT64_C(0x00000000000000f0) //!< Depth test state bit mask

//Use GFX_STATE_BLEND_FUNC(_src, _dst) or GFX_STATE_BLEND_FUNC_SEPARATE(_srcRGB, _dstRGB, _srcA, _dstA)
//helper macros.

#define GFX_STATE_BLEND_ZERO                     UINT64_C(0x0000000000001000) //!< 0, 0, 0, 0
#define GFX_STATE_BLEND_ONE                      UINT64_C(0x0000000000002000) //!< 1, 1, 1, 1
#define GFX_STATE_BLEND_SRC_COLOR                UINT64_C(0x0000000000003000) //!< Rs, Gs, Bs, As
#define GFX_STATE_BLEND_INV_SRC_COLOR            UINT64_C(0x0000000000004000) //!< 1-Rs, 1-Gs, 1-Bs, 1-As
#define GFX_STATE_BLEND_SRC_ALPHA                UINT64_C(0x0000000000005000) //!< As, As, As, As
#define GFX_STATE_BLEND_INV_SRC_ALPHA            UINT64_C(0x0000000000006000) //!< 1-As, 1-As, 1-As, 1-As
#define GFX_STATE_BLEND_DST_ALPHA                UINT64_C(0x0000000000007000) //!< Ad, Ad, Ad, Ad
#define GFX_STATE_BLEND_INV_DST_ALPHA            UINT64_C(0x0000000000008000) //!< 1-Ad, 1-Ad, 1-Ad ,1-Ad
#define GFX_STATE_BLEND_DST_COLOR                UINT64_C(0x0000000000009000) //!< Rd, Gd, Bd, Ad
#define GFX_STATE_BLEND_INV_DST_COLOR            UINT64_C(0x000000000000a000) //!< 1-Rd, 1-Gd, 1-Bd, 1-Ad
#define GFX_STATE_BLEND_SRC_ALPHA_SAT            UINT64_C(0x000000000000b000) //!< f, f, f, 1; f = min(As, 1-Ad)
#define GFX_STATE_BLEND_FACTOR                   UINT64_C(0x000000000000c000) //!< Blend factor
#define GFX_STATE_BLEND_INV_FACTOR               UINT64_C(0x000000000000d000) //!< 1-Blend factor
#define GFX_STATE_BLEND_SHIFT                    12                           //!< Blend state bit shift
#define GFX_STATE_BLEND_MASK                     UINT64_C(0x000000000ffff000) //!< Blend state bit mask

//Use GFX_STATE_BLEND_EQUATION(_equation) or GFX_STATE_BLEND_EQUATION_SEPARATE(_equationRGB, _equationA)
//helper macros.
#define GFX_STATE_BLEND_EQUATION_ADD             UINT64_C(0x0000000000000000) //!< Blend add: src + dst.
#define GFX_STATE_BLEND_EQUATION_SUB             UINT64_C(0x0000000010000000) //!< Blend subtract: src - dst.
#define GFX_STATE_BLEND_EQUATION_REVSUB          UINT64_C(0x0000000020000000) //!< Blend reverse subtract: dst - src.
#define GFX_STATE_BLEND_EQUATION_MIN             UINT64_C(0x0000000030000000) //!< Blend min: min(src, dst).
#define GFX_STATE_BLEND_EQUATION_MAX             UINT64_C(0x0000000040000000) //!< Blend max: max(src, dst).
#define GFX_STATE_BLEND_EQUATION_SHIFT           28                           //!< Blend equation bit shift
#define GFX_STATE_BLEND_EQUATION_MASK            UINT64_C(0x00000003f0000000) //!< Blend equation bit mask

//Cull state. When `GFX_STATE_CULL_*` is not specified culling will be disabled.
#define GFX_STATE_CULL_CW                        UINT64_C(0x0000001000000000) //!< Cull clockwise triangles.
#define GFX_STATE_CULL_CCW                       UINT64_C(0x0000002000000000) //!< Cull counter-clockwise triangles.
#define GFX_STATE_CULL_SHIFT                     36                           //!< Culling mode bit shift
#define GFX_STATE_CULL_MASK                      UINT64_C(0x0000003000000000) //!< Culling mode bit mask

//Alpha reference value.

#define GFX_STATE_ALPHA_REF_SHIFT                40                           //!< Alpha reference bit shift
#define GFX_STATE_ALPHA_REF_MASK                 UINT64_C(0x0000ff0000000000) //!< Alpha reference bit mask
#define GFX_STATE_ALPHA_REF(v) ( ( (uint64_t)(v)<<GFX_STATE_ALPHA_REF_SHIFT )&GFX_STATE_ALPHA_REF_MASK)

#define GFX_STATE_PT_TRISTRIP                    UINT64_C(0x0001000000000000) //!< Tristrip.
#define GFX_STATE_PT_LINES                       UINT64_C(0x0002000000000000) //!< Lines.
#define GFX_STATE_PT_LINESTRIP                   UINT64_C(0x0003000000000000) //!< Line strip.
#define GFX_STATE_PT_POINTS                      UINT64_C(0x0004000000000000) //!< Points.
#define GFX_STATE_PT_SHIFT                       48                           //!< Primitive type bit shift
#define GFX_STATE_PT_MASK                        UINT64_C(0x0007000000000000) //!< Primitive type bit mask

//Point size value.
#define GFX_STATE_POINT_SIZE_SHIFT               52                           //!< Point size bit shift
#define GFX_STATE_POINT_SIZE_MASK                UINT64_C(0x00f0000000000000) //!< Point size bit mask
#define GFX_STATE_POINT_SIZE(v) ( ( (uint64_t)(v)<<GFX_STATE_POINT_SIZE_SHIFT )&GFX_STATE_POINT_SIZE_MASK)

//Enable MSAA write when writing into MSAA frame buffer.
//This flag is ignored when not writing into MSAA frame buffer.
#define GFX_STATE_MSAA                           UINT64_C(0x0100000000000000) //!< Enable MSAA rasterization.
#define GFX_STATE_LINEAA                         UINT64_C(0x0200000000000000) //!< Enable line AA rasterization.
#define GFX_STATE_CONSERVATIVE_RASTER            UINT64_C(0x0400000000000000) //!< Enable conservative rasterization.
#define GFX_STATE_NONE                           UINT64_C(0x0000000000000000) //!< No state.
#define GFX_STATE_FRONT_CCW                      UINT64_C(0x0000008000000000) //!< Front counter-clockwise (default is clockwise).
#define GFX_STATE_BLEND_INDEPENDENT              UINT64_C(0x0000000400000000) //!< Enable blend independent.
#define GFX_STATE_BLEND_ALPHA_TO_COVERAGE        UINT64_C(0x0000000800000000) //!< Enable alpha to coverage.
       /// Default state is write to RGB, alpha, and depth with depth test less enabled, with clockwise
       /// culling and MSAA (when writing into MSAA frame buffer, otherwise this flag is ignored).
#define GFX_STATE_DEFAULT (0 \
	| GFX_STATE_WRITE_RGB \
	| GFX_STATE_WRITE_A \
	| GFX_STATE_WRITE_Z \
	| GFX_STATE_DEPTH_TEST_LESS \
	| GFX_STATE_CULL_CW \
	| GFX_STATE_MSAA \
	)

#define GFX_STATE_MASK                           UINT64_C(0xffffffffffffffff) //!< State bit mask

void setGPUState(uint64 state);
*/
