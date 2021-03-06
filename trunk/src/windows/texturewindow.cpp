#include <common.h>
#include "texturewindow.h"

#include <wm/windowbutton.h>
#include <wm/windowlistbox.h>
#include <wm/windowscrollpanel.h>
#include <wm/windowlabel.h>
#include "rsmeditwindow.h"

#include <graphics.h>
#include <settings.h>
#include <world.h>
#include <bengine/util.h>
#include <bengine/util/filesystem.h>
#include <bengine/texturecache.h>

cTextureWindow::cWindowTexture::cWindowTexture( cWindow* parent ) : cWindowPictureBox(parent)
{
	
}

void cTextureWindow::cWindowTexture::onClick()
{
	cWindow* w = cWM::getWindow(WT_RSMEDIT);
	if(w != NULL)
	{
		((cRSMEditWindow*)w)->changetexture("data\\texture\\" + data);
		parent->close();
	}
	else
	{
		if(SDL_GetModState() & KMOD_SHIFT)
		{
			int id = cGraphics::worldContainer->settings.texturestart + (int)(cGraphics::worldContainer->settings.selectionstart.y - 32) / 288;
			bEngine::cTextureCache::unload(cGraphics::world->textures[id].texture);
			
			cTextureContainer t;
			t.RoFilename = data;
			char buf[40];
			ZeroMemory(buf, 40);
			sprintf(buf, "%i%i", rand(), rand());
			t.RoFilename2 = std::string(buf,40);
			t.texture = bEngine::cTextureCache::load(cSettings::roDir + "data\\texture\\" + data);
			cGraphics::world->textures[id] = t;
			
		}
		else
		{
			cTextureContainer t;
			t.RoFilename = data;
			char buf[40];
			ZeroMemory(buf, 40);
			sprintf(buf, "%i%i", rand(), rand());
			t.RoFilename2 = std::string(buf,40);
			t.texture = bEngine::cTextureCache::load(cSettings::roDir + "data\\texture\\" + data);
			cGraphics::world->textures.push_back(t);
			cGraphics::worldContainer->settings.texturestart = max((int)cGraphics::world->textures.size() - 2,0);
		}
	}
}

void cTextureWindow::cWindowTexture::setText( int i, std::string s )
{
	cWindowPictureBox::setText(i, s);
	if(i == 1)
	{
		data = s;
	}
}

cTextureWindow::cWindowTextureCatSelect::cWindowTextureCatSelect( cWindow* parent, std::vector<cWindowTree::cTreeNode*> n, Json::Value &skin ) : cWindowTree(parent, n,skin)
{
	
}

void cTextureWindow::cWindowTextureCatSelect::onClick()
{
	unsigned int i;
	cWindowTree::onClick();
	cWindowScrollPanel* box = (cWindowScrollPanel*)parent->objects["textures"];
	for(i = 0; i < box->objects.size(); i++)
		delete box->objects[i];
	box->objects.clear();
	int a = selected;
	cWindowTree::cTreeNode* node = NULL;
	for(i = 0; i < nodes.size(); i++)
	{
		node = nodes[i]->getnode(a);
		if(node != NULL)
			break;
	}
	
	std::vector<std::pair<std::string, std::string> > v;
	v = *((std::vector<std::pair<std::string, std::string> >*)parent->userfunc(node));
	
	std::string filter = parent->objects["filter"]->getText(0);
	cWindowObject* o;
	for(i = 0; i < v.size(); i++)
	{
		std::pair<std::string, std::string> p = v[i];

		if(filter != "" && p.first.find(filter) == std::string::npos)
			continue;
		
		o = new cWindowTexture(parent);
		o->alignment = ALIGN_TOPLEFT;
		o->moveTo(i*130, 32);
		o->resizeTo(128,128);
		o->setText(0,cSettings::roDir + "data\\texture\\" + p.second);
		o->setText(1,p.second);
		o->setPopup(p.first);
		box->objects.push_back(o);


		o = new cWindowLabel(parent);
		o->alignment = ALIGN_TOPLEFT;
		o->setText(0,"#000000" + p.first);
		box->objects.push_back(o);

		o = new cWindowLabel(parent);
		o->alignment = ALIGN_TOPLEFT;
		o->setText(0,"#FF0000" + p.first);
		box->objects.push_back(o);

	}
	parent->resizeTo(parent->getWidth(), parent->getHeight());
	cWM::draggingWindow = NULL;
	cWM::draggingObject = NULL;
}

