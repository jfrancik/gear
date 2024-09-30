#include "stdafx.h"
#include "MyGame.h"

#pragma warning (disable: 4244)

#define SPEED_PLAYER	100

#define SPEED_GUARD_0	120
#define SPEED_GUARD_1	80
#define SPEED_GUARD_2	80

char* CMyGame::m_tileLayout[12] =
{
	"XXXXXXXXXXXXXXXXXXXX",
	"X                  X",
	"X XXXX       XXXXX X",
	"X XXXX       XXXXX X",
	"X XXXX   XX  XXX   X",
	"  XXXX   XX  XXXXXXX",
	"X XXXX   XX  XXXXXXX",
	"X XXXX       XXXXXXX",
	"X XXXX XX XX XXXXX X",
	"X XXXX XX XX   XXX X",
	"X                  X",
	"XXXXXXXXXXXXXXXXXXXX",
};

CMyGame::CMyGame(void) :
	m_player(0, 0, "boy.png", 0)
{
	m_pKiller = NULL;
}

CMyGame::~CMyGame(void)
{
}

// MATHS! Intersection function
// Provides information about the interception point between the line segments: a-b  and c-d
// Returns true if the lines intersect (not necessarily  within the segments); false if they are parallel
// k1 (returned value): position of the intersection point along a-b direction:
//                      k1==0: at the point a; 0>k1>1: between a and b; k1==1: at b; k1<0 beyond a; k1>1: beyond b
// k2 (returned value): position of the intersection point along c-d direction
//                      k2==0: at the point c; 0>k2>1: between c and d; k2==1: at d; k2<0 beyond c; k2>1: beyond d
// Intersection point can be found as X = a + k1 * (b - a) = c + k2 (d - c)
bool Intersection(CVector a, CVector b, CVector c, CVector d, float &k1, float &k2)
{
	CVector v1 = b - a;
	CVector v2 = d - c;
	CVector con = c - a;
	float det = v1.m_x * v2.m_y - v1.m_y * v2.m_x;
	if (det != 0)
	{
		k1 = (v2.m_y * con.m_x - v2.m_x * con.m_y) / det;
		k2 = (v1.m_y * con.m_x - v1.m_x * con.m_y) / det;
		return true;
	}
	else
		return false;
}

// returns true is the line segments a-b and c-d intersect
bool Intersection(CVector a, CVector b, CVector c, CVector d)
{
	float k1, k2;
	if (!Intersection(a, b, c, d, k1, k2))
		return false;
	return k1 >= 0 && k1 <= 1.f && k2 >= 0 && k2 <= 1.f;
}


/////////////////////////////////////////////////////
// Per-Frame Callback Funtions (must be implemented!)

