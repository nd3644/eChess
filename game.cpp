#include <iostream>
#include <thread>
#include <application.h>
#include <audio_sound.h>
#include <vector>
#include <font.h>
#include <map>
#include <fstream>

#include <SDL2/SDL_net.h>

const int SQUARE_SIZE = 96;
const int WIN_W = SQUARE_SIZE * 8;
const int WIN_H = SQUARE_SIZE * 8;

SDL_Thread *myThread;

enum GAME_MODE {
	HOT_SEAT = 1,
	COMPUTER,
	NET_SERV,
	NET_CLIENT
};

enum GAME_STATE {
	TITLE = 1,
	CONNECTING,
	GAMEPLAY
};

// black can take pieces into checks- pls fix
enum COLOR {
	WHITE = 1,
	BLACK
};

enum PIECE_TYPE {
	PAWN = 1,
	ROOK,
	KNIGHT,
	BISHOP,
	QUEEN,
	KING,
	NUM_PIECES
};

struct position_t {
	position_t() {
		x = y = 0;
	}
	int x, y;
};

std::map<std::string, std::string>mySettingsMap;

extern int thread_main(void *);
bool bListening = false;
bool bBoardFlipped = false;
struct piece_t {
	piece_t() {
		x = y = 0;
		type = PAWN;
		bTaken = false;
		
		bFloating = false;
	}
	PIECE_TYPE type;
	int x, y;
	bool bTaken;
	
	float fShake;
	
	bool bFloating;
	COLOR color;
};

class App : public Eternal::Application {
	public:
		GAME_MODE myGameMode;
		COLOR currentTurn;
		int iEndGameTimer;
		App() {
			currentTurn = WHITE;
			iSelectedSquareX = iSelectedSquareY = 0;
			bMouseWasDown = false;
			
			heldPiece = nullptr;
			
			myGameMode = HOT_SEAT;
			myGameState = TITLE;
			iMenuCursor = 0;
			iEndGameTimer = 0;
		}
		
		~App() {
		}
		
		bool bMouseWasDown;
		Eternal::Sprite sprites_White[NUM_PIECES];
		Eternal::Sprite sprites_Black[NUM_PIECES];
		Eternal::Sound sound_Move;
		
		std::vector<piece_t>pieces_White;
		std::vector<piece_t>pieces_Black;
		
		std::vector<position_t>nodeList;
		
		piece_t *heldPiece;

		char key[128];
		char val[128];
		void LoadCFG() {
			std::ifstream infile("settings.cfg");
			std::string sLine;
			while(!infile.eof()) {
				for(int i = 0; i < 128;i++) {
					key[i] = val[i] = 0x00;
				}
				std::getline(infile, sLine);
				std::sscanf(sLine.c_str(), "%s = %s", key, val);
				mySettingsMap[key] = val;
			}
			infile.close();

			for(auto i = mySettingsMap.cbegin();i != mySettingsMap.cend();i++) {
				std::cout << i->first << " = " << i->second << std::endl;
			}
		}

		bool IsBlackKingInCheck() {
			piece_t *blackKing = nullptr;
			for(unsigned int i = 0;i < pieces_Black.size();i++) {
				if(pieces_Black[i].type == KING) {
					blackKing = &pieces_Black[i];
				}
			}
			if(blackKing == nullptr) {
				printf("couldn't find black king?\n");
				return false;
			}
			
			for(unsigned int i = 0;i < pieces_White.size();i++) {
				// taken pieces are to be ignored
				if(pieces_White[i].bTaken) {
					continue;
				}
				std::vector<position_t> positionList = ListPossibleMoves(&pieces_White[i]);
				for(unsigned int j = 0;j < positionList.size();j++) {
					if(positionList[j].x == blackKing->x && positionList[j].y == blackKing->y) {
						return true;
					}
				}
			}
			
			return false;
		}
		bool IsWhiteKingInCheck() {
			piece_t *whiteKing = nullptr;
			for(unsigned int i = 0;i < pieces_White.size();i++) {
				if(pieces_White[i].type == KING) {
					whiteKing = &pieces_White[i];
				}
			}
			if(whiteKing == nullptr) {
				printf("couldn't find white king?\n");
				return false;
			}
			
			for(unsigned int i = 0;i < pieces_Black.size();i++) {
				// taken pieces are to be ignored
				if(pieces_Black[i].bTaken) {
					continue;
				}
				std::vector<position_t> positionList = ListPossibleMoves(&pieces_Black[i]);
				for(unsigned int j = 0;j < positionList.size();j++) {
					if(positionList[j].x == whiteKing->x && positionList[j].y == whiteKing->y) {
						return true;
					}
				}
			}
			return false;
		}
		
