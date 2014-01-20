/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2012 David Gow <david@ingeniumdigital.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "ck_def.h"
#include "ck_phys.h"
#include "ck_play.h"
#include "ck_act.h"
#include "ck5_ep.h"
#include "ck_def.h"
#include "id_rf.h"
#include "id_us.h"

#include <stdio.h>

extern int ck_ticsThisFrame;

#define ABS(x) ((x)>0?(x):(-(x)))

// Shikadi Mine Funcs
int CK5_Walk(CK_object *obj, CK_Controldir dir);

void CK5_SpawnMine(int tileX, int tileY)
{
	int i;
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 10; // ShikadiMine
	obj->active = OBJ_ACTIVE;
	obj->clipped = false;
	obj->posX = tileX * 0x100;
	obj->posY = tileY * 0x100 - 0x1F1;

	// X and Y offsets of the Dot relative to the mine
	obj->user2 = 0x100;
	obj->user3 = 0xD0;
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_Mine2"));
	obj->velX = 0x100;

	for (i = 0; i < 4; i++)
		if (CK5_Walk(obj, i))
			break;
	return;
}

/* Check a 3 x 2 square originating from (tileX, tileY)
 * If blocked, then return false
 */
int CK5_MinePathClear(int tileX, int tileY)
{

	int t, x, y;

	for (y = 0; y < 2; y++)
	{
		for (x = 0; x < 3; x++)
		{
			t = CA_TileAtPos(tileX + x, tileY + y, 1);
			if (TI_ForeTop(t) || TI_ForeBottom(t) || TI_ForeLeft(t)
					|| TI_ForeRight(t))
				return 0; //path is blocked
		}
	}
	return 1; // didn't hit anything

}

int CK5_Walk(CK_object *obj, CK_Controldir dir)
{

	int tx, ty;

	tx = (obj->posX + obj->nextX) >> 8;
	ty = (obj->posY + obj->nextY) >> 8;

	switch (dir)
	{

	case CD_north: //up
		if (CK5_MinePathClear(tx, ty - 1))
		{
			obj->xDirection = IN_motion_None;
			obj->yDirection = IN_motion_Up;
			return 1;
		}
		else
		{
			return 0;
		}

	case CD_east: //right
		if (CK5_MinePathClear(tx + 1, ty))
		{
			obj->xDirection = IN_motion_Right;
			obj->yDirection = IN_motion_None;
			return 1;
		}
		else
		{
			return 0;
		}
	case CD_south: //down
		if (CK5_MinePathClear(tx, ty + 1))
		{
			obj->xDirection = IN_motion_None;
			obj->yDirection = IN_motion_Down;
			return 1;
		}
		else
		{
			return 0;
		}
	case CD_west: //left
		if (CK5_MinePathClear(tx - 1, ty))
		{
			obj->xDirection = IN_motion_Left;
			obj->yDirection = IN_motion_None;
			return 1;
		}
		else
		{
			return 0;
		}
	}
	Quit("CK5_Walk: Bad Dir");
}

/*
 * Pick direction to chase keen
 */
void CK5_SeekKeen(CK_object *obj)
{

	// What is the point of the ordinal directions?
	// The mine only ever moves in cardinal directions.
	// Perhaps it was supposed to move in all eight?
	int mine_dirs[9] ={Dir_north, Dir_northwest, Dir_east, Dir_northeast,
		Dir_south, Dir_southeast, Dir_west, Dir_southwest, Dir_nodir};

	int i, deltaX, deltaY, principalDir, closestAxis, farthestAxis, cardinalDir;

	// Convert x and y motions into current controldirection
	if (obj->xDirection == IN_motion_Right)
		cardinalDir = CD_east;
	else if (obj->xDirection == IN_motion_Left)
		cardinalDir = CD_west;
	else if (obj->yDirection == IN_motion_Up)
		cardinalDir = CD_north;
	else if (obj->yDirection == IN_motion_Down)
		cardinalDir = CD_south;

	// Should this not be cardinalDir * 2?
	principalDir = mine_dirs[cardinalDir];

	// Determine which component (x or y) has the greatest absolute difference from keen
	// We want to move in that direction first?

	// Get position difference between Keen and Mine
	deltaX = ck_keenObj->posX - (obj->posX + obj->nextX);
	deltaY = ck_keenObj->posY - (obj->posY + obj->nextY);
	closestAxis = Dir_nodir;
	farthestAxis = Dir_nodir;

	if (deltaX > 0)
		farthestAxis = CD_east;
	if (deltaX < 0)
		farthestAxis = CD_west;
	if (deltaY > 0)
		closestAxis = CD_south;
	if (deltaY < 0)
		closestAxis = CD_north;

	if (ABS(deltaY) > ABS(deltaX))
	{
		int s = farthestAxis;
		farthestAxis = closestAxis;
		closestAxis = s;
	}

	// If one of the intended components is already the mine's movement
	// Then there's no need to check twice
	if (farthestAxis == principalDir)
		farthestAxis = Dir_nodir;
	if (closestAxis == principalDir)
		closestAxis = Dir_nodir;

	// Check if there's free space ahead first in the desired directions
	// and then finally in the current direction
	if (closestAxis != Dir_nodir && CK5_Walk(obj, closestAxis))
		return;
	if (farthestAxis != Dir_nodir && CK5_Walk(obj, farthestAxis))
		return;
	if (CK5_Walk(obj, cardinalDir))
		return;

	// Otherwise, look for some free space
	if (US_RndT() > 0x80)
	{
		for (i = 0; i <= 3; i++)
			if (i != principalDir)
				if (CK5_Walk(obj, i))
					return;
	}
	else
	{
		for (i = 3; i >= 0; i--)
			if (i != principalDir)
				if (CK5_Walk(obj, i))
					return;
	}

	// Finally, just keep going forward
	CK5_Walk(obj, principalDir);
	return;
}

#define SOUND_MINEEXPLODE 5

