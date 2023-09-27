#pragma once
/*
	Motion FPS is actually SET each time when we update the models,
	thus i had to remove the Update from some files which allowed us to have the same
	granny motion experience as with the LOD Controller.
*/

class CGrannyMotion
{
public:
	CGrannyMotion();
	virtual ~CGrannyMotion();

	bool IsEmpty();

	void Destroy();
	bool BindGrannyAnimation(granny_animation* pgrnAni);

	granny_animation* GetGrannyAnimationPointer() const;

	const char* GetName() const;
	float GetDuration() const;
	void GetTextTrack(const char* c_szTextTrackName, int32_t* pCount, float* pArray) const;
	void SetFromFilename(const std::string& fromFilename)
	{
		m_fromFilename = fromFilename;
	}

	const std::string& GetFromFilename() const
	{
		return m_fromFilename;
	}

protected:
	void Initialize();

	std::string m_fromFilename;
	granny_animation* m_pgrnAni;
};
