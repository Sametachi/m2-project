#pragma once
#include <Basic/Singleton.h>

class CGraphicTextInstance;
class CGraphicMarkInstance;
class CGraphicObjectInstance;

template <size_t N>
class TextInstanceList
{
public:
	// Automatically calculate the positions of each Text Instance while rendering it!
	enum NameTextInstanceOrder
	{
		kLevel,
		kTitle,
		kName,
	};

	TextInstanceList(CGraphicTextInstance* instances[N]);

	int32_t GetWidth() const { return m_width; }
	int32_t GetHeight() const { return m_height; }

	void Render(int32_t x, int32_t y, int32_t z);

private:
	struct Entry
	{
		Entry() : delta(0.0f), textInstance(nullptr) {}

		int32_t delta;
		CGraphicTextInstance* textInstance;
	};

	Entry m_list[N];
	int32_t m_width;
	int32_t m_height;
};

class CPythonTextTail : public Singleton<CPythonTextTail>
{
public:
	struct TextTail
	{
		TextTail(uint32_t vid, const char* text, CGraphicObjectInstance* owner, float height, const D3DXCOLOR& color);
		~TextTail();

		//CGraphicImageInstance* pGuildCrownInstance;
		//CGraphicImageInstance* pLanguageImageInstance;
		std::unique_ptr<CGraphicTextInstance> pTextInstance;
		std::unique_ptr<CGraphicTextInstance> pOwnerTextInstance;

		std::unique_ptr<CGraphicMarkInstance> pMarkInstance;
		std::unique_ptr<CGraphicTextInstance> pGuildNameTextInstance;
		//std::unique_ptr<CGraphicTextInstance> pAIFlagTextInstance;
		std::unique_ptr<CGraphicTextInstance> pTitleTextInstance;
		std::unique_ptr<CGraphicTextInstance> pLevelTextInstance;

		CGraphicObjectInstance* pOwner;

		uint32_t dwVirtualID;

		int32_t x, y, z;
		float fDistanceFromPlayer;
		D3DXCOLOR Color;
		bool bNameFlag;
		bool	isPc;

		int32_t xStart, yStart;
		int32_t xEnd, yEnd;
		uint32_t LivingTime;
		float fHeight;
	};

	typedef std::unordered_map<uint32_t, std::unique_ptr<TextTail>> TTextTailMap;

	CPythonTextTail(void);
	virtual ~CPythonTextTail(void) = default;

	void DestroyEveryTextTail();
	//void Initialize();
	//void Destroy();
	void Clear();

	void UpdateAllTextTail();
	void UpdateShowingTextTail();
	void Render();

	void ArrangeTextTail();
	void HideAllTextTail();
	void ShowAllNPCTextTail();
	void ShowAllMonsterTextTail();
	void ShowAllItemTextTail();
	void ShowAllPCTextTail();
	//void ShowAllPrivateShopTextTail();

	void ShowAllTextTail();
	void ShowCharacterTextTail(uint32_t VirtualID);
	void ShowPlayerTextTail(uint32_t VirtualID);
	void ShowMonsterTextTail(uint32_t VirtualID);
	void ShowNpcTextTail(uint32_t VirtualID);
	void ShowItemTextTail(uint32_t VirtualID);

	void RegisterCharacterTextTail(uint32_t dwGuildID, uint32_t dwVirtualID, const D3DXCOLOR& c_rColor, float fAddHeight = 10.0f);
	void RegisterItemTextTail(uint32_t VirtualID, const char* c_szText, CGraphicObjectInstance* pOwner);
	void RegisterChatTail(uint32_t VirtualID, const char* c_szChat);
	void RegisterInfoTail(uint32_t VirtualID, const char* c_szChat);
	void SetCharacterTextTailColor(uint32_t VirtualID, const D3DXCOLOR& c_rColor);
	void UpdateCharacterName(uint32_t VirtualID, const std::string& name);
	void SetItemTextTailOwner(uint32_t dwVID, const char* c_szName);
	void DeleteCharacterTextTail(uint32_t VirtualID);
	void DeleteItemTextTail(uint32_t VirtualID);

	int32_t Pick(int32_t ixMouse, int32_t iyMouse);
	void SelectItemName(uint32_t dwVirtualID);

	bool GetTextTailPosition(uint32_t dwVID, float* px, float* py, float* pz);
	bool IsChatTextTail(uint32_t dwVID);

	void EnablePKTitle(bool bFlag);
	void AttachTitle(uint32_t dwVID, const char* c_szName, const D3DXCOLOR& c_rColor);
	void DetachTitle(uint32_t dwVID);

	void AttachLevel(uint32_t dwVID, const char* c_szText, uint32_t gColor1, uint32_t gColor2);
	void DetachLevel(uint32_t dwVID);

	void ChangeFontType(std::string fontType);

	//void AttachLanguageImage(uint32_t dwVID, uint8_t bLanguage);

	//void ShowPrivateShopTextTail(uint32_t dwVirtualID);
	//void RegisterPrivateShopTextTail(uint32_t dwVirtualID);
	//void DeletePrivateShopTextTail(uint32_t dwVirtualID);

protected:
	void UpdateTextTail(TextTail* pTextTail);
	void RenderTextTailBox(TextTail* pTextTail);
	void UpdateDistance(const TPixelPosition& c_rCenterPosition, TextTail* pTextTail);
	bool isIn(TextTail* pSource, TextTail* pTarget);


	TTextTailMap m_CharacterTextTailMap;
	TTextTailMap m_ItemTextTailMap;
	TTextTailMap m_ChatTailMap;
	//TTextTailMap m_PrivateShopTextTailMap;

	std::unordered_set<TextTail*> m_CharacterTextTailSet;
	std::unordered_set<TextTail*> m_ItemTextTailSet;
	//std::unordered_set<TextTail*> m_PrivateShopTextTailSet;
};