#include "StdAfx.h"
#include "Util.h"
#include <d3dx9tex.h>

granny_data_type_definition GrannyPNT3322VertexType[5] =
{
	{GrannyReal32Member, GrannyVertexPositionName, 0, 3},
	{GrannyReal32Member, GrannyVertexNormalName, 0, 3},
	{GrannyReal32Member, GrannyVertexTextureCoordinatesName"0", 0, 2},
	{GrannyReal32Member, GrannyVertexTextureCoordinatesName"1", 0, 2},
	{GrannyEndMember}
};

granny_data_type_definition GrannyPNT33222VertexType[6] =
{
	{GrannyReal32Member, GrannyVertexPositionName, 0, 3},
	{GrannyReal32Member, GrannyVertexNormalName, 0, 3},
	{GrannyReal32Member, GrannyVertexTextureCoordinatesName"0", 0, 2},
	{GrannyReal32Member, GrannyVertexTextureCoordinatesName"1", 0, 2},
	{GrannyReal32Member, GrannyVertexTextureCoordinatesName"2", 0, 2},

	{GrannyEndMember}
};

std::string GetGrannyTypeString(const granny_data_type_definition* type)
{
	assert(type && "Type is null");

	std::string s;
	for (; type->Type != GrannyEndMember; ++type) 
	{
		s.append(type->Name ? type->Name : "<no name>");
		s.append(1, '[');
		s.append(std::to_string(type->ArrayWidth));
		s.append(1, ']');
		s.append(1, ',');
	}

	return s;
}

void CheckVertexFormat(const granny_data_type_definition* type, uint32_t fvf)
{
	const auto fvfStride = D3DXGetFVFVertexSize(fvf);
	const auto grnStride = GrannyGetTotalObjectSize(type);

	assert(fvfStride == grnStride && "Granny vertex format doesn't match D3D FVF");
}

bool FindBestRigidVertexFormat(const granny_model* model, const granny_data_type_definition*& type, uint32_t& fvf)
{
	assert(model != nullptr && "Parameter check");

	// This is our default type - suitable for most models/meshes.
	type = GrannyPNT332VertexType;
	fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;

	for (int i = 0; i < model->MeshBindingCount; ++i) 
	{
		granny_mesh* mesh = model->MeshBindings[i].Mesh;

		// Ignore deforming meshes
		if (!GrannyMeshIsRigid(mesh))
			continue;

		granny_data_type_definition* currentType = GrannyGetMeshVertexType(mesh);
		if (!currentType) 
		{
			SysLog("Mesh {0} has no vertex type", mesh->Name);
			continue;
		}

		// This type has an additional texture position it is therefore preferred, since we'd otherwise lose this information.
		if (GrannyDataTypesAreEqual(GrannyPNT3322VertexType, currentType)) 
		{
			type = currentType;
			fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2;
			ConsoleLog("Mesh {0} uses PNT3322 -> Texture 2", mesh->Name);

			break;
		}

		if (GrannyDataTypesAreEqual(GrannyPNT33222VertexType, currentType)) 
		{
			type = currentType;
			fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX3;
			ConsoleLog("Mesh {0} uses PNT33222 -> Texture 3", mesh->Name);

			break;
		}

		// We only support these two types, so if this fails, we encountered an unknown one.
		if (!GrannyDataTypesAreEqual(GrannyPNT332VertexType, currentType)) 
		{
			ConsoleLog("Ignoring unknown vertex format type: {0}", GetGrannyTypeString(currentType).c_str());
		}
	}

	CheckVertexFormat(type, fvf);
	return true;
}
