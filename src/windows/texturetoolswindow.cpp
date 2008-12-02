#include <common.h>
#include "texturetoolswindow.h"
#include "texturebrushwindow.h"

#include <graphics.h>

cTextureToolsWindow::cWindowToolbarButton::cWindowToolbarButton( cWindow* parent, std::string image, eTool t, TiXmlDocument* totalskin ) : cWindowPictureBox(parent)
{
	tool = t;
	activated = t == cGraphics::textureTool;
	if(!totalskin)
		totalskin = &cWM::skin;
	TiXmlElement* skin = totalskin->FirstChildElement("skin")->FirstChildElement("button");
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
	
	setText(0, image);
	resizeTo(20,20);
	alignment = ALIGN_TOPLEFT;
}

void cTextureToolsWindow::cWindowToolbarButton::draw( int a,int b,int c,int d )
{
	GLfloat colors[4];
	glGetFloatv(GL_CURRENT_COLOR, colors);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	if(activated)
		glColor4f(0.7f, 0.7f, 0.9f, colors[3]);
	if(inObject())
		glColor3f(0.6f, 0.6f, 0.9f);
	cWindowObject::draw();
	x+=2;
	y+=2;
	w=16;
	h=16;
	cWindowPictureBox::draw(a,b,c,d);
	x-=2;
	y-=2;
	h=20;
	w=20;
	
	if(activated)
	{
		int xx = realX();
		int yy = realY();
		
		glEnable(GL_BLEND);
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

void cTextureToolsWindow::cWindowToolbarButton::onClick()
{
	for(std::map<std::string, cWindowObject*, std::less<std::string> >::iterator i = parent->objects.begin(); i != parent->objects.end(); i++)
	{
		if(i->second->type == this->type)
			((cWindowToolbarButton*)i->second)->activated = false;
	}
	activated = true;
	cGraphics::textureTool = tool;
}

cTextureToolsWindow::cWindowBrushShape::cWindowBrushShape( cWindow* parent, TiXmlDocument* totalskin ) : cWindowButton(parent, totalskin)
{
	resizeTo(20,20);
	alignment = ALIGN_TOPLEFT;
	text = "";
}

void cTextureToolsWindow::cWindowBrushShape::draw( int a,int b,int c,int d )
{
	GLfloat colors[4];
	glGetFloatv(GL_CURRENT_COLOR, colors);
	if(inObject())
		glColor3f(0.6f, 0.6f, 0.9f);
	cWindowObject::draw();
	if(cGraphics::textureBrush.size() == 0)
		return;
	
	//draw brush


	float sx = (getWidth()-8)  / (float)max(cGraphics::textureBrush[0].size(), cGraphics::textureBrush.size());
	float sy = (getHeight()-8) / (float)max(cGraphics::textureBrush[0].size(), cGraphics::textureBrush.size());

	int xx = realX();
	int yy = realY();
	glDisable(GL_TEXTURE_2D);
	glColor4f(0,0,0,colors[3]);
	glBegin(GL_QUADS);
	for(unsigned int yyy = 0; yyy < cGraphics::textureBrush.size(); yyy++)
	{
		for(unsigned int xxx = 0; xxx < cGraphics::textureBrush[yyy].size(); xxx++)
		{
			if(cGraphics::textureBrush[cGraphics::textureBrush.size()-1-yyy][xxx])
			{
				glVertex2f(xx + 4 + xxx*sx,		yy + 4 + yyy*sy);
				glVertex2f(xx + 4 + xxx*sx+sx,	yy + 4 + yyy*sy);
				glVertex2f(xx + 4 + xxx*sx+sx,	yy + 4 + yyy*sy+sy);
				glVertex2f(xx + 4 + xxx*sx,		yy + 4 + yyy*sy+sy);
			}
		}
	}
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glColor4fv(colors);
}

void cTextureToolsWindow::cWindowBrushShape::onClick()
{
	cWM::addWindow(new cTextureBrushWindow());
}

cTextureToolsWindow::cWindowSelectArea::cWindowSelectArea( cWindow* parent, TiXmlDocument* totalskin ) : cWindowToolbarButton(parent, "data/buttons/selectarea.tga",TOOL_SELECTAREA, totalskin)
{
	
}

cTextureToolsWindow::cWindowSelectBrush::cWindowSelectBrush( cWindow* parent, TiXmlDocument* totalskin ) : cWindowToolbarButton(parent, "data/buttons/selectbrush.tga",TOOL_SELECTBRUSH, totalskin)
{
	
}

cTextureToolsWindow::cWindowSelectWand::cWindowSelectWand( cWindow* parent, TiXmlDocument* totalskin ) : cWindowToolbarButton(parent, "data/buttons/selectwand.tga",TOOL_SELECTWAND, totalskin)
{
	
}

cTextureToolsWindow::cWindowBrush::cWindowBrush( cWindow* parent, TiXmlDocument* totalskin ) : cWindowToolbarButton(parent, "data/buttons/brush.tga",TOOL_BRUSH, totalskin)
{
	
}

cTextureToolsWindow::cTextureToolsWindow( ) : cWindow()
{
	windowType = WT_TEXTURETOOLS;
	resizable = true;
	visible = true;
	modal = false;
	
	h = 100;
	w = 50;
	x = 0;
	y = cGraphics::h()-h;
	minWidth = 20 + skinOffLeft+skinOffRight;
	minHeight = 20 + skinOffTop+skinOffBottom;
	
	title = "";
	initProps("texturetools");
	
	TiXmlElement* wSkin = cWM::skin.FirstChildElement("skin")->FirstChildElement("miniwindow");
	
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
	
	objects["selectarea"] = new cWindowSelectArea(this);
	objects["selectbrush"] = new cWindowSelectBrush(this);
	objects["selectwand"] = new cWindowSelectWand (this);
	objects["brush"] = new cWindowBrush(this);
	objects["aaa_brushshape"] = new cWindowBrushShape(this);
	
	cWindowObject* o;
	
	o = new cWindowLimitedFloatInputBox(this);
	((cWindowFloatInputBox*)o)->floatje = &cGraphics::textureGridSizeX;
	o->alignment = ALIGN_TOPLEFT;
	o->resizeTo(innerWidth(), o->getHeight());
	objects["aa_gridsizex"] = o;
	
	o = new cWindowLimitedFloatInputBox(this);
	((cWindowFloatInputBox*)o)->floatje = &cGraphics::textureGridSizeY;
	o->alignment = ALIGN_TOPLEFT;
	o->resizeTo(innerWidth(), o->getHeight());
	objects["aa_gridsizey"] = o;
	
	reOrder();
}

void cTextureToolsWindow::reOrder()
{
	int xx = 0,	 yy = 0, lineheight = 0;
	for(objectlist::iterator i = objects.begin(); i != objects.end(); i++)
	{
		if(xx+i->second->getWidth() > innerWidth())
		{
			xx = 0;
			yy += lineheight;
			lineheight = 0;
		}
		lineheight = max(lineheight, i->second->getHeight());
		i->second->moveTo(xx,yy);
		xx += i->second->getWidth();
	}
	h = yy+lineheight+skinOffBottom+skinOffTop;
}

void cTextureToolsWindow::resizeTo( int ww, int hh )
{
	cWindow::resizeTo(floor(ww/20.0f)*20+skinOffLeft+skinOffRight,hh);
	objects["aa_gridsizex"]->resizeTo(min(innerWidth(),40), objects["aa_gridsizex"]->getHeight());
	objects["aa_gridsizey"]->resizeTo(min(innerWidth(),40), objects["aa_gridsizey"]->getHeight());
	reOrder();
}

bool cTextureToolsWindow::cWindowLimitedFloatInputBox::onChar(char c,bool shift)
{
	if(!(c >= '0' && c <= '9' && c != '.'))
		return false;
	return cWindowFloatInputBox::onChar(c,shift);
}
