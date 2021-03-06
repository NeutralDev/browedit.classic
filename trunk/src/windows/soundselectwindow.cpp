#include "soundselectwindow.h"

#include <SDL/SDL_mixer.h>
#include "rsmeditwindow.h"
#include <undo/soundnew.h>
#include <settings.h>
#include <bengine/util.h>
#include <bengine/util/filesystem.h>

extern std::vector<std::pair<std::string, std::string> > translations;
extern void mainloop();

cSoundSelectWindow::cWindowSoundCatSelect::cWindowSoundCatSelect( cWindow* parent, std::vector<cWindowTree::cTreeNode*> n, Json::Value &skin) : cWindowTree(parent, n,skin)
{
	
}

void cSoundSelectWindow::cWindowSoundCatSelect::onClick()
{
	unsigned int i;
	cWindowTree::onClick();
	cWindowDataListBox<std::string>* box = (cWindowDataListBox<std::string>*)parent->objects["sounds"];
	box->values.clear();
	box->data.clear();
	box->setInt(-2,0);
	
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
	
	for(i = 0; i < v.size(); i++)
	{
		std::pair<std::string, std::string> p = v[i];
		box->values.push_back(p.first);
		box->data.push_back(p.second);
		
	}
	parent->resizeTo(parent->getWidth(), parent->getHeight());
	cWM::draggingWindow = NULL;
	cWM::draggingObject = NULL;
}

cSoundSelectWindow::cSoundList::cSoundList( cWindow* parent, Json::Value &skin) : cWindowDataListBox<std::string>(parent,skin)
{
	
}

void cSoundSelectWindow::cSoundList::onDoubleClick()
{
	parent->objects["play"]->onClick();
}

cSoundSelectWindow::cWindowPlayButton::cWindowPlayButton( cWindow* w, Json::Value &skin) : cWindowButton(w,skin)
{
	alignment = ALIGN_BOTTOMRIGHT;
	moveTo(0,0);
	resizeTo(100,20);
	text = "Play";
}

void cSoundSelectWindow::cWindowPlayButton::onClick()
{
	if(text == "Play")
	{
		int selected = parent->objects["sounds"]->getInt(-1);
		if(selected >= (int)((cWindowDataListBox<std::string>*)parent->objects["sounds"])->data.size())
			return;
		
		std::string filename = ((cWindowDataListBox<std::string>*)parent->objects["sounds"])->data[selected];
		
		text = "Stop";
		
		Mix_Chunk *sample;
		std::string data = bEngine::util::cFileSystem::openData(cSettings::roDir+"data/wav/" + filename);
		sample = Mix_QuickLoad_WAV((BYTE*)data.c_str());
		Mix_Volume(-1,MIX_MAX_VOLUME);
		Mix_PlayChannel(0, sample, 0);
		while(Mix_Playing(-1) > 0 && text == "Stop")
		{
			mainloop();
		}
		if(text == "Play")
			Mix_HaltChannel(-1);
		Mix_FreeChunk(sample);
		text = "Play";
	}
	else
	{
		text = "Play";
	}
}

cSoundSelectWindow::cWindowOkButton::cWindowOkButton( cWindow* w, Json::Value &skin) : cWindowButton(w,skin)
{
	alignment = ALIGN_BOTTOMRIGHT;
	moveTo(100,0);
	resizeTo(100,20);
	text = "Ok";
}

