#ifndef MENU_H
#define MENU_H

#include <gui_panel.h>

class Menu : public Eternal::PanelUI {
    public:
        Menu() : Eternal::PanelUI(200, 200, 256, 256) {
            button_OnePlay.SetGeometry(8, 8, 24, 24);
            button_OnePlay.SetText("Play computer");
            

            button_Hostgame.SetGeometry(8, 8 + 32, 48, 24);
            button_Hostgame.SetText("Host game");


            button_Joingame.SetGeometry(8, 8 + 64, 48, 24);
            button_Joingame.SetText("Join game");
            
            AddWidget((Eternal::Widget*)&button_OnePlay);
            AddWidget((Eternal::Widget*)&button_Hostgame);
            AddWidget((Eternal::Widget*)&button_Joingame);
            Show(true);
        }
        void OnUpdate(Eternal::InputHandle *myInputHandle) {
            if(!bShown) {
                return;
            }
            PanelUI::OnUpdate(myInputHandle);


        }
        
        void OnDraw(Eternal::Renderer *myRenderer) {
            if(!bShown) {
                return;
            }
            PanelUI::OnDraw(myRenderer);
        }
    public:
    Eternal::Button button_OnePlay,
                    button_Hostgame,
                    button_Joingame;
};

#endif
