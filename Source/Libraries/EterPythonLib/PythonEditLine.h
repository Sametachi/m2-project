#pragma once
#include "PythonWindow.h"
#include <EterLib/Engine.h>
#include <EterLib/Clipboard.h>
#include <EterLib/StateManager.h>

class CPythonEditLine : public CTextLine
{
    public:
        CPythonEditLine();

        void Enable();
        void Disable();

        void SetMax(uint32_t count);
        void SetMaxVisible(uint32_t count);

        uint32_t GetCursorPosition() const;
        void SetCursorPosition(uint32_t position);

        void SetPlaceholderText(std::string text);
        void SetPlaceholderColor(D3DXCOLOR color);

        void MoveCursor(int32_t offset, bool updateSelection = false);
        void MoveToBeginning();
        void MoveToEnd();

        bool Insert(uint32_t offset, const std::string& text);
        void Erase(uint32_t offset, uint32_t count);
        void EraseGlyphs(uint32_t offset, uint32_t count);

        virtual void OnUpdate();
        virtual void OnRender();

        virtual void OnSetFocus();
        virtual void OnKillFocus();

        virtual bool OnMouseLeftButtonDown();
        virtual void OnMouseDrag(int32_t x, int32_t y);

        virtual bool OnChar(uint32_t ch);
        virtual bool OnKeyDown(KeyCode code);
        virtual bool OnKeyUp(KeyCode code);

    protected:
        virtual void OnChangeText();

        void EraseSelection();

        void RenderSelection();
        void RenderCursor();

        uint32_t GetIndexFromPosition(int32_t x, int32_t y) const;

        void ForceShowCursor();

        bool m_enabled = true;
        bool m_focused;

        bool m_renderCursor;
        uint32_t m_cursorVisibilityTime;
        uint32_t m_cursorPosition;

        uint32_t m_selectionStart;
        uint32_t m_selectionEnd;

        uint32_t m_maxLength;
        uint32_t m_maxVisibleLength;

        std::string m_placeholderText;
        CGraphicTextInstance m_placeholderInstance;
        D3DXCOLOR m_placeHolderColor;
};