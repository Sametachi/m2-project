#include "StdAfx.h"
#include "InstanceBase.h"
#include "resource.h"
#include "PythonTextTail.h"
#include "PythonCharacterManager.h"
#include "PythonGuild.h"
#include "Locale.h"
#include "MarkManager.h"
#include "PythonBackground.h"
#include "PythonSystem.h"
//#include "PythonPrivateShop.h"
#include <EterLib/GrpImage.h>
#include <EterLib/ResourceManager.h>
#include <EterLib/GrpFontManager.h>
#include <EterLib/Engine.h>

std::string textTailFontName = "BeaufortforLOL-Bold:14b";
// TEXTTAIL_LIVINGTIME_CONTROL
int32_t gs_TextTail_LivingTime = 5000;
int32_t TextTail_GetLivingTime()
{
	assert(gs_TextTail_LivingTime > 1000);
	return gs_TextTail_LivingTime;
}
void TextTail_SetLivingTime(int32_t livingTime)
{
	gs_TextTail_LivingTime = livingTime;
}
// END_OF_TEXTTAIL_LIVINGTIME_CONTROL

// Setting up the Paddings and shits
template <size_t N>
TextInstanceList<N>::TextInstanceList(CGraphicTextInstance* instances[N]) : m_width(0), m_height(0)
{
	for (size_t i = 0, j = 0; i < N; ++i)
	{
		CGraphicTextInstance* instance = instances[i];
		if (!instance)
			continue;

		if (kLevel != j)
			m_width += 4; // Padding between elements

		m_list[j].delta = m_width;
		m_list[j].textInstance = instance;

		m_width += instance->GetWidth();
		m_height = std::max<int32_t>(m_height, instance->GetHeight());
		++j;
	}
}

// We have to render the Glyphin text with the positions
template <size_t N>
void TextInstanceList<N>::Render(int32_t x, int32_t y, int32_t z)
{
	for (size_t i = 0; i < N; ++i)
	{
		if (!m_list[i].textInstance)
			continue;

		m_list[i].textInstance->Render(x + m_list[i].delta, y, z);
	}
}

const D3DXCOLOR c_TextTail_Player_Color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
const D3DXCOLOR c_TextTail_Monster_Color = D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f);
const D3DXCOLOR c_TextTail_Item_Color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
const D3DXCOLOR c_TextTail_Chat_Color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
const D3DXCOLOR c_TextTail_Info_Color = D3DXCOLOR(1.0f, 0.785f, 0.785f, 1.0f);
const D3DXCOLOR c_TextTail_Guild_Name_Color = 0xFFEFD3FF;
const D3DXCOLOR c_TextTail_Item_Red_Color = 0xFFFF6969;
const float c_TextTail_Name_Position = -10.0f;
const float c_fxMarkPosition = 1.5f;
const float c_fyGuildNamePosition = 15.0f;
const float c_fyMarkPosition = 15.0f + 11.0f;
bool bPKTitleEnable = true;

CPythonTextTail::TextTail::TextTail(uint32_t vid, const char* text, CGraphicObjectInstance* owner, float height, const D3DXCOLOR& color)
	: pTextInstance(new CGraphicTextInstance())
	, pMarkInstance(new CGraphicMarkInstance())
	//, pGuildCrownInstance(nullptr)
	//, pLanguageImageInstance(nullptr)
	, pOwner(owner)
	, dwVirtualID(vid)
	, x(-100), y(-100), z(0)
	, fDistanceFromPlayer(0.0f)
	, Color(color), bNameFlag(false), xStart(-2), yStart(-1), LivingTime(0), fHeight(height)
	, isPc(nullptr)
{
	pTextInstance->SetTextPointer(Engine::GetFontManager().LoadFont(textTailFontName));
	pTextInstance->SetValue(text);
	pTextInstance->SetColor(color.r, color.g, color.b);
	pTextInstance->Update();

	xEnd = pTextInstance->GetWidth() + 2;
	yEnd = pTextInstance->GetHeight() + 1;
}

CPythonTextTail::TextTail::~TextTail()
{
	//if (pGuildCrownInstance)
//		CGraphicImageInstance::Delete(pGuildCrownInstance);

	//if (pLanguageImageInstance)
		//CGraphicImageInstance::Delete(pLanguageImageInstance);
}

// This will update every existing Tail, even if it's not visible!
void CPythonTextTail::UpdateAllTextTail()
{
	CInstanceBase* pInstance = CPythonCharacterManager::GetInstance()->GetMainInstancePtr();
	if (pInstance)
	{
		TPixelPosition pixelPos;
		pInstance->NEW_GetPixelPosition(&pixelPos);

		TTextTailMap::iterator itorMap;

		for (itorMap = m_CharacterTextTailMap.begin(); itorMap != m_CharacterTextTailMap.end(); ++itorMap)
			UpdateDistance(pixelPos, itorMap->second.get());

		for (itorMap = m_ItemTextTailMap.begin(); itorMap != m_ItemTextTailMap.end(); ++itorMap)
			UpdateDistance(pixelPos, itorMap->second.get());

		for (auto& itorChat : m_ChatTailMap)
		{
			UpdateDistance(pixelPos, itorChat.second.get());
			if (itorChat.second->bNameFlag)
			{
				uint32_t dwVID = itorChat.first;
				ShowCharacterTextTail(dwVID);
			}
		}

		//for (const auto& [dwKey, pTextTail] : m_PrivateShopTextTailMap)
		//	UpdateDistance(pixelPos, pTextTail.get());
	}
}

