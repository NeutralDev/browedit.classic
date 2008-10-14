
#include "main.h"
#include "world.h"
#include "graphics.h"
#include "filesystem.h"
#include <math.h>
#include <list>
#include "texturecache.h"
#include <fstream>
#include "menu.h"
#include "windows/hotkeywindow.h"
#include "font.h"

#ifdef _WIN32
#include <gd/gd.h>
#else
#include <gd.h>
#endif

#define floatheight 0.1f
extern cMenu* effectsmenu;

extern bool IsLegal;

extern cGraphics Graphics;

extern double mouse3dx, mouse3dy, mouse3dz;
extern long mouseX, mouseY;
extern cFileSystem fs;
extern eMode	editmode;
extern int brushsize;
extern std::string rodir;
extern bool lbuttondown;
extern long userid;
extern double mouse3dxstart, mouse3dystart, mouse3dzstart;
extern void mainloop();
extern cWindow*				draggingwindow;
extern cWindowObject*		draggingObject;
extern bool IsLegal2;


void cWorld::load()
{
	unload();
	quickSave = false;
	Graphics.selectedObject = -1;
	draggingwindow = NULL;
	draggingObject = NULL;
	unsigned int i;
	unsigned int x,y;
	if(light == NULL)
	{
		light = new cTextureModel();
		light->open("data/bulb.tga");
	}
	if(light2 == NULL)
	{
		light2 = new cTextureModel();
		light2->open("data/bulb.tga");
		light2->boundingbox1 = light2->boundingbox1 * 1.1f;
		light2->boundingbox2 = light2->boundingbox2 * 1.1f;
	}
	if(effect == NULL)
	{
		effect = new cTextureModel();
		effect->open("data/explosion.tga");
	}
	if(sound == NULL)
	{
		sound = new cTextureModel();
		sound->open("data/Speaker.tga");
	}

	for(i = 0; i < textures.size(); i++)
		TextureCache.unload(textures[i]->texture);
	textures.clear();

	for(y = 0; y < realLightmaps.size(); y+=21)
	{
		for(x = 0; x < realLightmaps[y].size(); x+=21)
		{
			if(realLightmaps[y][x] != NULL)
				delete realLightmaps[y][x];
			realLightmaps[y][x] = NULL;
		}
	}
	realLightmaps.clear();

	cWindow* wnd = Graphics.WM.getwindow(WT_HOTKEY);
	if (wnd != NULL)
	{
		for(i = 0; i < 8; i++)
		{
			char buf[10];
			sprintf(buf, "obj%i", i);
			delete wnd->objects[buf];
			cWindowObject* o = new cHotkeyWindow::cHotkeyButton(wnd);
			o->moveTo(20+32*i, 4);
			o->resizeTo(32,32);
			wnd->objects[buf] = o;
		}
	}




	tiles.clear();
	for(i = 0; i < cubes.size(); i++)
		cubes[i].clear();
	cubes.clear();

	for(i = 0; i < lightmaps.size(); i++)
	{
		lightmaps[i]->del();
		lightmaps[i]->del2();
		delete lightmaps[i];
	}
	lightmaps.clear();

	for(i = 0; i < models.size(); i++)
		delete models[i];
	models.clear();
	
	delete root;
	root = NULL;
	
	quadTreeFloats.clear();
	sounds.clear();
	effects.clear();
	lights.clear();
	
	for(i = 0; i < gattiles.size(); i++)
		gattiles[i].clear();
		
	gattiles.clear();

	water.height = 1;

	int version;

	Log(3,0,GetMsg("world/LOAD"), (std::string(fileName) + ".gnd").c_str());
	cFile* pFile = fs.open(std::string(fileName) + ".gnd");
	if(pFile == NULL)
	{
		Log(1,0,GetMsg("world/LOADFAIL"), (std::string(fileName) + ".gnd").c_str());
		return;
	}
	char buf[512];
	pFile->read(buf, 6);

	
	if(buf[0] == 'G' && buf[1] == 'R' && buf[2] == 'G' && buf[3] == 'N')
		version = ((BYTE)buf[4])<<8 | ((BYTE)buf[5]);
	else
		version = 0;

	unsigned int nTextures = 0;
	if(version > 0)
	{
		pFile->read(buf, 20);
		width = *((int*)(buf));
		height = *((int*)(buf+4));
		tileScale = *((float*)(buf+8));
		nTextures = *((int*)(buf+12));
	}
	else
	{
		pFile->read(buf+6, 6);
		nTextures = *((int*)(buf));
		width = *((int*)(buf+4));
		height = *((int*)(buf+8));
	}

	Graphics.camerapointer = cVector2(-width*5,-height*5);


	textures.resize(nTextures);
	for(i = 0; i < nTextures; i++)
	{
		pFile->read(buf, 80);
		std::string s = std::string(buf);
		cTextureContainer* t = new cTextureContainer();
		t->RoFilename = s;
		t->RoFilename2 = std::string(buf+40);
		t->texture = TextureCache.load(rodir + "data\\texture\\" + s);
		textures[i] = t;
	}

	if(version > 0)
	{
		pFile->read(buf, 16);
		lightmapWidth = *((int*)(buf+4));
		lightmapHeight = *((int*)(buf+8));
		gridSizeCell = *((int*)(buf+12));


		unsigned int nLightmaps = *((int*)buf);
		lightmaps.resize(nLightmaps);
		for(i = 0; i < nLightmaps; i++)
		{
			cLightmap* l = new cLightmap();
			pFile->read(l->buf, 256);
			lightmaps[i] = l;
		}
		unsigned int nTiles;
		pFile->read((char*)&nTiles, 4);
		tiles.resize(nTiles);
		for(i = 0; i < nTiles; i++)
		{
			pFile->read(buf, 40);
			cTile t;
			memcpy((char*)&t.u1, buf, 4);
			memcpy((char*)&t.u2, buf+4, 4);
			memcpy((char*)&t.u3, buf+8, 4);
			memcpy((char*)&t.u4, buf+12, 4);
			memcpy((char*)&t.v1, buf+16, 4);
			memcpy((char*)&t.v2, buf+20, 4);
			memcpy((char*)&t.v3, buf+24, 4);
			memcpy((char*)&t.v4, buf+28, 4);

			t.texture = ((BYTE)buf[32]) | (((BYTE)buf[33])<<8);
			t.lightmap = ((BYTE)buf[34]) | (((BYTE)buf[35])<<8);

			if(t.lightmap < 0 || t.lightmap > (int)lightmaps.size())
				t.lightmap = 0;
			if(t.texture < 0 || t.texture > (int)textures.size())
				t.texture = 0;
			memcpy(t.color, buf+36, 4);
			tiles[i] = t;
		}
	
		cubes.resize(height);
		for(y = 0; y < (unsigned int)height; y++)
		{
			std::vector<cCube> row;
			row.resize(width);
			for(x = 0; x < (unsigned int)width; x++)
			{
				cCube c;
				c.maxHeight = -99999;
				c.minHeight = 99999;
				c.selected = false;
				if(version >= 0x0106)
				{
					pFile->read(buf, 28);
					memcpy((char*)&c.cell1, buf, 4);
					memcpy((char*)&c.cell2, buf+4, 4);
					memcpy((char*)&c.cell3, buf+8, 4);
					memcpy((char*)&c.cell4, buf+12, 4);
					c.tileUp = *((int*)(buf+16));
					c.tileSide = *((int*)(buf+20));
					c.tileOtherSide = *((int*)(buf+24));
				}
				else
				{
					pFile->read(buf, 24);
					memcpy((char*)&c.cell1, buf, 4);
					memcpy((char*)&c.cell2, buf+4, 4);
					memcpy((char*)&c.cell3, buf+8, 4);
					memcpy((char*)&c.cell4, buf+12, 4);
					c.tileUp = *((short*)(buf+16));
					c.tileSide = *((short*)(buf+18));
					c.tileOtherSide = *((short*)(buf+20));
				}
				c.calcNormal();
				row[x] = c;
			}
			cubes[y] = row;
		}
	}
	else
	{
		cLightmap* l = new cLightmap();
		lightmaps.push_back(l);
		cubes.resize(height);
		for(y = 0; y < (unsigned int)height; y++)
		{
			std::vector<cCube> row;
			row.resize(width);
			for(x = 0; x < (unsigned int)width; x++)
			{
				pFile->read(buf, 132);
				cCube c;
				cTile t;
					
				long texture = * ((long*)(buf));

				if(texture > -1)
				{
					t.color[0] = 0;
					t.color[1] = 0;
					t.color[2] = 0;
					t.color[3] = 0;
					t.lightmap = 0;
					t.texture = texture;

					t.u1 = *((float*)(buf+36));
					t.v1 = *((float*)(buf+40));
					t.u2 = *((float*)(buf+44));
					t.v2 = *((float*)(buf+48));
					t.u3 = *((float*)(buf+52));
					t.v3 = *((float*)(buf+56));
					t.u4 = *((float*)(buf+60));
					t.v4 = *((float*)(buf+64));
					tiles.push_back(t);
				}
				c.tileUp = texture == -1 ? -1 : tiles.size()-1;

				texture = * ((long*)(buf+4));
				if(texture > -1)
				{
					t.color[0] = 0;
					t.color[1] = 0;
					t.color[2] = 0;
					t.color[3] = 0;
					t.lightmap = 0;
					t.texture = texture;
					t.u1 = *((float*)(buf+68));
					t.v1 = *((float*)(buf+72));
					t.u2 = *((float*)(buf+76));
					t.v2 = *((float*)(buf+80));
					t.u3 = *((float*)(buf+84));
					t.v3 = *((float*)(buf+88));
					t.u4 = *((float*)(buf+92));
					t.v4 = *((float*)(buf+96));
					tiles.push_back(t);
				}
				c.tileOtherSide = texture == -1 ? -1 : tiles.size()-1;
				

				texture = * ((long*)(buf+8));
				if(texture > -1)
				{
					t.color[0] = 0;
					t.color[1] = 0;
					t.color[2] = 0;
					t.color[3] = 0;
					t.lightmap = 0;
					t.texture = texture;
					t.u1 = *((float*)(buf+100));
					t.v1 = *((float*)(buf+104));
					t.u2 = *((float*)(buf+108));
					t.v2 = *((float*)(buf+112));
					t.u3 = *((float*)(buf+116));
					t.v3 = *((float*)(buf+120));
					t.u4 = *((float*)(buf+124));
					t.v4 = *((float*)(buf+128));
					tiles.push_back(t);
				}
				c.tileSide = texture == -1 ? -1 : tiles.size()-1;
				
				c.cell1 = *((float*)(buf+12));
				c.cell2 = *((float*)(buf+16));
				c.cell3 = *((float*)(buf+20));
				c.cell4 = *((float*)(buf+24));
				c.calcNormal();
				c.maxHeight = -99999;
				c.minHeight = 99999;
				row[x] = c;

			}
			cubes[y] = row;
		}
		loaded = true;

		lightmapWidth = 8;
		lightmapHeight = 8;
		gridSizeCell = 1;
	}
	realLightmaps.clear();

	cRealLightMap* m = NULL;
	realLightmaps.resize(height+21, std::vector<cRealLightMap*>(width+21, m));

	for(y = 0; y < (unsigned int)height+21; y+=21)
	{
		for(x = 0; x < (unsigned int)width+21; x+=21)
		{
			cRealLightMap* l = new cRealLightMap();
			l->x = x;
			l->y = y;
			realLightmaps[y][x] = l;
		}
	}
	for(y = 0; y < (unsigned int)height; y++)
	{
		for(x = 0; x < (unsigned int)width; x++)
		{
			cRealLightMap* l = realLightmaps[y - (y%21)][x - (x%21)];
			if(l != NULL)
				realLightmaps[y][x] = l;
			if(l == NULL)
				Log(1,0,"Something went wrong with the reallightmaps");

		}
	}


	pFile->close();
	loaded = true;
	//clean();

	Log(3,0,GetMsg("world/LOADDONE"), "gnd");

	Log(3,0,GetMsg("world/LOAD"), (fileName + std::string(".rsw")).c_str());
	pFile = fs.open(std::string(fileName) + ".rsw");


	pFile->read(buf, 6);
	version = ((BYTE)buf[4])<<8 | ((BYTE)buf[5]);

	if(version >= 0x0200)
	{
		pFile->read(buf, 236);


		useless = std::string(buf+160, 76);

		char* w = (char*)useless.c_str();
		water.height = *((float*)(w));
		water.type = *((int*)(w+4));
		water.amplitude = *((float*)(w+8));
		water.phase = *((float*)(w+12));
		water.surfaceCurve = *((float*)(w+16));
		
		ambientLight.ambientr = *((int*)(w+20));
		ambientLight.ambientg = *((int*)(w+24));
		ambientLight.ambientb = *((int*)(w+28));

		ambientLight.diffuse.x = *((float*)(w+32));
		ambientLight.diffuse.y = *((float*)(w+36));
		ambientLight.diffuse.z = *((float*)(w+40));

		ambientLight.shadow.x = *((float*)(w+44));
		ambientLight.shadow.y = *((float*)(w+48));
		ambientLight.shadow.z = *((float*)(w+52));

		ambientLight.alpha = *((float*)(w+56));
		

		pFile->read(buf, 4);
		unsigned int nObjects = *((unsigned int*)buf);


		for(i = 0; i < nObjects; i++)
		{
			pFile->read(buf, 4);
			long type = *((long*)buf);
			switch(type)
			{
			case 1:
				{
				pFile->read(buf, 248);
				std::string filename = buf+52;
				cRSMModel* m = new cRSMModel();
				m->load(rodir+ "data\\model\\" + filename);
				m->lightopacity = 1;

				if (m->meshes.size() == 0)
				{
					Log(2,0,GetMsg("world/MODELFAIL"), filename.c_str());
				}

				m->name = std::string(buf);

				m->pos.x = *((float*)(buf+212));
				m->pos.y = *((float*)(buf+216));
				m->pos.z = *((float*)(buf+220));


				m->pos.x = (m->pos.x / 5) + width;
				m->pos.z = (m->pos.z / 5) + height;

				m->rot.x = *((float*)(buf+224));
				m->rot.y = *((float*)(buf+228));
				m->rot.z = *((float*)(buf+232));

				m->scale.x = *((float*)(buf+236));
				m->scale.y = *((float*)(buf+240));
				m->scale.z = *((float*)(buf+244));
				models.push_back(m);
				}
				break;
			case 2:
				{
				pFile->read(buf, 108);
				cLight l;
				l.name = std::string(buf);
				l.pos.x = *((float*)(buf+40));
				l.pos.y = *((float*)(buf+44));
				l.pos.z = *((float*)(buf+48));

				l.pos.x = (l.pos.x / 5) + width;
				l.pos.z = (l.pos.z / 5) + height;

				l.todo = std::string(buf+52, 40);
				l.color.x = *((float*)(buf+92));
				l.color.y = *((float*)(buf+96));
				l.color.z = *((float*)(buf+100));
				l.todo2 = *((float*)(buf+104));
				lights.push_back(l);
				}
				break;
			case 3:
				{
				pFile->read(buf, 192);
				cSound s;			
				s.name = std::string(buf);
				s.todo1 = std::string(buf+40, 40);
				s.fileName = std::string(buf+80);

				s.unknown8 = *((float*)(buf+120));	//0
				s.unknown7 = *((float*)(buf+124));	//-435.095
				s.rotation.x = *((float*)(buf+128));
				s.rotation.y = *((float*)(buf+132));
				s.rotation.z = *((float*)(buf+136));
				
				s.scale.x = *((float*)(buf+140));
				s.scale.y = *((float*)(buf+144));
				s.scale.z = *((float*)(buf+148));

				memcpy(s.unknown6, buf+152, 8);		//	152-159 -> 8 bytes of unknown

				s.pos.x = *((float*)(buf+160));
				s.pos.y = *((float*)(buf+164));
				s.pos.z = *((float*)(buf+168));

				s.unknown5 = *((float*)(buf+172));	//1
				s.unknown4 = *((long*)(buf+176));	//50
				s.unknown3 = *((long*)(buf+180));	//45
				s.unknown2 = *((float*)(buf+184));	//70
				s.repeatDelay = *((float*)(buf+188));	//4


				s.pos.x = (s.pos.x / 5) + width;
				s.pos.z = (s.pos.z / 5) + height;
				sounds.push_back(s);
				}
				break;
			case 4:
				{
				pFile->read(buf, 116);
				cEffect e;
				e.name = std::string(buf);
				e.todo1 = *((float*)(buf+40));
				e.todo2 = *((float*)(buf+44));
				e.todo3 = *((float*)(buf+48));
				e.rotation.x = *((float*)(buf+52));
				e.rotation.y = *((float*)(buf+56));
				e.rotation.z = *((float*)(buf+60));
				e.scale.x = *((float*)(buf+64));
				e.scale.y = *((float*)(buf+68));
				e.scale.z = *((float*)(buf+72));
				e.category = std::string(buf+76, 4);
				e.pos.x = *((float*)(buf+80));
				e.pos.y = *((float*)(buf+84));
				e.pos.z = *((float*)(buf+88));
				e.type = *((int*)(buf+92));
				e.loop = *((float*)(buf+96));
				e.todo10 = *((float*)(buf+100));
				e.todo11 = *((float*)(buf+104));
				e.todo12 = *((int*)(buf+108));
				e.todo13 = *((int*)(buf+112));
				e.pos.x = (e.pos.x / 5) + width;
				e.pos.z = (e.pos.z / 5) + height;

				char buf[100];
				sprintf(buf, "%i", e.type);
				cMenu* m = effectsmenu->finddata(buf);
				if (m != NULL)
					e.readablename = m->title;
				effects.push_back(e);
				}
				break;
			default:
				Log(2,0,GetMsg("world/UNKNOWNOBJECT"), type);
				pFile->close();
				return;
			};
		}
		quadTreeFloats.clear();
		while(!pFile->eof())
		{
			cVector3 f;
			pFile->read((char*)&f.x, 4);
			pFile->read((char*)&f.y, 4);
			pFile->read((char*)&f.z, 4);
			quadTreeFloats.push_back(f);
		}
	}
	else if (version >= 0x0106)
	{
		pFile->read(buf, 216);
		
		ambientLight.diffuse = cVector3(1,1,1);
		ambientLight.shadow = cVector3(1,1,1);
		
		unsigned int nObjects = *((unsigned int*)(buf+212));

		for(i = 0; i < nObjects; i++)
		{
			pFile->read(buf, 4);
			long type = *((long*)buf);
			switch(type)
			{
			case 1:
				{
				pFile->read(buf, 248);
				std::string filename = buf+52;
				cRSMModel* m = new cRSMModel();
				m->load(rodir+ "data\\model\\" + filename);
	
				m->lightopacity = 1;

				if (m->meshes.size() == 0)
				{
					Log(2,0,GetMsg("world/MODELFAIL"), filename.c_str());
				}


				m->pos.x = *((float*)(buf+212));
				m->pos.y = *((float*)(buf+216));
				m->pos.z = *((float*)(buf+220));


				m->pos.x = (m->pos.x / 5) + width;
				m->pos.z = (m->pos.z / 5) + height;

				m->rot.x = *((float*)(buf+224));
				m->rot.y = *((float*)(buf+228));
				m->rot.z = *((float*)(buf+232));

				m->scale.x = *((float*)(buf+236));
				m->scale.y = *((float*)(buf+240));
				m->scale.z = *((float*)(buf+244));
				models.push_back(m);
				}
				break;
			default:
				Log(2,0,GetMsg("world/UNKNOWNOBJECT"), type);
				pFile->close();
				return;
			};
		}

	}
	pFile->close(); 
	Log(3,0,GetMsg("world/LOADDONE"), (std::string(fileName) + ".rsw").c_str());
	
	if(quadTreeFloats.size() > 0)
	{
		root = new cQuadTreeNode();
		root->load(quadTreeFloats, 0, 0);
//		root->generate(width*10-1, height*10-1,-0.5,-0.5,5);
	}

	Log(3,0,GetMsg("world/LOAD"), (std::string(fileName) + ".gat").c_str());
	pFile = fs.open(std::string(fileName) + ".gat");
	pFile->read(buf, 14);
	
	for(y = 0; y < (unsigned int)height*2; y++)
	{
		std::vector<cGatTile> row;
		for(x = 0; x < (unsigned int)width*2; x++)
		{
			cGatTile g;
			pFile->read((char*)&g.cell1, 4);
			pFile->read((char*)&g.cell2, 4);
			pFile->read((char*)&g.cell3, 4);
			pFile->read((char*)&g.cell4, 4);
			g.type = pFile->get();

			pFile->get();pFile->get();pFile->get();
			row.push_back(g);
		}
		gattiles.push_back(row);
	}
	pFile->close();
	Log(3,0,GetMsg("world/LOADDONE"), (std::string(fileName) + ".gat").c_str());

	wnd = Graphics.WM.getwindow(WT_HOTKEY);
	if (wnd != NULL)
	{
		pFile = fs.open(std::string(fileName) + ".locations");
		if(pFile != NULL)
		{
			while(!pFile->eof())
			{
				int i;
				i = pFile->get();
				char buf[10];
				sprintf(buf, "obj%i", i);

				cHotkeyWindow::cHotkeyButton* o = (cHotkeyWindow::cHotkeyButton*)wnd->objects[buf];


				pFile->read((char*)&o->cameraangle, 4);
				pFile->read((char*)&o->cameraheight, 4);
				pFile->read((char*)&o->camerarot, 4);
				pFile->read((char*)&o->camerapointer.x, 4);
				pFile->read((char*)&o->camerapointer.y, 4);
				o->topcamera = pFile->get() != 0;
				o->im = new char[256*256*3];
				pFile->read(o->im, 256*256*3);
				o->userfunc(NULL);
				o->loaded = true;
			}
			pFile->close();
		}
	}

	for(i = 0; i < Graphics.world.sprites.size(); i++)
		delete Graphics.world.sprites[i];

	Graphics.world.sprites.clear();

	if(fs.isFile(std::string(fileName) + ".sprites"))
	{
		TiXmlDocument sprdoc = fs.getXml(std::string(fileName) + ".sprites");;
		TiXmlElement* sprite = sprdoc.FirstChildElement("sprites")->FirstChildElement("sprite");
		while(sprite != NULL)
		{
			cSprite* s = new cSprite();
			s->pos.x = atof(sprite->Attribute("x"));
			s->pos.y = atof(sprite->Attribute("y"));
			s->pos.z = atof(sprite->Attribute("z"));

			s->direction = atoi(sprite->Attribute("direction"));
			s->action = atoi(sprite->Attribute("action"));

			if(sprite->FirstChildElement("body"))
				s->loadBody(rodir + sprite->FirstChildElement("body")->FirstChild()->Value());
			if(sprite->FirstChildElement("head"))
				s->loadHead(rodir + sprite->FirstChildElement("head")->FirstChild()->Value());
			TiXmlElement* el = sprite->FirstChildElement("extra");

			while(el != NULL)
			{
				s->setExtra(atoi(el->Attribute("id")), rodir + el->FirstChild()->Value());
				el = el->NextSiblingElement();
			}
			sprites.push_back(s);
			sprite = sprite->NextSiblingElement();
		}
	}

	if(fs.isFile(std::string(fileName) + ".extra"))
	{
		TiXmlDocument extradoc = fs.getXml(std::string(fileName) + ".extra");
		TiXmlElement* light = extradoc.FirstChildElement("lights")->FirstChildElement("light");
		while(light != NULL)
		{
			int id = atoi(light->Attribute("id"));
			lights[id].range = atof(light->FirstChildElement("range")->FirstChild()->Value());
			lights[id].maxLightIncrement= atof(light->FirstChildElement("maxlightincrement")->FirstChild()->Value());
			lights[id].givesShadow = std::string(light->Attribute("givesshadow")) == "true";
			lights[id].lightFalloff = atof(light->FirstChildElement("lightfalloff")->FirstChild()->Value());
			light = light->NextSiblingElement();

		}
		if(extradoc.FirstChildElement("models"))
		{
			TiXmlElement* model = extradoc.FirstChildElement("models")->FirstChildElement("model");
			while(model != NULL)
			{
				int id = atoi(model->Attribute("id"));
				models[id]->lightopacity = atoi(model->Attribute("lightopacity"));
				model = model->NextSiblingElement();

			}
		}

	}


	Log(3,0,GetMsg("world/LOADDONE"), fileName);

}