		void LoadSprites() {
			sprites_White[PAWN].Load("data/white-pawn.png");
			sprites_Black[PAWN].Load("data/black-pawn.png");
			
			sprites_White[ROOK].Load("data/white-rook.png");
			sprites_Black[ROOK].Load("data/black-rook.png");
			
			sprites_White[KNIGHT].Load("data/white-knight.png");
			sprites_Black[KNIGHT].Load("data/black-knight.png");
			
			sprites_White[BISHOP].Load("data/white-bishop.png");
			sprites_Black[BISHOP].Load("data/black-bishop.png");
			
			sprites_White[QUEEN].Load("data/white-queen.png");
			sprites_Black[QUEEN].Load("data/black-queen.png");
			
			sprites_White[KING].Load("data/white-king.png");
			sprites_Black[KING].Load("data/black-king.png");

			sound_Move.Load("data/move.wav");
		}
		
		std::vector<position_t> ListPossibleMoves(piece_t *myPiece) {
			std::vector<position_t>list;
			position_t pos;
			
			const int forwardDir = (myPiece->color == WHITE) ? -1 : 1;
			if(myPiece->type == PAWN) {
				if(IsSquareEmpty(myPiece->x, myPiece->y + forwardDir)) {
					pos.x = myPiece->x; pos.y = myPiece->y + forwardDir;
					list.push_back(pos);
					// Can a second jump be made? (first move)
					if((myPiece->color == WHITE && myPiece->y == 6) || (myPiece->color == BLACK && myPiece->y == 1)) {
						if(IsSquareEmpty(myPiece->x, myPiece->y + (forwardDir*2))) {
							pos.x = myPiece->x; pos.y = myPiece->y + (forwardDir*2);
							list.push_back(pos);
						}
					}
				}
				
				piece_t *foundPiece = FindAnyPiece(myPiece->x - 1, myPiece->y + forwardDir);
				if(foundPiece != nullptr && foundPiece->color != myPiece->color) {
					pos.x = myPiece->x - 1; pos.y = myPiece->y + forwardDir;
					list.push_back(pos);
				}
				foundPiece = FindAnyPiece(myPiece->x + 1, myPiece->y + forwardDir);
				if(foundPiece != nullptr && foundPiece->color != myPiece->color) {
					pos.x = myPiece->x + 1; pos.y = myPiece->y + forwardDir;
					list.push_back(pos);
				}
			}
					
			if(myPiece->type == KNIGHT)  {
				piece_t *foundPiece = nullptr;
					
				pos.x = myPiece->x - 2; pos.y = myPiece->y - 1;
				foundPiece = FindAnyPiece(pos.x, pos.y);
				if((foundPiece != nullptr && foundPiece->color != myPiece->color) || foundPiece == nullptr)
					list.push_back(pos);
				
				pos.x = myPiece->x - 2; pos.y = myPiece->y + 1;
				foundPiece = FindAnyPiece(pos.x, pos.y);
				if((foundPiece != nullptr && foundPiece->color != myPiece->color) || foundPiece == nullptr)
					list.push_back(pos);
				
				pos.x = myPiece->x + 2; pos.y = myPiece->y + 1;
				foundPiece = FindAnyPiece(pos.x, pos.y);
				if((foundPiece != nullptr && foundPiece->color != myPiece->color) || foundPiece == nullptr)
					list.push_back(pos);
				
				pos.x = myPiece->x + 2; pos.y = myPiece->y - 1;
				foundPiece = FindAnyPiece(pos.x, pos.y);
				if((foundPiece != nullptr && foundPiece->color != myPiece->color) || foundPiece == nullptr)
					list.push_back(pos);
				
				
				pos.x = myPiece->x - 1; pos.y = myPiece->y - 2;
				foundPiece = FindAnyPiece(pos.x, pos.y);
				if((foundPiece != nullptr && foundPiece->color != myPiece->color) || foundPiece == nullptr)
					list.push_back(pos);
				
				pos.x = myPiece->x - 1; pos.y = myPiece->y + 2;
				foundPiece = FindAnyPiece(pos.x, pos.y);
				if((foundPiece != nullptr && foundPiece->color != myPiece->color) || foundPiece == nullptr)
					list.push_back(pos);
				
				pos.x = myPiece->x + 1; pos.y = myPiece->y + 2;
				foundPiece = FindAnyPiece(pos.x, pos.y);
				if((foundPiece != nullptr && foundPiece->color != myPiece->color) || foundPiece == nullptr)
					list.push_back(pos);
				
				pos.x = myPiece->x + 1; pos.y = myPiece->y - 2;
				foundPiece = FindAnyPiece(pos.x, pos.y);
				if((foundPiece != nullptr && foundPiece->color != myPiece->color) || foundPiece == nullptr)
					list.push_back(pos);
			}
			if(myPiece->type == BISHOP || myPiece->type == QUEEN) {
				int x = myPiece->x;
				int y = myPiece->y;
				for(;;) {
					x--;
					y--;
					if(x < 0 || y < 0) {
						break;
					}
					pos.x = x; pos.y = y;
					list.push_back(pos);
					piece_t *foundPiece = FindAnyPiece(x,y);
					if(foundPiece != nullptr) {
						// Dont allow to take a friendly piece
						if(foundPiece->color == myPiece->color) {
							list.erase(list.end()-1);
						}
						break;
					}
				}
				
				x = myPiece->x;
				y = myPiece->y;
				for(;;) {
					x++;
					y--;
					if(x > 7 || y < 0) {
						break;
					}
					pos.x = x; pos.y = y;
					list.push_back(pos);
					piece_t *foundPiece = FindAnyPiece(x,y);
					if(foundPiece != nullptr) {
						// Dont allow to take a friendly piece
						if(foundPiece->color == myPiece->color) {
							list.erase(list.end()-1);
						}
						break;
					}
				}
				
				x = myPiece->x;
				y = myPiece->y;
				for(;;) {
					x--;
					y++;
					if(x < 0 || y > 7) {
						break;
					}
					pos.x = x; pos.y = y;
					list.push_back(pos);
					piece_t *foundPiece = FindAnyPiece(x,y);
					if(foundPiece != nullptr) {
						// Dont allow to take a friendly piece
						if(foundPiece->color == myPiece->color) {
							list.erase(list.end()-1);
						}
						break;
					}
				}
				
				x = myPiece->x;
				y = myPiece->y;
				for(;;) {
					x++;
					y++;
					if(x > 7 || y > 7) {
						break;
					}
					pos.x = x; pos.y = y;
					list.push_back(pos);
					piece_t *foundPiece = FindAnyPiece(x,y);
					if(foundPiece != nullptr) {
						// Dont allow to take a friendly piece
						if(foundPiece->color == myPiece->color) {
							list.erase(list.end()-1);
						}
						break;
					}
				}
			}
			
			if(myPiece->type == ROOK || myPiece->type == QUEEN) {
				int x = myPiece->x;
				int y = myPiece->y;
				for(;;) {
					x--;
					if(x < 0) {
						break;
					}
					pos.x = x;
					pos.y = y;
					list.push_back(pos);
					piece_t *foundPiece = FindAnyPiece(x,y);
					if(foundPiece != nullptr) {
						// Dont allow to take a friendly piece
						if(foundPiece->color == myPiece->color) {
							list.erase(list.end()-1);
						}
						break;
					}
				}
				
				x = myPiece->x;
				y = myPiece->y;
				for(;;) {
					x++;
					if(x > 7) {
						break;
					}
					pos.x = x;
					pos.y = y;
					list.push_back(pos);
					piece_t *foundPiece = FindAnyPiece(x,y);
					if(foundPiece != nullptr) {
						// Dont allow to take a friendly piece
						if(foundPiece->color == myPiece->color) {
							list.erase(list.end()-1);
						}
						break;
					}
				}
				
				x = myPiece->x;
				y = myPiece->y;
				for(;;) {
					y--;
					if(y < 0) {
						break;
					}
					pos.x = x;
					pos.y = y;
					list.push_back(pos);
					piece_t *foundPiece = FindAnyPiece(x,y);
					if(foundPiece != nullptr) {
						// Dont allow to take a friendly piece
						if(foundPiece->color == myPiece->color) {
							list.erase(list.end()-1);
						}
						break;
					}
				}
				
				x = myPiece->x;
				y = myPiece->y;
				for(;;) {
					y++;
					if(y > 7) {
						break;
					}
					pos.x = x;
					pos.y = y;
					list.push_back(pos);
					piece_t *foundPiece = FindAnyPiece(x,y);
					if(foundPiece != nullptr) {
						// Dont allow to take a friendly piece
						if(foundPiece->color == myPiece->color) {
							list.erase(list.end()-1);
						}
						break;
					}
				}
			}
			
			
			if(myPiece->type == KING) {
				piece_t *foundPiece = nullptr;
				for(int x = myPiece->x - 1;x < myPiece->x + 2;x++) {
					if(x < 0)
						continue;
					if(x > 7)
						break;
					for(int y = myPiece->y - 1;y < myPiece->y + 2;y++) {
						if(y < 0)
							continue;
						if(y > 7)
							break;

						pos.x = x;
						pos.y = y;
						list.push_back(pos);
						
						foundPiece = FindAnyPiece(pos.x, pos.y);
						if(foundPiece != nullptr) {
							if(foundPiece->color == myPiece->color) {
								list.erase(list.end()-1);
							}
						}
					}
				}
			}
			return list;
		}
		