void CMyGame::OnUpdate()
{
	if (!IsGameMode()) return;

	Uint32 t = GetTime();

	// Player Control:a
	// The Player movement is contrained to the tile system.
	// Whilst moving, the player always heads towards the destination point at the centre of one of the neighbouring tiles - stored as m_dest.
	// Player control only activates when player either motionless or passed across the destination point (dest)
	if (m_player.GetSpeed() < 0.1 || m_player.GetSpeed() > 0.1 && Dot(m_dest - m_player.GetPosition(), m_player.GetVelocity()) < 0)
	{
		CVector newDir(0, 0);		// new direction - according to the keyboard input
		char *newAnim = "idle";		// new animation - according to the keyboard input
		
		if (IsKeyDown(SDLK_a))
		{
			newDir = CVector(-1, 0);
			newAnim = "walkL";
		}
		else if (IsKeyDown(SDLK_d))
		{
			newDir = CVector(1, 0);
			newAnim = "walkR";
		}
		else if (IsKeyDown(SDLK_w))
		{
			newDir = CVector(0, 1);
			newAnim = "walkU";
		}
		else if (IsKeyDown(SDLK_s))
		{
			newDir = CVector(0, -1);
			newAnim = "walkD";
		}

		// Collision test of the new heading point
		CVector newDest = m_dest + 64 * newDir;
		for (CSprite* pTile : m_tiles)
			if (pTile->HitTest(newDest))
				newDest = m_dest;	// no change of destination in case of a collision with a tile
		m_dest = newDest;

		// Set new velocity and new animation only if new direction different than current direction (dot product test)
		if (Dot(m_player.GetVelocity(), newDir) < 0.1)
		{
			m_player.SetVelocity(100 * newDir);
			m_player.SetAnimation(newAnim);
		}

		// a little bit of trickery to ensure the player is always alogned to the tiles
		m_player.SetPosition(64 * floorf(m_player.GetPosition().m_x / 64) + 32, 64 * floorf(m_player.GetPosition().m_y / 64) + 32);
	}
	m_player.Update(t);

	// Guards control
	if (m_guards[0]->GetPosition().m_x < 64 * 1 + 32)
	{
		m_guards[0]->SetAnimation("walkR");
		m_guards[0]->SetVelocity(CVector(SPEED_GUARD_0, 0));
	}
	if (m_guards[0]->GetPosition().m_x > 64 * 18 + 32)
	{
		m_guards[0]->SetAnimation("walkL");
		m_guards[0]->SetVelocity(CVector(-SPEED_GUARD_0, 0));
	}

	if (m_guards[1]->GetPosition().m_y > 64 * 9 + 32)
	{
		m_guards[1]->SetAnimation("walkD");
		m_guards[1]->SetVelocity(CVector(0, -SPEED_GUARD_1));
	}
	if (m_guards[1]->GetPosition().m_y < 64 * 2 + 32)
	{
		m_guards[1]->SetAnimation("walkU");
		m_guards[1]->SetVelocity(CVector(0, SPEED_GUARD_1));
	}

	if (m_guards[2]->GetPosition().m_x < 64 * 1 + 32)
	{
		m_guards[2]->SetAnimation("walkR");
		m_guards[2]->SetVelocity(CVector(SPEED_GUARD_2, 0));
	}
	if (m_guards[2]->GetPosition().m_x > 64 * 18 + 32)
	{
		m_guards[2]->SetAnimation("walkL");
		m_guards[2]->SetVelocity(CVector(-SPEED_GUARD_2, 0));
	}

	for (CSprite* pGuard : m_guards)
		pGuard->Update(GetTime());

	// LINE OF SIGHT TEST
	for (CSprite* pGuard : m_guards)
	{
		// by default, we assume each guard can become a killer
		m_pKiller = pGuard;

		// browse through all tiles - if line of sight test shows any tile to obscure the player, then we have no killer after all
		for (CSprite* pTile : m_tiles)
		{
			// Check intersection of the "Guard - Player" sight line with both diagonals of the tile.
			// If there is intersection - there is no killer - so, m_pKiller = NULL;

			// TO DO:
			// Call the Intersection function twice, once for each diagonal of the tile
			// If the function returns true in any case, call the following:
				m_pKiller = NULL;
			


			if (m_pKiller == NULL)
				break;	// small optimisation, if line of sight test already failed, no point to look further
		}

		// if the player is in plain sight of the guard...
		if (m_pKiller)
		{
			// Additional test - only killing if the player within 60 degrees from the guard's front (they have no eyes in the back of their head)
			CVector v = m_player.GetPosition() - pGuard->GetPosition();
			
			// TO DO: Calculate the Dot Product of the displacement vector (v - calculated above) and the guard's velocity vector.
			// Normalise both vectors for the dot!
			// If the result is greater than 0.5, the player is within 60 degrees from the front of the guard.
			// Otherwise, the guard should not see the player (again, m_pKiller = NULL)
		
		


		
		}
		
		// if still the killer found - the game is over and look no more!
		if (m_pKiller)
		{
			GameOver();
			return;
		}
	}

	// WINNING TEST
	if (m_player.GetLeft() < 0)
		GameOver();
}

void CMyGame::OnDraw(CGraphics* g)
{
	m_tiles.for_each(&CSprite::Draw, g);
	m_player.Draw(g);
	for (CSprite* pGuard : m_guards)
		pGuard->Draw(g);

	if (m_pKiller)
	{
		g->DrawLine(m_pKiller->GetPosition(), m_player.GetPosition(), 4, CColor::Red());
		*g << font(48) << color(CColor::Red()) << vcenter << center << "WASTED!" << endl;
	}
	else if (IsGameOver())
		*g << font(48) << color(CColor::DarkBlue()) << vcenter << center << "MISSION ACCOMPLISHED" << endl;
}

/////////////////////////////////////////////////////
// Game Life Cycle