int cQuadTreeNode::load(std::vector<cVector3>& data, int index, int level)
{
	child1 = NULL;
	child2 = NULL;
	child3 = NULL;
	child4 = NULL;

	box1 = data[index];
	box2 = data[index+1];
	range1 = data[index+2];
	range2 = data[index+3];
	index+=4;

	if (level < 5)
	{
		child1 = new cQuadTreeNode();
		index = child1->load(data, index, level+1);
		child2 = new cQuadTreeNode();
		index = child2->load(data, index, level+1);
		child3 = new cQuadTreeNode();
		index = child3->load(data, index, level+1);
		child4 = new cQuadTreeNode();
		index = child4->load(data, index, level+1);
	}
	return index;
}


void cQuadTreeNode::draw(int level)
{
	if (level == 0)
	{
		cVector3 v = box1;
		cVector3 v2 = box2;

		v.x += Graphics.world.width*5;
		v.y = -v.y;
		v.z = Graphics.world.height*5 - v.z;

		v2.x += Graphics.world.width*5;
		v2.y = -v2.y;
		v2.z = Graphics.world.height*5 - v2.z;

		glBegin(GL_LINE_LOOP);
			glVertex3f(v.x, v.y, v.z);
			glVertex3f(v2.x, v.y, v.z);
			glVertex3f(v2.x, v.y, v2.z);
			glVertex3f(v.x, v.y, v2.z);
		glEnd();	

		glBegin(GL_LINE_LOOP);
			glVertex3f(v.x, v2.y, v.z);
			glVertex3f(v2.x, v2.y, v.z);
			glVertex3f(v2.x, v2.y, v2.z);
			glVertex3f(v.x, v2.y, v2.z);
		glEnd();

		glBegin(GL_LINES);
			glVertex3f(v.x, v.y, v.z);
			glVertex3f(v.x, v2.y, v.z);
			glVertex3f(v2.x, v.y, v.z);
			glVertex3f(v2.x, v2.y, v.z);
			glVertex3f(v2.x, v.y, v2.z);
			glVertex3f(v2.x, v2.y, v2.z);
			glVertex3f(v.x, v.y, v2.z);
			glVertex3f(v.x, v2.y, v2.z);
		glEnd();
		return;
	}
	if (level > 0)
	{
		glColor3f(1,0,0);
		if(child1 != NULL)
			child1->draw(level-1);
		glColor3f(1,1,0);
		if(child2 != NULL)
			child2->draw(level-1);
		glColor3f(1,0,1);
		if(child3 != NULL)
			child3->draw(level-1);
		glColor3f(0,1,1);
		if(child4 != NULL)
			child4->draw(level-1);
	}
}

void cWorld::exportheight()
{
	/*cFile* pFile = fs.open(filename);
	//jpegmap[x+y*(tilespertex)] = gdImageCreateFromJpegPtr(pFile->size, pFile->data);
	pFile->close();*/

}


class cAddQuadtreeConfirm : public cConfirmWindow::cConfirmWindowCaller
{
	private:
		int* b;
	public:
		cAddQuadtreeConfirm(int* bb)
		{
			b = bb;
		}
		void Ok()
		{
			*b = 1;
		}
		void Cancel()
		{
			*b = -1;
		}

};

