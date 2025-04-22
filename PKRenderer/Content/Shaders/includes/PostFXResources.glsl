#pragma once
#include "Common.glsl"

PK_DECLARE_SET_PASS uniform sampler3D pk_CC_LutTex;
PK_DECLARE_SET_PASS uniform sampler3D pk_Tonemap_LutTex;

#define FX_FEAT_VIGNETTE 0x1u
#define FX_FEAT_BLOOM 0x2u
#define FX_FEAT_TONEMAP 0x4u
#define FX_FEAT_FILMGRAIN 0x8u
#define FX_FEAT_COLORGRADING 0x10u
#define FX_FEAT_LUTCOLORGRADING 0x20u

#define FX_FEAT_DEBUG_GI_DIFF 0x40u
#define FX_FEAT_DEBUG_GI_SPEC 0x80u
#define FX_FEAT_DEBUG_GI_VX 0x100u
#define FX_FEAT_DEBUG_NORMAL 0x200u
#define FX_FEAT_DEBUG_ROUGHNESS 0x400u
#define FX_FEAT_DEBUG_LIGHT_TILES 0x800u
#define FX_FEAT_DEBUG_HALFSCREEN 0x1000u
#define FX_FEAT_DEBUG_ZOOM 0x2000u

#if defined(FX_APPLY_ALL)
#define IF_FX_FEATURE_ENABLED(feature)
#define IS_FX_FEATURE_ENABLED(feature)
#else
#define IF_FX_FEATURE_ENABLED(feature) if ((pk_PostEffectsFeatureMask & feature) != 0u)
#define IS_FX_FEATURE_ENABLED(feature) ((pk_PostEffectsFeatureMask & feature) != 0u)
#endif