// Only if Visible
void CPythonTextTail::UpdateShowingTextTail()
{
	for (auto& tt : m_ItemTextTailSet)
		UpdateTextTail(tt);

	for (const auto& pair : m_ChatTailMap)
		UpdateTextTail(pair.second.get());

	for (auto& tt : m_CharacterTextTailSet)
	{
		UpdateTextTail(tt);
		const auto itor = m_ChatTailMap.find(tt->dwVirtualID);
		if (m_ChatTailMap.end() != itor && itor->second->bNameFlag)
			tt->y = itor->second->y - 17.0f;
	}

	//for (const auto& pTextTail : m_PrivateShopTextTailSet)
	//	UpdateTextTail(pTextTail);
}

// Update a specific TextTail
void CPythonTextTail::UpdateTextTail(TextTail* pTextTail)
{
	if (!pTextTail->pOwner)
		return;

	CPythonGraphic* rpyGraphic = CPythonGraphic::GetInstance();
	rpyGraphic->Identity();

	const D3DXVECTOR3& c_rv3Position = pTextTail->pOwner->GetPosition();
	float x, y, z;

	rpyGraphic->ProjectPosition(c_rv3Position.x, c_rv3Position.y, c_rv3Position.z + pTextTail->fHeight, &x, &y, &z);

	pTextTail->x = floorf(x);
	pTextTail->y = floorf(y);

	if (pTextTail->fDistanceFromPlayer < 3500.0f)
	{
		pTextTail->z = 0.0f;
	}
	else
	{
		pTextTail->z = pTextTail->z * CPythonGraphic::GetInstance()->GetOrthoDepth() * -1.0f;
		pTextTail->z += 10.0f;
	}
}

// Brainstorm...
void CPythonTextTail::ArrangeTextTail()
{
	uint32_t dwTime = ELTimer_GetMSec();

	for (auto tt : m_ItemTextTailSet)
	{
		int32_t yTemp = 5;
		int32_t LimitCount = 0;

		for (auto it = m_ItemTextTailSet.begin(); it != m_ItemTextTailSet.end(); )
		{
			TextTail* pCompareTextTail = *it;
			if (*it == tt)
			{
				++it;
				continue;
			}

			if (LimitCount >= 20)
				break;

			if (isIn(tt, pCompareTextTail))
			{
				tt->y = pCompareTextTail->y + pCompareTextTail->yEnd + yTemp;
				it = m_ItemTextTailSet.begin();
				++LimitCount;
				continue;
			}
			++it;
		}

		tt->pTextInstance->SetColor(tt->Color.r, tt->Color.g, tt->Color.b);
	}

	for (auto tt : m_ItemTextTailSet)
		tt->pTextInstance->SetColor(tt->Color.r, tt->Color.g, tt->Color.b);

	for (auto itorChat = m_ChatTailMap.begin(); itorChat != m_ChatTailMap.end();)
	{
		TextTail* pTextTail = itorChat->second.get();

		if (pTextTail->LivingTime < dwTime)
		{
			itorChat = m_ChatTailMap.erase(itorChat);
			continue;
		}
		else
			++itorChat;

		pTextTail->pTextInstance->SetColor(pTextTail->Color);
	}

	//for (const auto& pTextTail : m_PrivateShopTextTailSet)
	//{
	//	pTextTail->pTextInstance->SetColor(pTextTail->Color);
	//	pTextTail->pTextInstance->Update();
	//}
}