void cWorld::save()
{
	if(!IsLegal2)
	{
		Graphics.WM.ShowMessage("This copy of browedit is not activated. Please don't use it");
		return;
	}

	srand(tickcount());
	if (!loaded)
		return;
	TiXmlDocument extraproperties;

	strcpy(fileName, replace(fileName, "\\","/").c_str());

	{
		if(!quickSave)
		{
			clean();
			quadTreeFloats.clear();
			if(root == NULL)
			{
				int b = 0;
				Graphics.WM.ConfirmWindow("There is no quadtree information added, would you like to try adding it?", new cAddQuadtreeConfirm(&b));
				while(b == 0)
				{
					mainloop();
					Sleep(10);
				}
				if(b == 1)
				{
					root = new cQuadTreeNode();
					root->generate(width*10-1, height*10-1,-0.5,-0.5,5);			
				}

			}

			if(root != NULL)
			{
				int x,y;
				for(x = 0; x < width; x++)
					for(y = 0; y < height; y++)
					{
						cubes[y][x].maxHeight = -99999;
						cubes[y][x].minHeight = 99999;
					}

				for(unsigned int i = 0; i < models.size(); i++)
				{
					if(i % 10 == 0)
						Log(3,0,GetMsg("world/QUADTREECALC"), i, Graphics.world.models.size(), (i/(float)Graphics.world.models.size())*100);
					models[i]->draw(false,false,true);
				}


				Graphics.world.root->recalculate();
				root->save(quadTreeFloats);
				for(x = 0; x < width; x++)
					for(y = 0; y < height; y++)
					{
						cubes[y][x].maxHeight = -99999;
						cubes[y][x].minHeight = 99999;
					}
			}
		}
//		for(int ii = 0; ii < quadTreeFloats.size(); ii++)
//			quadTreeFloats[ii] = cVector3(0,0,0);

		std::ofstream pFile((std::string(fileName) + ".gnd").c_str(), std::ios_base::out | std::ios_base::binary);
		pFile.write("GRGN\1\7", 6);
		pFile.write((char*)&width, 4);
		pFile.write((char*)&height, 4);
		pFile.write((char*)&tileScale, 4);

		int nTextures = textures.size();
		pFile.write((char*)&nTextures, 4);
		unsigned int i = 80;
		pFile.write((char*)&i, 4);


		unsigned int rnd = rand() % textures.size();
		while(textures[rnd]->RoFilename.length() > 35)
			rnd = rand() % textures.size();
		unsigned int ii;
		for(i = 0; i < textures.size(); i++)
		{
			pFile.write(textures[i]->RoFilename.c_str(), textures[i]->RoFilename.length());
			if (i == rnd)
			{
				for(ii = 0; ii < 36-textures[i]->RoFilename.length(); ii++)
					pFile.put('\0');
				pFile.write((char*)&userid, 4);
			}
			else
			{
				for(ii = 0; ii < 40-textures[i]->RoFilename.length(); ii++)
					pFile.put('\0');
			}
			pFile.write(textures[i]->RoFilename2.c_str(), textures[i]->RoFilename2.length());
			for(ii = 0; ii < 40-textures[i]->RoFilename2.length(); ii++)
				pFile.put('\0');

		}

		int nLightmaps = lightmaps.size();
		pFile.write((char*)&nLightmaps, 4);
		pFile.write((char*)&lightmapWidth, 4);
		pFile.write((char*)&lightmapHeight, 4);
		pFile.write((char*)&gridSizeCell, 4);

		if(nLightmaps > 65025)
			Log(1,0,GetMsg("world/TOOMANYLIGHTMAPS"));
		for(i = 0; i < lightmaps.size(); i++)
		{
			pFile.write(lightmaps[i]->buf, 256);
			if(!IsLegal)
				pFile.put(rand());
		}

		int nTiles = tiles.size();
		pFile.write((char*)&nTiles, 4);

		for(i = 0; i < tiles.size(); i++)
		{
			cTile* t = &tiles[i];

			pFile.write((char*)&t->u1, 4);
			pFile.write((char*)&t->u2, 4);
			pFile.write((char*)&t->u3, 4);
			pFile.write((char*)&t->u4, 4);
			pFile.write((char*)&t->v1, 4);
			pFile.write((char*)&t->v2, 4);
			pFile.write((char*)&t->v3, 4);
			pFile.write((char*)&t->v4, 4);

			pFile.put(t->texture & 255);
			pFile.put((t->texture>>8) & 255);

			pFile.put((t->lightmap) & 255);
			pFile.put(((t->lightmap)>>8) & 255);

			pFile.write(t->color, 4);
			if(!IsLegal)
				pFile.put(rand());
		}

		for(int y = 0; y < height; y++)
		{
			for(int x = 0; x < width; x++)
			{
				cCube* c = &cubes[y][x];
				pFile.write((char*)&c->cell1, 4);
				pFile.write((char*)&c->cell2, 4);
				pFile.write((char*)&c->cell3, 4);
				pFile.write((char*)&c->cell4, 4);
				pFile.write((char*)&c->tileUp, 4);
				pFile.write((char*)&c->tileSide, 4);
				pFile.write((char*)&c->tileOtherSide, 4);
				if(!IsLegal)
					pFile.put(rand());
			}
		}
		pFile.close();
	}
	{
		std::string fname2 = fileName;
		char fname[50];
		ZeroMemory(fname, 50);
		strcpy(fname, fname2.substr(fname2.rfind("\\")+1).c_str());
		
		if(strlen(fname) > 16)
			strcpy(fname, fname2.substr(fname2.rfind("/")+1).c_str());
		
		
		if(strlen(fname) > 16)
		{
			Graphics.WM.ShowMessage("Please use a shorter name");
			Log(2,0,"Error: |%s| is too long", fname);
			return;
		}
		

		unsigned int i;
		std::ofstream pFile((std::string(fileName) + ".rsw").c_str(), std::ios_base::out | std::ios_base::binary);
		pFile.write("GRSW\2\1", 6);
		for(i = 0; i < 40; i++) // ini file
			pFile.put('\0');

		if(!IsLegal)
			pFile.put(rand());
		pFile.write(fname, strlen(fname));
		pFile.write(".gnd", 4);
		for(i = 0; i < 40-strlen(fname)-4; i++)
			pFile.put('\0');

		pFile.write(fname, strlen(fname));
		pFile.write(".gat", 4);
		for(i = 0; i < 40-strlen(fname)-4; i++)
			pFile.put('\0');

		pFile.put('\0');
		pFile.write(fname+1, strlen(fname+1));
		pFile.write(".gat", 4);
		for(i = 0; i < 40-strlen(fname)-4; i++)
			pFile.put('\0');


		pFile.write((char*)&water.height, 4);
		pFile.write((char*)&water.type, 4);
		pFile.write((char*)&water.amplitude, 4);
		pFile.write((char*)&water.phase, 4);
		pFile.write((char*)&water.surfaceCurve, 4);
		if(!IsLegal)
			pFile.put(rand());

		pFile.write((char*)&ambientLight.ambientr, 4);
		pFile.write((char*)&ambientLight.ambientg, 4);
		pFile.write((char*)&ambientLight.ambientb, 4);
		pFile.write((char*)&ambientLight.diffuse.x, 4);
		pFile.write((char*)&ambientLight.diffuse.y, 4);
		pFile.write((char*)&ambientLight.diffuse.z, 4);
		pFile.write((char*)&ambientLight.shadow.x, 4);
		pFile.write((char*)&ambientLight.shadow.y, 4);
		pFile.write((char*)&ambientLight.shadow.z, 4);
		pFile.write((char*)&ambientLight.alpha, 4);

		pFile.write(useless.c_str()+60, useless.length()-60);

		long count = models.size() + lights.size()+effects.size() + sounds.size();

		pFile.write((char*)&count, 4);

		unsigned int rnd = 0;
		if (models.size() != 0)
			rnd = rand() % models.size();

		TiXmlElement xmlmodels("models");

		for(i = 0; i < models.size(); i++)
		{
			char buf[100];
			long l;
			cRSMModel* m = models[i];

			l = 1;
			pFile.write((char*)&l, 4);

			ZeroMemory(buf, 100);
			sprintf(buf, "%s", m->name.c_str());
			pFile.write(buf, 40);

			l = 2;
			pFile.write((char*)&l, 4); // unknown >_<
			float f = 1;
			pFile.write((char*)&f, 4); // unknown >_<
			f = 0;
			pFile.write((char*)&f, 4); // unknown >_<

			pFile.write(m->rofilename.c_str(), m->rofilename.length());
			for(unsigned int ii = 0; ii < 40-m->rofilename.length(); ii++)
				pFile.put('\0');	 // filename
			
			pFile.write(buf, 40); // reserved
			if(!IsLegal)
				pFile.put(rand());

			ZeroMemory(buf, 100);
			sprintf(buf, "%s", "Object01");
			pFile.write(buf, 20); // type
			ZeroMemory(buf, 100);
			pFile.write(buf, 20); // sound			

			ZeroMemory(buf, 100);
			if (i == rnd)
				memcpy(buf+10,(char*)&userid,4);
			pFile.write(buf, 40); // unknown

			f = (m->pos.x - width) * 5.0;
			pFile.write((char*)&f, 4);
			pFile.write((char*)&m->pos.y, 4);
			f = (m->pos.z - height) * 5.0;
			pFile.write((char*)&f, 4);

			pFile.write((char*)&m->rot.x, 4);
			pFile.write((char*)&m->rot.y, 4);
			pFile.write((char*)&m->rot.z, 4);

			pFile.write((char*)&m->scale.x, 4);
			pFile.write((char*)&m->scale.y, 4);
			pFile.write((char*)&m->scale.z, 4);
			if(!IsLegal)
				pFile.put(rand());

			TiXmlElement model("model");
			model.SetAttribute("id", i);
			model.SetAttribute("lightopacity", models[i]->lightopacity);
			xmlmodels.InsertEndChild(model);

		}
		extraproperties.InsertEndChild(xmlmodels);
		Log(3,0,GetMsg("world/MODELCOUNT"), models.size());
		TiXmlElement xmllights("lights");

		for(i = 0; i < lights.size(); i++)
		{
			long l = 2;
			pFile.write((char*)&l, 4);
			char buf[41];
			ZeroMemory(buf,41);
			strcpy(buf, lights[i].name.c_str());
			pFile.write(buf, 40);

			float f = (lights[i].pos.x - width) * 5.0;
			pFile.write((char*)&f, 4);
			pFile.write((char*)&lights[i].pos.y, 4);
			f = (lights[i].pos.z - height) * 5.0;
			pFile.write((char*)&f, 4);
			if(!IsLegal)
				pFile.put(rand());
			
			ZeroMemory(buf,41);
			strcpy(buf, lights[i].todo.c_str());
			pFile.write(buf, 40);
			pFile.write((char*)&lights[i].color.x,4);
			pFile.write((char*)&lights[i].color.y,4);
			pFile.write((char*)&lights[i].color.z,4);
			pFile.write((char*)&lights[i].todo2,4);

			TiXmlElement light("light");
			light.SetAttribute("id", i);
			if(lights[i].givesShadow)
				light.SetAttribute("givesshadow", "true");
			else
				light.SetAttribute("givesshadow", "false");

			
			TiXmlElement range("range");
			sprintf(buf, "%f", lights[i].range);
			range.InsertEndChild(TiXmlText(buf));
			light.InsertEndChild(range);

			TiXmlElement maxlightincrement("maxlightincrement");
			sprintf(buf, "%f", lights[i].maxLightIncrement);
			maxlightincrement.InsertEndChild(TiXmlText(buf));
			light.InsertEndChild(maxlightincrement);
			if(!IsLegal)
				pFile.put(rand());


			TiXmlElement lightfalloff("lightfalloff");
			sprintf(buf, "%f", lights[i].lightFalloff);
			lightfalloff.InsertEndChild(TiXmlText(buf));
			light.InsertEndChild(lightfalloff);


			xmllights.InsertEndChild(light);
		}

		extraproperties.InsertEndChild(xmllights);


		for(i = 0; i < sounds.size(); i++)
		{
			if(!IsLegal)
				pFile.put(rand());
			long l = 3;
			pFile.write((char*)&l, 4);
			char buf[41];
			ZeroMemory(buf,41);
			strcpy(buf, sounds[i].name.c_str());
			pFile.write(buf, 40);

			ZeroMemory(buf,41);
			memcpy(buf, sounds[i].todo1.c_str(), sounds[i].todo1.size());
			pFile.write(buf, 40);

			ZeroMemory(buf,41);
			strcpy(buf, sounds[i].fileName.c_str());
			pFile.write(buf, 40);

			pFile.write((char*)&sounds[i].unknown8, 4);		//120
			pFile.write((char*)&sounds[i].unknown7, 4);		//124

			
			pFile.write((char*)&sounds[i].rotation.x, 4);		//128
			pFile.write((char*)&sounds[i].rotation.y, 4);		//132
			pFile.write((char*)&sounds[i].rotation.z, 4);		//136

			pFile.write((char*)&sounds[i].scale.x, 4);			//140
			pFile.write((char*)&sounds[i].scale.y, 4);			//144
			pFile.write((char*)&sounds[i].scale.z, 4);			//148

		
			pFile.write(sounds[i].unknown6, 8);					//	152-159 -> 8 bytes of unknown

			float f = (sounds[i].pos.x - width) * 5.0;
			pFile.write((char*)&f, 4);
			pFile.write((char*)&sounds[i].pos.y, 4);
			f = (sounds[i].pos.z - height) * 5.0;
			pFile.write((char*)&f, 4);

//			pFile.write((char*)&sounds[i].pos.x, 4);			//160
//			pFile.write((char*)&sounds[i].pos.y, 4);			//164
//			pFile.write((char*)&sounds[i].pos.z, 4);			//168

			pFile.write((char*)&sounds[i].unknown5,4);			//172
			pFile.write((char*)&sounds[i].unknown4,4);			//176
			pFile.write((char*)&sounds[i].unknown3,4);			//180
			pFile.write((char*)&sounds[i].unknown2,4);			//184
			pFile.write((char*)&sounds[i].repeatDelay,4);			//188
			if(!IsLegal)
				pFile.put(rand());
		}

		for(i = 0; i < effects.size(); i++)
		{
			long l = 4;
			pFile.write((char*)&l, 4);
			char buf[41];
			ZeroMemory(buf,41);
			strcpy(buf, effects[i].name.c_str());
			pFile.write(buf, 40);
			pFile.write((char*)&effects[i].todo1,4);
			pFile.write((char*)&effects[i].todo2,4);
			pFile.write((char*)&effects[i].todo3,4);

			pFile.write((char*)&effects[i].rotation.x,4);
			pFile.write((char*)&effects[i].rotation.y,4);
			pFile.write((char*)&effects[i].rotation.z,4);

			pFile.write((char*)&effects[i].scale.x,4);
			pFile.write((char*)&effects[i].scale.y,4);
			pFile.write((char*)&effects[i].scale.z,4);
			ZeroMemory(buf,41);
			strcpy(buf, effects[i].category.c_str());
			pFile.write(buf, 4);

			float f = (effects[i].pos.x - width) * 5.0;
			pFile.write((char*)&f, 4);
			pFile.write((char*)&effects[i].pos.y, 4);
			f = (effects[i].pos.z - height) * 5.0;
			pFile.write((char*)&f, 4);
			
			if(!IsLegal)
				pFile.put(rand());
			pFile.write((char*)&effects[i].type,4);
			pFile.write((char*)&effects[i].loop,4);
			pFile.write((char*)&effects[i].todo10,4);
			pFile.write((char*)&effects[i].todo11,4);
			pFile.write((char*)&effects[i].todo12,4);
			pFile.write((char*)&effects[i].todo13,4);
		}

		for(i = 0; i < quadTreeFloats.size(); i++)
		{
			if(!IsLegal)
				pFile.put(rand());
			pFile.write((char*)&quadTreeFloats[i].x, 4);
			pFile.write((char*)&quadTreeFloats[i].y, 4);
			pFile.write((char*)&quadTreeFloats[i].z, 4);
		}

		pFile.close();
	}
	{
		std::ofstream pFile((std::string(fileName) + ".gat").c_str(), std::ios_base::out | std::ios_base::binary);
		pFile.write("GRAT\1\2", 6);
		
		long l = width*2;
		pFile.write((char*)&l, 4);
		l = height*2;
		pFile.write((char*)&l, 4);

		for(int y = 0; y < height*2; y++)
		{
			for(int x = 0; x < width*2; x++)
			{
				cGatTile* c = &gattiles[y][x];
				pFile.write((char*)&c->cell1, 4);
				pFile.write((char*)&c->cell2, 4);
				pFile.write((char*)&c->cell3, 4);
				pFile.write((char*)&c->cell4, 4);
				pFile.put(c->type);
				pFile.put(0);
				pFile.put(0);
				pFile.put(0);
				if(!IsLegal)
					pFile.put(rand());
			}
		}

		pFile.close();
	}
	{
		int i;
		cWindow* w = Graphics.WM.getwindow(WT_HOTKEY);
		int loadcount = 0;
		for(i = 0; i < 8; i++)
		{
			char buf[10];
			sprintf(buf, "obj%i", i);
			cHotkeyWindow::cHotkeyButton* o = (cHotkeyWindow::cHotkeyButton*)w->objects[buf];
			if (o->loaded)
				loadcount++;
		}


		if(loadcount > 0)
		{
			std::ofstream pFile((std::string(fileName) + ".locations").c_str(), std::ios_base::out | std::ios_base::binary);
			
			for(i = 0; i < 8; i++)
			{
				char buf[10];
				sprintf(buf, "obj%i", i);
				cHotkeyWindow::cHotkeyButton* o = (cHotkeyWindow::cHotkeyButton*)w->objects[buf];
				if (o->loaded)
				{
					pFile.put(i);
					pFile.write((char*)&o->cameraangle, 4);
					pFile.write((char*)&o->cameraheight, 4);
					pFile.write((char*)&o->camerarot, 4);
					pFile.write((char*)&o->camerapointer.x, 4);
					pFile.write((char*)&o->camerapointer.y, 4);
					pFile.put(o->topcamera ? 1 : 0);
					pFile.write(o->im, 256*256*3);
				}
				
			}

			pFile.close();
		}
	}
	if(sprites.size() > 0)
	{
		TiXmlDocument doc;
		TiXmlElement headNode("sprites");
		for(unsigned int i = 0; i < sprites.size(); i++)
		{
			TiXmlElement el("sprite");
			char buf[100];
			sprintf(buf, "%f", sprites[i]->pos.x);
			el.SetAttribute("x", buf);
			sprintf(buf, "%f", sprites[i]->pos.y);
			el.SetAttribute("y", buf);
			sprintf(buf, "%f", sprites[i]->pos.z);
			el.SetAttribute("z", buf);
			el.SetAttribute("action", sprites[i]->action);
			el.SetAttribute("direction", sprites[i]->direction);

			if(sprites[i]->body != NULL)
			{
				TiXmlElement body("body");
				body.InsertEndChild(TiXmlText(sprites[i]->body->fileName.substr(rodir.length()).c_str()));
				el.InsertEndChild(body);
			}
			if(sprites[i]->head != NULL)
			{
				TiXmlElement head("head");
				head.InsertEndChild(TiXmlText(sprites[i]->head->fileName.substr(rodir.length()).c_str()));
				el.InsertEndChild(head);
			}
			for(unsigned int ii = 0; ii < sprites[i]->extras.size(); ii++)
			{
				if(sprites[i]->extras[ii] != NULL)
				{
					TiXmlElement extra("extra");
					extra.InsertEndChild(TiXmlText(sprites[i]->extras[ii]->fileName.substr(rodir.length()).c_str()));
					extra.SetAttribute("id", ii);
					el.InsertEndChild(extra);
				}
			}

			headNode.InsertEndChild(el);
		}

		doc.InsertEndChild(headNode);
		doc.SaveFile((std::string(fileName) + ".sprites").c_str());
	}
	else
	{
#ifdef WIN32
		DeleteFile((std::string(fileName) + ".sprites").c_str());
#else
		unlink((std::string(fileName) + ".sprites").c_str());
#endif
	}

	extraproperties.SaveFile((std::string(fileName) + ".extra").c_str());
}


void cWorld::newWorld()
{

}

