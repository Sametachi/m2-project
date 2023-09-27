#include "StdAfx.h"
#include "Granny3D.h"

granny_data_type_definition MyFileInfoType[] =
{
	{ GrannyUInt32Member, "Sentinel" },
	{ GrannyReferenceMember, "FileInfo", GrannyFileInfoType },
	{ GrannyEndMember }
};

Granny3D::~Granny3D()
{
	for (auto& elem : m_meshDeformers)
        GrannyFreeMeshDeformer(elem.second);
}

granny_mesh_deformer* Granny3D::GetMeshDeformerByVertexType(granny_data_type_definition* type)
{
    if (auto it = m_meshDeformers.find(type); it != m_meshDeformers.end())
        return it->second;

    RegisterVertexType(type);

    if (auto it = m_meshDeformers.find(type); it != m_meshDeformers.end())
        return it->second;

    return nullptr;
}

void Granny3D::RegisterVertexType(granny_data_type_definition* type)
{
    if(m_meshDeformers.find(type) != m_meshDeformers.end())
        return;

    const auto meshDeformer = GrannyNewMeshDeformer(type, m_outputVertexType, GrannyDeformPositionNormal, GrannyDontAllowUncopiedTail);
    m_meshDeformers.emplace(type, meshDeformer);
}

/* Granny official Timer */
void Granny3D::InitiateClocks()
{
    GrannyGetSystemSeconds(&BootClock);
    GrannyGetSystemSeconds(&LastClock);
}

void Granny3D::UpdateClock()
{
    GrannyGetSystemSeconds(&CurrentClock);

    SecondsElapsed = GrannyGetSecondsElapsed(&LastClock, &CurrentClock);
    LastClock = CurrentClock;
};

granny_real32 Granny3D::Granny3DGetElapsedSeconds()
{
    return SecondsElapsed;
}

granny_real32 Granny3D::Granny3DTotalSecondsElapsed()
{
    return GrannyGetSecondsElapsed(&BootClock, &CurrentClock);
}

granny_control* Granny3D::PlayAnimation(float startTime, granny_animation* ani, granny_model_instance* mi)
{
	auto builder = GrannyBeginControlledAnimation(startTime, ani);
	if (!builder)
		return nullptr;

	granny_int32 trackGroupIndex;
	if (!GrannyFindTrackGroupForModel(ani, GrannyGetSourceModel(mi)->Name, &trackGroupIndex))
		// Animation cannot be played on this model - ignore it
		return nullptr;

	GrannySetTrackGroupLOD(builder, trackGroupIndex, true, 1.0f); // TODO03: Check the documentation (Lods)
	GrannySetTrackGroupTarget(builder, trackGroupIndex, mi);
	return GrannyEndControlledAnimation(builder);
}

Granny3D::my_file_info* Granny3D::GetMyFileInfo(granny_file* File)
{
	granny_variant Root;
	GrannyGetDataTreeFromFile(File, &Root);

	if (File->Header->TypeTag == kCurrentTag)
	{
		return (my_file_info*)Root.Object;
	}
	else
	{
		if (File->ConversionBuffer == nullptr)
		{
			// Log Warning about conversion operation
			File->ConversionBuffer = GrannyConvertTree(Root.Type, Root.Object, MyFileInfoType, nullptr, nullptr);
		}
		return (my_file_info*)File->ConversionBuffer;
	}
}
