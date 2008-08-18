#ifndef __TEXTURETOOLSWINDOW_H__
#define __TEXTURETOOLSWINDOW_H__

#include "window.h"

#include "windowobject.h"
#include "windowroundbutton.h"
#include "windowbutton.h"
#include "windowpicturebox.h"
#include "../graphics.h"
extern cGraphics Graphics;

class cTextureToolsWindow : public cWindow
{
	class cWindowToggleButton : public cWindowRoundButton
	{
		
	};

	class cWindowToolbarButton : public cWindowPictureBox
	{
	public:
		bool activated;
		eTool tool;


		cWindowToolbarButton(cWindow* parent, TiXmlDocument &totalskin, string image, eTool t) : cWindowPictureBox(parent)
		{
			tool = t;
			activated = t == Graphics.texturetool;
			TiXmlElement* skin = totalskin.FirstChildElement("skin")->FirstChildElement("button");
			if(skin != NULL)
			{
				skinTopHeight = atoi(skin->FirstChildElement("top")->Attribute("height"));
				skinTop =		512 - atoi(skin->FirstChildElement("top")->FirstChild()->Value());
				skinBottomHeight = atoi(skin->FirstChildElement("bottom")->Attribute("height"));
				skinBottom =		512 - atoi(skin->FirstChildElement("bottom")->FirstChild()->Value());
				
				skinLeftWidth = atoi(skin->FirstChildElement("left")->Attribute("width"));
				skinLeft =		atoi(skin->FirstChildElement("left")->FirstChild()->Value());
				skinRightWidth = atoi(skin->FirstChildElement("right")->Attribute("width"));
				skinRight =		atoi(skin->FirstChildElement("right")->FirstChild()->Value());
			}

			SetText(0, image);
			resizeto(16,16);
			alignment = ALIGN_TOPLEFT;
		}

		virtual void draw(int a,int b,int c,int d)
		{
			GLfloat colors[4];
			glGetFloatv(GL_CURRENT_COLOR, colors);
			if(inobject())
				glColor3f(0.6f, 0.6f, 0.9f);
			if(activated)
				glColor3f(0.7f, 0.7f, 0.9f);
			cWindowObject::draw();
			cWindowPictureBox::draw(a,b,c,d);

			if(inobject() || activated)
			{
				int xx = realx();
				int yy = realy();

				glDisable(GL_TEXTURE_2D);
				glColor4f(0,0,0,colors[3]);
				glBegin(GL_LINE_LOOP);
					glVertex2f(xx,	yy);
					glVertex2f(xx+w,yy);
					glVertex2f(xx+w,yy+h);
					glVertex2f(xx,	yy+h);
				glEnd();
				glEnable(GL_TEXTURE_2D);
			}

			glColor4fv(colors);
		}
		virtual void click()
		{
			for(map<string, cWindowObject*, less<string> >::iterator i = parent->objects.begin(); i != parent->objects.end(); i++)
			{
				if(i->second->type == this->type)
					((cWindowToolbarButton*)i->second)->activated = false;
			}
			activated = true;
			Graphics.texturetool = tool;
		}
	};

	class cWindowBrushShape : public cWindowButton
	{
	public:
		cWindowBrushShape(cWindow* parent, TiXmlDocument &totalskin) : cWindowButton(parent, totalskin)
		{
			resizeto(16,16);
			alignment = ALIGN_TOPLEFT;
			text = "";
		}
		void draw(int a,int b,int c,int d)
		{
			GLfloat colors[4];
			glGetFloatv(GL_CURRENT_COLOR, colors);
			if(inobject())
				glColor3f(0.6f, 0.6f, 0.9f);
			cWindowObject::draw();

//draw brush

			if(inobject())
			{
				int xx = realx();
				int yy = realy();

				glDisable(GL_TEXTURE_2D);
				glColor4f(0,0,0,colors[3]);
				glBegin(GL_LINE_LOOP);
					glVertex2f(xx,	yy);
					glVertex2f(xx+w,yy);
					glVertex2f(xx+w,yy+h);
					glVertex2f(xx,	yy+h);
				glEnd();
				glEnable(GL_TEXTURE_2D);
			}

			glColor4fv(colors);
		}

		void click()
		{

		}

	};


