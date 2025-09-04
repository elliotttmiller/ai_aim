// src/Overlay/UI/Menu.h
#pragma once
#include "../Utils/Singleton.h"

class Menu : public Singleton<Menu> {
public:
    void Draw();

    bool IsVisible() const { return m_bIsVisible; }
    void Toggle() { m_bIsVisible = !m_bIsVisible; }

private:
    bool m_bIsVisible = true;
};
