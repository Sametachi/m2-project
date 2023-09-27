#pragma once
#include "vector.h"

enum ViewState
{
	VS_INSIDE,   // completely inside the frustum.
	VS_PARTIAL,  // partially inside and partially outside the frustum.
	VS_OUTSIDE   // completely outside the frustum
};

class Frustum
{
public:
	Frustum(bool m_b_using_sphere, const D3DXVECTOR3& m_v3_center, float m_f_radius) : m_bUsingSphere(m_b_using_sphere), m_v3Center(m_v3_center), m_fRadius(m_f_radius) {}
	Frustum() : m_bUsingSphere(false), m_v3Center(0.f, 0.f, 0.f), m_fRadius(0.0f) {}

	void BuildViewFrustum(D3DXMATRIX& mat);
	void BuildViewFrustum2(D3DXMATRIX& mat, float fNear, float fFar, float fFov, float fAspect, const D3DXVECTOR3& vCamera, const D3DXVECTOR3& vLook);
	ViewState ViewVolumeTest(const Vector3d& c_v3Center, const float c_fRadius) const;

private:
	bool m_bUsingSphere;
	D3DXVECTOR3 m_v3Center;
	float m_fRadius;
	D3DXPLANE m_plane[6];
};