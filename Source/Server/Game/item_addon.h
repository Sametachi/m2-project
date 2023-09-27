#pragma once

class CItemAddonManager : public Singleton<CItemAddonManager>
{
	public:
		CItemAddonManager();
		virtual ~CItemAddonManager();

		void ApplyAddonTo(int32_t iAddonType, LPITEM pItem);
};
