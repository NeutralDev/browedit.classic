#ifndef __FAVORITELIGHTSWINDOW_H__
#define __FAVORITELIGHTSWINDOW_H__

#include <wm/window.h>

#include <wm/windowobject.h>
#include <wm/windowroundbutton.h>
#include <wm/windowbutton.h>
#include <wm/windowlabel.h>
#include <wm/windowtree.h>
#include <wm/windowinputbox.h>
#include "objectwindow.h"
#include <wm/windowcheckbox.h>
extern TiXmlDocument favoritelights;
extern cMenu* popupmenu;



class cFavoriteLightsWindow : public cWindow
{
public:
	class cFavoriteTreeNode : public cWindowTree::cTreeNode
	{
	public:
		cFavoriteTreeNode() : cWindowTree::cTreeNode() {}
		cFavoriteTreeNode(std::string t) : cWindowTree::cTreeNode(t) {}
		cFavoriteTreeNode(std::string t, std::vector<cWindowTree::cTreeNode*> items) : cWindowTree::cTreeNode(t, items) {}


		std::vector<int> keys;
		bool		iscat;

	};

	
	static void addlights(cFavoriteTreeNode* parent, TiXmlNode* n)
	{
		while(n != NULL)
		{
			cFavoriteTreeNode* windowtreenode = NULL;
			if(strcmp(n->Value(), "light") == 0)
			{
				windowtreenode = new cFavoriteTreeNode(n->FirstChildElement("name")->FirstChild()->Value());
				windowtreenode->iscat = false;
				parent->addchild(windowtreenode);
			}
			else
			{
				windowtreenode = new cFavoriteTreeNode(n->Value());
				windowtreenode->open = true;
				windowtreenode->iscat = true;;
				parent->addchild(windowtreenode);
				addlights(windowtreenode, n->FirstChildElement());
			}
			std::vector<int> keys;
			TiXmlNode* el = n;
			while(el != NULL)
			{
				TiXmlNode* pel = el->Parent();
				if(pel != NULL)
				{
					TiXmlNode* child = pel->FirstChild();
					for(int i = 0; child != NULL; i++)
					{
						if(child == el)
						{
							keys.push_back(i);
							break;
						}
						child=child->NextSibling();
					}
				}
				el = el->Parent();
			}
			windowtreenode->keys = keys;

			
			n = n->NextSiblingElement();
		}
	}


	class cFavoritesTree : public cWindowTree
	{
	public:
		cFavoritesTree(cWindow* parent, std::vector<cWindowTree::cTreeNode*> nodes, TiXmlDocument &skin) : cWindowTree(parent, nodes,skin)
		{
			moveTo(0,0);
			resizeTo(200, parent->innerHeight());
			alignment = ALIGN_TOPLEFT;
		}

		void rightClick()
		{
			cWindowTree::rightClick();
			popupmenu = new cMenu();
			popupmenu->parent = NULL;
			popupmenu->drawstyle = 1;
			popupmenu->x = (int)mouseX;
			popupmenu->y = (int)mouseY;
			popupmenu->w = 150;
			popupmenu->opened = true;
			cMenuItem* mm;
			ADDMENUITEM(mm,popupmenu,"Remove light/category",		&MenuCommand_removefavlight); //new
			ADDMENUITEM(mm,popupmenu,"Insert light",		&MenuCommand_addfavlight); //new
			ADDMENUITEM(mm,popupmenu,"Insert category",		&MenuCommand_addfavlightcat); //new
		}