void CPythonTextTail::Render()
{
	for (auto& pTextTail : m_CharacterTextTailSet)
	{
		// Guild Related
		std::unique_ptr<CGraphicMarkInstance>& guildMark = pTextTail->pMarkInstance;
		std::unique_ptr<CGraphicTextInstance>& guildName = pTextTail->pGuildNameTextInstance;
		//CGraphicImageInstance* guildCrown = pTextTail->pGuildCrownInstance;


		if (guildMark && guildName)
		{
			// First we update then we Render the text to X position and renderposition the Text
			guildName->Update();
			int32_t textW = guildName->GetWidth();
			int32_t markW = guildMark->GetWidth();

			guildMark->SetPosition(pTextTail->x - textW / static_cast<float>(2) - markW - c_fxMarkPosition, pTextTail->y - c_fyMarkPosition);
			guildMark->Render();
			guildName->Render(pTextTail->x - textW / 2, pTextTail->y - c_fyGuildNamePosition - guildName->GetHeight(), pTextTail->z);
		}

		// Guild Leader Icon
		//if (guildCrown) 
		//{
		//	float renderPosY = c_fyMarkPosition + 24;
		//	guildCrown->SetPosition(pTextTail->x - guildCrown->GetWidth() + 16, pTextTail->y - renderPosY);
		//	guildCrown->Render();
		//}

		auto const levelTextInstance =/* Engine::GetSettings().IsShowMobLevel() || */pTextTail->isPc ? pTextTail->pLevelTextInstance.get() : nullptr;
		//auto const aiFlagTextInstance = Engine::GetSettings().IsShowMobAIFlag() && !pTextTail->isPc ? pTextTail->pAIFlagTextInstance.get() : nullptr;

		const uint8_t MAX_INSTANCE_COUNT = 3;
		CGraphicTextInstance* instances[MAX_INSTANCE_COUNT] =
		{
			levelTextInstance,
			pTextTail->pTitleTextInstance.get(),
			pTextTail->pTextInstance.get(),
			//aiFlagTextInstance,
		};

		TextInstanceList<MAX_INSTANCE_COUNT> list(instances);
		const auto listX = pTextTail->x - list.GetWidth() + instances[TextInstanceList<MAX_INSTANCE_COUNT>::kName]->GetWidth() / 2;
		const auto listY = pTextTail->y - list.GetHeight();
		// Position Render
		list.Render(listX, listY, pTextTail->z);

		// Language Flag Icon
		//CGraphicImageInstance* pLangImage = pTextTail->pLanguageImageInstance;
		//if (pLangImage && ClientConfig::GetSettings().IsShowPlayerLanguage())
		//{
		//	pLangImage->SetPosition(pTextTail->x + instances[TextInstanceList<MAX_INSTANCE_COUNT>::kName]->GetWidth() / 2 + pLangImage->GetWidth() / 2, listY + 6);
		//	pLangImage->Render();
		//}
	}

	// Render boxes first to avoid the lag
	for (auto& pTextTail : m_ItemTextTailSet)
		RenderTextTailBox(pTextTail);

	for (auto& pTextTail : m_ItemTextTailSet)
	{
		if (pTextTail->pOwnerTextInstance)
		{
			auto halfW = pTextTail->pTextInstance->GetWidth() / 2;
			// Position Render
			pTextTail->pTextInstance->Render(pTextTail->x - halfW, pTextTail->y + 15.0f, pTextTail->z);

			halfW = pTextTail->pOwnerTextInstance->GetWidth() / 2;
			// Position Render
			pTextTail->pOwnerTextInstance->Render(pTextTail->x - halfW, pTextTail->y, pTextTail->z);
		}
		else
		{
			auto halfW = pTextTail->pTextInstance->GetWidth() / 2;
			// Position Render
			pTextTail->pTextInstance->Render(pTextTail->x - halfW, pTextTail->y, pTextTail->z);
		}
	}

	for (const auto& p : m_ChatTailMap)
	{
		auto pTextTail = p.second.get();
		if (pTextTail->pOwner->isShow())
		{
			const uint32_t iHalfWidht = pTextTail->pTextInstance->GetWidth() / 2;
			// Position Render
			pTextTail->pTextInstance->Render(pTextTail->x - iHalfWidht, pTextTail->y - pTextTail->pTextInstance->GetHeight(), pTextTail->z);
		}
	}

	//for (const auto pTextTail : m_PrivateShopTextTailSet)
	//{
	//	const uint32_t iHalfWidht = pTextTail->pTextInstance->GetWidth() / 2;
	//	pTextTail->pTextInstance->Render(pTextTail->x - iHalfWidht, pTextTail->y - pTextTail->pTextInstance->GetHeight(), pTextTail->z);
	//}
}

void CPythonTextTail::RenderTextTailBox(TextTail* pTextTail)
{
	const int32_t halfW = (pTextTail->xEnd - pTextTail->xStart) / 2;

	// First we Render the Black Board
	CPythonGraphic::GetInstance()->SetDiffuseColor(0.0f, 0.0f, 0.0f, 1.0f);
	CPythonGraphic::GetInstance()->RenderBox2d(pTextTail->x - halfW + pTextTail->xStart, pTextTail->y + pTextTail->yStart, pTextTail->x - halfW + pTextTail->xEnd, pTextTail->y + pTextTail->yEnd, pTextTail->z);

	// Then we render the Transparent Box inside it
	CPythonGraphic::GetInstance()->SetDiffuseColor(0.0f, 0.0f, 0.0f, 0.3f);
	CPythonGraphic::GetInstance()->RenderBar2d(pTextTail->x - halfW + pTextTail->xStart, pTextTail->y + pTextTail->yStart, pTextTail->x - halfW + pTextTail->xEnd, pTextTail->y + pTextTail->yEnd, pTextTail->z);
}

void CPythonTextTail::HideAllTextTail()
{
	m_CharacterTextTailSet.clear();
	m_ItemTextTailSet.clear();
	//m_PrivateShopTextTailSet.clear();
}

void CPythonTextTail::UpdateDistance(const TPixelPosition& c_rCenterPosition, TextTail* pTextTail)
{
	const D3DXVECTOR3& c_rv3Position = pTextTail->pOwner->GetPosition();
	D3DXVECTOR2 v2Distance(c_rv3Position.x - c_rCenterPosition.x, -c_rv3Position.y - c_rCenterPosition.y);
	pTextTail->fDistanceFromPlayer = D3DXVec2Length(&v2Distance);
}

// Control Visible TextTails
void CPythonTextTail::ShowAllPCTextTail()
{
	for (const auto& p : m_CharacterTextTailMap)
	{
		if (p.second->fDistanceFromPlayer < 3500.0f)
			ShowPlayerTextTail(p.first);
	}
}