	class cWindowSelectArea : public cWindowToolbarButton
	{
	public:
		cWindowSelectArea(cWindow* parent, TiXmlDocument &totalskin) : cWindowToolbarButton(parent, totalskin, "data/buttons/selectarea.tga",TOOL_SELECTAREA)
		{
		}
	};
	class cWindowSelectBrush : public cWindowToolbarButton
	{
	public:
		cWindowSelectBrush(cWindow* parent, TiXmlDocument &totalskin) : cWindowToolbarButton(parent, totalskin, "data/buttons/selectbrush.tga",TOOL_SELECTBRUSH)
		{
		}
	};
	class cWindowSelectWand : public cWindowToolbarButton
	{
	public:
		cWindowSelectWand(cWindow* parent, TiXmlDocument &totalskin) : cWindowToolbarButton(parent, totalskin, "data/buttons/selectwand.tga",TOOL_SELECTWAND)
		{
		}
	};
	class cWindowBrush : public cWindowToolbarButton
	{
	public:
		cWindowBrush(cWindow* parent, TiXmlDocument &totalskin) : cWindowToolbarButton(parent, totalskin, "data/buttons/brush.tga",TOOL_BRUSH)
		{
		}
	};

public:
	cTextureToolsWindow(cTexture* t, cFont* f, TiXmlDocument &skin) : cWindow(t,f,skin)
	{
		wtype = WT_TEXTURETOOLS;
		resizable = true;
		visible = true;
		modal = false;

		h = 100;
		w = 50;
		x = 0;
		y = Graphics.h()-32;
		minw = 20 + (pw()-innerw());
		minh = 20 + (ph()-innerh());

		title = "";
		initprops("texturetools");

		TiXmlElement* wSkin = skin.FirstChildElement("skin")->FirstChildElement("miniwindow");

		skinTopHeight = atoi(wSkin->FirstChildElement("top")->Attribute("height"));
		skinTop =		512 - atoi(wSkin->FirstChildElement("top")->FirstChild()->Value());
		skinBottomHeight = atoi(wSkin->FirstChildElement("bottom")->Attribute("height"));
		skinBottom =		512 - atoi(wSkin->FirstChildElement("bottom")->FirstChild()->Value());
		
		skinLeftWidth = atoi(wSkin->FirstChildElement("left")->Attribute("width"));
		skinLeft =		atoi(wSkin->FirstChildElement("left")->FirstChild()->Value());
		skinRightWidth = atoi(wSkin->FirstChildElement("right")->Attribute("width"));
		skinRight =		atoi(wSkin->FirstChildElement("right")->FirstChild()->Value());

		wSkin = wSkin->FirstChildElement("offsets");
		skinOffLeft =	atoi(wSkin->FirstChildElement("left")->FirstChild()->Value());
		skinOffRight =	atoi(wSkin->FirstChildElement("right")->FirstChild()->Value());
		skinOffTop =	atoi(wSkin->FirstChildElement("top")->FirstChild()->Value());
		skinOffBottom = atoi(wSkin->FirstChildElement("bottom")->FirstChild()->Value());

		objects["selectarea"] = new cWindowSelectArea(this,skin);
		objects["selectbrush"] = new cWindowSelectBrush(this,skin);
		objects["selectwand"] = new cWindowSelectWand (this,skin);
		objects["brush"] = new cWindowBrush(this,skin);
		objects["aaa_brushshape"] = new cWindowBrushShape(this,skin);

		cWindowObject* o;

		o = new cWindowFloatInputBox(this, skin);
		((cWindowFloatInputBox*)o)->floatje = &Graphics.texturegridsize;
		o->alignment = ALIGN_TOPLEFT;
		o->resizeto(innerw(), o->ph());
		objects["aa_gridsize"] = o;

		reorder();

	}
	void reorder()
	{
		int xx = 0,	 yy = 0, lineheight = 0;
		for(map<string, cWindowObject*, less<string> >::iterator i = objects.begin(); i != objects.end(); i++)
		{
			if(xx+i->second->pw() >= innerw())
			{
				xx = 0;
				yy += lineheight;
				lineheight = 0;
			}
			lineheight = max(lineheight, i->second->ph());
			i->second->moveto(xx,yy);
			xx += i->second->pw();
		}
		h = yy+lineheight+skinOffBottom+skinOffTop;
	}

	void resizeto(int ww, int hh)
	{
		cWindow::resizeto(floor(ww/18.0f)*18+4,hh);
		objects["aa_gridsize"]->resizeto(min(innerw(),64), objects["aa_gridsize"]->ph());
		reorder();
	}

};

#endif
