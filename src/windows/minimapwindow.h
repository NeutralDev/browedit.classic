#ifndef __MINIMAPWINDOW_H__
#define __MINIMAPWINDOW_H__

#include <wm/window.h>

#include <wm/windowobject.h>
#include <wm/windowroundbutton.h>
#include <wm/windowbutton.h>
#include <wm/windowpicturebox.h>
#include "../graphics.h"
extern cGraphics Graphics;

class cMiniMapWindow : public cWindow
{
public:

	class cMiniMap : public cWindowObject
	{
	public:
		cMiniMap(cWindow* parent) : cWindowObject(parent)
		{
			moveTo(0,0);
			resizeTo(Graphics.world.width,Graphics.world.height);
			alignment = ALIGN_TOPLEFT;
		}
		void draw(int cutoffleft, int cutoffright, int cutofftop, int cutoffbottom)
		{
			int xx, yy;
			xx = realX();
			yy = realY();

			glEnable(GL_TEXTURE_2D);

			for(int y = 0; y < Graphics.world.height; y++)
			{
				for(int x = 0; x < Graphics.world.width; x++)
				{
					if(Graphics.world.cubes[y][x].tileUp == -1)
						continue;

					cTile* t = &Graphics.world.tiles[Graphics.world.cubes[y][x].tileUp];
					glBindTexture(GL_TEXTURE_2D, Graphics.world.textures[t->texture]->texId());
					glBegin(GL_TRIANGLE_STRIP);
						glTexCoord2f(t->u1, 1-t->v1);				glVertex2f(xx+x,yy+y);
						glTexCoord2f(t->u3, 1-t->v3);				glVertex2f(xx+x,yy+y+1);
						glTexCoord2f(t->u2, 1-t->v2);				glVertex2f(xx+x+1,yy+y);
						glTexCoord2f(t->u4, 1-t->v4);				glVertex2f(xx+x+1,yy+y+1);
					glEnd();
				}
			}
		}
		void click()
		{
			if(!inObject())
				return;
			int xx, yy;
			xx = realX()+parent->getX();
			yy = realY()+parent->getY();

			Graphics.camerapointer.x = -10*(mouseX - xx);
			Graphics.camerapointer.y = -10*( Graphics.world.height - ((Graphics.h() -mouseY) - yy));
		}
		void drag()
		{
			click();
		}



	};



	cMiniMapWindow(cTexture* t, cFont* f, TiXmlDocument &skin) : cWindow(t,f,skin)
	{
		windowType = WT_MINIMAP;
		resizable = false;
		visible = true;
		modal = false;
		alwaysOnTop = true;
		noTransparency = true;


		h = Graphics.world.height+skinOffBottom+skinOffTop;
		w = Graphics.world.width+skinOffLeft+skinOffRight;
		x = Graphics.w()-w;
		y = 0;
		initprops("minimap");
		h = Graphics.world.height+skinOffBottom+skinOffTop;
		w = Graphics.world.width+skinOffLeft+skinOffRight;
		title = "";


		objects["minimap"] = new cMiniMap(this);

	}	

/*	void draw()
	{
		glTranslatef(x, y, 0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture->texid());
		glBegin(GL_QUADS);
			glTexCoord2f(0,				496.0f/512.0f);		glVertex2d(0,  h-16);
			glTexCoord2f(3/512.0f,		496.0f/512.0f);		glVertex2d(3, h-16);
			glTexCoord2f(3/512.0f,		1);					glVertex2d(3, h);
			glTexCoord2f(0,				1);					glVertex2d(0,  h);

			glTexCoord2f(3/512.0f,		496.0f/512.0f);		glVertex2d(3,  h-16);
			glTexCoord2f(509/512.0f,	496.0f/512.0f);		glVertex2d(w-3, h-16);
			glTexCoord2f(509/512.0f,	1);					glVertex2d(w-3, h);
			glTexCoord2f(3/512.0f,		1);					glVertex2d(3,  h);

			glTexCoord2f(509/512.0f,	496.0f/512.0f);		glVertex2d(w-3,  h-16);
			glTexCoord2f(1,				496.0f/512.0f);		glVertex2d(w, h-16);
			glTexCoord2f(1,				1);					glVertex2d(w, h);
			glTexCoord2f(509/512.0f,	1);					glVertex2d(w-3,  h);

			glTexCoord2f(0,				474.0f/512.0f);		glVertex2d(0,  4);
			glTexCoord2f(3/512.0f,		474.0f/512.0f);		glVertex2d(3, 4);
			glTexCoord2f(3/512.0f,		493.0f/512.0f);		glVertex2d(3, h-16);
			glTexCoord2f(0,				493.0f/512.0f);		glVertex2d(0,  h-16);

			glTexCoord2f(3/512.0f,		474.0f/512.0f);		glVertex2d(3,  4);
			glTexCoord2f(509/512.0f,	474.0f/512.0f);		glVertex2d(w-3, 4);
			glTexCoord2f(509/512.0f,	493.0f/512.0f);		glVertex2d(w-3, h-16);
			glTexCoord2f(3/512.0f,		493.0f/512.0f);		glVertex2d(3,  h-16);

			glTexCoord2f(509/512.0f,	474.0f/512.0f);		glVertex2d(w-3,  4);
			glTexCoord2f(1,				474.0f/512.0f);		glVertex2d(w, 4);
			glTexCoord2f(1,				493.0f/512.0f);		glVertex2d(w, h-16);
			glTexCoord2f(509/512.0f,	493.0f/512.0f);		glVertex2d(w-3,  h-16);

			glTexCoord2f(0,				470.0f/512.0f);		glVertex2d(0,  0);
			glTexCoord2f(3/512.0f,		470.0f/512.0f);		glVertex2d(3, 0);
			glTexCoord2f(3/512.0f,		474.0f/512.0f);		glVertex2d(3, 4);
			glTexCoord2f(0,				474.0f/512.0f);		glVertex2d(0,  4);

			glTexCoord2f(3/512.0f,		470.0f/512.0f);		glVertex2d(3,  0);
			glTexCoord2f(509/512.0f,	470.0f/512.0f);		glVertex2d(w-3, 0);
			glTexCoord2f(509/512.0f,	474.0f/512.0f);		glVertex2d(w-3, 4);
			glTexCoord2f(3/512.0f,		474.0f/512.0f);		glVertex2d(3,  4);

			glTexCoord2f(509/512.0f,	470.0f/512.0f);		glVertex2d(w-3,  0);
			glTexCoord2f(1,				470.0f/512.0f);		glVertex2d(w, 0);
			glTexCoord2f(1,				474.0f/512.0f);		glVertex2d(w, 4);
			glTexCoord2f(509/512.0f,	474.0f/512.0f);		glVertex2d(w-3,  4);
		glEnd();

		
		map<string, cWindowObject*, less<string> >::iterator i;
		for(i = objects.begin(); i != objects.end(); i++)
		{
			cWindowObject* o = i->second;
			if (!rolledup || o->realy2() > h-19)
			{
				//if(i->second->realY() > 0)
					o->draw();
			}
		}
		glTranslatef(-x, -y, 0);
	}*/
};

#endif