void CPythonTextTail::ShowAllNPCTextTail()
{
	for (const auto& p : m_CharacterTextTailMap)
	{
		if (p.second->fDistanceFromPlayer < 3500.0f)
			ShowNpcTextTail(p.first);
	}
}

void CPythonTextTail::ShowAllMonsterTextTail()
{
	for (const auto& p : m_CharacterTextTailMap)
	{
		if (p.second->fDistanceFromPlayer < 3500.0f)
			ShowMonsterTextTail(p.first);
	}
}

void CPythonTextTail::ShowAllItemTextTail()
{
	for (const auto& p : m_ItemTextTailMap)
	{
		if (p.second->fDistanceFromPlayer < 3500.0f)
			ShowItemTextTail(p.first);
	}
}
/*
void CPythonTextTail::ShowAllPrivateShopTextTail()
{
	for (const auto& [dwKey, pTextTail] : m_PrivateShopTextTailMap)
	{
		if (pTextTail->fDistanceFromPlayer < 3500.0f)
			ShowPrivateShopTextTail(dwKey);
	}
}
*/
void CPythonTextTail::ShowAllTextTail()
{
	for (const auto& [dwKey, pTextTail] : m_CharacterTextTailMap)
	{
		if (pTextTail->fDistanceFromPlayer < 3500.0f)
			ShowCharacterTextTail(dwKey);
	}

	for (const auto& [dwKey, pTextTail] : m_ItemTextTailMap)
	{
		if (pTextTail->fDistanceFromPlayer < 3500.0f)
			ShowItemTextTail(dwKey);
	}

	//for (const auto& [dwKey, pTextTail] : m_PrivateShopTextTailMap)
	//{
	//	if (pTextTail->fDistanceFromPlayer < 3500.0f)
	//		ShowPrivateShopTextTail(dwKey);
	//}
}

void CPythonTextTail::ShowCharacterTextTail(uint32_t VirtualID)
{
	const auto itor = m_CharacterTextTailMap.find(VirtualID);
	if (m_CharacterTextTailMap.end() == itor)
		return;

	TextTail* pTextTail = itor->second.get();

	if (m_CharacterTextTailSet.end() != std::find(m_CharacterTextTailSet.begin(), m_CharacterTextTailSet.end(), pTextTail))
		return;

	if (!pTextTail->pOwner->isShow())
		return;

	CInstanceBase* pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(pTextTail->dwVirtualID);
	if (!pInstance)
		return;

	if (pInstance->IsGuildWall())
		return;

	if (pInstance->CanPickInstance())
		m_CharacterTextTailSet.emplace(pTextTail);
}

void CPythonTextTail::ShowPlayerTextTail(uint32_t VirtualID)
{
	const auto itor = m_CharacterTextTailMap.find(VirtualID);
	if (m_CharacterTextTailMap.end() == itor)
		return;

	TextTail* pTextTail = itor->second.get();

	if (!pTextTail->pOwner->isShow())
		return;

	CInstanceBase* pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(pTextTail->dwVirtualID);
	if (!pInstance)
		return;

	if (!pInstance->IsPC())
		return;

	if (pInstance->CanPickInstance())
		m_CharacterTextTailSet.emplace(pTextTail);
}

void CPythonTextTail::ShowMonsterTextTail(uint32_t VirtualID)
{
	const auto itor = m_CharacterTextTailMap.find(VirtualID);
	if (m_CharacterTextTailMap.end() == itor)
		return;

	TextTail* pTextTail = itor->second.get();

	if (!pTextTail->pOwner->isShow())
		return;

	CInstanceBase* pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(pTextTail->dwVirtualID);
	if (!pInstance)
		return;

	if (!pInstance->IsEnemy() && !pInstance->IsStone())
		return;

	if (pInstance->CanPickInstance())
		m_CharacterTextTailSet.emplace(pTextTail);
}

void CPythonTextTail::ShowNpcTextTail(uint32_t VirtualID)
{
	const auto itor = m_CharacterTextTailMap.find(VirtualID);
	if (m_CharacterTextTailMap.end() == itor)
		return;

	TextTail* pTextTail = itor->second.get();

	if (!pTextTail->pOwner->isShow())
		return;

	CInstanceBase* pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(pTextTail->dwVirtualID);
	if (!pInstance)
		return;

	if (!pInstance->IsNPC())
		return;

	if (pInstance->CanPickInstance())
		m_CharacterTextTailSet.emplace(pTextTail);
}

void CPythonTextTail::ShowItemTextTail(uint32_t VirtualID)
{
	const auto itor = m_ItemTextTailMap.find(VirtualID);
	if (m_ItemTextTailMap.end() == itor)
		return;

	m_ItemTextTailSet.emplace(itor->second.get());
}
// End of Control Visible TextTails

bool CPythonTextTail::isIn(CPythonTextTail::TextTail* pSource, CPythonTextTail::TextTail* pTarget)
{
	float x1Source = pSource->x + pSource->xStart; // Right align
	float y1Source = pSource->y + pSource->yStart; // Top align

	float x2Source = pSource->x + pSource->xEnd; // Left align
	float y2Source = pSource->y + pSource->yEnd; // Bottom align

	float x1Target = pTarget->x + pTarget->xStart; // Right align
	float y1Target = pTarget->y + pTarget->yStart; // Top align

	float x2Target = pTarget->x + pTarget->xEnd; // Left align
	float y2Target = pTarget->y + pTarget->yEnd; // Bottom align

	if (x1Source <= x2Target && x2Source >= x1Target && y1Source <= y2Target && y2Source >= y1Target)
		return true;

	return false;
}