void cSoundSelectWindow::cWindowOkButton::onClick()
{
	int selected = parent->objects["sounds"]->getInt(-1);
	if(selected >= (int)((cWindowDataListBox<std::string>*)parent->objects["sounds"])->data.size())
		return;
	
	std::string filename = ((cWindowDataListBox<std::string>*)parent->objects["sounds"])->data[selected];
	
	cSound* selectedSound = ((cSoundSelectWindow*)parent)->selectedSound;
	if(selectedSound)
	{
		selectedSound->fileName = filename;
	}
	else
	{
		bEngine::math::cVector3 newPos = ((cSoundSelectWindow*)parent)->newPos;
		cSound s;
		s.fileName = filename;
		s.name = ((cWindowDataListBox<std::string>*)parent->objects["sounds"])->values[selected];
		s.pos = bEngine::math::cVector3(newPos.x/5, newPos.y-17, newPos.z/5);
		s.rot = bEngine::math::cVector3(0,-45,0);
		s.scale = bEngine::math::cVector3(0.94f,0.94f,0.94f);
		s.unknown6[0] = (BYTE)0xc7;
		s.unknown6[1] = (BYTE)0xc1;
		s.unknown6[2] = (BYTE)0xb7;
		s.unknown6[3] = (BYTE)0xd0;
		s.unknown6[4] = (BYTE)0x4e;
		s.unknown6[5] = (BYTE)0x2d;
		s.unknown6[6] = (BYTE)0xe3;
		s.unknown6[7] = (BYTE)0x43;
		
		s.name = "sound" + bEngine::util::intToString(rand() % 99999);
		s.repeatDelay = 0.5f;
		s.unknown2 = 70.0f;
		s.unknown3 = 1;
		s.unknown2 = 1;
		s.unknown5 = 1;
		s.unknown7 = -435.095f;
		s.unknown8 = 0;
		
		cGraphics::world->sounds.push_back(s);
		cGraphics::worldContainer->undoStack->push(new cUndoNewSound());
	}
	parent->close();
	
	cWindow* w = cWM::getWindow(WT_SOUNDOVERVIEW);
	if(w != NULL)
		w->userfunc(NULL);
}

cSoundSelectWindow::cWindowCancelButton::cWindowCancelButton( cWindow* w, Json::Value &skin) : cWindowButton(w,skin)
{
	alignment = ALIGN_BOTTOMRIGHT;
	moveTo(200,0);
	resizeTo(100,20);
	text = "Cancel";
}

void cSoundSelectWindow::cWindowCancelButton::onClick()
{
	parent->close();
}

cSoundSelectWindow::cSoundSelectWindow(bEngine::math::cVector3 pNewPos ) : cWindow()
{
	selectedSound = NULL;
	newPos = pNewPos; 
	windowType = WT_SOUNDSELECT;
	closeType = HIDE;
	resizable = true;
	visible = true;
	modal = false;
	
	h = cGraphics::h()-50;
	w = cGraphics::w()-50;
	title = GetMsg("wm/soundselect/TITLE");
	center();
	
	cWindowObject* o;
	
	std::vector<cWindowTree::cTreeNode*> nodes;
	std::map<std::string, cWindowTree::cTreeNode*, std::less<std::string> > lookup;
	
	std::string line, pre, filename, cat, name;
	
	std::map<std::string, std::string, std::less<std::string> > translationcache;
	
	for(unsigned int i = 0; i < cSettings::soundFiles.size(); i++)
	{
		bEngine::util::cInStream* pFile = bEngine::util::cFileSystem::open(cSettings::soundFiles[i]);
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
				for(unsigned int ii = 0; ii < translations.size(); ii++)
					cat = bEngine::util::replace(cat, translations[ii].first, translations[ii].second);
				translationcache[origcat] = cat;
			}
			
			if(lookup.find(cat) == lookup.end())
			{
				if(cat.find("/") != std::string::npos)
				{
					std::string p = cat.substr(0, cat.rfind("/"));
					if(lookup.find(p) == lookup.end())
					{
						Log(1,0,"Invalid nesting in soundfile...");
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
	}
	
	o = new cWindowSoundCatSelect(this, nodes);
	o->alignment = ALIGN_TOPLEFT;
	o->moveTo(0,0);
	o->resizeTo(400,400);
	objects["tree"] = o;
	
	o = new cSoundList(this);
	o->alignment = ALIGN_TOPLEFT;
	o->moveTo(0, 0);
	o->resizeTo(100,100);
	objects["sounds"] = o;
	
	objects["play"] = new cWindowPlayButton(this);
	objects["ok"] = new cSoundSelectWindow::cWindowOkButton(this);
	objects["cancel"] = new cSoundSelectWindow::cWindowCancelButton(this);
	
	
	objects["tree"]->onClick();
	
	//		objects["rollup"] = new cWindowRollupButton(this);
	objects["close"] = new cWindowCloseButton(this);
	
	resizeTo(w,h);
}

void cSoundSelectWindow::resizeTo( int ww, int hh )
{
	cWindow::resizeTo(ww,hh);
	objects["tree"]->resizeTo(200, innerHeight());
	objects["sounds"]->moveTo(200, 0);
	objects["sounds"]->resizeTo(innerWidth()-200, innerHeight()-20);
}

void* cSoundSelectWindow::userfunc( void* param )
{
	return &items[(cWindowTree::cTreeNode*)param];
}