cTextureWindow::cTextureWindow( ) : cWindow()
{
	iconSize = 128;
	windowType = WT_TEXTURE;
	closeType = HIDE;
	resizable = true;
	visible = true;
	modal = false;
	
	h = cGraphics::h()-50;
	w = cGraphics::w()-50;
	title = GetMsg("wm/texture/TITLE");
	center();
	initProps("textures");
	
	cWindowObject* o;
	
	std::vector<cWindowTree::cTreeNode*> nodes;
	std::map<std::string, cWindowTree::cTreeNode*, std::less<std::string> > lookup;
	
	std::string line, pre, filename, cat, name;
	
	std::map<std::string, std::string, std::less<std::string> > translationcache;
	
	for(unsigned int i = 0; i < cSettings::textureFiles.size(); i++)
	{
		bEngine::util::cInStream* pFile = bEngine::util::cFileSystem::open(cSettings::textureFiles[i]);
		if(pFile == NULL)
			continue;
		while(!pFile->eof())
		{
			line = pFile->readline();
			if (line == "")
				continue;
			pre = line.substr(0, line.find("|"));
			filename = line.substr(line.find("|")+1);
			
			cat = pre.substr(0, pre.rfind("/"));
			name = pre.substr(pre.rfind("/")+1);
			
			if(translationcache.find(cat) != translationcache.end())
				cat = translationcache[cat];
			else
			{
				std::string origcat = cat;
				for(unsigned int ii = 0; ii < cSettings::translations.size(); ii++)
					cat = bEngine::util::replace(cat, cSettings::translations[ii].first, cSettings::translations[ii].second);
				translationcache[origcat] = cat;
			}
			
			if(lookup.find(cat) == lookup.end())
			{
				if(cat.find("/") != std::string::npos)
				{
					std::string p = cat.substr(0, cat.rfind("/"));
					if(lookup.find(p) == lookup.end())
					{
						Log(1,0,"Invalid nesting in texturefile...");
					}
					else
					{
						cWindowTree::cTreeNode* n = new cWindowTree::cTreeNode(cat.substr(cat.rfind("/")+1));
						lookup[cat] = n;
						lookup[p]->addchild(n);
					}
				}
				else
				{
					cWindowTree::cTreeNode* n = new cWindowTree::cTreeNode(cat);
					lookup[cat] = n;
					nodes.push_back(n);
				}
			}
			if(filename != "")
			{
				if (items.find(lookup[cat]) == items.end())
				{
					items[lookup[cat]] = std::vector<std::pair<std::string, std::string> >();
					items[lookup[cat]].reserve(100);
				}
				items[lookup[cat]].push_back(std::pair<std::string,std::string>(name,filename));
			}
		}
		delete pFile;
	}
	
	o = new cWindowTextureCatSelect(this, nodes);
	o->alignment = ALIGN_TOPLEFT;
	o->moveTo(0,0);
	o->resizeTo(400,400);
	objects["tree"] = o;
	
	/*		o = new cWindowListBox(this);
	o->alignment = ALIGN_TOPLEFT;
	o->moveto(20,20);
	o->resizeTo(100,100);
	objects["textures"] = o;*/
	
	o = new cWindowScrollPanel(this);
	o->alignment = ALIGN_TOPLEFT;
	o->moveTo(0, 0);
	o->resizeTo(100,100);
	objects["textures"] = o;
	

	o = new cWindowFilterBox(this);
	objects["filter"] = o;

	//		objects["rollup"] = new cWindowRollupButton(this);
	objects["close"] = new cWindowCloseButton(this);
	selectedObject = objects["filter"];
	resizeTo(w,h);
}

void cTextureWindow::resizeTo( int ww, int hh )
{
	cWindow::resizeTo(ww,hh);
	objects["tree"]->resizeTo(200, innerHeight());
	objects["filter"]->resizeTo(ww-220, objects["filter"]->getHeight());
	cWindowScrollPanel* panel = (cWindowScrollPanel*)objects["textures"];
	panel->moveTo(200, 0);
	panel->resizeTo(innerWidth()-200, innerHeight()-20);
	panel->innerwidth = innerWidth()-200;
	
	int x = 0;
	int y = 0;
	for(unsigned int i = 0; i < panel->objects.size(); i+=3)
	{
		panel->objects[i]->moveTo(x,y);
		panel->objects[i]->resizeTo(iconSize,iconSize);
		panel->objects[i+1]->moveTo(x+5,y+iconSize-36);
		panel->objects[i+1]->resizeTo(iconSize,12);
		panel->objects[i+2]->moveTo(x+4,y+iconSize-37);
		panel->objects[i+2]->resizeTo(iconSize,12);

		x+=iconSize+2;
		if(x+iconSize+2 > panel->innerwidth-18)
		{
			x = 0;
			y += iconSize+2;
		}
	}
	panel->scrollposx = 0;
	panel->scrollposy = 0;
	panel->innerheight = y+iconSize+2;
}

void* cTextureWindow::userfunc( void* param )
{
	return &items[(cWindowTree::cTreeNode*)param];
}

bool cTextureWindow::onKeyDown( int keyid, bool shift )
{
	if(keyid == SDLK_MINUS)
	{
		iconSize=(int)(iconSize/1.5);
		if(iconSize < 1)
			iconSize = 1;
		resizeTo(w,h);
		return true;
	}
	else if(keyid == SDLK_EQUALS)
	{
		iconSize=(int)(iconSize*1.5);
		resizeTo(w,h);
		return true;
	}
	else
		return cWindow::onKeyDown(keyid, shift);
}

cTextureWindow::cWindowFilterBox::cWindowFilterBox(cWindow* parent, Json::Value &skin) : cWindowInputBox(parent,skin)
{
	alignment = ALIGN_BOTTOMRIGHT;
	text = "";
	moveTo(0,0);
}


bool cTextureWindow::cWindowFilterBox::onKeyDown(int keyId, bool shift)
{
	bool b = cWindowInputBox::onKeyDown(keyId, shift);
	parent->objects["tree"]->onClick();
	return b;
}