		// finds a piece given a piece list (i.e. black list, white list)
		piece_t *FindPiece(std::vector<piece_t> &list, int posX, int posY) {
			for(unsigned int i = 0;i < list.size();i++) {
				if(list[i].x == posX && list[i].y == posY) {
					return &list[i];
				}
			}
			return nullptr;
		}
		
		piece_t *FindAnyPiece(int x, int y) {
			for(unsigned int i = 0; i < pieces_White.size();i++) {
				if(pieces_White[i].x == x && pieces_White[i].y == y) {
					return &pieces_White[i];
				}
			}
			for(unsigned int i = 0;i < pieces_Black.size();i++) {
				if(pieces_Black[i].x == x && pieces_Black[i].y == y) {
					return &pieces_Black[i];
				}
			}
			return nullptr;
		}
		
		bool IsSquareEmpty(int x, int y) {
			for(unsigned int i = 0;i < pieces_White.size();i++) {
				if(pieces_White[i].x == x && pieces_White[i].y == y) {
					return false;
				}
			}
			for(unsigned int i = 0;i < pieces_Black.size();i++) {
				if(pieces_Black[i].x == x && pieces_Black[i].y == y) {
					return false;
				}
			}
			return true;
		}
		
		void NextTurn() {
			currentTurn = (currentTurn == WHITE) ? BLACK : WHITE;
		}
		
		
		// function assumes the given move is legal
		// but makes sure the piece resolves a check
		bool PlacePiece(piece_t &which, int x, int y) {
			COLOR otherColor = (which.color == WHITE) ? BLACK : WHITE;
			piece_t *originalPiece = FindAnyPiece(x,y);
			
			bool bChecked = false;
			if(IsWhiteKingInCheck() || IsBlackKingInCheck()) {
				bChecked = true;
			}
			
			piece_t oldCopy = which;
			
			// No piece is occupying the target square- go ahead and perform the move
			if(originalPiece == nullptr) {
				which.x = x;
				which.y = y;
				
				// undo the move
				if(bChecked && (IsWhiteKingInCheck() || IsBlackKingInCheck())) {
					which = oldCopy;
					return false;
				}
				else {
					NextTurn();
				}
			}
			else if(originalPiece->color != which.color) {
				which.x = x;
				which.y = y;
				originalPiece->bTaken = true;
				// undo the move
				if(bChecked && (IsWhiteKingInCheck() || IsBlackKingInCheck())) {
					which = oldCopy;
					originalPiece->bTaken = false;
					return false;
				}
				else {
					NextTurn();
				}
			}
			
			// The square is occupied with a friendly piece- the move is impossible
			else if(originalPiece->color == which.color) {
				return false;
			}
			return true;
		}
		
