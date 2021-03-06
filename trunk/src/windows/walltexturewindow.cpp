#include <common.h>
#include "walltexturewindow.h"
#include <graphics.h>
#include <undo/wallchange.h>
#include <wm/windowinputbox.h>
#include <bengine/texture.h>

cWallTextureWindow::cWallTextureWindow(int tileX, int tileY, bool horizontal ) : cWindow()
{
	windowType = WT_WALLTEXTURE;
	resizable = false;
	visible = true;
	modal = false;
	
	h = 400+skinOffTop+skinOffBottom;
	w = 500+skinOffLeft+skinOffRight;
	title = "Walltexture";
	center();

	
	if(horizontal)
	{
		if(cGraphics::world->cubes[tileY][tileX].tileSide != -1)
		{
			cGraphics::world->tiles.push_back(cGraphics::world->tiles[cGraphics::world->cubes[tileY][tileX].tileSide]);
			undo = new cUndoWallChange(0, tileX, tileY, cGraphics::world->cubes[tileY][tileX].tileSide);
			cGraphics::world->cubes[tileY][tileX].tileSide = cGraphics::world->tiles.size()-1;
		}
		else
			Log(2,0,"uhoh");
	}
	else
	{
		if(cGraphics::world->cubes[tileY][tileX].tileOtherSide != -1)
		{
			cGraphics::world->tiles.push_back(cGraphics::world->tiles[cGraphics::world->cubes[tileY][tileX].tileOtherSide]);
			undo = new cUndoWallChange(1, tileX, tileY, cGraphics::world->cubes[tileY][tileX].tileOtherSide);
			cGraphics::world->cubes[tileY][tileX].tileOtherSide = cGraphics::world->tiles.size()-1;
		}
		else
			Log(2,0,"uhoh");
	}



	objects["texture"] = new cWindowWallTextureBox(tileX, tileY, horizontal, this);
	objects["btnClose"] = new cWindowMyCloseButton(this);
	objects["btnOk"] = new cWindowOkButton(this);
	objects["btnCancel"] = new cWindowCancelButton(this);
	objects["btnFlipH"] = new cWindowHorizontalFlipButton(this);
	objects["btnFlipV"] = new cWindowVerticalFlipButton(this);
	addLabel("lblGrid", 0, 4*objects["btnOk"]->getHeight()-2, "Gridsize")->alignment = ALIGN_TOPRIGHT;
	cWindowObject* o = new cWindowInputBox(this);
	o->alignment = ALIGN_TOPRIGHT;
	o->moveTo(0, 4*objects["btnOk"]->getHeight()+12);
	o->resizeTo(100, o->getHeight());
	o->setText(0, "4");
	objects["grid"] = o;

}

cWallTextureWindow::cWindowWallTextureBox::cWindowWallTextureBox( int tileX, int tileY, bool horizontal, cWindow* parent, Json::Value &skin) : cWindowObject(parent, skin["frame"])
{
	alignment = ALIGN_TOPLEFT;
	moveTo(0,0);
	resizeTo(400,400);

	if(horizontal)
	{
		if(cGraphics::world->cubes[tileY][tileX].tileSide != -1)
		{
			applyTile = &cGraphics::world->tiles[cGraphics::world->cubes[tileY][tileX].tileSide];
			tile = cGraphics::world->tiles[cGraphics::world->cubes[tileY][tileX].tileSide];
			wallHeight = (cGraphics::world->cubes[tileY][tileX].cell3	+ cGraphics::world->cubes[tileY][tileX].cell4) -
				(cGraphics::world->cubes[tileY+1][tileX].cell1 + cGraphics::world->cubes[tileY+1][tileX].cell2);
		}
		else
			parent->close();
	}
	else
	{
		if(cGraphics::world->cubes[tileY][tileX].tileOtherSide != -1)
		{
			applyTile = &cGraphics::world->tiles[cGraphics::world->cubes[tileY][tileX].tileOtherSide];
			tile = cGraphics::world->tiles[cGraphics::world->cubes[tileY][tileX].tileOtherSide];
			wallHeight = (cGraphics::world->cubes[tileY][tileX].cell4	+ cGraphics::world->cubes[tileY][tileX].cell2) -
				(cGraphics::world->cubes[tileY][tileX+1].cell1 + cGraphics::world->cubes[tileY][tileX+1].cell3);
		}
		else
			parent->close();
	}

}