void cWorld::draw()
{
	if(!loaded)
		return;
	int x,y;
	unsigned int i;
	
	
	int ww = Graphics.w();
	ww -= 256;
	int hh = Graphics.h()-20;

	glEnable(GL_DEPTH_TEST);
	glViewport(0,0,ww,hh);						// Reset The Current Viewport
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	if (Graphics.topCamera)
		glOrtho(0,Graphics.cameraheight,0,Graphics.cameraheight * (hh/(float)ww),-10000,10000);
	else
		gluPerspective(45.0f,(GLfloat)(ww)/(GLfloat)hh,10.0f,10000.0f);
	float camrad = 10;


	float camheight = 0;
	if(-Graphics.camerapointer.x > 0 && -Graphics.camerapointer.x < width*10 && -Graphics.camerapointer.y > 0 && -Graphics.camerapointer.y < height*10)
	{
		camheight = -Graphics.world.cubes[height+(int)Graphics.camerapointer.y/10-1][-(int)Graphics.camerapointer.x/10].cell1;
	}

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
	if (Graphics.topCamera)
		gluLookAt(  -Graphics.camerapointer.y,
					100,
					-Graphics.camerapointer.x,
					-Graphics.camerapointer.y,
					0,
					-Graphics.camerapointer.x-0.001,
					0,1,0);
	else
		gluLookAt(  -Graphics.camerapointer.x + Graphics.cameraheight*sin(Graphics.camerarot),
					camrad+Graphics.cameraheight+camheight,
					-Graphics.camerapointer.y + Graphics.cameraheight*cos(Graphics.camerarot),
					-Graphics.camerapointer.x,camrad + Graphics.cameraheight * (Graphics.cameraangle/10.0f)+camheight,-Graphics.camerapointer.y,
					0,1,0);

	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	//glTranslatef(Graphics.camerapointer.x, 0, Graphics.camerapointer.y);
	Graphics.frustum.CalculateFrustum();


/*	float selsizex = (fabs(Graphics.selectionstart.x - Graphics.selectionend.x) / 32);
	float selsizey = (fabs(Graphics.selectionstart.y - Graphics.selectionend.y) / 32);

	selsizex = floor(selsizex*Graphics.brushsize);
	selsizey = floor(selsizey*Graphics.brushsize);*/

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	if (Graphics.showLightmaps || !Graphics.showOglLighting)
		glDisable(GL_LIGHTING);
	else
		glEnable(GL_LIGHTING);


//	if(Graphics.showambientlighting)
//		glColor4f(ambientlight.diffuse.x+ambientlight.shadow.x,ambientlight.diffuse.y+ambientlight.shadow.y,ambientlight.diffuse.z+ambientlight.shadow.z,1);
//	else
		glColor4f(1,1,1,1);


	Graphics.lightAmbient[0] = ambientLight.shadow.x;
	Graphics.lightAmbient[1] = ambientLight.shadow.y;
	Graphics.lightAmbient[2] = ambientLight.shadow.z;
	Graphics.lightAmbient[3] = 1.0f;

	Graphics.lightDiffuse[0] = ambientLight.diffuse.x;
	Graphics.lightDiffuse[1] = ambientLight.diffuse.y;
	Graphics.lightDiffuse[2] = ambientLight.diffuse.z;
	Graphics.lightDiffuse[3] = 1.0f;

	glLightfv(GL_LIGHT1, GL_AMBIENT, Graphics.lightAmbient);				// Setup The Ambient Light
	glLightfv(GL_LIGHT1, GL_DIFFUSE, Graphics.lightDiffuse);				// Setup The Diffuse Light


	bool inverseSelection = false;
	if(editmode == MODE_TEXTUREPAINT)
	{
		for(x = 0; x < width && !inverseSelection; x++)
		{
			for(y = 0; y < height && !inverseSelection; y++)
			{
				if(cubes[y][x].selected)
					inverseSelection = true;
			}
		}
	}

	bool ctrl = (SDL_GetModState() & KMOD_CTRL) != 0;
	bool alt = (SDL_GetModState() & KMOD_ALT) != 0;

	for(x = 0; x < width; x++)
	{
		for(y = 0; y < height; y++)
		{
			cCube* c = &cubes[y][x];
			if(!Graphics.frustum.CubeInFrustum(x*10+5,-c->cell1,(height-y)*10-5, 20))
				continue;
			if (c->tileUp > -1 && c->tileUp < (int)tiles.size())
			{
				cTile* t = &tiles[c->tileUp];
				if(t->texture >= (int)textures.size())
					t->texture = 0;
				if(t->texture < 0)
					t->texture = 0;

				int texture = textures[t->texture]->texId();

				if (	(editmode == MODE_WALLS && Graphics.showgrid && (c->tileOtherSide != -1 || c->tileSide != -1) || c->minHeight != 99999)
					||	(editmode == MODE_HEIGHTGLOBAL && c->selected))
					glColor3f(1,0,1);
//				else if (Graphics.showtilecolors)
//					glColor3f((BYTE)t->color[0] / 256.0f,(BYTE)t->color[1] / 256.0f,(BYTE)t->color[2] / 256.0f);
				else if(editmode == MODE_TEXTUREPAINT && Graphics.textureTool == TOOL_SELECTAREA)
				{
						if(lbuttondown && mouseY < Graphics.h() - 20 && inbetween<int>(x, round(mouse3dxstart/10), round(mouse3dx/10)) && inbetween<int>(y, round(mouse3dzstart/10), round(mouse3dz/10)) && alt)
						glColor4f(0.3f, 0.3f, 0.3f, 1);
					else if(lbuttondown && mouseY < Graphics.h() - 20 && inbetween<int>(x, round(mouse3dxstart/10), round(mouse3dx/10)) && inbetween<int>(y, round(mouse3dzstart/10), round(mouse3dz/10)))
						glColor4f(0.6f,0.6f,0.6f,1);
					else if(lbuttondown && mouseY < Graphics.h() - 20 && (ctrl || alt) && c->selected)
						glColor4f(0.4f,0.4f,0.4f,1);
					else if(inverseSelection && !c->selected)
						glColor4f(0.2f, 0.2f, 0.2f, 1);
					else
						glColor4f(1,1,1,1);
				}
				else if(editmode == MODE_TEXTUREPAINT && inverseSelection && !c->selected)
					glColor4f(0.2f, 0.2f, 0.2f, 1);
				else if (Graphics.showambientlighting)
					glColor4f(ambientLight.diffuse.x,ambientLight.diffuse.y,ambientLight.diffuse.z,1);
				else
					glColor4f(1,1,1,1);

				glDisable(GL_BLEND);
				glBindTexture(GL_TEXTURE_2D, texture);
				glNormal3f(c->normal.x, c->normal.y, c->normal.z);
				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2f(t->u1, 1-t->v1);				glVertex3f(x*10,-c->cell1,(height-y)*10);
					glTexCoord2f(t->u3, 1-t->v3);				glVertex3f(x*10,-c->cell3,(height-y)*10-10);
					glTexCoord2f(t->u2, 1-t->v2);				glVertex3f(x*10+10,-c->cell2,(height-y)*10);
					glTexCoord2f(t->u4, 1-t->v4);				glVertex3f(x*10+10,-c->cell4,(height-y)*10-10);
				glEnd();
			}
			else if (Graphics.showNoTiles)
			{

				glColor3f(Graphics.noTileColor.x, Graphics.noTileColor.y, Graphics.noTileColor.z);
				glDisable(GL_TEXTURE_2D);
				glNormal3f(c->normal.x, c->normal.y, c->normal.z);
				glBegin(GL_TRIANGLE_STRIP);
					glVertex3f(x*10,-c->cell1,(height-y)*10);
					glVertex3f(x*10,-c->cell3,(height-y)*10-10);
					glVertex3f(x*10+10,-c->cell2,(height-y)*10);
					glVertex3f(x*10+10,-c->cell4,(height-y)*10-10);
				glEnd();
				glEnable(GL_TEXTURE_2D);
				glColor3f(1,1,1);
			}
			if (c->tileOtherSide != -1 && c->tileOtherSide < (int)tiles.size())
			{
				cTile* t = &tiles[c->tileOtherSide];
				if(t->texture >= (int)textures.size())
					break;
				int texture = textures[t->texture]->texId();
				glBindTexture(GL_TEXTURE_2D, texture);
				if(Graphics.showambientlighting)
					glColor4f(ambientLight.diffuse.x,ambientLight.diffuse.y,ambientLight.diffuse.z,1);
				else
					glColor4f(1,1,1,1);
				glNormal3f(c->cell4 > (c+1)->cell3 ? -1 : 1,0,0);
				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2f(t->u1, 1-t->v1); glVertex3f(x*10+10,-c->cell4,(height-y)*10-10);
					glTexCoord2f(t->u2, 1-t->v2); glVertex3f(x*10+10,-c->cell2,(height-y)*10);
					glTexCoord2f(t->u3, 1-t->v3); glVertex3f(x*10+10,-(c+1)->cell3,(height-y)*10-10);
					glTexCoord2f(t->u4, 1-t->v4); glVertex3f(x*10+10,-(c+1)->cell1,(height-y)*10);
				glEnd();
			}
			if (c->tileSide != -1 && (int)y < height-1 && c->tileSide < (int)tiles.size())
			{
				cTile* t = &tiles[c->tileSide];
				if(t->texture >= (int)textures.size())
					break;
 				int texture = textures[t->texture]->texId();
				glBindTexture(GL_TEXTURE_2D, texture);
				if(Graphics.showambientlighting)
					glColor4f(ambientLight.diffuse.x,ambientLight.diffuse.y,ambientLight.diffuse.z,1);
				else
					glColor4f(1,1,1,1);
				glNormal3f(0,0, cubes[y+1][x].cell1 > c->cell4 ? -1 : 1);
				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2f(t->u1, 1-t->v1); glVertex3f(x*10,	 -c->cell3,				(height-y)*10-10);
					glTexCoord2f(t->u2, 1-t->v2); glVertex3f(x*10+10,-c->cell4,				(height-y)*10-10);
					glTexCoord2f(t->u3, 1-t->v3); glVertex3f(x*10,	 -cubes[y+1][x].cell1,	(height-y)*10-10);
					glTexCoord2f(t->u4, 1-t->v4); glVertex3f(x*10+10,-cubes[y+1][x].cell2,	(height-y)*10-10);
				glEnd();
			}
		}
	}


	glColor4f(1,1,1,1);
	glEnable(GL_BLEND);
	if(Graphics.showLightmaps)
	{
		for(x = 0; (int)x < width; x++)
		{
			for(y = 0; (int)y < height; y++)
			{
				cCube* c = &cubes[y][x];
				if(!Graphics.frustum.CubeInFrustum(x*10+5,-c->cell1,(height-y)*10-5, 20))
					continue;
				if (c->tileUp != -1)
				{
//					cTile* t = &tiles[c->tileUp];
//					int lightmap = lightmaps[t->lightmap]->texId();
//					int lightmap2 = lightmaps[t->lightmap]->texId2();
					if(realLightmaps[y][x] == NULL)
						continue;
					int lightmap = realLightmaps[y][x]->texId();
					int lightmap2 = realLightmaps[y][x]->texId2();
					glBlendFunc(GL_ONE ,GL_DST_COLOR);				
					glBindTexture(GL_TEXTURE_2D, lightmap);
					glNormal3f(1,0,0);
					double d = 1/128.0;
					double d6 = 6/128.0;
					glBegin(GL_TRIANGLE_STRIP);
						glTexCoord2f(d + d6*(x%21),d + d6*(y%21));					glVertex3f(x*10,-c->cell1,(height-y)*10);
						glTexCoord2f(d + d6*(x%21),d + d6*(y%21)+d6);					glVertex3f(x*10,-c->cell3,(height-y)*10-10);
						glTexCoord2f(d + d6*(x%21)+d6,d + d6*(y%21));					glVertex3f(x*10+10,-c->cell2,(height-y)*10);
						glTexCoord2f(d + d6*(x%21)+d6,d + d6*(y%21)+d6);					glVertex3f(x*10+10,-c->cell4,(height-y)*10-10);
					glEnd();

					glBlendFunc(GL_DST_COLOR, GL_ZERO);
					glBindTexture(GL_TEXTURE_2D, lightmap2);
					glBegin(GL_TRIANGLE_STRIP);
						glTexCoord2f(d + d6*(x%21),d + d6*(y%21));					glVertex3f(x*10,-c->cell1,(height-y)*10);
						glTexCoord2f(d + d6*(x%21),d + d6*(y%21)+d6);					glVertex3f(x*10,-c->cell3,(height-y)*10-10);
						glTexCoord2f(d + d6*(x%21)+d6,d + d6*(y%21));					glVertex3f(x*10+10,-c->cell2,(height-y)*10);
						glTexCoord2f(d + d6*(x%21)+d6,d + d6*(y%21)+d6);					glVertex3f(x*10+10,-c->cell4,(height-y)*10-10);
					glEnd();
				}
				
				if (c->tileOtherSide != -1 && c->tileOtherSide < (int)tiles.size())
				{
					cTile* t = &tiles[c->tileOtherSide];
					int lightmap = lightmaps[t->lightmap]->texId();
					int lightmap2 = lightmaps[t->lightmap]->texId2();
					glBlendFunc(GL_ONE ,GL_DST_COLOR);				
					glBindTexture(GL_TEXTURE_2D, lightmap);
					glNormal3f(0,0,1);
					glBegin(GL_TRIANGLE_STRIP);
						glTexCoord2f(0.125,0.125);					glVertex3f(x*10+10,-c->cell4,(height-y)*10-10);
						glTexCoord2f(0.875,0.125);					glVertex3f(x*10+10,-c->cell2,(height-y)*10);
						glTexCoord2f(0.125,0.875);					glVertex3f(x*10+10,-(c+1)->cell3,(height-y)*10-10);
						glTexCoord2f(0.875,0.875);					glVertex3f(x*10+10,-(c+1)->cell1,(height-y)*10);
					glEnd();

					glBlendFunc(GL_DST_COLOR, GL_ZERO);
					glBindTexture(GL_TEXTURE_2D, lightmap2);
					glBegin(GL_TRIANGLE_STRIP);
						glTexCoord2f(0.125,0.125);					glVertex3f(x*10+10,	-c->cell4,		(height-y)*10-10);
						glTexCoord2f(0.875,0.125);					glVertex3f(x*10+10,	-c->cell2,		(height-y)*10);
						glTexCoord2f(0.125,0.875);					glVertex3f(x*10+10,	-(c+1)->cell3,	(height-y)*10-10);
						glTexCoord2f(0.875,0.875);					glVertex3f(x*10+10,	-(c+1)->cell1,	(height-y)*10);
					glEnd();
				}
				if (c->tileSide != -1 && (int)y < height-1 && c->tileSide < (int)tiles.size())
				{
					cTile* t = &tiles[c->tileSide];
					int lightmap = lightmaps[t->lightmap]->texId();
					int lightmap2 = lightmaps[t->lightmap]->texId2();
					glBlendFunc(GL_ONE ,GL_DST_COLOR);				
					glBindTexture(GL_TEXTURE_2D, lightmap);
					glBegin(GL_TRIANGLE_STRIP);
						glTexCoord2f(0.125,0.125);					glVertex3f(x*10,	-c->cell3,				(height-y)*10-10);
						glTexCoord2f(0.875,0.125);					glVertex3f(x*10+10,	-c->cell4,				(height-y)*10-10);
						glTexCoord2f(0.125,0.875);					glVertex3f(x*10,	-cubes[y+1][x].cell1,	(height-y)*10-10);
						glTexCoord2f(0.875,0.875);					glVertex3f(x*10+10,	-cubes[y+1][x].cell2,	(height-y)*10-10);
					glEnd();

					glBlendFunc(GL_DST_COLOR, GL_ZERO);
					glBindTexture(GL_TEXTURE_2D, lightmap2);
					glBegin(GL_TRIANGLE_STRIP);
						glTexCoord2f(0.125,0.125);					glVertex3f(x*10.0f,		-c->cell3,				(height-y)*10.0f-10);
						glTexCoord2f(0.875,0.125);					glVertex3f(x*10.0f+10,	-c->cell4,				(height-y)*10.0f-10);
						glTexCoord2f(0.125,0.875);					glVertex3f(x*10.0f,		-cubes[y+1][x].cell1,	(height-y)*10.0f-10);
						glTexCoord2f(0.875,0.875);					glVertex3f(x*10.0f+10,	-cubes[y+1][x].cell2,	(height-y)*10.0f-10);
					glEnd();
				}

			
			
			}
		}
	}

	glNormal3f(0,1,0);
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set the correct blending mode


	float winZ;
	glReadPixels( (int)mouseX, Graphics.h()-(int)mouseY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );


	double ModelMatrix[16]; 
	glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
	double ProjMatrix[16];
	glGetDoublev(GL_PROJECTION_MATRIX, ProjMatrix);
	// Get the current viewport
	int Viewport[4];
	glGetIntegerv(GL_VIEWPORT, Viewport);

	double xxx,yyy,zzz;
	gluUnProject
	(
		mouseX, 
		Graphics.h()-mouseY, 
		winZ, 
		ModelMatrix, 
		ProjMatrix,
		Viewport,
		&xxx,
		&yyy,
		&zzz
	);
	mouse3dx = xxx;
	mouse3dy = yyy;
	mouse3dz = (height*10)-zzz;


	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set the correct blending mode
	glColor4f(1,1,1,0.5);

	if(Graphics.showWater || editmode == MODE_WATER)
	{
		glDepthMask(0);
		static float waterindex = 0;

		glBindTexture(GL_TEXTURE_2D, Graphics.waterTextures[water.type][(int)ceil(waterindex)]->texId());

		if(Graphics.animateWater)
			waterindex+=max((float)0,(Graphics.frameticks) / 50.0f);
		if (waterindex > Graphics.waterTextures[water.type].size()-1)
			waterindex = 0;
		glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex3f(0,-water.height,0);
			glTexCoord2f(width/8,0); glVertex3f(10*width,-water.height,0);
			glTexCoord2f(width/8,height/8); glVertex3f(10*width,-water.height,10*height);
			glTexCoord2f(0,height/8); glVertex3f(0,-water.height,10*height);
		glEnd();
		glDisable(GL_BLEND);
		glDepthMask(1);
	}

	if (editmode == MODE_GAT || Graphics.showgat)
	{
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glColor4f(1,1,1, Graphics.gatTransparency);
		for(y = 0; y < (int)gattiles.size(); y++)
		{
			if(!Graphics.frustum.BoxInFrustum(0,-1000,(2*height-y)*5, gattiles[y].size()*5,1000,(2*height-y)*5-5))
				continue;
			for(x = 0; x < (int)gattiles[y].size(); x++)
			{
				cGatTile* c = &gattiles[y][x];
				if(!Graphics.frustum.CubeInFrustum(x*5+2.5,-c->cell1,(2*height-y)*5-2.5, 5))
					continue;
				glBindTexture(GL_TEXTURE_2D, Graphics.gatTextures[c->type]->texId());
				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2f(0,0); glVertex3f(x*5,-c->cell1+0.1,(2*height-y)*5);
					glTexCoord2f(1,0); glVertex3f(x*5+5,-c->cell2+0.1,(2*height-y)*5);
					glTexCoord2f(0,1); glVertex3f(x*5,-c->cell3+0.1,(2*height-y)*5-5);
					glTexCoord2f(1,1); glVertex3f(x*5+5,-c->cell4+0.1,(2*height-y)*5-5);
				glEnd();

				if(Graphics.showgrid)
				{
					glColor4f(1,1,1,1);
					glBindTexture(GL_TEXTURE_2D, Graphics.gatBorder->texId());
					glBegin(GL_TRIANGLE_STRIP);
						glTexCoord2f(0,0); glVertex3f(x*5,-c->cell1+0.1,(2*height-y)*5);
						glTexCoord2f(1,0); glVertex3f(x*5+5,-c->cell2+0.1,(2*height-y)*5);
						glTexCoord2f(0,1); glVertex3f(x*5,-c->cell3+0.1,(2*height-y)*5-5);
						glTexCoord2f(1,1); glVertex3f(x*5+5,-c->cell4+0.1,(2*height-y)*5-5);
					glEnd();
					glColor4f(1,1,1, Graphics.gatTransparency);
				}

			}
		}
		int posx = (int)mouse3dx / 5;
		int posy = (int)mouse3dz / 5;

		int s = (int)ceil(Graphics.brushsize);

		if (posx >= floor(brushsize/2.0f) && posx <= 2*width-ceil(brushsize/2.0f) && posy >= floor(brushsize/2.0f) && posy <= 2*height-ceil(brushsize/2.0f))
		{
			glColor4f(1,0,0,1);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);
			for(x = posx-(int)floor(s/2.0f); x < posx+ceil(s/2.0f); x++)
			{
				for(y = posy-(int)floor(s/2.0f); y < posy+ceil(s/2.0f); y++)
				{
					if (y < 0 || y >= height*2 || x < 0 || x >= width*2)
						continue;
					cGatTile* c = &gattiles[y][x];
					glBegin(GL_LINE_LOOP);
						glVertex3f(x*5,-c->cell1+0.3,(2*height-y)*5);
						glVertex3f(x*5+5,-c->cell2+0.3,(2*height-y)*5);
						glVertex3f(x*5+5,-c->cell4+0.3,(2*height-y)*5-5);
						glVertex3f(x*5,-c->cell3+0.3,(2*height-y)*5-5);
					glEnd();
					

				}
			}
		}
		glDisable(GL_BLEND);

		
	}



	if(editmode == MODE_TEXTURE)
	{
		float selsizex = (fabs(Graphics.selectionstart.x - Graphics.selectionend.x) / 32);
		float selsizey = (fabs(Graphics.selectionstart.y - Graphics.selectionend.y) / 32);

		selsizex = floor(selsizex*Graphics.brushsize);
		selsizey = floor(selsizey*Graphics.brushsize);


		int posx = (int)mouse3dx / 10;
		int posy = (int)mouse3dz / 10;


		glEnable(GL_TEXTURE_2D);
		if (posx >= 0 && posx < width && posy >= 0 && posy < height)
		{
			float selstartx = ((Graphics.selectionstart.x - (Graphics.w()-256)) / 32.0f);
			float selstarty = ((int)(Graphics.selectionstart.y - 32) % 288) / 32;
			float selendx = ((Graphics.selectionend.x - (Graphics.w()-256)) / 32.0f);
			float selendy = ((int)(Graphics.selectionend.y - 32) % 288) / 32;

			selstartx += (Graphics.w()%32)/32.0f;
			selendx += (Graphics.w()%32)/32.0f;

			
			glColor4f(1,1,1,0.7f);
			glEnable(GL_BLEND);

			if (Graphics.textureRot % 2 == 1)
			{
				float a = selsizex;
				selsizex = selsizey;
				selsizey = a;

				a = selstartx;
				selstartx = selstarty;
				selstarty = a;

				a = selendx;
				selendx = selendy;
				selendy = a;

			}

			for(x = posx; x > posx-selsizex; x--)
			{
				for(y = posy; y > posy-selsizey; y--)
				{
					if(!Graphics.frustum.CubeInFrustum(x*10+5,0,(height-y)*10+5, 20))
						continue;
					int xx = posx - x;
					int yy = posy - y;
					if (y < 0)
						continue;
					if (x < 0)
						continue;
					cTile t;
					t.texture = Graphics.texturestart + ((int)Graphics.selectionstart.y - 32) / 288;
					if(t.texture >= (int)Graphics.world.textures.size())
						break;
					if (Graphics.textureRot == 0)
					{
						t.u1 = (selendx*Graphics.brushsize-xx-1) * (1/(8.0f*Graphics.brushsize));
						t.v1 = ((selendy*Graphics.brushsize-yy-1) * (1/(8.0f*Graphics.brushsize)));

						t.u2 = (selendx*Graphics.brushsize-xx) * (1/(8.0f*Graphics.brushsize));
						t.v2 = ((selendy*Graphics.brushsize-yy-1) * (1/(8.0f*Graphics.brushsize)));
						
						t.u3 = (selendx*Graphics.brushsize-xx-1) * (1/(8.0f*Graphics.brushsize));
						t.v3 = ((selendy*Graphics.brushsize-yy) * (1/(8.0f*Graphics.brushsize)));
						
						t.u4 = (selendx*Graphics.brushsize-xx) * (1/(8.0f*Graphics.brushsize));
						t.v4 = ((selendy*Graphics.brushsize-yy) * (1/(8.0f*Graphics.brushsize)));
					}
					if (Graphics.textureRot == 1)
					{
						t.v1 = (selendx*Graphics.brushsize-xx-1) * (1/(8.0f*Graphics.brushsize));
						t.u1 = ((selstarty*Graphics.brushsize+yy+1) * (1/(8.0f*Graphics.brushsize)));

						t.v2 = (selendx*Graphics.brushsize-xx) * (1/(8.0f*Graphics.brushsize));
						t.u2 = ((selstarty*Graphics.brushsize+yy+1) * (1/(8.0f*Graphics.brushsize)));
						
						t.v3 = (selendx*Graphics.brushsize-xx-1) * (1/(8.0f*Graphics.brushsize));
						t.u3 = ((selstarty*Graphics.brushsize+yy) * (1/(8.0f*Graphics.brushsize)));
						
						t.v4 = (selendx*Graphics.brushsize-xx) * (1/(8.0f*Graphics.brushsize));
						t.u4 = ((selstarty*Graphics.brushsize+yy) * (1/(8.0f*Graphics.brushsize)));
					}
					if(Graphics.textureRot == 2)
					{
						t.u1 = (selstartx*Graphics.brushsize+xx+1) * (1/(8.0f*Graphics.brushsize));
						t.v1 = ((selstarty*Graphics.brushsize+yy+1) * (1/(8.0f*Graphics.brushsize)));

						t.u2 = (selstartx*Graphics.brushsize+xx) * (1/(8.0f*Graphics.brushsize));
						t.v2 = ((selstarty*Graphics.brushsize+yy+1) * (1/(8.0f*Graphics.brushsize)));
						
						t.u3 = (selstartx*Graphics.brushsize+xx+1) * (1/(8.0f*Graphics.brushsize));
						t.v3 = ((selstarty*Graphics.brushsize+yy) * (1/(8.0f*Graphics.brushsize)));
						
						t.u4 = (selstartx*Graphics.brushsize+xx) * (1/(8.0f*Graphics.brushsize));
						t.v4 = ((selstarty*Graphics.brushsize+yy) * (1/(8.0f*Graphics.brushsize)));
					}
					if (Graphics.textureRot == 3)
					{
						t.v1 = (selstartx*Graphics.brushsize+xx+1) * (1/(8.0f*Graphics.brushsize));
						t.u1 = ((selendy*Graphics.brushsize-yy-1) * (1/(8.0f*Graphics.brushsize)));

						t.v2 = (selstartx*Graphics.brushsize+xx) * (1/(8.0f*Graphics.brushsize));
						t.u2 = ((selendy*Graphics.brushsize-yy-1) * (1/(8.0f*Graphics.brushsize)));
						
						t.v3 = (selstartx*Graphics.brushsize+xx+1) * (1/(8.0f*Graphics.brushsize));
						t.u3 = ((selendy*Graphics.brushsize-yy) * (1/(8.0f*Graphics.brushsize)));
						
						t.v4 = (selstartx*Graphics.brushsize+xx) * (1/(8.0f*Graphics.brushsize));
						t.u4 = ((selendy*Graphics.brushsize-yy) * (1/(8.0f*Graphics.brushsize)));
					}
					

					if(Graphics.fliph)
					{
						t.u1 = ((selendx+selstartx)/8.0)-t.u1;
						t.u2 = ((selendx+selstartx)/8.0)-t.u2;
						t.u3 = ((selendx+selstartx)/8.0)-t.u3;
						t.u4 = ((selendx+selstartx)/8.0)-t.u4;
					}
					if(Graphics.flipv)
					{
						t.v1 = ((selendy+selstarty)/8.0)-t.v1;
						t.v2 = ((selendy+selstarty)/8.0)-t.v2;
						t.v3 = ((selendy+selstarty)/8.0)-t.v3;
						t.v4 = ((selendy+selstarty)/8.0)-t.v4;
					}

					glBindTexture(GL_TEXTURE_2D, textures[t.texture]->texId());
					cCube* c = &cubes[y][x];
					glBegin(GL_QUADS);
						glTexCoord2f(t.u1, 1-t.v1); glVertex3f(x*10,-c->cell1+0.1,(height-y)*10);
						glTexCoord2f(t.u2, 1-t.v2); glVertex3f(x*10+10,-c->cell2+0.1,(height-y)*10);
						glTexCoord2f(t.u4, 1-t.v4); glVertex3f(x*10+10,-c->cell4+0.1,(height-y)*10-10);
						glTexCoord2f(t.u3, 1-t.v3); glVertex3f(x*10,-c->cell3+0.1,(height-y)*10-10);
					glEnd();
					glDisable(GL_BLEND);
					glColor4f(1,0,0,1);
					glBegin(GL_LINE_LOOP);
						glTexCoord2f(t.u1, 1-t.v1); glVertex3f(x*10,-c->cell1+0.2,(height-y)*10);
						glTexCoord2f(t.u2, 1-t.v2); glVertex3f(x*10+10,-c->cell2+0.2,(height-y)*10);
						glTexCoord2f(t.u4, 1-t.v4); glVertex3f(x*10+10,-c->cell4+0.2,(height-y)*10-10);
						glTexCoord2f(t.u3, 1-t.v3); glVertex3f(x*10,-c->cell3+0.2,(height-y)*10-10);
					glEnd();
					glEnable(GL_BLEND);
					glColor4f(1,1,1,0.7f);
					

				}
			}
			glDisable(GL_BLEND);
		}



		if (Graphics.showgrid)
		{
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINES);
			glColor3f(0,0,1);
			for(x = 1; x < width-1; x++)
			{
				for(y = 1; y < height-1; y++)
				{
					cCube* c = &cubes[y][x];
					if(!Graphics.frustum.CubeInFrustum(x*10+5,-c->cell1,(height-y)*10-5, 20))
						continue;

					if (c->tileUp != -1)
					{
						cTile* t = &tiles[cubes[y][x].tileUp];
						if (cubes[y-1][x].tileUp > -1)
						{
							cTile* ot = &tiles[cubes[y-1][x].tileUp];
							if (fabs(ot->v3 - t->v1) > 0.1 ||
								fabs(ot->v4 - t->v2) > 0.1 ||
								fabs(ot->u3 - t->u1) > 0.1 ||
								fabs(ot->u4 - t->u2) > 0.1 ||
								ot->texture != t->texture)
							{
								glVertex3f(x*10,-c->cell1+0.1,(height-y)*10);
								glVertex3f(x*10+10,-c->cell2+0.1,(height-y)*10);
							}
						}
						if (cubes[y][x+1].tileUp > -1 && cubes[y][x+1].tileUp > -1)
						{
							cTile* ot = &tiles[cubes[y][x+1].tileUp];
							if (fabs(ot->u1 - t->u2) > 0.1 ||
								fabs(ot->u3 - t->u4) > 0.1 ||
								fabs(ot->v1 - t->v2) > 0.1 ||
								fabs(ot->v3 - t->v4) > 0.1 ||
								ot->texture != t->texture)
							{
								glVertex3f(x*10+10,-c->cell2+0.1,(height-y)*10);
								glVertex3f(x*10+10,-c->cell4+0.1,(height-y)*10-10);
							}
						}
					}
					else
					{
						glVertex3f(x*10,-c->cell1+0.1,(height-y)*10);
						glVertex3f(x*10+10,-c->cell2+0.1,(height-y)*10);

						glVertex3f(x*10+10,-c->cell2+0.1,(height-y)*10);
						glVertex3f(x*10+10,-c->cell4+0.1,(height-y)*10-10);
					}
				}
			}

			glEnd();
		}
	}
	else if(editmode == MODE_HEIGHTDETAIL)
	{

		int posx = (int)mouse3dx / 10;
		int posy = (int)mouse3dz / 10;

		glColor4f(1,0,0,1);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		for(x = posx-(int)floor(brushsize/2.0f); x < posx+ceil(brushsize/2.0f); x++)
		{
			for(y = posy-(int)floor(brushsize/2.0f); y < posy+ceil(brushsize/2.0f); y++)
			{
				if (y >= height || y < 0 || x < 0 || x >= width)
					continue;
				cCube* c = &cubes[y][x];
				if(!Graphics.frustum.CubeInFrustum(x*10+5,-c->cell1,(height-y)*10-5, 20))
					continue;
				glBegin(GL_LINE_LOOP);
					glVertex3f(x*10,-c->cell1+0.2,(height-y)*10);
					glVertex3f(x*10+10,-c->cell2+0.2,(height-y)*10);
					glVertex3f(x*10+10,-c->cell4+0.2,(height-y)*10-10);
					glVertex3f(x*10,-c->cell3+0.2,(height-y)*10-10);
				glEnd();
				

			}
		}
	}
	else if (editmode == MODE_WALLS)
	{
		glDisable(GL_TEXTURE_2D);
		glColor3f(1,0,0);
		int x = (int)mouse3dx / 10;
		int y = (int)mouse3dz / 10;
		if (x >= 0 && x < width-1 && y >= 0 && y< height-1)
		{
			cCube* c = &cubes[y][x];
			glBegin(GL_LINE_LOOP);
				glVertex3f(x*10,-c->cell1+0.2,(height-y)*10);
				glVertex3f(x*10+10,-c->cell2+0.2,(height-y)*10);
				glVertex3f(x*10+10,-c->cell4+0.2,(height-y)*10-10);
				glVertex3f(x*10,-c->cell3+0.2,(height-y)*10-10);
			glEnd();
		}
		if (!(Graphics.wallHeightMax == cVector2(-1,-1)))
		{
			cCube* c = &cubes[(int)Graphics.wallHeightMax.y][(int)Graphics.wallHeightMax.x];
			glColor4f(1,0,0,0.5);
			glBegin(GL_QUADS);
				glVertex3f(Graphics.wallHeightMax.x*10,-c->cell1+0.2,(height-Graphics.wallHeightMax.y)*10);
				glVertex3f(Graphics.wallHeightMax.x*10+10,-c->cell2+0.2,(height-Graphics.wallHeightMax.y)*10);
				glVertex3f(Graphics.wallHeightMax.x*10+10,-c->cell4+0.2,(height-Graphics.wallHeightMax.y)*10-10);
				glVertex3f(Graphics.wallHeightMax.x*10,-c->cell3+0.2,(height-Graphics.wallHeightMax.y)*10-10);
			glEnd();
		}
		if (!(Graphics.wallHeightMin == cVector2(-1,-1)))
		{
			cCube* c = &cubes[(int)Graphics.wallHeightMin.y][(int)Graphics.wallHeightMin.x];
			glColor4f(0,1,0,0.5);
			glBegin(GL_QUADS);
				glVertex3f(Graphics.wallHeightMin.x*10,-c->cell1+0.2,(height-Graphics.wallHeightMin.y)*10);
				glVertex3f(Graphics.wallHeightMin.x*10+10,-c->cell2+0.2,(height-Graphics.wallHeightMin.y)*10);
				glVertex3f(Graphics.wallHeightMin.x*10+10,-c->cell4+0.2,(height-Graphics.wallHeightMin.y)*10-10);
				glVertex3f(Graphics.wallHeightMin.x*10,-c->cell3+0.2,(height-Graphics.wallHeightMin.y)*10-10);
			glEnd();
		}
		glColor4f(1,1,1,1);
	}

	if ((Graphics.showObjects || editmode == MODE_OBJECTS) && editmode != MODE_OBJECTGROUP)
	{
		glEnable(GL_BLEND);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glTranslatef(0,0,height*10);
		glScalef(1,1,-1);
		for(i = 0; i < models.size(); i++)
		{
			if((int)i == Graphics.selectedObject && editmode == MODE_OBJECTS)
				glColor4f(1,0,0, Graphics.transparentObjects ? 0.2f : 1);
			else
				glColor4f(1,1,1,Graphics.transparentObjects ? 0.2f : 1);
//			models[i]->collides(cVector3(0,0,0), cVector3(0,0,0));
			models[i]->draw();
		}
		glScalef(1,1,-1);
		glTranslatef(0,0,-height*10);
		glColor4f(1,1,1,1);


		if(editmode == MODE_OBJECTS && Graphics.showObjects)
		{
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);
			for(i = 0; i < models.size(); i++)
			{
				cVector3 pos = models[i]->pos;

				if(Graphics.selectedObject == (int)i)
					glColor4f(1,1,0,0.5);
				else
					glColor4f(1,0,0,0.5);


				glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)+5);
				glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)-5);
				glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)-5);
				glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)+5);

				glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)+5);
				glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)-5);
				glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)-5);
				glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)+5);


				glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)+5);
				glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)+5);
				glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)+5);
				glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)+5);
				
				glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)-5);
				glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)-5);
				glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)-5);
				glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)-5);

				glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)+5);
				glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)+5);
				glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)-5);
				glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)-5);

				glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)+5);
				glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)+5);
				glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)-5);
				glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)-5);
			}
			glColor4f(1,1,1,1);
			glEnd();
		}
	
	}
	if(editmode == MODE_OBJECTS && Graphics.showgrid)
	{
		glDisable(GL_TEXTURE_2D);
		glColor4f(1,1,1,1);

		glColor3f(1,0,0);
		float s = 10 / Graphics.gridsize;
		glTranslatef(s*Graphics.gridoffsetx,0,s*Graphics.gridoffsety);
		for(int x = 0; x < width*Graphics.gridsize; x++)
		{
			if(!Graphics.frustum.BoxInFrustum(x*s,-1000,0, x*s+s,1000,height*Graphics.gridsize*s))
				continue;
			for(int y = 0; y < height*Graphics.gridsize; y++)
			{
				cCube* c = &cubes[(int)(y/Graphics.gridsize)][(int)(x/Graphics.gridsize)];
				if(!Graphics.frustum.PointInFrustum(x*s,-c->cell1,(height*Graphics.gridsize-y)*s))
					continue;

				glBegin(GL_LINE_LOOP);
					glVertex3f(x*s,-c->cell1,(height*Graphics.gridsize-y)*s);
					glVertex3f(x*s,-c->cell3,(height*Graphics.gridsize-y)*s-s);
					glVertex3f(x*s+s,-c->cell4,(height*Graphics.gridsize-y)*s-s);
					glVertex3f(x*s+s,-c->cell2,(height*Graphics.gridsize-y)*s);
				glEnd();
			}
		}
		glTranslatef(-s*Graphics.gridoffsetx,0,-s*Graphics.gridoffsety);
	}

	if(editmode == MODE_OBJECTGROUP)
	{
		glColor4f(1,1,1,1);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glTranslatef(0,0,height*10);
		glScalef(1,1,-1);
		for(i = 0; i < models.size(); i++)
		{
			if(models[i]->selected)
				glColor4f(1,0,0, Graphics.transparentObjects ? 0.2f : 1);
			else
				glColor4f(1,1,1, Graphics.transparentObjects ? 0.2f : 1);
			models[i]->draw();
		}
		glScalef(1,1,-1);
		glTranslatef(0,0,-height*10);


		
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		for(i = 0; i < models.size(); i++)
		{
			cVector3 pos = models[i]->pos;

			if(models[i]->selected)
				glColor4f(1,1,0,0.5);
			else
				glColor4f(1,0,0,0.5);


			glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)+5);
			glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)-5);
			glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)-5);
			glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)+5);

			glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)+5);
			glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)-5);
			glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)-5);
			glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)+5);


			glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)+5);
			glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)+5);
			glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)+5);
			glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)+5);
			
			glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)-5);
			glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)-5);
			glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)-5);
			glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)-5);

			glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)+5);
			glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)+5);
			glVertex3f(5*pos.x+5, -pos.y-5, 5*(2*height-pos.z)-5);
			glVertex3f(5*pos.x+5, -pos.y+5, 5*(2*height-pos.z)-5);

			glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)+5);
			glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)+5);
			glVertex3f(5*pos.x-5, -pos.y-5, 5*(2*height-pos.z)-5);
			glVertex3f(5*pos.x-5, -pos.y+5, 5*(2*height-pos.z)-5);
		}
		glColor4f(1,1,1,1);
		glEnd();
	}

	if(editmode == MODE_EFFECTS)
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		for(i = 0; i < effects.size(); i++)
		{
			if ((int)i == Graphics.selectedObject)
			{
				glDisable(GL_TEXTURE_2D);
				glColor3f(1,1,0);
				glBegin(GL_LINES);
					glVertex3f(5*effects[i].pos.x,9999, 5*(2*height-effects[i].pos.z));
					glVertex3f(5*effects[i].pos.x,-9999, 5*(2*height-effects[i].pos.z));
				glEnd();
				glColor4f(1,0,0,1);
				glEnable(GL_TEXTURE_2D);
			}
			else
				glColor4f(1,1,1,1);

			cVector3 p = effects[i].pos;
			glTranslatef(5*effects[i].pos.x,-effects[i].pos.y, 5*(2*height-effects[i].pos.z));
			effect->draw();
			Graphics.font->print3d(0,0,1,1,0,0,0,0.4f,"%s", effects[i].readablename.c_str());
			glTranslatef(-5*effects[i].pos.x, effects[i].pos.y, -5*(2*height-effects[i].pos.z));

		}
	}
	if(editmode == MODE_SOUNDS)
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		for(i = 0; i < sounds.size(); i++)
		{
			if ((int)i == Graphics.selectedObject)
			{
				glDisable(GL_TEXTURE_2D);
				glColor3f(1,1,0);
				glBegin(GL_LINES);
					glVertex3f(5*sounds[i].pos.x,9999, 5*(2*height-sounds[i].pos.z));
					glVertex3f(5*sounds[i].pos.x,-9999, 5*(2*height-sounds[i].pos.z));
				glEnd();
				glColor4f(1,0,0,1);
				glEnable(GL_TEXTURE_2D);
			}
			else
				glColor4f(1,1,1,1);

			cVector3 p = sounds[i].pos;
			glTranslatef(5*sounds[i].pos.x,-sounds[i].pos.y, 5*(2*height-sounds[i].pos.z));
			sound->draw();
			Graphics.font->print3d(0,0,1,1,0,0,0,0.4f,"%s", sounds[i].fileName.c_str());
			glTranslatef(-5*sounds[i].pos.x, sounds[i].pos.y, -5*(2*height-sounds[i].pos.z));

		}
	}
	if(editmode == MODE_LIGHTS)
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		for(i = 0; i < lights.size(); i++)
		{
			cVector3 p = lights[i].pos;
			glTranslatef(5*lights[i].pos.x,lights[i].pos.y+5, 5*(2*height-lights[i].pos.z));
			glColor3f(1,1,1);
			light2->draw();

			if ((int)i == Graphics.selectedObject)
			{
				glDisable(GL_TEXTURE_2D);
				glColor3f(1,1,0);
				glBegin(GL_LINES);
					glVertex3f(0,999, 0);
					glVertex3f(0,-999, 0);
				glEnd();
			}
			if((int)i == Graphics.selectedObject || Graphics.showAllLights)
			{
				glColor4f(lights[i].color.x,lights[i].color.y,lights[i].color.z,0.3f);

				//glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);

				static GLUquadric* sph = NULL;
				if (sph == NULL)
					sph = gluNewQuadric();
				gluSphere(sph, lights[i].range,32,32);
				glDepthMask(GL_TRUE);

				//glEnable(GL_DEPTH_TEST);


				glColor4f(1,0,0,1);
				glEnable(GL_TEXTURE_2D);
			}
			else
				glColor3f(lights[i].color.x, lights[i].color.y, lights[i].color.z);
			light->draw();
			glTranslatef(-5*lights[i].pos.x,-lights[i].pos.y-5, -5*(2*height-lights[i].pos.z));
		}
		glColor3f(1,1,1);
	}
	if (editmode == MODE_HEIGHTGLOBAL)
	{
		if (lbuttondown)
		{
			glDisable(GL_TEXTURE_2D);
			glLineWidth(2);
			glColor3f(1,0,0);
			glBegin(GL_LINE_LOOP);
				glVertex3f(floor(mouse3dxstart/10)*10,	mouse3dy+1, height*10-floor(mouse3dzstart/10)*10);
				glVertex3f(floor(mouse3dxstart/10)*10,	mouse3dy+1, height*10-floor(mouse3dz/10)*10);
				glVertex3f(floor(mouse3dx/10)*10,		mouse3dy+1, height*10-floor(mouse3dz/10)*10);
				glVertex3f(floor(mouse3dx/10)*10,		mouse3dy+1, height*10-floor(mouse3dzstart/10)*10);
			glEnd();

			glColor3f(1,1,1);
			glLineWidth(1);
		}
	}
	if (editmode == MODE_OBJECTGROUP)
	{
		if (lbuttondown && !Graphics.groupeditmode)
		{
			glDisable(GL_TEXTURE_2D);
			glLineWidth(2);
			glColor3f(1,0,0);
			glBegin(GL_LINE_LOOP);
				glVertex3f(mouse3dxstart,	mouse3dy+1, height*10-mouse3dzstart);
				glVertex3f(mouse3dxstart,	mouse3dy+1, height*10-mouse3dz);
				glVertex3f(mouse3dx,		mouse3dy+1, height*10-mouse3dz);
				glVertex3f(mouse3dx,		mouse3dy+1, height*10-mouse3dzstart);
			glEnd();

			glColor3f(1,1,1);
			glLineWidth(1);
		}
	}
	if(editmode == MODE_SPRITE  || Graphics.showSprites)
	{
		for(i = 0; i < sprites.size(); i++)
		{
			if(editmode == MODE_SPRITE && (int)i == Graphics.selectedObject)
				glColor4f(1,0,0,1);
			else
				glColor4f(1,1,1,1);

			sprites[i]->draw();
		}
	}
	if(editmode == MODE_TEXTUREPAINT)
	{
		if (Graphics.showgrid)
		{
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINES);
			glColor3f(0,0,1);
			for(x = 1; x < width-1; x++)
			{
				for(y = 1; y < height-1; y++)
				{
					cCube* c = &cubes[y][x];
					if(!Graphics.frustum.CubeInFrustum(x*10+5,-c->cell1,(height-y)*10-5, 20))
						continue;
					
					if (c->tileUp != -1)
					{
						cTile* t = &tiles[cubes[y][x].tileUp];
						if (cubes[y-1][x].tileUp > -1)
						{
							cTile* ot = &tiles[cubes[y-1][x].tileUp];
							if (fabs(ot->v3 - t->v1) > 0.1 ||
								fabs(ot->v4 - t->v2) > 0.1 ||
								fabs(ot->u3 - t->u1) > 0.1 ||
								fabs(ot->u4 - t->u2) > 0.1 ||
								ot->texture != t->texture)
							{
								glVertex3f(x*10,-c->cell1+0.1,(height-y)*10);
								glVertex3f(x*10+10,-c->cell2+0.1,(height-y)*10);
							}
						}
						if (cubes[y][x+1].tileUp > -1 && cubes[y][x+1].tileUp > -1)
						{
							cTile* ot = &tiles[cubes[y][x+1].tileUp];
							if (fabs(ot->u1 - t->u2) > 0.1 ||
								fabs(ot->u3 - t->u4) > 0.1 ||
								fabs(ot->v1 - t->v2) > 0.1 ||
								fabs(ot->v3 - t->v4) > 0.1 ||
								ot->texture != t->texture)
							{
								glVertex3f(x*10+10,-c->cell2+0.1,(height-y)*10);
								glVertex3f(x*10+10,-c->cell4+0.1,(height-y)*10-10);
							}
						}
					}
					else
					{
						glVertex3f(x*10,-c->cell1+0.1,(height-y)*10);
						glVertex3f(x*10+10,-c->cell2+0.1,(height-y)*10);
						
						glVertex3f(x*10+10,-c->cell2+0.1,(height-y)*10);
						glVertex3f(x*10+10,-c->cell4+0.1,(height-y)*10-10);
					}
				}
			}
			
			glEnd();
		}

		
		if(Graphics.textureTool == TOOL_BRUSH)
		{
			if(inverseSelection && lbuttondown)
			{
				int texture = textures[Graphics.texturestart + ((int)Graphics.selectionstart.y - 32) / 288]->texId();
				glEnable(GL_BLEND);
				glColor4f(1,1,1,0.8f);
		
				float offx = (mouse3dxstart - mouse3dx) / 40.0f;
				float offy = (mouse3dzstart - mouse3dz) / 40.0f;

				for(x = 0; x < width; x++)
				{
					for(y = 0; y < height; y++)
					{
						cCube* c = &cubes[y][x];
						if(!c->selected || !Graphics.frustum.CubeInFrustum(x*10+5,-c->cell1,(height-y)*10-5, 20))
							continue;
						if (c->tileUp > -1 && c->tileUp < (int)tiles.size())
						{
							cTile* t = &tiles[c->tileUp];

							glBindTexture(GL_TEXTURE_2D, texture);
							glNormal3f(c->normal.x, c->normal.y, c->normal.z);
							glBegin(GL_TRIANGLE_STRIP);
								glTexCoord2f(((x%4))/4.0f + offx,		((y%4))/4.0f+offy);			glVertex3f(x*10,-c->cell1,(height-y)*10);
								glTexCoord2f(((x%4))/4.0f + offx,		((y%4)+1)/4.0f+offy);		glVertex3f(x*10,-c->cell3,(height-y)*10-10);
								glTexCoord2f(((x%4)+1)/4.0f + offx,		((y%4))/4.0f+offy);			glVertex3f(x*10+10,-c->cell2,(height-y)*10);
								glTexCoord2f(((x%4)+1)/4.0f + offx,		((y%4)+1)/4.0f+offy);		glVertex3f(x*10+10,-c->cell4,(height-y)*10-10);
							glEnd();
						}
					}
				}
			}
			else if(!inverseSelection)
			{
				int texture = textures[Graphics.texturestart]->texId();
				glEnable(GL_BLEND);
				glEnable(GL_TEXTURE_2D);
				glColor4f(1,1,1,0.8f);
				glBindTexture(GL_TEXTURE_2D, texture);

				int x = (int)mouse3dx / 10;
				int y = (int)mouse3dz / 10;
				for(int yy = 0; yy < Graphics.textureGridSizeY*Graphics.textureBrushSize; yy++)
				{
					for(int xx = 0; xx < Graphics.textureGridSizeX*Graphics.textureBrushSize; xx++)
					{
						int xxx,yyy;
						int xi = 1;
						int yi = 1;

						if		(Graphics.textureRot == 0)	{ yyy = yy;																	xxx = xx;										}
						else if (Graphics.textureRot == 1)	{ yyy = xx;																	xxx = Graphics.textureBrush[0].size()*Graphics.textureBrushSize-1-yy;		}
						else if (Graphics.textureRot == 2)	{ yyy = Graphics.textureBrush.size()*Graphics.textureBrushSize-1-yy;		xxx = Graphics.textureBrush[0].size()*Graphics.textureBrushSize-1-xx;		}
						else if	(Graphics.textureRot == 3)	{ yyy = Graphics.textureBrush.size()*Graphics.textureBrushSize-1-xx;		xxx = yy;										}

						if( Graphics.textureBrush.size()*Graphics.textureBrushSize <= yyy ||
							Graphics.textureBrush[0].size()*Graphics.textureBrushSize <= xxx)
							continue;

						if(Graphics.textureBrush[yyy/Graphics.textureBrushSize][xxx/Graphics.textureBrushSize])
						{
							if(y-yy >= 0 && y-yy < height && x+xx >= 0 && x+xx < width)
							{
								cCube* c = &cubes[y-yy][x+xx];
								glNormal3f(c->normal.x, c->normal.y, c->normal.z);

								cVector2 v1,v2,v3,v4;

								if(Graphics.textureRot == 0)
								{
									v1 = cVector2((xxx+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+1+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v2 = cVector2((xxx+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v3 = cVector2((xxx+1+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+1+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v4 = cVector2((xxx+1+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
								}
								else if (Graphics.textureRot == 1)
								{
									v1 = cVector2((xxx+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v2 = cVector2((xxx+1+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v3 = cVector2((xxx+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+1+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v4 = cVector2((xxx+1+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+1+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
								}
								else if (Graphics.textureRot == 2)
								{
									v1 = cVector2((xxx+1+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v2 = cVector2((xxx+1+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+1+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v3 = cVector2((xxx+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v4 = cVector2((xxx+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+1+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
								}
								else if (Graphics.textureRot == 3)
								{
									v1 = cVector2((xxx+1+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+1+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v2 = cVector2((xxx+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+1+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v3 = cVector2((xxx+1+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
									v4 = cVector2((xxx+Graphics.textureBrushOffset.x) * 1/ Graphics.textureGridSizeX,		1-(yyy+Graphics.textureBrushOffset.y) / Graphics.textureGridSizeY);
								}
								v1.x = v1.x / (float)Graphics.textureBrushSize;
								v2.x = v2.x / (float)Graphics.textureBrushSize;
								v3.x = v3.x / (float)Graphics.textureBrushSize;
								v4.x = v4.x / (float)Graphics.textureBrushSize;

								v1.y = 1 - ((1-v1.y) / (float)Graphics.textureBrushSize);
								v2.y = 1 - ((1-v2.y) / (float)Graphics.textureBrushSize);
								v3.y = 1 - ((1-v3.y) / (float)Graphics.textureBrushSize);
								v4.y = 1 - ((1-v4.y) / (float)Graphics.textureBrushSize);

								if(Graphics.fliph)
								{
									v1.x = 1-v1.x;
									v2.x = 1-v2.x;
									v3.x = 1-v3.x;
									v4.x = 1-v4.x;
								}
								if(Graphics.flipv)
								{
									v1.y = 1-v1.y;
									v2.y = 1-v2.y;
									v3.y = 1-v3.y;
									v4.y = 1-v4.y;
								}

								glBegin(GL_TRIANGLE_STRIP);
									glTexCoord2f(v1.x, v1.y);		glVertex3f((x+xx)*10,-c->cell1,(height-(y-yy))*10);
									glTexCoord2f(v2.x, v2.y);		glVertex3f((x+xx)*10,-c->cell3,(height-(y-yy))*10-10);
									glTexCoord2f(v3.x, v3.y);		glVertex3f((x+xx)*10+10,-c->cell2,(height-(y-yy))*10);
									glTexCoord2f(v4.x, v4.y);		glVertex3f((x+xx)*10+10,-c->cell4,(height-(y-yy))*10-10);
								glEnd();
							}
						}
						if( (Graphics.textureRot == 0 && yy == 0 && xx == 0) ||
							(Graphics.textureRot == 1 && yy == Graphics.textureBrush[0].size()-1 && xx == 0) ||
							(Graphics.textureRot == 2 && yy == Graphics.textureBrush.size()-1 && xx == Graphics.textureBrush[0].size()-1) ||
							(Graphics.textureRot == 3 && yy == 0 && xx == Graphics.textureBrush.size()-1)
							)
						{
							cCube tempCube;
							cCube* c;
							if(y-yy >= 0 && y-yy < height && x+xx >= 0 && x+xx < width)
								c = &cubes[y-yy][x+xx];
							else
							{
								c = &tempCube;
								c->cell1 = -mouse3dy;
								c->cell2 = -mouse3dy;
								c->cell3 = -mouse3dy;
								c->cell4 = -mouse3dy;
							}
							
							glDisable(GL_TEXTURE_2D);
							glColor4f(1,0,0,0.5f);
							glBegin(GL_TRIANGLE_STRIP);
							glVertex3f((x+xx)*10+(xx == 0 ? 0 : 5),-c->cell1+0.01f,(height-(y-yy))*10-(yy == 0 ? 5 : 0));
							glVertex3f((x+xx)*10+(xx == 0 ? 0 : 5),-c->cell3+0.01f,(height-(y-yy))*10-(yy == 0 ? 10 :5));
							glVertex3f((x+xx)*10+(xx == 0 ? 5 : 10),-c->cell2+0.01f,(height-(y-yy))*10-(yy == 0 ? 5 : 0));
							glVertex3f((x+xx)*10+(xx == 0 ? 5 : 10),-c->cell4+0.01f,(height-(y-yy))*10-(yy == 0 ? 10 : 5));
							glEnd();
							glColor4f(1,1,1,0.8f);
							glEnable(GL_TEXTURE_2D);
						}
					}
				}	
			}
		}
	}



	if(Graphics.showWater || editmode == MODE_WATER)
	{
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set the correct blending mode
		glColor4f(1,1,1,0.5);
		static float waterindex = 0;

		glBindTexture(GL_TEXTURE_2D, Graphics.waterTextures[water.type][(int)ceil(waterindex)]->texId());

		if(Graphics.animateWater)
			waterindex+=max((float)0,(Graphics.frameticks) / 50.0f);
		if (waterindex > Graphics.waterTextures[water.type].size()-1)
			waterindex = 0;
		glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex3f(0,-water.height,0);
			glTexCoord2f(width/8,0); glVertex3f(10*width,-water.height,0);
			glTexCoord2f(width/8,height/8); glVertex3f(10*width,-water.height,10*height);
			glTexCoord2f(0,height/8); glVertex3f(0,-water.height,10*height);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
	}

	if(Graphics.showDot)
	{
		glColor4f(1,1,0,1);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_POLYGON);
			for(double ii = 0; ii < 2*PI; ii+= 2*PI/20.0)
				if(Graphics.topCamera)
					glVertex3f(-Graphics.camerapointer.y+cos(ii)*2 + Graphics.cameraheight/2.0f + 15 ,camheight+0.1,-Graphics.camerapointer.x-Graphics.cameraheight/2.0f+sin(ii)*2-15);
				else
					glVertex3f(-Graphics.camerapointer.x+cos(ii)*2,camheight+0.1,-Graphics.camerapointer.y+sin(ii)*2);
		glEnd();
	}

//	cVector3 colors[] = {cVector3(1,1,1), cVector3(1,0,0), cVector3(1,1,0), cVector3(1,0,1), cVector3(0,1,0), cVector3(0,1,1), cVector3(0,0,1) };


	glDisable(GL_TEXTURE_2D);
	glColor3f(1,1,0);
	root->draw(Graphics.quadtreeView);
	/*
	for(int i = 0; i < quadTreeFloats.size(); i+=4)
	{
		int color = (i / 12) % (sizeof(colors) / sizeof(cVector3));

		glColor4f(colors[color].x,colors[color].y,colors[color].z,1);
		cVector3 v = quadTreeFloats[i];
		cVector3 v2 = quadTreeFloats[i+1];

	
		v.x += width*5;
		v.y = -v.y;
		v.z = height*5 - v.z;

		v2.x += width*5;
		v2.y = -v2.y;
		v2.z = height*5 - v2.z;
/////////



		glBegin(GL_LINE_LOOP);
			glVertex3f(v.x, v.y, v.z);
			glVertex3f(v2.x, v.y, v.z);
			glVertex3f(v2.x, v.y, v2.z);
			glVertex3f(v.x, v.y, v2.z);
		glEnd();	

		glBegin(GL_LINE_LOOP);
			glVertex3f(v.x, v2.y, v.z);
			glVertex3f(v2.x, v2.y, v.z);
			glVertex3f(v2.x, v2.y, v2.z);
			glVertex3f(v.x, v2.y, v2.z);
		glEnd();

		glBegin(GL_LINES);
			glVertex3f(v.x, v.y, v.z);
			glVertex3f(v.x, v2.y, v.z);
			glVertex3f(v2.x, v.y, v.z);
			glVertex3f(v2.x, v2.y, v.z);
			glVertex3f(v2.x, v.y, v2.z);
			glVertex3f(v2.x, v2.y, v2.z);
			glVertex3f(v.x, v.y, v2.z);
			glVertex3f(v.x, v2.y, v2.z);
		glEnd();



		if(i / 12 == Graphics.quadtreeView)
			break;
	}
	glColor3f(1,1,1);
*/


	//glTranslatef(-Graphics.camerapointer.x, 0, -Graphics.camerapointer.y);




//	float	x = tileScale*Graphics.camerapointer.x,
//			y = heightMap(Graphics.camerapointer.x,Graphics.camerapointer.y),
//			z = tileScale*Graphics.camerapointer.y;


}




void cWorld::clean()
{
	int count = 0 ;

	unsigned int i,ii;
	int xx,yy;
	std::map<int, bool, std::less<int> > tilesused;

	cCube* c;
	for(yy = 0; yy < height; yy++)
	{
		for(xx = 0; xx < width; xx++)
		{
			c = &cubes[yy][xx];
			tilesused[c->tileUp] = true;
			tilesused[c->tileSide] = true;
			tilesused[c->tileOtherSide] = true;
		}
	}

	Log(3,0,GetMsg("world/TILESUSED"), tilesused.size());

	for(i = tiles.size()-1; i > 0; i--)
	{
		if(tilesused.find(i) == tilesused.end())
		{
			tiles[i].used = false;
			for(int yy = 0; yy < height; yy++)
			{
				for(int xx = 0; xx < width; xx++)
				{
					c = &cubes[yy][xx];
					if (c->tileUp > (int)i)
						c->tileUp--;
					if (c->tileSide > (int)i)
						c->tileSide--;
					if (c->tileOtherSide > (int)i)
						c->tileOtherSide--;

				}
			}

			count++;
			if(count % 100 == 0)
				Log(3,0,GetMsg("world/TILESLEFT"), tiles.size() - count);
		}
		else
			tiles[i].used = true;
	}

	for(i = 1; i < tiles.size(); i++)
	{
		if (!tiles[i].used)
		{
			tiles.erase(tiles.begin() + i);
			i--;
		}
	}


	tilesused.clear();

	std::map<int, bool, std::less<int> >	lightmapsused;
	


	for(yy = 0; yy < height; yy++)
	{
		for(xx = 0; xx < width; xx++)
		{
			if(cubes[yy][xx].tileUp != -1)
				lightmapsused[tiles[cubes[yy][xx].tileUp].lightmap] = true;
			if(cubes[yy][xx].tileSide != -1)	
				lightmapsused[tiles[cubes[yy][xx].tileSide].lightmap] = true;
			if(cubes[yy][xx].tileOtherSide != -1)
				lightmapsused[tiles[cubes[yy][xx].tileOtherSide].lightmap] = true;
		}
	}

	count = 0;
	for(i = lightmaps.size()-1; (signed int)i >= 0; i--)
	{
		if (lightmapsused.find(i) == lightmapsused.end())
		{
			lightmaps[i]->del();
			lightmaps[i]->del2();
			delete lightmaps[i];
			for(ii = i; ii < lightmaps.size()-1; ii++)
			{
				lightmaps[ii] = lightmaps[ii+1];
			}
			for(ii = 0; ii < tiles.size(); ii++)
			{
				if(tiles[ii].lightmap > (int)i)
					tiles[ii].lightmap--;
			}


			lightmaps.resize(lightmaps.size()-1);
			count++;
			if(count % 50 == 0)
				printf(".");
			if(count % 500 == 0)
				printf("%i ", i);
		}
	}


}



void cWorld::unload()
{
	delete light;
	delete light2;
	delete sound;
	delete effect;

	light = NULL;
	light2 = NULL;
	sound = NULL;
	effect = NULL;


	unsigned int x,y,i;
	for(i = 0; i < textures.size(); i++)
		TextureCache.unload(textures[i]->texture);
	textures.clear();

	for(y = 0; y < realLightmaps.size(); y+=21)
	{
		for(x = 0; x < realLightmaps[y].size(); x+=21)
		{
			if(realLightmaps[y][x] != NULL)
				delete realLightmaps[y][x];
		}
	}
	realLightmaps.clear();

	tiles.clear();
	for(i = 0; i < cubes.size(); i++)
		cubes[i].clear();
	cubes.clear();

	for(i = 0; i < lightmaps.size(); i++)
	{
		lightmaps[i]->del();
		lightmaps[i]->del2();
		delete lightmaps[i];
	}
	lightmaps.clear();

	for(i = 0; i < models.size(); i++)
		delete models[i];
	models.clear();
	
	delete root;
	root = NULL;
	
	quadTreeFloats.clear();
	sounds.clear();
	effects.clear();
	lights.clear();
	
	for(i = 0; i < gattiles.size(); i++)
		gattiles[i].clear();
		
	gattiles.clear();

}




void cQuadTreeNode::recalculate()
{
	if(child1 != NULL)
		child1->recalculate();
	if(child2 != NULL)
		child2->recalculate();
	if(child3 != NULL)
		child3->recalculate();
	if(child4 != NULL)
		child4->recalculate();


	if(child1 != NULL)
	{
		box1.y = child1->box1.y;
		box1.y = max(box1.y, child2->box1.y);
		box1.y = max(box1.y, child3->box1.y);
		box1.y = max(box1.y, child4->box1.y);

		box2.y = child1->box2.y;
		box2.y = min(box2.y, child2->box2.y);
		box2.y = min(box2.y, child3->box2.y);
		box2.y = min(box2.y, child4->box2.y);
	}
	else
	{
		box1.y = -9999;
		box2.y = 9999;
		for(float x = box2.x; x < box1.x; x+= (box1.x - box2.x)/10.0)
		{
			for(float y = box2.z; y < box1.z; y+= (box1.z - box2.z)/10.0)
			{
				int xx = (int)x + Graphics.world.width*5;
				int yy = Graphics.world.height*5 - (int)y;

				int tilex = xx / 10;
				int tiley = yy / 10;

				if(tiley > -1 && tiley < Graphics.world.height && tilex > -1 && tilex < Graphics.world.width)
				{
					cCube* c = &Graphics.world.cubes[Graphics.world.height - tiley-1][tilex];
					box1.y = max(box1.y, c->cell1);
					box2.y = min(box2.y, c->cell1);

					box1.y = max(box1.y, c->cell2);
					box2.y = min(box2.y, c->cell2);

					box1.y = max(box1.y, c->cell3);
					box2.y = min(box2.y, c->cell3);

					box1.y = max(box1.y, c->cell4);
					box2.y = min(box2.y, c->cell4);

					
					box1.y = max(box1.y, c->maxHeight);
					box2.y = min(box2.y, c->minHeight);

				}
				else
				{
				}
			}
		}



	}

	range2.x = (box1.x+box2.x)/2.0f;
	range1.x = box1.x-range2.x;

	range2.y = (box1.y+box2.y)/2.0f;
	range1.y = -range2.y;

	range2.z = (box1.z+box2.z)/2.0f;
	range1.z = box1.z-range2.z;



}

void cQuadTreeNode::save(std::vector<cVector3>& v)
{
	v.push_back(box1);
	v.push_back(box2);
	v.push_back(range1);
	v.push_back(range2);
	if(child1 != NULL)
		child1->save(v);
	if(child2 != NULL)
		child2->save(v);
	if(child3 != NULL)
		child3->save(v);
	if(child4 != NULL)
		child4->save(v);
}



int cLightmap::texId()
{
	if(generated)
		return tid;
	// Generate a texture with the associative texture ID stored in the array
	glGenTextures(1, &tid);

	// This sets the alignment requirements for the start of each pixel row in memory.
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

	// Bind the texture to the texture arrays index and init the texture
	glBindTexture(GL_TEXTURE_2D, tid);

	// Build Mipmaps (builds different versions of the picture for distances - looks better)
	glTexImage2D(GL_TEXTURE_2D, 0, 3, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, buf+64);
//	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, size, size, GL_RGB, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);


	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	generated = true;
	return tid;
}

int cLightmap::texId2()
{
	if(generated2)
		return tid2;

	// Generate a texture with the associative texture ID stored in the array
	glGenTextures(1, &tid2);

	// This sets the alignment requirements for the start of each pixel row in memory.
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

	// Bind the texture to the texture arrays index and init the texture
	glBindTexture(GL_TEXTURE_2D, tid2);

	// Build Mipmaps (builds different versions of the picture for distances - looks better)
	glTexImage2D(GL_TEXTURE_2D, 0, 1, 8, 8, 0, GL_RED, GL_UNSIGNED_BYTE, buf);
//	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, size, size, GL_RGB, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);


	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	generated2 = true;
	return tid2;
}

void cLightmap::del()
{
	if (generated)
		glDeleteTextures(1, &tid);
	generated = false;
}


void cLightmap::del2()
{
	if (generated2)
		glDeleteTextures(1, &tid2);
	generated2 = false;
}


// saves an array of pixels as a TGA image
int tgaSave(    char            *filename,
			short int       width,
			short int       height,
			unsigned char   pixelDepth,
			unsigned char   *imageData) {
	
	unsigned char cGarbage = 0, type,mode,aux;
	short int iGarbage = 0;
	int i;
	FILE *file;
	
	// open file and check for errors
	file = fopen(filename, "wb");
	if (file == NULL) {
		return(-1);
	}
	
	// compute image type: 2 for RGB(A), 3 for greyscale
	mode = (unsigned char)(pixelDepth / 8);
	if ((pixelDepth == 24) || (pixelDepth == 32))
		type = 2;
	else
		type = 3;
	
	// write the header
	fwrite(&cGarbage, sizeof(unsigned char), 1, file);
	fwrite(&cGarbage, sizeof(unsigned char), 1, file);
	
	fwrite(&type, sizeof(unsigned char), 1, file);
	
	fwrite(&iGarbage, sizeof(short int), 1, file);
	fwrite(&iGarbage, sizeof(short int), 1, file);
	fwrite(&cGarbage, sizeof(unsigned char), 1, file);
	fwrite(&iGarbage, sizeof(short int), 1, file);
	fwrite(&iGarbage, sizeof(short int), 1, file);
	
	fwrite(&width, sizeof(short int), 1, file);
	fwrite(&height, sizeof(short int), 1, file);
	fwrite(&pixelDepth, sizeof(unsigned char), 1, file);
	
	fwrite(&cGarbage, sizeof(unsigned char), 1, file);
	
	// convert the image data from RGB(a) to BGR(A)
	if (mode >= 3)
		for (i=0; i < width * height * mode ; i+= mode) {
			aux = imageData[i];
			imageData[i] = imageData[i+2];
			imageData[i+2] = aux;
		}
		
		// save the image data
		fwrite(imageData, sizeof(unsigned char),
			width * height * mode, file);
		fclose(file);
		// release the memory
		// free(imageData);
		
		return(0);
}

void cWorld::savelightmap()
{
	int x,y,xx,yy;
	char* imgdata = new char[width*height*6*6*3];
	ZeroMemory(imgdata, width*height*6*6*3);
	for(x = 0; x < width; x++)
	{
		for(y = 0; y < height; y++)
		{
			for(xx = 0; xx < 6; xx++)
			{
				for(yy = 0; yy < 6; yy++)
				{
					if (cubes[y][x].tileUp != -1)
					{
						imgdata[3*6*x + 6*6*3*width * y + 3*xx + 6*3*width*yy] = lightmaps[tiles[cubes[y][x].tileUp].lightmap]->buf[xx+yy*8+8+1];
						imgdata[3*6*x + 6*6*3*width * y + 3*xx + 6*3*width*yy+1] = lightmaps[tiles[cubes[y][x].tileUp].lightmap]->buf[xx+yy*8+8+1];
						imgdata[3*6*x + 6*6*3*width * y + 3*xx + 6*3*width*yy+2] = lightmaps[tiles[cubes[y][x].tileUp].lightmap]->buf[xx+yy*8+8+1];
					}
				}
			}
		}
	}
	tgaSave((char*)(std::string(fileName) + ".lightmap.tga").c_str(), width*6, height*6, 24, (BYTE*)imgdata);
	delete[] imgdata;
	imgdata = new char[width*height*6*6*3];
	ZeroMemory(imgdata, width*height*6*6*3);
	for(x = 0; x < width; x++)
	{
		for(y = 0; y < height; y++)
		{
			for(xx = 0; xx < 6; xx++)
			{
				for(yy = 0; yy < 6; yy++)
				{
					if (cubes[y][x].tileUp != -1)
					{
						imgdata[3*6*x + 6*6*3*width * y + 3*xx + 6*3*width*yy] = lightmaps[tiles[cubes[y][x].tileUp].lightmap]->buf[64+3*(xx+yy*8+8+1)];
						imgdata[3*6*x + 6*6*3*width * y + 3*xx + 6*3*width*yy+1] = lightmaps[tiles[cubes[y][x].tileUp].lightmap]->buf[64+3*(xx+yy*8+8+1)+1];
						imgdata[3*6*x + 6*6*3*width * y + 3*xx + 6*3*width*yy+2] = lightmaps[tiles[cubes[y][x].tileUp].lightmap]->buf[64+3*(xx+yy*8+8+1)+2];
					}
				}
			}
		}
	}
	tgaSave((char*)(std::string(fileName) + ".lightmap2.tga").c_str(), width*6, height*6, 24, (BYTE*)imgdata);
	delete[] imgdata;

/*	imgdata = new char[width*height*12*12*3];

	for(x = 0; x < width; x++)
	{
		for(int y = 0; y < height; y++)
		{
			for(int xx = 0; xx < 6; xx++)
			{
				for(int yy = 0; yy < 6; yy++)
				{
					if (cubes[y][x].tileUp != -1)
					{
						imgdata[3*12*x + 12*12*3*width * y + 3*xx + 12*3*width*yy] = lightmaps[tiles[cubes[y][x].tileUp].lightmap]->buf[xx+yy*8+8+1];
						imgdata[3*12*x + 12*12*3*width * y + 3*xx + 12*3*width*yy+1] = lightmaps[tiles[cubes[y][x].tileUp].lightmap]->buf[xx+yy*8+8+1];
						imgdata[3*12*x + 12*12*3*width * y + 3*xx + 12*3*width*yy+2] = lightmaps[tiles[cubes[y][x].tileUp].lightmap]->buf[xx+yy*8+8+1];
					}
					if (cubes[y][x].tileSide != -1)
					{
						imgdata[3*12*x+6*3 + 12*12*3*width * y + 3*xx + 12*3*width*yy] = lightmaps[tiles[cubes[y][x].tileSide].lightmap]->buf[xx+yy*8+8+1];
						imgdata[3*12*x+6*3 + 12*12*3*width * y + 3*xx + 12*3*width*yy+1] = lightmaps[tiles[cubes[y][x].tileSide].lightmap]->buf[xx+yy*8+8+1];
						imgdata[3*12*x+6*3 + 12*12*3*width * y + 3*xx + 12*3*width*yy+2] = lightmaps[tiles[cubes[y][x].tileSide].lightmap]->buf[xx+yy*8+8+1];
					}
					if (cubes[y][x].tileOtherSide != -1)
					{
						imgdata[3*12*x + (int)(12*12*3*width * (y-0.5)) + 3*xx + 12*3*width*yy] = lightmaps[tiles[cubes[y][x].tileSide].lightmap]->buf[xx+yy*8+8+1];
						imgdata[3*12*x + (int)(12*12*3*width * (y-0.5)) + 3*xx + 12*3*width*yy+1] = lightmaps[tiles[cubes[y][x].tileSide].lightmap]->buf[xx+yy*8+8+1];
						imgdata[3*12*x + (int)(12*12*3*width * (y-0.5)) + 3*xx + 12*3*width*yy+2] = lightmaps[tiles[cubes[y][x].tileSide].lightmap]->buf[xx+yy*8+8+1];
					}
				}
			}
		}
	}
	tgaSave((char*)(string(filename) + ".lightmap.walls.tga").c_str(), width*12, height*12, 24, (BYTE*)imgdata);
	delete[] imgdata;*/
}

void cWorld::loadlightmap()
{
	{
		cFile* pFile = fs.open(std::string(fileName) + ".lightmap.tga");
		int color;
		for(int x = 0; x < width; x++)
		{
			for(int y = 0; y < height; y++)
			{
				for(int xx = 0; xx < 6; xx++)
				{
					for(int yy = 0; yy < 6; yy++)
					{
						if (cubes[y][x].tileUp != -1)
						{
							color = pFile->data[20 + 3*6*x + 6*6*3*width*y + 3*xx + 6*3*width*yy];
							lightmaps[tiles[cubes[y][x].tileUp].lightmap]->buf[xx+yy*8+8+1] = color;
							lightmaps[tiles[cubes[y][x].tileUp].lightmap]->del2();
						}
					}
				}
			}
		}
		pFile->close();
	}
	{
		cFile* pFile = fs.open(std::string(fileName) + ".lightmap2.tga");
		for(int x = 0; x < width; x++)
		{
			for(int y = 0; y < height; y++)
			{
				if(cubes[y][x].tileUp == -1)
					continue;
				cLightmap* l = lightmaps[tiles[cubes[y][x].tileUp].lightmap];
				for(int xx = 0; xx < 6; xx++)
				{
					for(int yy = 0; yy < 6; yy++)
					{
						if (cubes[y][x].tileUp != -1)
						{
							l->buf[64+3*(xx+yy*8+8+1)] = pFile->data[20 + 3*6*x + 6*6*3*width * y + 3*xx + 6*3*width*yy];
							l->buf[64+3*(xx+yy*8+8+1)+2] = pFile->data[20 + 3*6*x + 6*6*3*width * y + 3*xx + 6*3*width*yy+1];
							l->buf[64+3*(xx+yy*8+8+1)+1] = pFile->data[20 + 3*6*x + 6*6*3*width * y + 3*xx + 6*3*width*yy+2];
							l->del();
						}
					}
				}
			}
		}
		pFile->close();
	}
	
}






void cQuadTreeNode::generate(float w, float h, float centerx, float centery, int level)
{
	box1.x = centerx+w/2;
	box1.z = centery+h/2;
	box1.y = -100;

	box2.x = centerx-w/2;
	box2.z = centery-h/2;
	box2.y = 100;

	if (level == 0)
	{
		child1 = NULL;
		child2 = NULL;
		child3 = NULL;
		child4 = NULL;
	}
	else
	{
		child1 = new cQuadTreeNode();
		child2 = new cQuadTreeNode();
		child3 = new cQuadTreeNode();
		child4 = new cQuadTreeNode();


		child1->generate(w/2.0f,h/2.0f,centerx-w/4.0f,centery-w/4.0f, level-1);
		child2->generate(w/2.0f,h/2.0f,centerx+w/4.0f,centery-w/4.0f, level-1);
		child3->generate(w/2.0f,h/2.0f,centerx-w/4.0f,centery+w/4.0f, level-1);
		child4->generate(w/2.0f,h/2.0f,centerx+w/4.0f,centery+w/4.0f, level-1);
	}

}


int cRealLightMap::texId()
{
	if(generated)
		return tid;

	int xx, yy;

	char* buf = new char[128*128*3];
	for(xx = 0; xx < min(21, Graphics.world.width-x); xx++)
	{
		for(yy = 0; yy < min(21, Graphics.world.height-y); yy++)
		{
			if(Graphics.world.cubes[y+yy][x+xx].tileUp == -1)
				continue;
			char* b = Graphics.world.lightmaps[Graphics.world.tiles[Graphics.world.cubes[y+yy][x+xx].tileUp].lightmap]->buf;
			for(int xxx = 0; xxx < 6; xxx++)
			{
				for(int yyy = 0; yyy < 6; yyy++)
				{
					buf[3*(128 + 128*yy*6 + 1+xx*6 + 128*yyy + xxx)] = b[64 + 3*(xxx+yyy*8+1+8)];
					buf[3*(128 + 128*yy*6 + 1+xx*6 + 128*yyy + xxx)+1] = b[64 + 3*(xxx+yyy*8+1+8)+1];
					buf[3*(128 + 128*yy*6 + 1+xx*6 + 128*yyy + xxx)+2] = b[64 + 3*(xxx+yyy*8+1+8)+2];
				}
			}
		}
	}


	for(xx = 0; xx < min(21, Graphics.world.width-x); xx++)
	{
		for(int xxx = 0; xxx < 6; xxx++)
		{
			for(int i = 0; i < 3; i++)
			{
				buf[3*(1+xx*6+xxx)+i] = buf[3*(1+xx*6+xxx+128)+i];
				buf[3*(1+xx*6+xxx + 127*128)+i] = buf[3*(1+xx*6+xxx + 126*128)+i];
			}
		}
	}

	for(yy = 0; yy < min(21, Graphics.world.height-y); yy++)
	{
		for(int yyy = 0; yyy < 6; yyy++)
		{
			for(int i = 0; i < 3; i++)
			{
				buf[3*(yy*128*6 + yyy*128)+i]		= buf[3*(yy*128*6 + yyy*128+1)+i];
				buf[3*(yy*128*6 + yyy*128+127)+i] = buf[3*(yy*128*6 + yyy*128+127-1)+i];
			}
		}
	}
	// Generate a texture with the associative texture ID stored in the array
	glGenTextures(1, &tid);

	// This sets the alignment requirements for the start of each pixel row in memory.
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

	// Bind the texture to the texture arrays index and init the texture
	glBindTexture(GL_TEXTURE_2D, tid);

	glTexImage2D(GL_TEXTURE_2D, 0, 3, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	generated = true;
	delete[] buf;
	return tid;
}

int cRealLightMap::texId2()
{
	if(generated2)
		return tid2;

	int xx, yy;

	char* buf = new char[128*128];
	ZeroMemory(buf, 128*128);
	for(xx = 0; xx < min(21, Graphics.world.width-x); xx++)
	{
		for(yy = 0; yy < min(21, Graphics.world.height-y); yy++)
		{
			if(Graphics.world.cubes[y+yy][x+xx].tileUp == -1)
				continue;
			char* b = Graphics.world.lightmaps[Graphics.world.tiles[Graphics.world.cubes[y+yy][x+xx].tileUp].lightmap]->buf;
			for(int xxx = 0; xxx < 6; xxx++)
			{
				for(int yyy = 0; yyy < 6; yyy++)
				{
					buf[(128 + 128*yy*6 + 1+xx*6 + 128*yyy + xxx)] = b[(xxx+yyy*8+1+8)];
				}
			}
		}
	}

	for(xx = 0; xx < min(21, Graphics.world.width-x); xx++)
	{
		for(int xxx = 0; xxx < 6; xxx++)
		{
			buf[1+xx*6+xxx] = buf[1+xx*6+xxx+128];
			buf[1+xx*6+xxx + 127*128] = buf[1+xx*6+xxx + 126*128];
		}
	}

	for(yy = 0; yy < min(21, Graphics.world.height-y); yy++)
	{
		for(int yyy = 0; yyy < 6; yyy++)
		{
			buf[yy*128*6 + yyy*128]		= buf[yy*128*6 + yyy*128+1];
			buf[yy*128*6 + yyy*128+127] = buf[yy*128*6 + yyy*128+127-1];
		}
	}

	// Generate a texture with the associative texture ID stored in the array
	glGenTextures(1, &tid2);

	// This sets the alignment requirements for the start of each pixel row in memory.
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

	// Bind the texture to the texture arrays index and init the texture
	glBindTexture(GL_TEXTURE_2D, tid2);

	glTexImage2D(GL_TEXTURE_2D, 0, 1, 128, 128, 0, GL_RED, GL_UNSIGNED_BYTE, buf);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	generated2 = true;
	delete[] buf;
	return tid2;
}

void cRealLightMap::reset()
{
	if(generated)
	{
		glDeleteTextures(1,&tid);
		generated = false;
	}
	if(generated2)
	{
		glDeleteTextures(1,&tid2);
		generated2 = false;
	}
}

cRealLightMap::cRealLightMap()
{
	generated = false;
	generated2 = false;
}





cQuadTreeNode::~cQuadTreeNode()
{
	if(child1)
		delete child1;
	if(child2)
		delete child2;
	if(child3)
		delete child3;
	if(child4)
		delete child4;
		
	child1 = NULL;
	child2 = NULL;
	child3 = NULL;
	child4 = NULL;
	

}




void cWorld::fixGridding()
{
	int lightmap,lightmapleft,lightmaptop,lightmapright,lightmapbottom,i;
	cLightmap* map;
	cLightmap* mapleft;
	cLightmap* maptop;
	cLightmap* mapright;
	cLightmap* mapbottom;
	for(int x = 1; x < Graphics.world.width-1; x++)
	{
		for(int y = 1; y < Graphics.world.height-1; y++)
		{
			int tile = Graphics.world.cubes[y][x].tileUp;
			int tileleft = Graphics.world.cubes[y][x-1].tileUp;
			int tiletop = Graphics.world.cubes[y-1][x].tileUp;
			int tileright = Graphics.world.cubes[y][x+1].tileUp;
			int tilebottom = Graphics.world.cubes[y+1][x].tileUp;
			if (tile != -1)
			{
				if(tile != -1)
					lightmap = Graphics.world.tiles[tile].lightmap;
				if(tileleft != -1)
					lightmapleft = Graphics.world.tiles[tileleft].lightmap;
				if(tiletop != -1)
					lightmaptop = Graphics.world.tiles[tiletop].lightmap;
				if(tileright != -1)
					lightmapright = Graphics.world.tiles[tileright].lightmap;
				if(tilebottom != -1)
					lightmapbottom = Graphics.world.tiles[tilebottom].lightmap;

				if(tile != -1)
					map = Graphics.world.lightmaps[lightmap];
				if(tileleft != -1)
					mapleft = Graphics.world.lightmaps[lightmapleft];
				if(tiletop != -1)
					maptop = Graphics.world.lightmaps[lightmaptop];
				if(tileright != -1)
					mapright = Graphics.world.lightmaps[lightmapright];
				if(tilebottom != -1)
					mapbottom = Graphics.world.lightmaps[lightmapbottom];

				for(i = 0; i < 8; i++)
				{
					if(tileleft != -1)
						mapleft->buf[8*i+7] = map->buf[8*i+1];
					if(tiletop != -1)
						maptop->buf[7*8+i] = map->buf[i+8];
					if(tileright != -1)
						mapright->buf[8*i] = map->buf[8*i+6];
					if(tilebottom != -1)
						mapbottom->buf[i] = map->buf[6*8+i];
				}

				for(i = 0; i < 8; i++)
				{
					if(tileleft != -1)
					{
						mapleft->buf[64+3*(8*i+7)] = map->buf[64+3*(8*i+1)];
						mapleft->buf[64+3*(8*i+7)+1] = map->buf[64+3*(8*i+1)+1];
						mapleft->buf[64+3*(8*i+7)+2] = map->buf[64+3*(8*i+1)+2];
					}
					if(tiletop != -1)
					{
						maptop->buf[64+3*(7*8+i)] = map->buf[64+3*(i+8)];
						maptop->buf[64+3*(7*8+i)+1] = map->buf[64+3*(i+8)+1];
						maptop->buf[64+3*(7*8+i)+2] = map->buf[64+3*(i+8)+2];
					}
					if(tileright != -1)
					{
						mapright->buf[64+3*(8*i)] = map->buf[64+3*(8*i+6)];
						mapright->buf[64+3*(8*i)+1] = map->buf[64+3*(8*i+6)+1];
						mapright->buf[64+3*(8*i)+2] = map->buf[64+3*(8*i+6)+2];
					}
					if(tilebottom != -1)
					{
						mapbottom->buf[64+3*(i)] = map->buf[64+3*(6*8+i)];
						mapbottom->buf[64+3*(i)+1] = map->buf[64+3*(6*8+i)+1];
						mapbottom->buf[64+3*(i)+2] = map->buf[64+3*(6*8+i)+2];
					}
				}
			
			}
				
		}
	}

}


void cWorld::makeLightmapsUnique()
{
	int x,y;

	std::map<int, bool, std::less<int> > used;
	for(x = 0; x < Graphics.world.width; x++)
	{
		for(y = 0; y < Graphics.world.height; y++)
		{
			Graphics.world.realLightmaps[y][x]->reset();
			Graphics.world.realLightmaps[y][x]->reset();
			int tile = Graphics.world.cubes[y][x].tileUp;
			if(used.find(tile) != used.end() && tile != -1)
			{
				cTile t = Graphics.world.tiles[tile];
				tile = Graphics.world.tiles.size();
				Graphics.world.tiles.push_back(t);
				Graphics.world.cubes[y][x].tileUp = tile;
			}
			used[tile] = 1;
//////////////////////////////////
			tile = Graphics.world.cubes[y][x].tileSide;
			if(used.find(tile) != used.end() && tile != -1)
			{
				cTile t = Graphics.world.tiles[tile];
				tile = Graphics.world.tiles.size();
				Graphics.world.tiles.push_back(t);
				Graphics.world.cubes[y][x].tileSide = tile;
			}
			used[tile] = 1;
////////////////////////////////
			tile = Graphics.world.cubes[y][x].tileOtherSide;
			if(used.find(tile) != used.end() && tile != -1)
			{
				cTile t = Graphics.world.tiles[tile];
				tile = Graphics.world.tiles.size();
				Graphics.world.tiles.push_back(t);
				Graphics.world.cubes[y][x].tileOtherSide = tile;
			}
			used[tile] = 1;
		}
	}
////////// all tiles are now unique
	used.clear();

	for(x = 0; x < Graphics.world.width; x++)
	{
		for(y = 0; y < Graphics.world.height; y++)
		{
			int tile = Graphics.world.cubes[y][x].tileUp;
			if(tile != -1)
			{
				Graphics.world.realLightmaps[y][x]->reset();
				int lightmap = Graphics.world.tiles[tile].lightmap;
				if(lightmap != -1)
				{
					if(used.find(lightmap) == used.end())
					{
						cLightmap* map = new cLightmap();
						memcpy(map->buf, Graphics.world.lightmaps[lightmap]->buf, 256);
						lightmap = Graphics.world.lightmaps.size();
						Graphics.world.tiles[tile].lightmap = lightmap;
						Graphics.world.lightmaps.push_back(map);
					}
					used[lightmap] = 1;
				}
			}

			tile = Graphics.world.cubes[y][x].tileSide;
			if(tile != -1)
			{
				int lightmap = Graphics.world.tiles[tile].lightmap;
				if(lightmap != -1)
				{
					if(used.find(lightmap) == used.end())
					{
						cLightmap* map = new cLightmap();
						memcpy(map->buf, Graphics.world.lightmaps[lightmap]->buf, 256);
						lightmap = Graphics.world.lightmaps.size();
						Graphics.world.tiles[tile].lightmap = lightmap;
						Graphics.world.lightmaps.push_back(map);
					}
					used[lightmap] = 1;
				}
			}

			tile = Graphics.world.cubes[y][x].tileOtherSide;
			if(tile != -1)
			{
				int lightmap = Graphics.world.tiles[tile].lightmap;
				if(lightmap != -1)
				{
					if(used.find(lightmap) == used.end())
					{
						cLightmap* map = new cLightmap();
						memcpy(map->buf, Graphics.world.lightmaps[lightmap]->buf, 256);
						lightmap = Graphics.world.lightmaps.size();
						Graphics.world.tiles[tile].lightmap = lightmap;
						Graphics.world.lightmaps.push_back(map);
					}
					used[lightmap] = 1;
				}
			}

		}
	}

}


void cWorld::blackLightmaps()
{
	int x,y;
	for(x = 0; x < Graphics.world.width; x++)
	{
		for(y = 0; y < Graphics.world.height; y++)
		{
			int tile = Graphics.world.cubes[y][x].tileUp;
			if(tile != -1)
			{
				Graphics.world.realLightmaps[y][x]->reset();
				int lightmap = Graphics.world.tiles[tile].lightmap;
				if(lightmap != -1)
				{
					memset(Graphics.world.lightmaps[lightmap]->buf,		'\0', 256);
				}
			}
		}
	}



}

cWorld::cWorld()
{
	showTextures = false;
	loaded = false;
	light = NULL;
	light2 = NULL;
	sound = NULL;
	effect = NULL;
}

cWorld::~cWorld()
{
	unload();
}