		void TryPieceMove() {
			// heldPiece will probably never be null here but.. you know
			if(heldPiece == nullptr
			|| iSelectedSquareX < 0 || iSelectedSquareX > 7
			|| iSelectedSquareY < 0 || iSelectedSquareY > 7) {
				return;
			}
			
			for(unsigned int i = 0;i < nodeList.size();i++) {
				if(nodeList[i].x == iSelectedSquareX && nodeList[i].y == iSelectedSquareY) {
				
					int oldX = heldPiece->x;
					int oldY = heldPiece->y;
					// If a piece is placed successfully...
					if(PlacePiece(*heldPiece, iSelectedSquareX, iSelectedSquareY)) {
						if(myGameMode == NET_SERV || myGameMode == NET_CLIENT) {
							char buf[4];
							buf[0] = (char)oldX;
							buf[1] = (char)oldY;
							buf[2] = (char)heldPiece->x;
							buf[3] = (char)heldPiece->y;
							SDLNet_TCP_Send(clientSocket, buf, 5);
						}
						sound_Move.Play(0);
					}
				}
			}
		}
		
		int iSelectedSquareX, iSelectedSquareY;
		void CheckMoves() {
			std::vector<piece_t>* pieceList = (currentTurn == WHITE) ? &pieces_White : &pieces_Black;
			
			// MOUSE CLICKED ON THIS FRAME- SELECT A PIECE
			if(!bMouseWasDown && myInputHandle->IsMouseDown(Eternal::InputHandle::MouseButtons::MBUTTON_LEFT)) {
				heldPiece = FindPiece(*pieceList, iSelectedSquareX, iSelectedSquareY);
				if(heldPiece != nullptr) {
					heldPiece->bFloating = true;
				}
			}
			
			if(!myInputHandle->IsMouseDown(Eternal::InputHandle::MouseButtons::MBUTTON_LEFT)) {			
				// Release held piece
				if(heldPiece != nullptr) {
				
					TryPieceMove();					
				
					heldPiece->bFloating = false;
				}
				heldPiece = nullptr;
			}
		}
		
