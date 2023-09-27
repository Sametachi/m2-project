#pragma once
#include <granny.h>
#include <Basic/robin_hood.hpp>
#include <Basic/Singleton.h>

class Granny3D : public Singleton<Granny3D>
{
public:
	struct my_file_info
	{
		granny_uint32 Sentinel;
		granny_file_info* FileInfo;
	};

	// Explicitly declare them as uint32s here to avoid compiler warnings due to |GrannyFirstGRNStandardTag| being converted to signed int.
	const granny_uint32 kGrnCurrent = GrannyCurrentGRNStandardTag;
	const granny_uint32 kGrnStart = GrannyFirstGRNStandardTag;

	// (current - first) gives us the current "revision", which is necessary, since our tag needs to change when the underlying GrannyFileInfoType changes.
	const granny_uint32 kCurrentTag = kGrnCurrent - kGrnStart + 0x70000000;
	const granny_uint32 kSentinel = 0xdeadbeef;

public:
	Granny3D() = default;
	~Granny3D();

	granny_mesh_deformer* GetMeshDeformerByVertexType(granny_data_type_definition* type);

	/* Register a new VertexType and create a mesh deformer for it */
	void RegisterVertexType(granny_data_type_definition* type);

	/* Granny default Animation Controller */
	granny_control* PlayAnimation(float startTime, granny_animation* ani, granny_model_instance* mi);

	/* Getting the File info */
	my_file_info* GetMyFileInfo(granny_file* File);
	
private:
	robin_hood::unordered_map<granny_data_type_definition*, granny_mesh_deformer*> m_meshDeformers;
	const granny_data_type_definition* m_outputVertexType = GrannyPNT332VertexType;


/* Granny official Timer */
public:
	void InitiateClocks();
	void UpdateClock();
	granny_real32 Granny3DGetElapsedSeconds();
	granny_real32 Granny3DTotalSecondsElapsed();

	granny_system_clock BootClock;
	granny_system_clock LastClock;
	granny_system_clock CurrentClock;
	granny_real32 SecondsElapsed;
};