void CK5_MineMove(CK_object *obj)
{

	int deltaX, deltaY, delta, xDir, yDir;

	// Get distance to keen
	deltaX = obj->posX - ck_keenObj->posX;
	deltaY = obj->posY - ck_keenObj->posY;

	// Check if Mine should explode
	if (deltaX < 0x200 && deltaX > -0x500 && deltaY < 0x300 && deltaY > -0x50)
	{
		// TODO: Enable this
		//SD_PlaySound(SOUND_MINEEXPLODE);
		obj->currentAction = CK_GetActionByName("CK5_ACT_MineExplode0");
		RF_RemoveSpriteDraw((RF_SpriteDrawEntry **) & obj->user4);
		return;
	}

	// Move the mine to the next tile boundary
	// obj->velX is used as a ticker
	// When the ticker reaches zero, check if directional change needed
	delta = CK_GetTicksPerFrame() * 10;
	if (obj->velX <= delta)
	{
		// Move up to the tile boundary
		obj->nextX = obj->xDirection * obj->velX;
		obj->nextY = obj->yDirection * obj->velX;
		delta -= obj->velX;
		xDir = obj->xDirection;
		yDir = obj->yDirection;

		// Switch to the changing direction action if necessary
		CK5_SeekKeen(obj);
		obj->velX = 0x100;
		if (obj->xDirection != xDir || obj->yDirection != yDir)
		{
			obj->currentAction = CK_GetActionByName("CK5_ACT_Mine1");
			return;
		}
	}

	// Tick down velX and move mine
	obj->velX -= delta;
	obj->nextX += delta * obj->xDirection;
	obj->nextY += delta * obj->yDirection;
	return;
}

void CK5_MineCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
		CK_ShotHit(o2);
	return;
}

void CK5_MineShrapCol(CK_object *o1, CK_object *o2)
{

	// Explode stunner
	if (o2->type == CT_Stunner)
	{
		CK_ShotHit(o2);
		return;
	}

	// Kill keen
	if (o2->type == CT_Player)
	{
		//KeenDie();
		return;
	}

	//blow up QED
	if (o2->type == 0x19)
	{
		// TODO: implement this
		/*
			 FuseExplosionSpawn(o2->clipRects.tileX1, o2->clipRects.tileY1);
			 FuseExplosionSpawn(o2->clipRects.tileX2, o2->clipRects.tileY1);
			 FuseExplosionSpawn(o2->clipRects.tileX1, o2->clipRects.tileY2);
			 FuseExplosionSpawn(o2->clipRects.tileX2, o2->clipRects.tileY2);
			 RF_ReplaceTileBlock(0, 0, 0x10, 0xB, 4, 2);
			 RF_ReplaceTileBlock(4, 0, 0x10, 0xD, 4, 2);
			 LevelEndSpawn();
		 *
		 */
		CK_RemoveObj(o2);
	}

	return;
}

void CK5_MineMoveDotsToCenter(CK_object *obj)
{

	int deltaX, deltaY;
	int dotOffsetX, dotOffsetY;

	deltaX = obj->posX - ck_keenObj->posX;
	deltaY = obj->posY - ck_keenObj->posY;

	// Blow up if keen is nearby
	if (deltaX < 0x200 && deltaX > -0x300 && deltaY < 0x300
			&& deltaY > -0x300)
	{
		//SD_PlaySound(SOUND_MINEEXPLODE);
		obj->currentAction = CK_GetActionByName("CK5_ACT_MineExplode0");
		RF_RemoveSpriteDraw((RF_SpriteDrawEntry **) & obj->user4);
		return;
	}

	obj->visible = true;
	dotOffsetX = 0x100;
	dotOffsetY = 0xD0;

	// Move Dot to the center, then change Action
	if (obj->user2 < dotOffsetX)
	{
		obj->user2 += CK_GetTicksPerFrame() * 4;
		if (obj->user2 >= dotOffsetX)
		{
			obj->user2 = dotOffsetX;
			obj->currentAction = obj->currentAction->next;
		}
	}
	else if (obj->user2 > dotOffsetX)
	{
		obj->user2 -= CK_GetTicksPerFrame() * 4;
		if (obj->user2 <= dotOffsetX)
		{
			obj->user2 = dotOffsetX;
			obj->currentAction = obj->currentAction->next;
		}
	}

	// Do the same in the Y direction
	if (obj->user3 < dotOffsetY)
	{
		obj->user3 += CK_GetTicksPerFrame() * 4;
		if (obj->user3 >= dotOffsetY)
		{
			obj->user3 = dotOffsetY;
			obj->currentAction = obj->currentAction->next;
		}
	}
	else if (obj->user3 > dotOffsetY)
	{
		obj->user3 -= CK_GetTicksPerFrame() * 4;
		if (obj->user3 <= dotOffsetY)
		{
			obj->user3 = dotOffsetY;
			obj->currentAction = obj->currentAction->next;
		}
	}

	return;
}

void CK5_MineMoveDotsToSides(CK_object *obj)
{

	int deltaX, deltaY, dotOffsetX, dotOffsetY;

	deltaX = obj->posX - ck_keenObj->posX;
	deltaY = obj->posY - ck_keenObj->posY;

	// Explode if Keen is nearby
	if (deltaX < 0x200 && deltaX > -0x300 && deltaY < 0x300
			&& deltaY > -0x300)
	{
		//SD_PlaySound(SOUND_MINEEXPLODE);
		obj->currentAction = CK_GetActionByName("CK5_ACT_MineExplode0");
		RF_RemoveSpriteDraw((RF_SpriteDrawEntry **) & obj->user4);
		return;
	}

	obj->visible = 1;

	switch (obj->xDirection)
	{

	case IN_motion_Left:
		dotOffsetX = 0x80;
		break;
	case IN_motion_None:
		dotOffsetX = 0x100;
		break;
	case IN_motion_Right:
		dotOffsetX = 0x180;
		break;
	default:
		break;
	}

	switch (obj->yDirection)
	{

	case IN_motion_Up:
		dotOffsetY = 0x50;
		break;
	case IN_motion_None:
		dotOffsetY = 0xD0;
		break;
	case IN_motion_Down:
		dotOffsetY = 0x150;
		break;
	default:
		break;
	}

	// Move the dot and change action
	// when it reaches the desired offset
	if (obj->user2 < dotOffsetX)
	{
		obj->user2 += CK_GetTicksPerFrame() * 4;
		if (obj->user2 >= dotOffsetX)
		{
			obj->user2 = dotOffsetX;
			obj->currentAction = obj->currentAction->next;
		}
	}
	else if (obj->user2 > dotOffsetX)
	{
		obj->user2 -= CK_GetTicksPerFrame() * 4;
		if (obj->user2 <= dotOffsetX)
		{
			obj->user2 = dotOffsetX;
			obj->currentAction = obj->currentAction->next;
		}
	}

	// Do the same in the Y direction
	if (obj->user3 < dotOffsetY)
	{
		obj->user3 += CK_GetTicksPerFrame() * 4;
		if (obj->user3 >= dotOffsetY)
		{
			obj->user3 = dotOffsetY;
			obj->currentAction = obj->currentAction->next;
		}
	}
	else if (obj->user3 > dotOffsetY)
	{
		obj->user3 -= CK_GetTicksPerFrame() * 4;
		if (obj->user3 <= dotOffsetY)
		{
			obj->user3 = dotOffsetY;
			obj->currentAction = obj->currentAction->next;
		}
	}

	return;
}