		void ResetPieces() {
			pieces_White.clear();
			pieces_Black.clear();
			
			piece_t piece;
			// add pawns
			for(int x = 0;x < 8;x++) {
				piece.x = x;
				piece.y = 1;
				piece.type = PAWN;
				
				pieces_Black.push_back(piece);
				
				piece.y = 6;
				pieces_White.push_back(piece);
			}
			
			// rooks
			piece.type = ROOK;
			piece.x = 0;
			piece.y = 0;
			
			pieces_Black.push_back(piece);
			piece.x = 7;
			
			pieces_Black.push_back(piece);
			
			piece.y = 7;
			piece.x = 0;
			pieces_White.push_back(piece);
			
			piece.x = 7;
			pieces_White.push_back(piece);
			
			// knights
			piece.type = KNIGHT;
			piece.x = 1;
			piece.y = 0;
			
			pieces_Black.push_back(piece);
			
			piece.x = 6;
			pieces_Black.push_back(piece);
			
			
			piece.x = 1;
			piece.y = 7;
			pieces_White.push_back(piece);
			
			piece.x = 6;
			
			pieces_White.push_back(piece);
			
			// bishops
			piece.type = BISHOP;
			piece.x = 2;
			piece.y = 0;
			
			pieces_Black.push_back(piece);
			
			piece.x = 5;
			pieces_Black.push_back(piece);
			
			
			piece.x = 2;
			piece.y = 7;
			pieces_White.push_back(piece);
			
			piece.x = 5;
			
			pieces_White.push_back(piece);
			
			// queens
			piece.type = QUEEN;
			piece.x = 3;
			piece.y = 0;
			
			pieces_Black.push_back(piece);
			
			piece.x = 3;
			piece.y = 7;
			
			pieces_White.push_back(piece);
			
			// kings
			piece.type = KING;
			piece.x = 4;
			piece.y = 0;
			
			pieces_Black.push_back(piece);
			
			piece.x = 4;
			piece.y = 7;
			
			pieces_White.push_back(piece);
			
			// Arrays should be the same length here
			for(unsigned int i = 0;i < pieces_White.size();i++) {
				pieces_White[i].color = WHITE;
				pieces_Black[i].color = BLACK;
			}
			currentTurn = WHITE;
		}
		
		void DrawBackdrop() {
			Eternal::Rect r(0, 0, WIN_W, WIN_H);
			Eternal::Quad q;
			q.FromRect(r);
			myRenderer->SetColor(0.2f,0.2f,0.2f,1.0f);
			myRenderer->DrawQuad(q);
		}
		
		void DrawBoard() {
			myRenderer->SetColor(0.8f,0.8f,0.8f,1.0f);
			COLOR currentSquareColor = WHITE;
			for(int x = 0;x < 8;x++) {
				for(int y = 0;y < 8;y++) {
						currentSquareColor = (currentSquareColor == WHITE) ? BLACK : WHITE; 
						if(currentSquareColor == BLACK) {
							Eternal::Quad q;
							Eternal::Rect r((float) (x * SQUARE_SIZE), (float)(y * SQUARE_SIZE), (float)SQUARE_SIZE, (float)SQUARE_SIZE);
							q.FromRect(r);
							myRenderer->DrawQuad(q);
						}
				}
				currentSquareColor = (currentSquareColor == WHITE) ? BLACK : WHITE; 
			}
		}
		