void cWallTextureWindow::cWindowWallTextureBox::draw( int a,int b,int c,int d)
{
	*applyTile = tile;
	GLfloat colors[4];
	glGetFloatv(GL_CURRENT_COLOR, colors);
	cWindowObject::draw(a,b,c,d);
	glTranslatef(realX(), realY(),0);
	
	glBindTexture(GL_TEXTURE_2D, cGraphics::world->textures[tile.texture].texture->texId());

//	int width = 200 / (wallHeight/15);

	glColor4f(1,1,1,1);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2d(0,0); glVertex2f(50,	50);
		glTexCoord2d(0,1); glVertex2f(50,	350);
		glTexCoord2d(1,0); glVertex2f(350,	50);
		glTexCoord2d(1,1); glVertex2f(350,	350);
	glEnd();


	glDisable(GL_TEXTURE_2D);
	glColor4f(0,0,0,1);
	glBegin(GL_LINE_LOOP);
		glVertex2f(50+300*tile.u1,	350-300*tile.v1);
		glVertex2f(50+300*tile.u2,	350-300*tile.v2);
		glVertex2f(50+300*tile.u4,	350-300*tile.v4);
		glVertex2f(50+300*tile.u3,	350-300*tile.v3);
	glEnd();
	glColor4f(1,1,1,0.25f);
	glBegin(GL_QUADS);
		glVertex2f(50+300*tile.u1,	350-300*tile.v1);
		glVertex2f(50+300*tile.u2,	350-300*tile.v2);
		glVertex2f(50+300*tile.u4,	350-300*tile.v4);
		glVertex2f(50+300*tile.u3,	350-300*tile.v3);
	glEnd();

	glColor4fv(colors);
}