void CK5_MineExplode(CK_object *obj)
{

	CK_object *new_object;
	//SD_PlaySound(SOUND_MINEEXPLODE);

	// upleft
	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX;
	new_object->posY = obj->posY;
	new_object->velX = -US_RndT() / 8;
	new_object->velY = -48;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	// upright
	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX + 0x100;
	new_object->posY = obj->posY;
	new_object->velX = US_RndT() / 8;
	new_object->velY = -48;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX;
	new_object->posY = obj->posY;
	new_object->velX = US_RndT() / 16 + 40;
	new_object->velY = -24;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX + 0x100;
	new_object->posY = obj->posY;
	new_object->velX = -40 - US_RndT() / 16;
	new_object->velY = -24;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX;
	new_object->posY = obj->posY;
	new_object->velX = 24;
	new_object->velY = 16;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	new_object = CK_GetNewObj(true);
	new_object->posX = obj->posX + 0x100;
	new_object->posY = obj->posY;
	new_object->velX = 24;
	new_object->velY = 16;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MineShrap0"));

	return;

}

void CK5_MineTileCol(CK_object *obj)
{

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0,
									obj->zLayer); //mine
	RF_AddSpriteDraw((RF_SpriteDrawEntry **) & obj->user4, obj->posX + obj->user2,
									obj->posY + obj->user3, 0x17B, 0, 2); //dot
	return;
}

/*
 * Tile collision for making the mine and shelley bits bounce
 * This was originally placed in CK5_MISC.C, but it makes more sense
 * to write it here
 */

void CK5_ShrapnelTileCol(CK_object *obj)
{
	int bouncePower; // is long in keen source
	int topflags_0, velY, absvelX;
	int d, motion;

	int bouncearray[8][8] ={
		{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
		{ 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0},
		{ 0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0xF, 0xE},
		{ 0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0xF, 0xE},
		{ 0x3, 0x2, 0x1, 0x0, 0xF, 0xE, 0xD, 0xC},
		{ 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2},
		{ 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2},
		{ 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4},
	};

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);


	// Bounce backwards if a side wall is hit
	if (obj->leftTI || obj->rightTI)
	{
		obj->velX = -obj->velX / 2;
	}

	// Bounce down if a top wall is hit
	if (obj->bottomTI)
	{
		obj->velY = -obj->velY / 2;
		return;
	}

	// Bounce sideways if a slope floor is hit
	topflags_0 = obj->topTI;
	if (!topflags_0)
		return;

	if (obj->velY < 0)
		obj->velY = 0;

	absvelX = ABS(obj->velX);
	velY = obj->velY;

	if (absvelX > velY)
	{
		if (absvelX > velY * 2)
		{
			d = 0;
			bouncePower = absvelX * 286;
		}
		else
		{
			d = 1;
			bouncePower = absvelX * 362;
		}
	}
	else
	{
		if (velY > absvelX * 2)
		{
			d = 3;
			bouncePower = velY * 256;
		}
		else
		{
			d = 2;
			bouncePower = velY * 286;
		}
	}

	if (obj->velX > 0) //mirror around y-axis
		d = 7 - d;

	bouncePower /= 2; // absorb energy

	motion = bouncearray[obj->topTI][d];

	switch (motion)
	{
	case 0:
		obj->velX = bouncePower / 286;
		obj->velY = -obj->velX / 2;
		break;
	case 1:
		obj->velX = bouncePower / 362;
		obj->velY = -obj->velX;
		break;
	case 2:
		obj->velY = -(bouncePower / 286);
		obj->velX = -obj->velY / 2;
		break;
	case 3:
	case 4:
		obj->velX = 0;
		obj->velY = -(bouncePower / 256);
		break;
	case 5:
		obj->velY = -(bouncePower / 286);
		obj->velX = obj->velY / 2;
		break;
	case 6:
		obj->velY = -(bouncePower / 362);
		obj->velX = obj->velY;
		break;
	case 7:
		obj->velX = -(bouncePower / 286);
		obj->velY = obj->velX / 2;
		break;
	case 8:
		obj->velX = -(bouncePower / 286);
		obj->velY = -obj->velX / 2;
		break;
	case 9:
		obj->velX = -(bouncePower / 362);
		obj->velY = -obj->velX;
		break;
	case 0xA:
		obj->velY = bouncePower / 286;
		obj->velX = -obj->velY / 2;
		break;
	case 0xB:
	case 0xC:
		obj->velX = 0;
		obj->velY = -(bouncePower / 256);
		break;
	case 0xD:
		obj->velY = bouncePower / 286;
		obj->velX = obj->velY / 2;
		break;
	case 0xE:
		obj->velX = bouncePower / 362;
		obj->velY = bouncePower / 362;
		break;
	case 0xF:
		obj->velX = bouncePower / 256;
		obj->velY = obj->velX / 2;
		break;
	default: break;
	}

	// if speed is lower than threshhold, then disappear
	if (bouncePower < 0x1000)
	{
		// CK_SetAction2(obj, obj->currentAction->next);
		// This is a mistake in the .exe... we can't advance to a NULL
		// action, because omnispeak will segfault (Keen5 wouldn't)
		// Just remove it instead
		RF_RemoveSpriteDraw(&obj->sde);
		CK_RemoveObj(obj);
	}

}

void CK5_SpawnRobo(int tileX, int tileY)
{

	CK_object *obj = CK_GetNewObj(false);
	obj->type = CT_Robo;
	obj->active = OBJ_ACTIVE;
	obj->posX = tileX << 8;
	obj->posY = (tileY << 8) - 0x400;
	obj->xDirection = US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left;
	obj->yDirection = IN_motion_Down;
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_Robo0"));
}