		void DrawPieces() {
		
			if(IsWhiteKingInCheck()) {
				sprites_White[KING].SetColor(255,0,0,255);
			}
			else {
				sprites_White[KING].SetColor(255,255,255,255);
			}
			
			
			if(IsBlackKingInCheck()) {
				sprites_Black[KING].SetColor(255,0,0,255);
			}
			else {
				sprites_Black[KING].SetColor(255,255,255,255);
			}
		
			Eternal::Rect r(0,0, (float)SQUARE_SIZE, (float)SQUARE_SIZE);
			Eternal::Rect c(0, 0, 0, 0);
			for(unsigned int i = 0;i < pieces_White.size();i++) {
				piece_t piece = pieces_White[i];
				
				if(bBoardFlipped) {
					piece.y = 7 - piece.y;
				}
				
				r.x = (float)piece.x * SQUARE_SIZE;
				r.y = (float)piece.y * SQUARE_SIZE;
				r.w = (float)SQUARE_SIZE;
				r.h = (float)SQUARE_SIZE;
				c.w = sprites_White[piece.type].GetWidth();
				c.h = sprites_White[piece.type].GetHeight();
				
				if(heldPiece == &pieces_White[i]) {
					r.w = (float)SQUARE_SIZE;
					r.h = (float)SQUARE_SIZE;
					r.x = myInputHandle->GetMouseX() - (r.w / 2);
					r.y = myInputHandle->GetMouseY() - (r.h / 2);
				}
				
				r.x += 5;
				r.y += 5;
				r.w -= 8;
				r.h -= 8;
				
				sprites_White[piece.type].Draw(r,c);
			}
			for(unsigned int i = 0;i < pieces_Black.size();i++) {
				piece_t piece = pieces_Black[i];
				
				if(bBoardFlipped) {
					piece.y = 7 - piece.y;
				}
				
				r.x = (float)piece.x * SQUARE_SIZE;
				r.y = (float)piece.y * SQUARE_SIZE;
				r.w = (float)SQUARE_SIZE;
				r.h = (float)SQUARE_SIZE;
				c.w = sprites_Black[piece.type].GetWidth();
				c.h = sprites_Black[piece.type].GetHeight();
				
				r.x += 5;
				r.y += 5;
				r.w -= 8;
				r.h -= 8;
				
				if(heldPiece == &pieces_Black[i]) {
					r.w = (float)SQUARE_SIZE;
					r.h = (float)SQUARE_SIZE;
					r.x = myInputHandle->GetMouseX() - (r.w / 2);
					r.y = myInputHandle->GetMouseY() - (r.h / 2);
				}
				
				sprites_Black[piece.type].Draw(r,c);
			}
		}
		
