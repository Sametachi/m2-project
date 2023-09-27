#pragma once
#include <Storm/StringFlags.hpp>
#include <boost/range/adaptor/reversed.hpp>

// Window Layers
enum WindowLayers
{
	kWindowLayerGame,
	kWindowLayerUiBottom,
	kWindowLayerUi,
	kWindowLayerTopMost,
	kWindowLayerCurtain,

	kWindowLayerMax,
};

// Window Types
enum WindowTypes
{
	kWindowNone,
	kWindowBar3D,
	kWindowExpandedImageBox,
	kWindowAniImageBox,
	kWindowSlot,
	kWindowGridSlot,
	kWindowDragBar,
	kWindowDragButton,
	kWindowTextLine,
	kWindowGrannyIllustrator,
};

enum WindowTransparentTypes
{
	WINDOW_TYPE_WINDOW,
	WINDOW_TYPE_EX_IMAGE,

	WINDOW_TYPE_MAX_NUM
};

// Flags
enum EFlags
{
	FLAG_MOVABLE = (1 << 0),
	FLAG_LIMIT = (1 << 1),
	FLAG_SNAP = (1 << 2),
	FLAG_DRAGABLE = (1 << 3),
	FLAG_ATTACH = (1 << 4),
	FLAG_RESTRICT_X = (1 << 5),
	FLAG_RESTRICT_Y = (1 << 6),
	FLAG_NOT_CAPTURE = (1 << 7),
	FLAG_FLOAT = (1 << 8),
	FLAG_NOT_PICK = (1 << 9),
	FLAG_IGNORE_SIZE = (1 << 10),
	FLAG_RTL = (1 << 11),
	FLAG_ALPHA_SENSITIVE = (1 << 12),
	FLAG_FOCUSABLE = (1 << 13),
	FLAG_ANIMATED_BOARD = (1 << 14),
	FLAG_COMPONENT = (1 << 15),
};