void CPythonTextTail::RegisterCharacterTextTail(uint32_t dwGuildID, uint32_t dwVirtualID, const D3DXCOLOR& c_rColor, float fAddHeight)
{
	CInstanceBase* pCharacterInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(dwVirtualID);
	if (!pCharacterInstance)
		return;

	// TODO(01): Set offline shop's name to "Your shop" if player sees his own shop. 
	/*
	std::string name = pCharacterInstance->GetNameString();
	if (pCharacterInstance->IsShop())
	{
		CInstanceBase* pMainCharacterInstance = CPythonCharacterManager::Instance().GetMainInstancePtr();
		//Do not display shop names for those shops that aren't ours
		if (!pMainCharacterInstance || pMainCharacterInstance->GetNameString() != name)
			return;

		//Override the display shop name
		auto localeInfo = PyImport_ImportModule("localeInfo");
		auto nameAttr = PyObject_GetAttrString(localeInfo, "YOUR_SHOP");
		name = PyString_AS_STRING(nameAttr);
		Py_DECREF(nameAttr);
		Py_DECREF(localeInfo);

	}
	*/

	auto pTextTail = std::make_unique<TextTail>(dwVirtualID,
		pCharacterInstance->GetNameString(), pCharacterInstance->GetGraphicThingInstancePtr(),
		pCharacterInstance->GetGraphicThingInstanceRef().GetHeight() + fAddHeight, c_rColor);

	pTextTail->isPc = pCharacterInstance->IsPC();

	//if (pCharacterInstance->IsPC())
	//{
	//	std::string stIconPath = GetLanguageIconPath(pCharacterInstance->GetLanguage());
	//	if (!stIconPath.empty())
	//	{
	//		CGraphicImage* pImage = CResourceManager::GetInstance()->GetResourcePointer<CGraphicImage>(stIconPath);
	//		if (pImage)
	//		{
	//			pTextTail->pLanguageImageInstance = CGraphicImageInstance::New();
	//			pTextTail->pLanguageImageInstance->SetImagePointer(pImage);
	//			pTextTail->pLanguageImageInstance->SetScale(0.73f, 0.73f);
	//		}
	//		else
	//		{
	//			ConsoleLog("CPythonTextTail::RegisterCharacterTextTail: Could not find language icon for language {0}", GetLanguageLocale(pCharacterInstance->GetLanguage()));
	//		}
	//	}
	//}

	// TODO(02): Append a crown icon next to the name if a player is a guild leader.
	//if (pCharacterInstance->IsGuildLeader())
	//{
	//	pTextTail->pGuildCrownInstance = CGraphicImageInstance::New();
	//	pTextTail->pGuildCrownInstance->SetImagePointer(CResourceManager::Instance().GetResourcePointer<CGraphicImage>("Assets/Design/TitleIcons/GuildCrown.tga"));
	//}

	if (0 != dwGuildID)
	{
		pTextTail->pMarkInstance = std::make_unique<CGraphicMarkInstance>();

		uint32_t dwMarkID = CGuildMarkManager::GetInstance()->GetMarkID(dwGuildID);

		if (dwMarkID != CGuildMarkManager::INVALID_MARK_ID)
		{
			std::string markImagePath;

			if (CGuildMarkManager::GetInstance()->GetMarkImageFilename(dwMarkID / CGuildMarkImage::MARK_TOTAL_COUNT, markImagePath))
			{
				pTextTail->pMarkInstance->SetImageFileName(markImagePath.c_str());
				pTextTail->pMarkInstance->Load();
				pTextTail->pMarkInstance->SetIndex(dwMarkID % CGuildMarkImage::MARK_TOTAL_COUNT);
			}
		}

		std::string strGuildName;
		if (!CPythonGuild::GetInstance()->GetGuildName(dwGuildID, &strGuildName))
			strGuildName = "Noname";

		std::unique_ptr<CGraphicTextInstance>& prGuildNameInstance = pTextTail->pGuildNameTextInstance;
		prGuildNameInstance.reset(new CGraphicTextInstance());
		prGuildNameInstance->SetTextPointer(Engine::GetFontManager().LoadFont(textTailFontName));
		prGuildNameInstance->SetOutline(true);
		prGuildNameInstance->SetValue(strGuildName);
		prGuildNameInstance->SetColorGradient(0xFFEFD3FF, 0xFFA06074);
		prGuildNameInstance->Update();
	}

	//if (IS_SET(pCharacterInstance->GetAIFlag(), CInstanceBase::AIFLAG_AGGRESSIVE))
	//{
	//	auto& prAIFlagInstance = pTextTail->pAIFlagTextInstance;
	//	prAIFlagInstance.reset(new CGraphicTextInstance());
	//	prAIFlagInstance->SetTextPointer(Engine::GetFontManager().LoadFont(textTailFontName));
	//	prAIFlagInstance->SetOutline(true);
	//	prAIFlagInstance->SetValue("*");
	//	prAIFlagInstance->SetColor(c_TextTail_Monster_Color.a, c_TextTail_Monster_Color.r, c_TextTail_Monster_Color.g);
	//	prAIFlagInstance->Update();
	//}

	// We set the Outline for ALL TextInstance ONE time to avoid flushing the render! (Except the Guild part, since it's not visible if there is no guild!)
	pTextTail->pTextInstance->SetOutline(true);
	m_CharacterTextTailMap.emplace(dwVirtualID, std::move(pTextTail));
}