		void OnInitialize() {
			SDLNet_Init();
			LoadSprites();
			ResetPieces();
			myFont.Load("data/hello");
			
			myVideoSystem->SetMaxFPS(60);
			
			myThread = SDL_CreateThread(thread_main, "thread_main", NULL);
			SDL_DetachThread(myThread);
			bListening = false;

			LoadCFG();
		}
		void OnDraw() {
			DrawBackdrop();
			DrawBoard();
			
			if(myGameState == TITLE) {
				Eternal::Rect r(0, 0, 400, 200);
				Eternal::Quad q; q.FromRect(r);
				myRenderer->SetColor(0,0,0,1);
				myRenderer->DrawQuad(q);
				
				
				Eternal::RGBA cols[3];
				for(int i = 0;i < 3;i++) {
					cols[i].r = cols[i].g = cols[i].b = cols[i].a = 255;
					if(i == iMenuCursor) {
						cols[i].r = 255;
						cols[i].g = 0;
						cols[i].b = 0;
					}
				}
				myFont.DrawString("SOLO PLAY", 32, 32, 4, cols[0].r, cols[0].g, cols[0].b);
				myFont.DrawString("CREATE GAME", 32, 64, 4, cols[1].r, cols[1].g, cols[1].b);
				myFont.DrawString("JOIN GAME", 32, 96, 4, cols[2].r, cols[2].g, cols[2].b);
				
			}
			else if(myGameState == GAMEPLAY) {
				DrawPieces();
				
				int selX = iSelectedSquareX;
				int selY = (bBoardFlipped) ? 7 - iSelectedSquareY : iSelectedSquareY;
				Eternal::Rect r(selX * SQUARE_SIZE, selY * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE);
				Eternal::Quad q;
				q.FromRect(r);
				myRenderer->SetColor(1,0,0,0.25f);
				myRenderer->DrawQuad(q);
				
				for(unsigned int i = 0; i < nodeList.size();i++) {
					r.x = nodeList[i].x * SQUARE_SIZE;
					r.y = nodeList[i].y * SQUARE_SIZE;
					if(bBoardFlipped) {
						r.y = (7 - nodeList[i].y) * SQUARE_SIZE;
					}
					r.w = (float)SQUARE_SIZE;
					r.h = (float)SQUARE_SIZE;
					myRenderer->SetColor(0,1.0f,0,0.25f);
					q.FromRect(r);
					myRenderer->DrawQuad(q);
				}
				nodeList.clear();
				if(heldPiece != nullptr) {
					nodeList = ListPossibleMoves(heldPiece);
				}

				if(iEndGameTimer > 0) {
					myFont.DrawString("MATE!", 232, 300, 8, 255, 0, 0);
					iEndGameTimer--;
					if(iEndGameTimer <= 0) {
						myGameState = TITLE;
						ResetPieces();
						if(serverSocket) {
							SDLNet_TCP_Close(serverSocket);
						}
						if(clientSocket) {
							SDLNet_TCP_Close(clientSocket);
						}
					}
				}
			}
			else if(myGameState == CONNECTING) {
				myFont.DrawString("Connecting...", 32, 96, 4);
			}
		}
		void OnUpdate() {
			if(myInputHandle->IsKeyDown(Eternal::InputHandle::KEY_ESCAPE)) {
				exit(0);
			}
			iSelectedSquareX = myInputHandle->GetMouseX() / SQUARE_SIZE;
			iSelectedSquareY = myInputHandle->GetMouseY() / SQUARE_SIZE;
			if(bBoardFlipped) {
				iSelectedSquareY = 7 - iSelectedSquareY;
			}

			if(myGameState == TITLE) {
				if(myInputHandle->IsKeyTap(Eternal::InputHandle::KEY_DOWN)) {
					iMenuCursor = (iMenuCursor == 2) ? 0 : iMenuCursor + 1;
				}
				else if(myInputHandle->IsKeyTap(Eternal::InputHandle::KEY_UP)) {
					iMenuCursor = (iMenuCursor == 0) ? 2 : iMenuCursor - 1;
				}
				if(myInputHandle->IsKeyTap(Eternal::InputHandle::KEY_START)) {
					switch(iMenuCursor) {
						case 0:
							myGameState = GAMEPLAY;
							myGameMode = HOT_SEAT;
						break;
						
						case 1:
							// Connect server
							myGameMode = NET_SERV;
							myGameState = CONNECTING;
							
							SDLNet_ResolveHost(&ip, NULL, 1234);
							serverSocket = SDLNet_TCP_Open(&ip);
							bBoardFlipped = false;
						break;
						
						case 2:
							// Connect client
							myGameMode = NET_CLIENT;
							myGameState = GAMEPLAY;
							
							SDLNet_ResolveHost(&ip, mySettingsMap["SERVER_ADDR"].c_str(), 1234);
							clientSocket = SDLNet_TCP_Open(&ip);
							if(!clientSocket) {
								myGameState = CONNECTING;
							}
							bBoardFlipped = true;
						break;
					};
				}
			}			
			else if(myGameState == GAMEPLAY) {
			
				if(myGameMode == NET_SERV && currentTurn == BLACK) {
					bListening = true;
					return;
				}
				else if(myGameMode == NET_CLIENT && currentTurn == WHITE) {
					bListening = true;
					return;
				}
				CheckMoves();


				// Look for checkmates
				if(IsWhiteKingInCheck()) {
					bool couldMove = false;
					for(unsigned int i = 0;i < pieces_White.size();i++) {
						piece_t originalPiece = pieces_White[i];
						piece_t &piece = pieces_White[i];
						std::vector<position_t> moves = ListPossibleMoves(&pieces_White[i]);
						for(unsigned int j = 0;j < moves.size();j++) {
							piece.x = moves[j].x;
							piece.y = moves[j].y;
							if(!IsWhiteKingInCheck()) {
								couldMove = true;
								break;
							}
						}
						piece = originalPiece; // Put the piece back after testing for moves
					}
					if(!couldMove && iEndGameTimer == 0) {
						iEndGameTimer = 60 * 5;
					}
				}
				else if(IsBlackKingInCheck()) {
					bool couldMove = false;
					for(unsigned int i = 0;i < pieces_Black.size();i++) {
						piece_t originalPiece = pieces_Black[i];
						piece_t &piece = pieces_Black[i];
						std::vector<position_t> moves = ListPossibleMoves(&pieces_Black[i]);
						for(unsigned int j = 0;j < moves.size();j++) {
							piece.x = moves[j].x;
							piece.y = moves[j].y;
							if(!IsBlackKingInCheck()) {
								couldMove = true;
								break;
							}
						}
						piece = originalPiece; // Put the piece back after testing for moves
					}
					if(!couldMove) {
						iEndGameTimer = 60 * 5;
					}
				}
			
				// remove dud pieces
				for(unsigned int i = 0;i < pieces_White.size();i++) {
					if(pieces_White[i].bTaken) {
						pieces_White.erase(pieces_White.begin() + i);
					}
				}
				for(unsigned int i = 0;i < pieces_Black.size();i++) {
					if(pieces_Black[i].bTaken) {
						pieces_Black.erase(pieces_Black.begin() + i);
					}
				}
			}
			else if(myGameState == CONNECTING && myGameMode == NET_SERV) {
				clientSocket = SDLNet_TCP_Accept(serverSocket);
				if(clientSocket) {
					std::cout << "connected..." << std::endl;
					myGameState = GAMEPLAY;
				}
			}
			else if(myGameState == CONNECTING && myGameMode == NET_CLIENT) {
				clientSocket = SDLNet_TCP_Open(&ip);
				if(clientSocket) {
					std::cout << "connected..." << std::endl;
					myGameState = GAMEPLAY;
				}
			}
			
/*			std::cout << "white king in check: " << IsWhiteKingInCheck() << std::endl;
			std::cout << "black king in check: " << IsBlackKingInCheck() << std::endl;*/
			
			bMouseWasDown = myInputHandle->IsMouseDown(Eternal::InputHandle::MouseButtons::MBUTTON_LEFT);
		}
		