void CK5_RoboMove(CK_object *obj)
{

	// Check for shot opportunity
	if ((obj->posX & 0x40) && ck_keenObj->clipRects.unitY2 > obj->clipRects.unitY1 && ck_keenObj->clipRects.unitY1 < obj->clipRects.unitY2 &&
			US_RndT() < 0x10)
	{
		obj->xDirection = obj->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		obj->user1 = 10;
		obj->currentAction = CK_GetActionByName("CK5_ACT_RoboShoot0");
	}
}
// time is number of shots

void CK5_RoboCol(CK_object *o1, CK_object *o2)
{

	if (o2-> type == CT_Stunner)
	{
		CK_ShotHit(o2);
		o1->xDirection = o1->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;
		o1->user1 = 10;
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_RoboShoot0"));
		return;
	}
	if (o2->type == CT_Player)
		; //KeenDie(o2);
}

#define SOUND_ROBOSHOOT 0x1D

void CK5_RoboShoot(CK_object *obj)
{

	CK_object *new_object;
	int shotSpawnX;

	if (--obj->user1 == 0)
		obj->currentAction = CK_GetActionByName("CK5_ACT_Robo0");

	shotSpawnX = obj->xDirection == IN_motion_Right ? obj->posX + 0x380 : obj->posX;

	if (new_object = CK5_SpawnEnemyShot(shotSpawnX, obj->posY + 0x200, CK_GetActionByName("CK5_ACT_RoboShot0")))
	{
		new_object->velX = obj->xDirection * 60;
		new_object->velY = obj->user1 & 1 ? -8 : 8;
		//SD_PlaySound(SOUND_ROBOSHOOT);
		obj->nextX = obj->xDirection == IN_motion_Right ? -0x40 : 0x40;
	}
}

void CK5_RoboShotCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Player)
	{
		//KeenDie();
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_RoboShotHit0"));
	}
}

#define SOUND_ROBOSHOTHIT 0x1E

void CK5_RoboShotTileCol(CK_object *obj)
{

	if (obj->topTI || obj->rightTI || obj->bottomTI || obj->leftTI)
	{
		//SD_PlaySound(SOUND_ROBOSHOTHIT);
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_RoboShotHit0"));
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, false, obj->zLayer);
}

// Spirogrip Funcs

void CK5_SpawnSpirogrip(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 13; // Spirogrip
	obj->active = true;
	obj->posX = (tileX << 8);
	obj->posY = (tileY << 8) - 256;

	obj->xDirection = -1; // Left
	obj->yDirection = 1; //Down

	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_SpirogripSpin1"));
}

void CK5_SpirogripSpin(CK_object *obj)
{
	// If we're bored of spinning...
	if (US_RndT() > 20)
		return;

	// TODO: Play sound (0x3D)

	// and we're in the right direction, fly!
	if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin1"))
		obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyUp");
	else if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin3"))
		obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyRight");
	else if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin5"))
		obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyDown");
	else if (obj->currentAction == CK_GetActionByName("CK5_ACT_SpirogripSpin7"))
		obj->currentAction = CK_GetActionByName("CK5_ACT_SpirogripFlyLeft");

}

void CK5_SpirogripFlyDraw(CK_object *obj)
{
	// Draw the sprite
	RF_AddSpriteDraw(&(obj->sde), obj->posX, obj->posY, obj->gfxChunk, 0,
									obj->zLayer);

	// Check if we've collided with a tile
	if (obj->topTI || obj->rightTI || obj->bottomTI || obj->leftTI)
	{
		obj->currentAction = obj->currentAction->next;
		//TODO: Play sound (0x1B)
	}
}

void CK5_SpawnSpindred(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT_Spindred;
	new_object->active = OBJ_ACTIVE;
	new_object->zLayer = 0;
	new_object->posX = (tileX << 8);
	new_object->posY = (tileY << 8) - 0x80;
	new_object->yDirection = IN_motion_Down;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Spindred0"));
}

void CK5_SpindredBounce(CK_object *obj)
{

	int timedelta;


	for (timedelta = CK_GetNumTotalTics() - CK_GetTicksPerFrame(); timedelta < CK_GetNumTotalTics();  timedelta++)
	{

		if (timedelta & 1)
		{
			if (obj->yDirection == IN_motion_Down)
			{
				if (obj->velY < 0 && obj->velY >= -3)
				{
					obj->nextY += obj->velY;
					obj->velY = 0;
					return;
				}

				if ((obj->velY += 3) > 70)
					obj->velY = 70;
			}
			else
			{
				if (obj->velY > 0 && obj->velY <= 3)
				{
					obj->nextY += obj->velY;
					obj->velY = 0;
					return;
				}
				if ((obj->velY -= 3) < -70)
					obj->velY = -70;
			}
		}
		obj->nextY += obj->velY;

	}
}

#define SOUND_SPINDREDFLYDOWN 0x25
#define SOUND_SPINDREDFLYUP 0x10
#define SOUND_SPINDREDSLAM 0x1C