void CPythonTextTail::RegisterItemTextTail(uint32_t VirtualID, const char* c_szText, CGraphicObjectInstance* pOwner)
{
	auto pTextTail = std::make_unique<TextTail>(VirtualID, c_szText, pOwner, c_TextTail_Name_Position, c_TextTail_Item_Color);
	m_ItemTextTailMap.emplace(VirtualID, std::move(pTextTail));
}

void CPythonTextTail::RegisterChatTail(uint32_t VirtualID, const char* c_szChat)
{
	CInstanceBase* pCharacterInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(VirtualID);
	if (!pCharacterInstance)
		return;

	auto itor = m_ChatTailMap.find(VirtualID);

	if (m_ChatTailMap.end() != itor)
	{
		auto& pTextTail = itor->second;

		pTextTail->Color = c_TextTail_Chat_Color;
		pTextTail->LivingTime = ELTimer_GetMSec() + TextTail_GetLivingTime();
		pTextTail->bNameFlag = true;

		pTextTail->pTextInstance->SetValue(c_szChat);
		pTextTail->pTextInstance->SetColor(c_TextTail_Chat_Color);
		pTextTail->pTextInstance->Update();
		return;
	}

	auto pTextTail = std::make_unique<TextTail>(VirtualID, c_szChat, pCharacterInstance->GetGraphicThingInstancePtr(),
		pCharacterInstance->GetGraphicThingInstanceRef().GetHeight() + pCharacterInstance->GetBaseHeight() + 10.0f, c_TextTail_Chat_Color);

	pTextTail->LivingTime = ELTimer_GetMSec() + TextTail_GetLivingTime();
	pTextTail->bNameFlag = true;
	pTextTail->pTextInstance->SetOutline(true);
	m_ChatTailMap.emplace(VirtualID, std::move(pTextTail));
}

void CPythonTextTail::RegisterInfoTail(uint32_t VirtualID, const char* c_szChat)
{
	CInstanceBase* pCharacterInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(VirtualID);
	if (!pCharacterInstance)
		return;

	auto itor = m_ChatTailMap.find(VirtualID);
	if (m_ChatTailMap.end() != itor)
	{
		auto& pTextTail = itor->second;

		pTextTail->Color = c_TextTail_Info_Color;
		pTextTail->LivingTime = ELTimer_GetMSec() + TextTail_GetLivingTime();
		pTextTail->bNameFlag = false;

		pTextTail->pTextInstance->SetValue(c_szChat);
		pTextTail->pTextInstance->SetColor(c_TextTail_Info_Color);
		pTextTail->pTextInstance->Update();
		return;
	}

	auto pTextTail = std::make_unique<TextTail>(VirtualID, c_szChat, pCharacterInstance->GetGraphicThingInstancePtr(),
		pCharacterInstance->GetGraphicThingInstanceRef().GetHeight() + 10.0f, c_TextTail_Info_Color);

	pTextTail->LivingTime = ELTimer_GetMSec() + TextTail_GetLivingTime();
	pTextTail->bNameFlag = false;
	pTextTail->pTextInstance->SetOutline(true);
	m_ChatTailMap.emplace(VirtualID, std::move(pTextTail));
}

bool CPythonTextTail::GetTextTailPosition(uint32_t dwVID, float* px, float* py, float* pz)
{
	const auto it = m_CharacterTextTailMap.find(dwVID);
	if (m_CharacterTextTailMap.end() == it)
		return false;

	*px = it->second->x;
	*py = it->second->y;
	*pz = it->second->z;
	return true;
}

bool CPythonTextTail::IsChatTextTail(uint32_t dwVID)
{
	return m_ChatTailMap.end() != m_ChatTailMap.find(dwVID);
}

void CPythonTextTail::SetCharacterTextTailColor(uint32_t VirtualID, const D3DXCOLOR& c_rColor)
{
	const auto it = m_CharacterTextTailMap.find(VirtualID);
	if (m_CharacterTextTailMap.end() == it)
		return;

	it->second->pTextInstance->SetColor(c_rColor);
	it->second->Color = c_rColor;
}

void CPythonTextTail::UpdateCharacterName(uint32_t VirtualID, const std::string& name)
{
	const auto it = m_CharacterTextTailMap.find(VirtualID);
	if (m_CharacterTextTailMap.end() == it)
		return;

	it->second->pTextInstance->SetValue(name);
	it->second->pTextInstance->Update();
}

