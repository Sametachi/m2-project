#pragma once
#include <granny.h>

class CGraphicImage;
struct SMaterialData
{
	CGraphicImage* pImage;
	float fSpecularPower;
	bool isSpecularEnable;
	uint8_t bSphereMapIndex;
};

struct granny_pnt3322_vertex
{
	granny_real32 Position[3];
	granny_real32 Normal[3];
	granny_real32 UV0[2];
	granny_real32 UV1[2];
};

extern granny_data_type_definition GrannyPNT3322VertexType[5];
extern granny_data_type_definition GrannyPNT33222VertexType[6];
std::string GetGrannyTypeString(const granny_data_type_definition* type);

// mode- Source model, which will be scanned for meshes
// type - Output vertex type. Might differ from source mesh vertex type.
// fvf - Flexible vertex format (D3D)
bool FindBestRigidVertexFormat(const granny_model* model, const granny_data_type_definition*& type, uint32_t& fvf);