// one time initialisation
void CMyGame::OnInitialize()
{
	// Create Tiles
	for (int y = 0; y < 12; y++)
		for (int x = 0; x < 20; x++)
		{
			if (m_tileLayout[y][x] == ' ')
				continue;

			int nTile = 5;
			if (y > 0 && m_tileLayout[y - 1][x] == ' ') nTile -= 3;
			if (y < 11 && m_tileLayout[y + 1][x] == ' ') nTile += 3;
			if (x > 0 && m_tileLayout[y][x - 1] == ' ') nTile--;
			if (x < 20 && m_tileLayout[y][x + 1] == ' ') nTile++;
			if (nTile == 5 && x > 0 && y > 0 && m_tileLayout[y - 1][x - 1] == ' ') nTile = 14;
			if (nTile == 5 && x < 20 && y > 0 && m_tileLayout[y - 1][x + 1] == ' ') nTile = 13;
			if (nTile == 5 && x > 0 && y < 11 && m_tileLayout[y + 1][x - 1] == ' ') nTile = 11;
			if (nTile == 5 && x < 20 && y < 11 && m_tileLayout[y + 1][x + 1] == ' ') nTile = 10;
			
			nTile--;
			m_tiles.push_back(new CSprite(x * 64.f + 32.f, y * 64.f + 32.f, new CGraphics("tiles.png", 3, 5, nTile % 3, nTile / 3), 0));
		}

	// Prepare the Player for the first use
	m_player.LoadAnimation("player.png", "walkR", CSprite::Sheet(13, 21).Row(9).From(0).To(8));
	m_player.LoadAnimation("player.png", "walkD", CSprite::Sheet(13, 21).Row(10).From(0).To(8));
	m_player.LoadAnimation("player.png", "walkL", CSprite::Sheet(13, 21).Row(11).From(0).To(8));
	m_player.LoadAnimation("player.png", "walkU", CSprite::Sheet(13, 21).Row(12).From(0).To(8));
	m_player.LoadAnimation("player.png", "idle",  CSprite::Sheet(13, 21).Row(13).From(0).To(0));  //  CSprite::Sheet(13, 21).Tile(0, 13));

	// Prepare the enemies for the first use
	for (int i = 0; i < 3; i++)
	{
		CSprite *pGuard = new CSprite(0, 0, "guard.png", 0);
		pGuard->LoadAnimation("guard.png", "walkR", CSprite::Sheet(13, 21).Row(9).From(0).To(8));
		pGuard->LoadAnimation("guard.png", "walkD", CSprite::Sheet(13, 21).Row(10).From(0).To(8));
		pGuard->LoadAnimation("guard.png", "walkL", CSprite::Sheet(13, 21).Row(11).From(0).To(8));
		pGuard->LoadAnimation("guard.png", "walkU", CSprite::Sheet(13, 21).Row(12).From(0).To(8));
		m_guards.push_back(pGuard);
	}
}

// called when a new game is requested (e.g. when F2 pressed)
// use this function to prepare a menu or a welcome screen
void CMyGame::OnDisplayMenu()
{
	StartGame();	// exits the menu mode and starts the game mode
}

// called when a new game is started
// as a second phase after a menu or a welcome screen
void CMyGame::OnStartGame()
{
	// Reinitialise the player
	m_player.SetPosition(64 * 18 + 32, 64 + 32);
	m_player.SetVelocity(0, 0);
	m_player.SetAnimation("idle");
	m_dest = m_player.GetPosition();

	// Reinitialise the guards
	m_guards[0]->SetPosition(64 * 17 + 32, 64 * 1 + 32);
	m_guards[0]->SetAnimation("walkL");
	m_guards[0]->SetVelocity(CVector(-SPEED_GUARD_0, 0));

	m_guards[1]->SetPosition(64 * 6 + 32, 64 * 2 + 32);
	m_guards[1]->SetAnimation("walkU");
	m_guards[1]->SetVelocity(CVector(0, SPEED_GUARD_1));

	m_guards[2]->SetPosition(64 * 1 + 32, 64 * 10 + 32);
	m_guards[2]->SetAnimation("walkR");
	m_guards[2]->SetVelocity(CVector(SPEED_GUARD_2, 0));

	m_pKiller = NULL;
}

// called when a new level started - first call for nLevel = 1
void CMyGame::OnStartLevel(Sint16 nLevel)
{
}

// called when the game is over
void CMyGame::OnGameOver()
{
}

// one time termination code
void CMyGame::OnTerminate()
{
}

/////////////////////////////////////////////////////
// Keyboard Event Handlers

void CMyGame::OnKeyDown(SDLKey sym, SDLMod mod, Uint16 unicode)
{
	if (sym == SDLK_F4 && (mod & (KMOD_LALT | KMOD_RALT)))
		StopGame();
	if (sym == SDLK_SPACE)
		PauseGame();
	if (sym == SDLK_F2)
		NewGame();


}

void CMyGame::OnKeyUp(SDLKey sym, SDLMod mod, Uint16 unicode)
{
}


/////////////////////////////////////////////////////
// Mouse Events Handlers

void CMyGame::OnMouseMove(Uint16 x,Uint16 y,Sint16 relx,Sint16 rely,bool bLeft,bool bRight,bool bMiddle)
{
}

void CMyGame::OnLButtonDown(Uint16 x, Uint16 y)
{
	CVector v(x, y);	// destination
}

void CMyGame::OnLButtonUp(Uint16 x,Uint16 y)
{
}

void CMyGame::OnRButtonDown(Uint16 x,Uint16 y)
{
}

void CMyGame::OnRButtonUp(Uint16 x,Uint16 y)
{
}

void CMyGame::OnMButtonDown(Uint16 x,Uint16 y)
{
}

void CMyGame::OnMButtonUp(Uint16 x,Uint16 y)
{
}