		void PollClientForTurn() {
			char text[4];
			for(int i = 0;i < 4;i++) {
				text[i] = 0x00;
			}
			SDLNet_TCP_Recv(clientSocket, text, 4);
			if(text[0] != -1) {
				piece_t *piece = FindAnyPiece((int)text[0], (int)text[1]);
				PlacePiece(*piece, (int)text[2], (int)text[3]);
				sound_Move.Play(0);
			}
			NextTurn();
		}
		void PollServerForTurn() {
			char text[4];
			for(int i = 0;i < 4;i++) {
				text[i] = 0x00;
			}
			SDLNet_TCP_Recv(clientSocket, text, 4);
			
			if(text[0] != -1) {
				piece_t *piece = FindAnyPiece((int)text[0], (int)text[1]);
				PlacePiece(*piece, (int)text[2], (int)text[3]);
				sound_Move.Play(0);
			}
			
			NextTurn();
		}
		
	public:
		IPaddress ip;
		TCPsocket serverSocket;
		TCPsocket clientSocket;
		SDLNet_SocketSet mySocketSet;
		
	
		Eternal::Font myFont;
		GAME_STATE myGameState;
		int iMenuCursor;
	
};

App app;

/*if(myGameMode == NET_SERV && currentTurn == BLACK) {
					PollClientForTurn();
					return; // wait on client to send a turn
				}
				else if(myGameMode == NET_CLIENT && currentTurn == WHITE) {
					PollServerForTurn();
					return; // wait on server to send a turn
				}*/

int thread_main(void *nothing) {
	for(;;) {
		if(app.myGameState == GAMEPLAY && bListening == true) {
			if(app.myGameMode == NET_SERV && app.currentTurn == BLACK) {
				app.PollClientForTurn();
			}
			else if(app.myGameMode == NET_CLIENT && app.currentTurn == WHITE) {
				app.PollServerForTurn();
			}
		}
	}
	return 0;
}

int main(int argc, char *args[]) {
	app.Start(0,0,WIN_W,WIN_H);
	return 0;
}