void cWallTextureWindow::cWindowWallTextureBox::drag()
{
	if((bEngine::math::cVector2(cGraphics::cMouse::xOld-parent->getX(), cGraphics::h()-cGraphics::cMouse::yOld-parent->getY()) - bEngine::math::cVector2(realX() + 50+300*tile.u1,	realY() + 350-300*tile.v1)).length() < 20)
	{
		tile.u1 += (cGraphics::cMouse::x - cGraphics::cMouse::xOld)/300.0f;
		tile.v1 += (cGraphics::cMouse::y - cGraphics::cMouse::yOld)/300.0f;
	}
	else if((bEngine::math::cVector2(cGraphics::cMouse::xOld-parent->getX(), cGraphics::h()-cGraphics::cMouse::yOld-parent->getY()) - bEngine::math::cVector2(realX() + 50+300*tile.u2,	realY() + 350-300*tile.v2)).length() < 20)
	{
		tile.u2 += (cGraphics::cMouse::x - cGraphics::cMouse::xOld)/300.0f;
		tile.v2 += (cGraphics::cMouse::y - cGraphics::cMouse::yOld)/300.0f;
	}
	else if((bEngine::math::cVector2(cGraphics::cMouse::xOld-parent->getX(), cGraphics::h()-cGraphics::cMouse::yOld-parent->getY()) - bEngine::math::cVector2(realX() + 50+300*tile.u3,	realY() + 350-300*tile.v3)).length() < 20)
	{
		tile.u3 += (cGraphics::cMouse::x - cGraphics::cMouse::xOld)/300.0f;
		tile.v3 += (cGraphics::cMouse::y - cGraphics::cMouse::yOld)/300.0f;
	}
	else if((bEngine::math::cVector2(cGraphics::cMouse::xOld-parent->getX(), cGraphics::h()-cGraphics::cMouse::yOld-parent->getY()) - bEngine::math::cVector2(realX() + 50+300*tile.u4,	realY() + 350-300*tile.v4)).length() < 20)
	{
		tile.u4 += (cGraphics::cMouse::x - cGraphics::cMouse::xOld)/300.0f;
		tile.v4 += (cGraphics::cMouse::y - cGraphics::cMouse::yOld)/300.0f;
	}



	else if(SDL_GetModState() & KMOD_ALT)
	{
		bEngine::math::cVector2 center = (bEngine::math::cVector2(tile.u1, tile.v1) + bEngine::math::cVector2(tile.u2, tile.v2) + bEngine::math::cVector2(tile.u3, tile.v3) + bEngine::math::cVector2(tile.u4, tile.v4)) / 4.0f;
		
		bEngine::math::cVector2 v;
		v = bEngine::math::cVector2(tile.u1, tile.v1) - center;;
		v = v * (1 + ((cGraphics::cMouse::x - cGraphics::cMouse::xOld)/50.0f));
		v = v + center;
		tile.u1 = v.x;		tile.v1 = v.y;
		
		v = bEngine::math::cVector2(tile.u2, tile.v2) - center;
		v = v * (1 + ((cGraphics::cMouse::x - cGraphics::cMouse::xOld)/50.0f));
		v = v + center;
		tile.u2 = v.x;		tile.v2 = v.y;
		
		v = bEngine::math::cVector2(tile.u3, tile.v3) - center;
		v = v * (1 + ((cGraphics::cMouse::x - cGraphics::cMouse::xOld)/50.0f));
		v = v + center;
		tile.u3 = v.x;		tile.v3 = v.y;
		
		v = bEngine::math::cVector2(tile.u4, tile.v4) - center;
		v = v * (1+((cGraphics::cMouse::x - cGraphics::cMouse::xOld)/50.0f));
		v = v + center;
		tile.u4 = v.x;		tile.v4 = v.y;
	}
 	else if(SDL_GetModState() & KMOD_CTRL)
	{
		bEngine::math::cVector2 center = (bEngine::math::cVector2(tile.u1, tile.v1) + bEngine::math::cVector2(tile.u2, tile.v2) + bEngine::math::cVector2(tile.u3, tile.v3) + bEngine::math::cVector2(tile.u4, tile.v4)) / 4.0f;

		bEngine::math::cVector2 v;
		v = bEngine::math::cVector2(tile.u1, tile.v1) - center;;
		v = v.getRotated((cGraphics::cMouse::x - cGraphics::cMouse::xOld)*1.0f);
		v = v + center;
		tile.u1 = v.x;		tile.v1 = v.y;
	
		v = bEngine::math::cVector2(tile.u2, tile.v2) - center;
		v = v.getRotated((cGraphics::cMouse::x - cGraphics::cMouse::xOld)*1.0f);
		v = v + center;
		tile.u2 = v.x;		tile.v2 = v.y;
		
		v = bEngine::math::cVector2(tile.u3, tile.v3) - center;
		v = v.getRotated((cGraphics::cMouse::x - cGraphics::cMouse::xOld)*1.0f);
		v = v + center;
		tile.u3 = v.x;		tile.v3 = v.y;
		
		v = bEngine::math::cVector2(tile.u4, tile.v4) - center;
		v = v.getRotated((cGraphics::cMouse::x - cGraphics::cMouse::xOld)*1.0f);
		v = v + center;
		tile.u4 = v.x;		tile.v4 = v.y;
	}
	else
	{
		tile.u1 += (cGraphics::cMouse::x - cGraphics::cMouse::xOld)/300.0f;
		tile.v1 += (cGraphics::cMouse::y - cGraphics::cMouse::yOld)/300.0f;
		
		tile.u2 += (cGraphics::cMouse::x - cGraphics::cMouse::xOld)/300.0f;
		tile.v2 += (cGraphics::cMouse::y - cGraphics::cMouse::yOld)/300.0f;
		
		tile.u3 += (cGraphics::cMouse::x - cGraphics::cMouse::xOld)/300.0f;
		tile.v3 += (cGraphics::cMouse::y - cGraphics::cMouse::yOld)/300.0f;
		
		tile.u4 += (cGraphics::cMouse::x - cGraphics::cMouse::xOld)/300.0f;
		tile.v4 += (cGraphics::cMouse::y - cGraphics::cMouse::yOld)/300.0f;
		

	}
	if(SDL_GetModState() & KMOD_SHIFT)
	{
		float gridSize = atof(parent->objects["grid"]->getText(0).c_str());

		if(gridSize != 0)
		{
			tile.u1 = round(tile.u1*gridSize) / gridSize;
			tile.v1 = round(tile.v1*gridSize) / gridSize;
			
			tile.u2 = round(tile.u2*gridSize) / gridSize;
			tile.v2 = round(tile.v2*gridSize) / gridSize;
			
			tile.u3 = round(tile.u3*gridSize) / gridSize;
			tile.v3 = round(tile.v3*gridSize) / gridSize;
			
			tile.u4 = round(tile.u4*gridSize) / gridSize;
			tile.v4 = round(tile.v4*gridSize) / gridSize;
		}		
	}

}