void CK5_SpindredTileCol(CK_object *obj)
{

	if (obj->bottomTI)
	{
		obj->velY = 0;
		if (obj->yDirection == IN_motion_Up)
		{
			// Reverse directions after three slams
			if (++obj->user1 == 3)
			{
				obj->user1 = 0;
				obj->velY = 68;
				obj->yDirection = -obj->yDirection;
				//SD_PlaySound(SOUND_SPINDREDFLYDOWN); // fly down
			}
			else
			{
				//SD_PlaySound(SOUND_SPINDREDSLAM); // slam once
				obj->velY = 40;
			}
		}
	}

	if (obj->topTI)
	{
		obj->velY = 0;
		if (obj->yDirection == IN_motion_Down)
		{
			if (++obj->user1 == 3)
			{ //time holds slamcount
				obj->user1 = 0;
				obj->velY = -68;
				obj->yDirection = -obj->yDirection;
				//SD_PlaySound(SOUND_SPINDREDFLYUP); // fly down
			}
			else
			{
				//SD_PlaySound(SOUND_SPINDREDSLAM); // slam once
				obj->velY = -40;
			}
		}
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

#if 0

void CK5_MasterSpawn(int tileX, int tileY)
{

	GetNewObj(0);
	new_object->type = 19;
	new_object->active = 1;
	new_object->posX = TILE2MU(tileX);
	new_object->posY = TILE2MU(tileY) - 0x180;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_MASTER0"));
	return;
}

void CK5_MasterStand(CK_object *o)
{

	if (US_RndT() < 0x40) return;

	if (obj->user1 != 0)
	{
		obj->currentAction =CK_GetActionByName("CK5_ACT_MASTERTELE0");
		obj->user1 = 0;
		return;
	}

	obj->user1 = 1;
	obj->xDirection = ck_keenObj->posX > obj->posX ? IN_motion_Right : motion_Left;
	obj->currentAction =CK_GetActionByName("CK5_ACT_MASTERBALL0");
	return;
}

#define SOUND_MASTERSHOT 0x26

void CK5_MasterShoot(CK_object *o)
{

	d = obj->xDirection == IN_motion_Right ? obj->posX : 0x100 + obj->posX;
	if (SpawnEnemyShot(d, obj->posY + 0x80, CK_GetActionByName("CK5_ACT_MASTERBALL0")) != -1)
	{
		new_object->velX = obj->xDirection * 48;
		new_object->velY = -16;
		//SD_PlaySound(SOUND_MASTERSHOT);
	}
	return;
}

// time is shooting flag

void CK5_MasterCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
	{
		StunnerHits(o2);
		obj->xDirection = ck_keenObj->posX - o1->posX ? IN_motion_Right : motion_Left;
		obj->user1 = 1;
		CK_SetAction2(o1, CK5_ACT_MASTERSHOOT0);
		return;
	}

	if (o2->type == CT_Player) KeenDie();
	return;
}
#define SOUND_MASTERTELE 0x27

void CK5_MasterTele(CK_object *o)
{

	int posX_0, posY_0, tx, ty, tx2, ty2, tries;
	int _far *t;

	posX_0 = obj->posX;
	posY_0 = obj->posY;

	GetNewObj(1);
	new_object->posX = obj->posX;
	new_object->posY = obj->posY;
	obj->velX = 0x30;
	CK_SetAction(new_object, CK5_ACT_MASTERSPARKS0);

	GetNewObj(1);
	new_object->posX = obj->posX;
	new_object->posY = obj->posY;
	obj->velX = -0x30;
	CK_SetAction(new_object, CK5_ACT_MASTERSPARKS0);

	//SD_PlaySound(SOUND_MASTERTELE);

	tries = 0;

	while (++tries < 10)
	{
tryagain:
		tx = US_RndT() / 8 + ck_keenObj->clipRects.tileXmid - 0x10;
		ty = US_RndT() / 8 + ck_keenObj->clipRects.tileY2   - 0x10;

		if (ty < 2 || tx < 2 || map_width_T - 5 < tx || map_height_T - 5 < ty) continue;

		obj->posX = TILE2MU(tx);
		obj->posY = TILE2MU(ty);
		obj->clipRects.tileX1 = tx - 1;
		obj->clipRects.tileX2 = tx + 4;
		obj->clipRects.tileY1 = ty - 1;
		obj->clipRects.tileY2 = ty + 4;

		t = (obj->clipRects.tileX1, obj->clipRects.tileY1, FGPLANE);
		mapXT = map_width_T - (obj->clipRects.tileX2 - obj->clipRects.tileX1 + 1);
		for (ty2 = obj->clipRects.tileY1; ty2 <= obj->clipRects.tileY2; ty2++)
		{
			for (tx2 = obj->clipRects.tileX2; tx2 <= obj->clipRects.tileX2; tx2++)
			{
				tile_num = *(t++);
				if (TI_FGMiscFlags[tile_num] & 0x80 || TI_FGTopFlags[tile_num] ||
						TI_FGRightFlags[tile_num] || TI_FGLeftFlags[tile_num] ||
						TI_FGBottomFlags[tile_num])
				{
					// don't spawn inside a tile, or behind a hidden tile
					goto tryagain;
				}
			}
		}
		// make it through previous nested loop == succesful tele

		obj->nextX = obj->nextY = 0;
		return;
	}

	// couldn't find a suitable spawn point, so reset to default master behaviour
	US_RndT();
	obj->currentAction =CK_GetActionByName("CK5_ACT_MASTER0");
	obj->posX = posX_0 - 1;
	obj->posY = posY_0;
	obj->nextX = 1;
	obj->nextY = 0;
	return;
}

void CK5_MasterBallCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Stunner)
	{
		StunnerHits(o2);
		CK_RemoveObj(o1);
		return;
	}

	if (o2->type == CT_Player)
	{
		KeenDie();
		CK_RemoveObj(o1);
	}
	return;
}
#define SOUND_MASTERSPARKSSPAWN 0x26