void CPythonTextTail::SetItemTextTailOwner(uint32_t dwVID, const char* c_szName)
{
	const auto itor = m_ItemTextTailMap.find(dwVID);
	if (m_ItemTextTailMap.end() == itor)
		return;

	auto& pTextTail = itor->second;
	if (strlen(c_szName) > 0)
	{
		if (!pTextTail->pOwnerTextInstance)
			pTextTail->pOwnerTextInstance.reset(new CGraphicTextInstance());

		std::string strName = c_szName;
		// (ipx)TODO: Adjust it with PyBind later on!
		strName += "'s";

		pTextTail->pOwnerTextInstance->SetTextPointer(Engine::GetFontManager().LoadFont(textTailFontName));
		pTextTail->pOwnerTextInstance->SetValue(strName);
		pTextTail->pOwnerTextInstance->SetColor(1.0f, 1.0f, 0.0f);
		CInstanceBase* pInstanceBase = CPythonCharacterManager::GetInstance()->GetMainInstancePtr();
		if (pInstanceBase)
		{
			if (strcmp(pInstanceBase->GetNameString(), c_szName))
				pTextTail->pOwnerTextInstance->SetColor(c_TextTail_Item_Red_Color.r, c_TextTail_Item_Red_Color.g, c_TextTail_Item_Red_Color.b);
		}

		pTextTail->pOwnerTextInstance->Update();

		int32_t xOwnerSize = pTextTail->pOwnerTextInstance->GetWidth();
		int32_t yOwnerSize = pTextTail->pOwnerTextInstance->GetHeight();
		pTextTail->xEnd = std::max(pTextTail->xEnd, xOwnerSize + 2);
		pTextTail->yEnd += yOwnerSize + 4;
	}
	else
	{
		if (pTextTail->pOwnerTextInstance)
			pTextTail->pOwnerTextInstance.reset();

		int32_t xSize = pTextTail->pTextInstance->GetWidth();
		int32_t ySize = pTextTail->pTextInstance->GetHeight();

		pTextTail->xEnd = xSize + 2;
		pTextTail->yEnd = ySize + 1;
	}
}

void CPythonTextTail::DeleteCharacterTextTail(uint32_t VirtualID)
{
	const auto itorCharacter = m_CharacterTextTailMap.find(VirtualID);
	const auto itorChat = m_ChatTailMap.find(VirtualID);

	if (m_CharacterTextTailMap.end() != itorCharacter)
		m_CharacterTextTailMap.erase(itorCharacter);
	else
		ConsoleLog("DeleteCharacterTextTail - Find VID[{0}] Error", VirtualID);

	if (m_ChatTailMap.end() != itorChat)
		m_ChatTailMap.erase(itorChat);
}

void CPythonTextTail::DeleteItemTextTail(uint32_t VirtualID)
{
	const auto itor = m_ItemTextTailMap.find(VirtualID);
	if (m_ItemTextTailMap.end() == itor)
	{
		SysLog("DeleteItemTextTail - No ItemTextTail");
		return;
	}

	m_ItemTextTailMap.erase(itor);
}

int32_t CPythonTextTail::Pick(int32_t ixMouse, int32_t iyMouse)
{
	for (const auto& position : m_ItemTextTailMap)
	{
		auto& pTextTail = position.second;

		const int32_t halfW = (pTextTail->xEnd - pTextTail->xStart) / 2;
		const int32_t left = pTextTail->x - halfW + pTextTail->xStart;
		const int32_t right = pTextTail->x - halfW + pTextTail->xEnd;
		const int32_t top = pTextTail->y + pTextTail->yStart;
		const int32_t bottom = pTextTail->y + pTextTail->yEnd;

		if (ixMouse >= left && ixMouse < right && iyMouse >= top && iyMouse < bottom)
		{
			SelectItemName(position.first);
			return position.first;
		}
	}
	return -1;
}

void CPythonTextTail::SelectItemName(uint32_t dwVirtualID)
{
	const auto itor = m_ItemTextTailMap.find(dwVirtualID);
	if (m_ItemTextTailMap.end() == itor)
		return;

	itor->second->pTextInstance->SetColor(0.1f, 0.9f, 0.1f);
}

void CPythonTextTail::AttachTitle(uint32_t dwVID, const char* c_szName, const D3DXCOLOR& c_rColor)
{
	if (!bPKTitleEnable)
		return;

	const auto itor = m_CharacterTextTailMap.find(dwVID);
	if (m_CharacterTextTailMap.end() == itor)
		return;

	auto& pTextTail = itor->second;

	auto& prTitle = pTextTail->pTitleTextInstance;
	if (!prTitle)
	{
		prTitle.reset(new CGraphicTextInstance());
		prTitle->SetTextPointer(Engine::GetFontManager().LoadFont(textTailFontName));
		prTitle->SetOutline(true);
	}

	prTitle->SetValue(c_szName);
	prTitle->SetColor(c_rColor.r, c_rColor.g, c_rColor.b);
	prTitle->Update();
}

void CPythonTextTail::DetachTitle(uint32_t dwVID)
{
	if (!bPKTitleEnable)
		return;

	const auto itor = m_CharacterTextTailMap.find(dwVID);
	if (m_CharacterTextTailMap.end() == itor)
		return;

	auto& pTextTail = itor->second;
	if (pTextTail->pTitleTextInstance)
		pTextTail->pTitleTextInstance = nullptr;
}