cWallTextureWindow::cWindowOkButton::cWindowOkButton( cWindow* parent) : cWindowButton(parent)
{
	alignment = ALIGN_TOPRIGHT;
	moveTo(0,0);
	resizeTo(100, h);
	text = "Ok";
}

void cWallTextureWindow::cWindowOkButton::onClick()
{
	cGraphics::worldContainer->undoStack->push(((cWallTextureWindow*)parent)->undo);
	parent->close();
}


cWallTextureWindow::cWindowCancelButton::cWindowCancelButton( cWindow* parent) : cWindowButton(parent)
{
	alignment = ALIGN_TOPRIGHT;
	moveTo(0,h);
	resizeTo(100, h);
	text = "Cancel";
}

void cWallTextureWindow::cWindowCancelButton::onClick()
{
	((cWallTextureWindow*)parent)->undo->undo();
	delete ((cWallTextureWindow*)parent)->undo;
	parent->close();
}

cWallTextureWindow::cWindowMyCloseButton::cWindowMyCloseButton( cWindow* parent) : cWindowCloseButton(parent)
{
}

void cWallTextureWindow::cWindowMyCloseButton::onClick()
{
	cWindowCloseButton::onClick();
	((cWallTextureWindow*)parent)->undo->undo();
	delete ((cWallTextureWindow*)parent)->undo;
}


cWallTextureWindow::cWindowHorizontalFlipButton::cWindowHorizontalFlipButton( cWindow* parent ) : cWindowButton(parent)
{
	alignment = ALIGN_TOPRIGHT;
	moveTo(0,2*h);
	resizeTo(100, h);
	text = "Flip Horizontal";
	
}

void cWallTextureWindow::cWindowHorizontalFlipButton::onClick()
{
	cTile* t = &((cWindowWallTextureBox*)parent->objects["texture"])->tile;
	float center = (t->u1 + t->u2 + t->u3 + t->u4) / 4.0f;
	t->u1 = center-(t->u1-center);
	t->u2 = center-(t->u2-center);
	t->u3 = center-(t->u3-center);
	t->u4 = center-(t->u4-center);	
}

cWallTextureWindow::cWindowVerticalFlipButton::cWindowVerticalFlipButton( cWindow* parent) : cWindowButton(parent)
{
	alignment = ALIGN_TOPRIGHT;
	moveTo(0,3*h);
	resizeTo(100, h);
	text = "Flip Vertical";
}

void cWallTextureWindow::cWindowVerticalFlipButton::onClick()
{
	cTile* t = &((cWindowWallTextureBox*)parent->objects["texture"])->tile;
	float center = (t->v1 + t->v2 + t->v3 + t->v4) / 4.0f;
	t->v1 = center-(t->v1-center);
	t->v2 = center-(t->v2-center);
	t->v3 = center-(t->v3-center);
	t->v4 = center-(t->v4-center);	
}