void CK5_MasterBallTileCol(CK_object *o)
{

	if (obj->rightTI || obj->leftTI) obj->velX = -obj->velX;
	if (obj->bottomTI) obj->velY = 0;
	if (obj->topTI)
	{
		//turn ball into sparks, spawn sparks in reverse direction
		//SD_PlaySound(SOUND_MASTERSPARKSSPAWN);
		obj->velX = 48;
		CK_SetAction2(o, CK_GetActionByName("CK5_ACT_MASTERSPARKS0"));
		GetNewObj(1);
		new_object->posX = obj->posX;
		new_object->posY = obj->posY;
		new_object->velX = -48;
		CK_SetAction(CK5_ACT_MASTERSPARKS0);
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
	return;
}

void CK5_MasterSparksTileCol(CK_object *o)
{

	if (obj->rightTI || obj->leftTI)
	{
		CK_RemoveObj(o);
		return;
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
	return;
}

#endif

void CK5_SpawnShikadi(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT_Shikadi;
	new_object->active = OBJ_ACTIVE;
	new_object->posX = (tileX << 8);
	new_object->posY = (tileY << 8) - 0x100;
	new_object->user2 = 4;
	new_object->xDirection = (US_RndT() < 0x80 ? IN_motion_Right : IN_motion_Left);
	new_object->yDirection = IN_motion_Down;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_ShikadiStand0"));
}
#define SOUND_POLEZAP 0x28
#define MISCFLAG_POLE 1

extern void CK_KeenSpecialColFunc(CK_object *, CK_object*);

void CK5_ShikadiWalk(CK_object *obj)
{
	int tx, tile;

	// By casting to unsigned, we also check if the difference in 
	// Y values is greater than zero
	if (ck_keenObj->currentAction->collide == &CK_KeenSpecialColFunc || (unsigned) (obj->clipRects.unitY2 - ck_keenObj->clipRects.unitY2 + 0x100) <= 0x200)
	{
		// if Keen on Pole or Keen at shikadi level , get to keen
		// walk back and forth one tile to the left or right underneath him
		if (obj->posX > ck_keenObj->posX + 0x100)
			obj->xDirection = IN_motion_Left;
		else if (obj->posX < ck_keenObj->posX - 0x100)
			obj->xDirection = IN_motion_Right;

		tx = obj->xDirection == IN_motion_Right ? obj->clipRects.tileX2 : obj->clipRects.tileX1;

		if (ck_keenObj->clipRects.tileXmid != tx)
			return; // no zapping if not below keen

	}
	else
	{
		if (US_RndT() < 0x10)
		{
			// 1/16 chance of stopping walk
			obj->nextX = 0;
			obj->currentAction = CK_GetActionByName("CK5_ACT_ShikadiStand0");
			return;
		}

		//if at tile boundary (i.e., ready for zap
		if ((obj->posX & 0xFF) || !CK_ObjectVisible(obj))
			return;

		tx = obj->xDirection == IN_motion_Right ? obj->clipRects.tileX2 : obj->clipRects.tileX1;
	}

	// Make a pole zap
	tile = CA_TileAtPos(tx, obj->clipRects.tileY1, 1);
	//tile = *MAPSPOT(tx, obj->clipRects.tileY1, FGPLANE);
	if (TI_ForeMisc(tile) == MISCFLAG_POLE)
	{
		obj->user1 = tx;
		obj->currentAction = CK_GetActionByName("CK5_ACT_ShikadiPole0");
		obj->nextX = 0;
		//SD_PlaySound(SOUND_POLEZAP);
	}
}


// int33 is white flash duration
// state = health

void CK5_ShikadiCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Player)
	{
		//KeenDie();
	}

	if (o2->type == CT_Stunner)
	{
		if (--o1->user2 == 0)
		{
			o1->user3 = 2;
			o1->visible = true;
			CK_ShotHit(o2);
		}
		else
		{
			o1->velX = o1->velY = 0;
			//StunCreature(o1, o2, CK_GetActionByName("CK5_ACT_SHIKADISTUN0"));
		}
	}
}

void CK5_ShikadiPole(CK_object *obj)
{
	int tileX;
	CK_object *new_object;

	obj->timeUntillThink = 2;
	tileX = obj->xDirection == IN_motion_Right ? obj->clipRects.tileX2 << 8 : obj->clipRects.tileX1 << 8;
	new_object = CK_GetNewObj(true);
	new_object->posX = tileX;
	new_object->posY = obj->posY + 0x80;
	new_object->type = CT_EnemyShot;
	new_object->active = OBJ_EXISTS_ONLY_ONSCREEN;
	new_object->clipped = false;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_PoleZap0"));
	new_object->yDirection = obj->posY > ck_keenObj->posY ? IN_motion_Up : IN_motion_Down;
	//SD_PlaySound(SOUND_POLEZAP);
}

void CK5_PoleZap(CK_object *obj)
{

	int tile;
	if (obj->nextY == 0)
	{
		// Slide up until there's no more pole
		obj->nextY = obj->yDirection * 48;
		tile = CA_TileAtPos(obj->clipRects.tileXmid, obj->clipRects.tileY1, 1);
		if (TI_ForeMisc(tile) != MISCFLAG_POLE)
			obj->currentAction = NULL;
	}
}

void CK5_ShikadiTileCol(CK_object *obj)
{

	if (obj->xDirection == IN_motion_Right && obj->leftTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
	else if (obj->xDirection == IN_motion_Left && obj->rightTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
	else if (!obj->topTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -obj->xDirection;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}

	//if flashing white
	if (obj->user3 != 0)
	{
		obj->user3--;
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 1, obj->zLayer);
	}
	else
	{
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
	}
	return;
}

void CK5_SpawnShocksund(int tileX, int tileY)
{

	CK_object *new_object = CK_GetNewObj(false);
	new_object->type = CT_Shocksund;
	new_object->active = OBJ_ACTIVE;
	new_object->posX = (tileX << 8);
	new_object->posY = (tileY << 8) - 0x80;
	new_object->user2 = 2;
	new_object->xDirection = IN_motion_Right;
	new_object->yDirection = IN_motion_Down;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_Shocksund0"));
}

void CK5_ShocksundSearch(CK_object *obj)
{

	obj->xDirection = obj->posX > ck_keenObj->posX ? IN_motion_Left : IN_motion_Right;

	if (US_RndT() < 0x10)
	{
		obj->nextX = 0;
		obj->currentAction = CK_GetActionByName("CK5_ACT_ShocksundStand0");
		obj->user1 = 0x10;
		return;
	}

	if (obj->clipRects.unitY2 != ck_keenObj->clipRects.unitY2)
	{

		obj->currentAction = CK_GetActionByName("CK5_ACT_ShocksundJump0");
		obj->velX = obj->xDirection == IN_motion_Right ? 40 : -40;
		obj->velY = -40;
	}

	if (US_RndT() < 0x80)
	{
		obj->nextX = 0;
		obj->currentAction = CK_GetActionByName("CK5_ACT_ShocksundShoot0");
	}
}

void CK5_ShocksundStand(CK_object *obj)
{

	if (--obj->user1 == 0)
		obj->currentAction = CK_GetActionByName("CK5_ACT_Shocksund0");
}

#define SOUND_SHOCKSUNDBARK 0x2A

void CK5_ShocksundShoot(CK_object *obj)
{
	CK_object *new_object;
	int posX = obj->xDirection == IN_motion_Right ? obj->posX + 0x70 : obj->posX;

	if (new_object = CK5_SpawnEnemyShot(posX, obj->posY + 0x40, CK_GetActionByName("CK5_ACT_BarkShot0")))
	{
		new_object->velX = obj->xDirection * 60;
		new_object->xDirection = obj->xDirection;
		//SD_PlaySound(SOUND_SHOCKSUNDBARK);
	}
}

void CK5_ShocksundCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Player)
	{
		//KeenDie();
	}

	if (o2->type == CT_Stunner)
	{
		if (--o1->user2 == 0)
		{
			o1->user3 = 2;
			o1->visible = true;
			CK_ShotHit(o2);
		}
		else
		{
			o1->velX = o1->velY = 0;
			//StunCreature(o1, o2, CK_GetActionByName("CK5_ACT_SHOCKSUNDSTUN0"));
		}
	}
	return;
}