		void onchange()
		{
			int i;
			int a = selected;
			cWindowTree::cTreeNode* node;
			for(i = 0; i < (int)nodes.size(); i++)
			{
				 node = nodes[i]->getnode(a);
				 if(node != NULL)
					 break;
			}
			if(node == NULL)
				return;


			if(((cFavoriteLightsWindow*)parent)->currentkey.size() > 0)
			{
				std::vector<int> key = ((cFavoriteLightsWindow*)parent)->currentkey;
				TiXmlNode* n = favoritelights.FirstChild();
				for(i = 0; i < key[key.size()-1]; i++)
					n = n->NextSibling();

				for(i = key.size()-2; i > -1; i--)
				{
					n = n->FirstChild();
					for(int ii = 0; ii < key[i]; ii++)
						n = n->NextSibling();
				}

				if(n != NULL)
				{
					n->FirstChildElement("name")->FirstChild()->SetValue(parent->objects["name"]->getText(0).c_str());
					n->FirstChildElement("color")->SetAttribute("r", parent->objects["colorr"]->getText(0).c_str());
					n->FirstChildElement("color")->SetAttribute("g", parent->objects["colorg"]->getText(0).c_str());
					n->FirstChildElement("color")->SetAttribute("b", parent->objects["colorb"]->getText(0).c_str());
					n->FirstChildElement("brightness")->FirstChild()->SetValue(parent->objects["intensity"]->getText(0).c_str());
					n->FirstChildElement("range")->FirstChild()->SetValue(parent->objects["range"]->getText(0).c_str());
					n->FirstChildElement("maxlight")->FirstChild()->SetValue(parent->objects["maxlightincrement"]->getText(0).c_str());
					n->FirstChildElement("lightfalloff")->FirstChild()->SetValue(parent->objects["lightfalloff"]->getText(0).c_str());
					n->FirstChildElement("givesshadow")->FirstChild()->SetValue(parent->objects["castshadow"]->getText(0).c_str());
					n->FirstChildElement("height")->FirstChild()->SetValue(parent->objects["height"]->getText(0).c_str());
				}
			}

			if(node->children.size() != 0 || ((cFavoriteTreeNode*)node)->iscat)
			{
				parent->objects["name"]->setText(0, "CATEGORY");
				parent->objects["colorr"]->setText(0, "");
				parent->objects["colorg"]->setText(0, "");
				parent->objects["colorb"]->setText(0, "");
				parent->objects["intensity"]->setText(0, "");
				parent->objects["range"]->setText(0, "");
				parent->objects["maxlightincrement"]->setText(0, "");
				parent->objects["lightfalloff"]->setText(0, "");
				parent->objects["castshadow"]->setText(0, "");
				parent->objects["height"]->setText(0, "");
				((cFavoriteLightsWindow*)parent)->currentkey.clear();
				return;
			}
			std::vector<int> keys = ((cFavoriteTreeNode*)node)->keys;
			((cFavoriteLightsWindow*)parent)->currentkey = keys;

			TiXmlNode* n = favoritelights.FirstChild();
			for(i = 0; i < keys[keys.size()-1]; i++)
				n = n->NextSibling();

			for(i = keys.size()-2; i > -1; i--)
			{
				n = n->FirstChild();
				for(int ii = 0; ii < keys[i]; ii++)
					n = n->NextSibling();
			}
				
			parent->objects["name"]->setText(0, n->FirstChildElement("name")->FirstChild()->Value());
			parent->objects["colorr"]->setText(0, n->FirstChildElement("color")->Attribute("r"));
			parent->objects["colorg"]->setText(0, n->FirstChildElement("color")->Attribute("g"));
			parent->objects["colorb"]->setText(0, n->FirstChildElement("color")->Attribute("b"));
			parent->objects["intensity"]->setText(0, n->FirstChildElement("brightness")->FirstChild()->Value());
			parent->objects["range"]->setText(0, n->FirstChildElement("range")->FirstChild()->Value());
			parent->objects["maxlightincrement"]->setText(0, n->FirstChildElement("maxlight")->FirstChild()->Value());
			parent->objects["lightfalloff"]->setText(0, n->FirstChildElement("lightfalloff")->FirstChild()->Value());
			parent->objects["castshadow"]->setText(0, n->FirstChildElement("givesshadow")->FirstChild()->Value());
			parent->objects["height"]->setText(0, n->FirstChildElement("height")->FirstChild()->Value());
		}
	};


	class cWindowOkButton : public cWindowButton
	{
	public:
		cWindowOkButton(cWindow* parent, TiXmlDocument &skin) : cWindowButton(parent,skin)
		{
			alignment = ALIGN_BOTTOM;
			moveTo(100, 0);
			resizeTo(100, 20);
			text = GetMsg("wm/OK");
		}

		void click()
		{
			((cWindowTree*)parent->objects["list"])->onchange();
			favoritelights.SaveFile("data/lights.txt");
			parent->close();
		}
	};
	class cWindowCancelButton : public cWindowButton
	{
	public:
		cWindowCancelButton(cWindow* parent, TiXmlDocument &skin) : cWindowButton(parent,skin)
		{
			alignment = ALIGN_BOTTOM;
			moveTo(-100, 0);
			resizeTo(100, 20);
			text = GetMsg("wm/CANCEL");
		}
		virtual ~cWindowCancelButton() {}
		void click()
		{
			favoritelights = fs.getXml("data/lights.txt");
			parent->close();
		}
	};


