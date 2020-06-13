#include "menu.h"

Menu::Menu() : Eternal::PanelUI(32, 32, 128, 200) {
    button_OnePlay.SetGeometry(8, 8, 48, 24);
    button_OnePlay.SetText("Play computer");
    

    button_Hostgame.SetGeometry(8, 8 + 32, 48, 24);
    button_Hostgame.SetText("Host game");


    button_Joingame.SetGeometry(8, 8 + 64, 48, 24);
    button_Joingame.SetText("Join game");

    button_PlayPGN.SetGeometry(8, 8 + 96, 72, 24);
    button_PlayPGN.SetText("Play PGN");

    button_Quit.SetGeometry(8, 8 + 128, 48, 24);
    button_Quit.SetText("Quit");
    
    AddWidget((Eternal::Widget*)&button_OnePlay);
    AddWidget((Eternal::Widget*)&button_Hostgame);
    AddWidget((Eternal::Widget*)&button_Joingame);
    AddWidget((Eternal::Widget*)&button_PlayPGN);
    AddWidget((Eternal::Widget*)&button_Quit);
    Show(true);
}
void Menu::OnUpdate(Eternal::InputHandle *myInputHandle) {
    if(!bShown) {
        return;
    }
    PanelUI::OnUpdate(myInputHandle);


}

void Menu::OnDraw(Eternal::Renderer *myRenderer) {
    if(!bShown) {
        return;
    }
    PanelUI::OnDraw(myRenderer);
}