void CK5_ShocksundGroundTileCol(CK_object *obj)
{

	if (obj->xDirection == IN_motion_Right && obj->leftTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Left;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
	else if (obj->xDirection == IN_motion_Left && obj->rightTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = IN_motion_Right;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, obj->currentAction);
	}
	else if (!obj->topTI)
	{

		//if facing ck_keenObj, jump towards him, else turn around at edge
		if (obj->xDirection == IN_motion_Right && ck_keenObj->posX > obj->posX ||
				obj->xDirection == IN_motion_Left && ck_keenObj->posX < obj->posX)
		{
			CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_ShocksundJump0"));
			obj->velX = obj->xDirection == IN_motion_Right ? 40 : -40;
			obj->velY = -40;
		}
		else
		{
			obj->posX -= obj->deltaPosX;
			obj->xDirection = -obj->xDirection;
			obj->timeUntillThink = US_RndT() / 32;
			CK_SetAction2(obj, obj->currentAction);
		}
	}

	//if flashing white
	if (obj->user3 != 0)
	{
		obj->user3--; //flash counter
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 1, obj->zLayer);
	}
	else
	{
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
	}
}

void CK5_ShocksundJumpTileCol(CK_object *obj)
{

	if (obj->topTI)
	{
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_Shocksund0"));
	}
	if (obj->user3 != 0)
	{
		obj->user3--; //flash counter
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 1, obj->zLayer);
	}
	else
	{
		RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
	}
}

void CK5_BarkShotCol(CK_object *o1, CK_object *o2)
{

	if (o2->type == CT_Player)
	{
		//KeenDie();
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_BarkShotDie0"));
		return;
	}

	if (o2->type == CT_Stunner)
	{
		CK_ShotHit(o2);
		CK_SetAction2(o1, CK_GetActionByName("CK5_ACT_BarkShotDie0"));
	}
}
#define SOUND_BarkShotDie 0x2D

void CK5_BarkShotTileCol(CK_object *obj)
{

	if (obj->topTI || obj->rightTI || obj->bottomTI || obj->leftTI)
	{
		//SD_PlaySound(SOUND_BarkShotDie);
		CK_SetAction2(obj, CK_GetActionByName("CK5_ACT_BarkShotDie0"));
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);
}

#if 0

void CK5_SpherefulSpawn(int tileX, int tileY)
{

	GetNewObj(0);
	new_object->type = 22;
	new_object->clipping = 2;
	new_object->active = 1;
	new_object->posX = TILE2MU(tileX);
	new_object->posY = TILE2MU(tileY) - 0x100;
	CK_SetAction(new_object, CK_GetActionByName("CK5_ACT_SPHEREFUL0"));
}

void CK5_SpherefulBounce(CK_object *o)
{

	int ticks_0;

	obj->int21 = 1;
	if (obj->nextY == 0 || obj->nextX == 0)
		return;

	for (ticks_0 = gameticks_2 - SpriteSync; ticks_0 < gameticks_2; ticks_0++)
	{

		if (!(ticks_0 & 0xF))
		{
			if (obj->velY < 8) obj->velY++;
			if (ck_keenObj->posX < obj->posX && obj->velX < 8) obj->velX++;
			if (ck_keenObj->posX > obj->posX && obj->velX > -8) obj->velX--;
			obj->nextY += obj->velY;
			obj->nextX += obj->velX;
		}

	}
	return;
}

#define SOUND_SPHEREFULCEILING 0x3B
#define SOUND_SPHEREFULGROUND 0x10

void CK5_SpherefulTileCol(CK_object *o)
{

	int diamondpos[16] ={0x180, 0x171, 0x148, 0x109, 0x0C0, 0x077, 0x03A, 0x00F,
		0x000, 0x00F, 0x03A, 0x077, 0x0C0, 0x109, 0x148, 0x171};

	if (obj->leftTI || obj->rightTI)
	{
		obj->velX = -obj->velX;
		//SD_PlaySound(SOUND_SPHEREFULCEILING);
	}

	if (obj->bottomTI)
	{
		obj->velY = -obj->velY;
		//SD_PlaySound(SOUND_SPHEREFULCEILING);
	}

	if (obj->topTI)
	{
		obj->velY = -obj->velY;
		if (ck_keenObj->posY < obj->posY) obj->velY--;
		else obj->velY++;

		if (obj->velY > -4) obj->velY = -4;
		else if (obj->velY < -12) obj->velY = -12;
		//SD_PlaySound(SOUND_SPHEREFULGROUND);
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0, obj->zLayer);

	//draw the diamonds
	//time state, int33, int34 are likely all pointers to obj
	time = gameticks_2 / 8 & 0xF;

	diamond_chunk = time / 4 + 362;
	var6 = time >= 8 ? 2 : 0;

	//topleft
	RF_AddSpriteDraw(&obj->user1, obj->posX + diamondpos[time], obj->posY + diamondpos[time],
									diamond_chunk, 0, var6);

	//topright
	RF_AddSpriteDraw(&obj->user2, obj->posX + 180 - diamondpos[time], obj->posY + diamondpos[time],
									diamond_chunk, 0, var6);

	time = time + 8 & 0xF;
	var6 = time >= 8 ? 2 : 0;

	//botleft
	RF_AddSpriteDraw(&obj->user3, obj->posX + diamondpos[time], obj->posY + diamondpos[time],
									diamond_chunk, 0, var6);

	//botright
	RF_AddSpriteDraw(&obj->user4, obj->posX + 180 - diamondpos[time], obj->posY + diamondpos[time],
									diamond_chunk, 0, var6);

	return;
}
#endif


// Korath Funcs

