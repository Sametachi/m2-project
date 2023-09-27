# from __future__ import print_function #it must be the first line
import sys
import os
import WorldEditor as we
import dbg

### defined methods
MAPBASE = 25600
SCENE_NAMES = ("Map", "Object", "Effect", "Fly", "Max")
PROPERTY_NAMES = ("None", "Tree", "Building", "Effect", "Ambience", "DungeonBlock")
PREFIX_ENV = "d:/ymir work/environment/"

def SceneName(v):
	return SCENE_NAMES[v]

def PropertyName(v):
	return PROPERTY_NAMES[v]

def RecalculateHeightPixel(x, y):
	h=we.GetHeightPixel(x, y)
	print("x {} y {} height {}".format(x, y, h))
	we.DrawHeightPixel(x, y, h)

def AdvanceHeightPixel(x, y, advance):
	h=we.GetHeightPixel(x, y)
	print("x {} y {} height {}".format(x, y, h))
	we.DrawHeightPixel(x, y, h+advance)

def AdjustBaseCoordinates(x, y):
	# y+=2 # trigger adjust
	isWrong = (x % MAPBASE) or (y % MAPBASE)
	if isWrong:
		x2 = x - (x % MAPBASE)
		y2 = y - (y % MAPBASE)
		dbg.LogBox("The Base coordinates are WRONG:\\nCHANGE X {} Y {}\\n          TO X {} Y {}".format(x, y, x2, y2))
		return x2, y2
	return x, y

def GetRealTargetPosition():
	x,y = we.GetTargetPosition()
	x = int32_t(x / 100)
	y = -int32_t(y / 100)
	return x,y

def MoveToRealTargetPosition(x, y):
	x = x*100.0
	y = -(y*100.0)
	x0,y0 = we.GetTargetPosition()
	we.UpdateTargetPosition(-x0, -y0)
	we.UpdateTargetPosition(x, y)

### TEST GENERAL
if False:
	pass
	print("IsMapReady", we.IsMapReady())
	print("Env: "+PREFIX_ENV+"{}".format(we.GetEnvironmentDataName()))
	print("Map Size {}x{}".format(*we.GetTerrainCount()))
	print("Base x {} y {}".format(*we.GetBaseXY()))
	# print("Adjusted Base x {} y {}".format(*AdjustBaseCoordinates(*we.GetBaseXY())))
	# we.UpdateTargetPosition(100, 100)
	print("Camera x {} y {}".format(*we.GetTargetPosition()))
	print("Real Camera x {} y {}".format(*GetRealTargetPosition()))
	MoveToRealTargetPosition(112, 112)

### TEST PROPERTY
if True:
	pass
	# print(we.GetObjectList(0, 0))
	# print(we.GetPropertyType(we.PROPERTY_TYPE_EFFECT))
	# print(we.GetPropertyExtension("Effect"))
	# print(we.GetPropertyExtension(PropertyName(we.PROPERTY_TYPE_EFFECT)))

### TEST DRAW HEIGHT
if False:
	pass
	RecalculateHeightPixel(1, 1)
	RecalculateHeightPixel(113, 113)
	AdvanceHeightPixel(113, 113, 500) #226,226
	AdvanceHeightPixel(130, 130, -2500) #230,230
	AdvanceHeightPixel(130, 130, 2500) #230,230
	we.DrawHeightPixel(113, 113, 500)

	we.DrawHeightPixel(1, 1, 500)
	we.DrawHeightPixel(1, 2, 500)
	we.DrawHeightPixel(1, 3, 500)
	we.DrawHeightPixel(1, 4, 500)

### TEST GET SCENE
if False:
	pass
	dbg.LogBox("Current scene: [{}] {}".format(we.GetSceneType(), SceneName(we.GetSceneType())))
	print(we.SCENE_MAP)
	print(we.SCENE_OBJECT)
	print(we.SCENE_EFFECT)
	print(we.SCENE_FLY)

### TEST BUILTIN
if False:
	pass
	dbg.LogBox(repr(sys.version_info))

	with open("kek", "wb"):
		pass

	print("kek2")

	print(sys.path)

	import dbg
	dbg.LogBox("kek")
	dbg.Trace("oooo1\\n")
	dbg.Tracen("oooo2")
	print("kek3")

	for i in (1,2,3,4):
		if i==1:
			print("kek4")

	if os.path.exists("kek"):
		print("kek5")