void CPythonTextTail::EnablePKTitle(bool bFlag)
{
	bPKTitleEnable = bFlag;
}

void CPythonTextTail::AttachLevel(uint32_t dwVID, const char* c_szText, uint32_t gColor1, uint32_t gColor2)
{
	if (!bPKTitleEnable)
		return;

	const auto itor = m_CharacterTextTailMap.find(dwVID);
	if (m_CharacterTextTailMap.end() == itor)
		return;

	auto& pTextTail = itor->second;

	auto& prLevel = pTextTail->pLevelTextInstance;
	if (!prLevel)
	{
		prLevel.reset(new CGraphicTextInstance());
		prLevel->SetTextPointer(Engine::GetFontManager().LoadFont(textTailFontName));
		prLevel->SetOutline(true);
	}

	prLevel->SetValue(c_szText);
	prLevel->SetColorGradient(gColor1, gColor2);
	prLevel->Update();
}

void CPythonTextTail::DetachLevel(uint32_t dwVID)
{
	if (!bPKTitleEnable)
		return;

	const auto itor = m_CharacterTextTailMap.find(dwVID);
	if (m_CharacterTextTailMap.end() == itor)
		return;

	auto& pTextTail = itor->second;
	if (pTextTail->pLevelTextInstance)
		pTextTail->pLevelTextInstance = nullptr;
}
/*
void CPythonTextTail::AttachLanguageImage(uint32_t dwVID, uint8_t bLanguage)
{
	auto it = m_CharacterTextTailMap.find(dwVID);
	if (it == m_CharacterTextTailMap.end())
		return;

	auto pTextTail = it->second.get();

	std::string stIconPath = GetLanguageIconPath(bLanguage);
	CGraphicImage* pImage = CResourceManager::GetInstance()->GetResourcePointer<CGraphicImage>(stIconPath);
	if (pImage)
	{
		if (!pTextTail->pLanguageImageInstance)
			pTextTail->pLanguageImageInstance = CGraphicImageInstance::New();

		pTextTail->pLanguageImageInstance->SetImagePointer(pImage);
		pTextTail->pLanguageImageInstance->SetScale(0.73f, 0.73f);
	}
	else
	{
		if (pTextTail->pLanguageImageInstance)
			CGraphicImageInstance::Delete(pTextTail->pLanguageImageInstance);
		pTextTail->pLanguageImageInstance = nullptr;

		ConsoleLog("CPythonTextTail::AttachLanguageImage: Could not find language icon for language {0}", GetLanguageLocale(bLanguage));
	}
}

void CPythonTextTail::ShowPrivateShopTextTail(uint32_t dwVirtualID)
{
	auto it = m_PrivateShopTextTailMap.find(dwVirtualID);

	if (it == m_PrivateShopTextTailMap.end())
		return;

	auto pTextTail = it->second.get();

	if (m_PrivateShopTextTailSet.end() != std::find(m_PrivateShopTextTailSet.begin(), m_PrivateShopTextTailSet.end(), pTextTail))
		return;

	m_PrivateShopTextTailSet.emplace(pTextTail);
}

void CPythonTextTail::RegisterPrivateShopTextTail(uint32_t dwVirtualID)
{
	CPythonPrivateShop::TPrivateShopInstance* pPrivateShopInstance = CPythonPrivateShop::GetInstance()->GetPrivateShopInstance(dwVirtualID);

	if (!pPrivateShopInstance)
		return;

	const D3DXCOLOR& c_rColor = D3DXCOLOR(1.0f, 0.41f, 0.0f, 1.0f);

	auto pTextTail = std::make_unique<TextTail>(dwVirtualID,
		pPrivateShopInstance->GetName(),
		pPrivateShopInstance->GetGraphicThingInstancePtr(),
		pPrivateShopInstance->GetGraphicThingInstancePtr()->GetHeight(),
		c_rColor);

	pTextTail->pTextInstance->SetOutline(true);
	pTextTail->pTextInstance->Update();

	m_PrivateShopTextTailMap.emplace(dwVirtualID, std::move(pTextTail));
}

void CPythonTextTail::DeletePrivateShopTextTail(uint32_t dwVirtualID)
{
	auto it = m_PrivateShopTextTailMap.find(dwVirtualID);

	if (it == m_PrivateShopTextTailMap.end())
	{
		WarnLog("Could not find private shop texttail with id {0}", dwVirtualID);
		return;
	}

	m_PrivateShopTextTailMap.erase(it);
}
*/
void CPythonTextTail::DestroyEveryTextTail()
{
	Clear();
}

void CPythonTextTail::Clear()
{
	m_CharacterTextTailMap.clear();
	m_CharacterTextTailSet.clear();
	m_ItemTextTailMap.clear();
	m_ItemTextTailSet.clear();
	m_ChatTailMap.clear();
	//m_PrivateShopTextTailMap.clear();
	//m_PrivateShopTextTailSet.clear();
}

void CPythonTextTail::ChangeFontType(std::string fontType)
{
	textTailFontName = fontType;
	HideAllTextTail();
	ArrangeTextTail();
	UpdateShowingTextTail();
	UpdateAllTextTail();
	ShowAllTextTail();
	Render();
}

CPythonTextTail::CPythonTextTail()
{
	Clear();
}
