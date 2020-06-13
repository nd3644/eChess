#ifndef MENU_H
#define MENU_H

#include <gui_panel.h>

class Menu : public Eternal::PanelUI {
    public:
        Menu();
        void OnUpdate(Eternal::InputHandle *myInputHandle);
        void OnDraw(Eternal::Renderer *myRenderer);
    public:
    Eternal::Button button_OnePlay,
                    button_Hostgame,
                    button_Joingame,
                    button_PlayPGN,
                    button_Quit;
};

#endif