	std::vector<int> currentkey;
	cFavoriteLightsWindow(cTexture* t, cFont* f, TiXmlDocument &skin) : cWindow(t,f,skin)
	{
		windowType = WT_FAVLIGHTS;
		resizable = false;
		visible = true;

		h = 270;
		w = 550;
		title = GetMsg("wm/favlight/TITLE");
		center();

		defaultObject = "OkButton";

		cWindowObject* o;
		objects["rollup"] = new cWindowRollupButton(this,skin);
		objects["close"] = new cWindowCloseButton(this,skin);


		std::vector<cWindowTree::cTreeNode*> nodes;
		cFavoriteTreeNode* windownode = new cFavoriteTreeNode("root");
		TiXmlNode* n = favoritelights.FirstChildElement();
		addlights(windownode, n);
		cWindowTree::cTreeNode* root = windownode->children[0];
		windownode->children.clear(); // to prevend the children from being deleted;
		delete windownode;
		root->parent = NULL;
		nodes.push_back(root);

		objects["list"] = new cFavoritesTree(this, nodes, skin);

		addLabel("lblName",				220, 0, GetMsg("wm/favlight/NAME"));
		addLabel("lblColor",			220, 20, GetMsg("wm/favlight/COLOR"));
		addLabel("lblIntensity",		220, 40, GetMsg("wm/favlight/INTENSITY"));
		addLabel("lblRange",			220, 60, GetMsg("wm/favlight/RANGE"));
		addLabel("lblMaxlightincrement",220, 80, GetMsg("wm/favlight/MAXLIGHTINCREMENT"));
		addLabel("lblLightfalloff",		220, 100, GetMsg("wm/favlight/LIGHTFALLOFF"));
		addLabel("lblCastsShadow",		220, 120, GetMsg("wm/favlight/CASTSSHADOW"));
		addLabel("lblHeight",			220, 140, GetMsg("wm/favlight/HEIGHT"));
		
		o = new cWindowInputBox(this,skin);
		o->alignment = ALIGN_TOPLEFT;
		o->moveTo(370,0);
		o->resizeTo(140,20);
		objects["name"] = o;
	
		o = new cWindowInputBox(this,skin);
		o->alignment = ALIGN_TOPLEFT;
		o->moveTo(300,20);
		o->resizeTo(70,20);
		objects["colorr"] = o;

		o = new cWindowInputBox(this,skin);
		o->alignment = ALIGN_TOPLEFT;
		o->moveTo(370,20);
		o->resizeTo(70,20);
		objects["colorg"] = o;
		
		o = new cWindowInputBox(this,skin);
		o->alignment = ALIGN_TOPLEFT;
		o->moveTo(440,20);
		o->resizeTo(70,20);
		objects["colorb"] = o;
		
		o = new cWindowInputBox(this,skin);
		o->alignment = ALIGN_TOPLEFT;
		o->moveTo(370,40);
		o->resizeTo(140,20);
		objects["intensity"] = o;

		o = new cWindowInputBox(this,skin);
		o->alignment = ALIGN_TOPLEFT;
		o->moveTo(370,60);
		o->resizeTo(140,20);
		objects["range"] = o;

		o = new cWindowInputBox(this,skin);
		o->alignment = ALIGN_TOPLEFT;
		o->moveTo(370,80);
		o->resizeTo(140,20);
		objects["maxlightincrement"] = o;

		o = new cWindowInputBox(this,skin);
		o->alignment = ALIGN_TOPLEFT;
		o->moveTo(370,100);
		o->resizeTo(140,20);
		objects["lightfalloff"] = o;

		o = new cWindowCheckBox(this,skin);
		o->alignment = ALIGN_TOPLEFT;
		o->moveTo(370,122);
		objects["castshadow"] = o;

		o = new cWindowInputBox(this,skin);
		o->alignment = ALIGN_TOPLEFT;
		o->moveTo(370,140);
		o->resizeTo(140,20);
		objects["height"] = o;


		((cWindowTree*)objects["list"])->onchange();

		objects["OkButton"] = new cWindowOkButton(this,skin);
		objects["CancelButton"] = new cWindowCancelButton(this,skin);
		objects["CancelButton"]->moveTo(0,0);
	}	
};

#endif