void CK5_KorathDraw(CK_object *obj)
{
	if (obj->xDirection == 1 && obj->leftTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -1;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction(obj, obj->currentAction);
	}
	else if (obj->xDirection == -1 && obj->rightTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = 1;
		obj->timeUntillThink = US_RndT() / 32;
		CK_SetAction(obj, obj->currentAction);
	}
	else if (!obj->topTI)
	{
		obj->posX -= obj->deltaPosX;
		obj->xDirection = -obj->xDirection;
		obj->timeUntillThink = US_RndT() / 32;
		obj->nextX = 0;

		CK_PhysUpdateNormalObj(obj);
		//CK_SetAction(obj, obj->currentAction);
	}

	RF_AddSpriteDraw(&obj->sde, obj->posX, obj->posY, obj->gfxChunk, 0,
									obj->zLayer);
}

void CK5_KorathWalk(CK_object *obj)
{
	if (US_RndT() < 10)
	{
		obj->nextX = 0;
		obj->xDirection = US_RndT() < 128 ? 1 : -1;
		obj->currentAction = CK_GetActionByName("CK5_ACT_KorathWait");
	}
}

void CK5_KorathColFunc(CK_object *obj, CK_object *other)
{
	//todo: if keen
	if (other->currentAction->collide)
	{
		CK_PhysPushX(other, obj);
		return;
	}
}

void CK5_SpawnKorath(int tileX, int tileY)
{
	CK_object *obj = CK_GetNewObj(false);

	obj->type = 23;
	obj->active = true;
	obj->posX = tileX << 8;
	obj->posY = (tileY << 8) - 128;
	obj->xDirection = US_RndT() < 128 ? 1 : -1;
	CK_SetAction(obj, CK_GetActionByName("CK5_ACT_KorathWalk1"));

	printf("Spawning Korath at %d,%d\n", tileX, tileY);
}

/*
 * Add the Obj3 functions to the function db
 */
void CK5_Obj3_SetupFunctions()
{
	// Shikadi Mine
	CK_ACT_AddFunction("CK5_MineMove", &CK5_MineMove);
	CK_ACT_AddFunction("CK5_MineMoveDotsToCenter", &CK5_MineMoveDotsToCenter);
	CK_ACT_AddFunction("CK5_MineMoveDotsToSides", &CK5_MineMoveDotsToSides);
	CK_ACT_AddFunction("CK5_MineExplode", &CK5_MineExplode);
	CK_ACT_AddColFunction("CK5_MineCol", &CK5_MineCol);
	CK_ACT_AddColFunction("CK5_MineShrapCol", &CK5_MineShrapCol);
	CK_ACT_AddFunction("CK5_MineTileCol", &CK5_MineTileCol);
	CK_ACT_AddFunction("CK5_ShrapnelTileCol", &CK5_ShrapnelTileCol);

	// Robo Red
	CK_ACT_AddFunction("CK5_RoboMove",  &CK5_RoboMove);
	CK_ACT_AddColFunction("CK5_RoboCol",  &CK5_RoboCol);
	CK_ACT_AddFunction("CK5_RoboShoot",  &CK5_RoboShoot);
	CK_ACT_AddColFunction("CK5_RoboShotCol",  &CK5_RoboShotCol);
	CK_ACT_AddFunction("CK5_RoboShotTileCol",  &CK5_RoboShotTileCol);

	// Spirogrip
	CK_ACT_AddFunction("CK5_SpirogripSpin", &CK5_SpirogripSpin);
	CK_ACT_AddFunction("CK5_SpirogripFlyDraw", &CK5_SpirogripFlyDraw);



	CK_ACT_AddFunction("CK5_SpindredBounce", &CK5_SpindredBounce);
	CK_ACT_AddFunction("CK5_SpindredTileCol", &CK5_SpindredTileCol);
#if 0
	CK_ACT_AddFunction("CK5_MasterSpawn", &CK5_MasterSpawn);
	CK_ACT_AddFunction("CK5_MasterStand", &CK5_MasterStand);
	CK_ACT_AddFunction("CK5_MasterShoot", &CK5_MasterShoot);
	CK_ACT_AddFunction("CK5_MasterCol", &CK5_MasterCol);
	CK_ACT_AddFunction("CK5_MasterTele", &CK5_MasterTele);
	CK_ACT_AddFunction("CK5_MasterBallCol", &CK5_MasterBallCol);
	CK_ACT_AddFunction("CK5_MasterBallTileCol", &CK5_MasterBallTileCol);
	CK_ACT_AddFunction("CK5_MasterSparksTileCol", &CK5_MasterSparksTileCol);
#endif
	CK_ACT_AddFunction("CK5_ShikadiWalk", &CK5_ShikadiWalk);
	CK_ACT_AddColFunction("CK5_ShikadiCol", &CK5_ShikadiCol);
	CK_ACT_AddFunction("CK5_ShikadiPole", &CK5_ShikadiPole);
	CK_ACT_AddFunction("CK5_PoleZap", &CK5_PoleZap);
	CK_ACT_AddFunction("CK5_ShikadiTileCol", &CK5_ShikadiTileCol);
	CK_ACT_AddFunction("CK5_ShocksundSearch", &CK5_ShocksundSearch);
	CK_ACT_AddFunction("CK5_ShocksundStand", &CK5_ShocksundStand);
	CK_ACT_AddFunction("CK5_ShocksundShoot", &CK5_ShocksundShoot);
	CK_ACT_AddColFunction("CK5_ShocksundCol", &CK5_ShocksundCol);
	CK_ACT_AddFunction("CK5_ShocksundGroundTileCol", &CK5_ShocksundGroundTileCol);
	CK_ACT_AddFunction("CK5_ShocksundJumpTileCol", &CK5_ShocksundJumpTileCol);
	CK_ACT_AddColFunction("CK5_BarkShotCol", &CK5_BarkShotCol);
	CK_ACT_AddFunction("CK5_BarkShotTileCol", &CK5_BarkShotTileCol);
#if 0
	CK_ACT_AddFunction("CK5_SpherefulBounce", &CK5_SpherefulBounce);
	CK_ACT_AddFunction("CK5_SpherefulTileCol", &CK5_SpherefulTileCol);
#endif

	// Korath
	CK_ACT_AddFunction("CK5_KorathDraw", &CK5_KorathDraw);
	CK_ACT_AddFunction("CK5_KorathWalk", &CK5_KorathWalk);
	CK_ACT_AddColFunction("CK5_KorathColFunc", &CK5_KorathColFunc);
}

