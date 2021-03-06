#include <common.h>
#include "sprite.h"
#include "graphics.h"
#include "settings.h"
#include <world.h>
#include <bengine/util/filesystem.h>
#include <bengine/math/math.h>

#define SPRITESIZE 128








cSprite::cSprite()
{
	body = NULL;
	head = NULL;
	scale = 0.15f;

	action = 0;
	direction = 0;
}

GLuint cSprite::cActSpr::cFrame::texId()
{
	if(loaded)
		return tex;

	return tex;


}


cSprite::~cSprite()
{
	if(head != NULL)
		delete head;
	if(body != NULL)
		delete body;
	for(unsigned int i = 0; i < extras.size(); i++)
	{
		if(extras[i] != NULL)
			delete extras[i];
	}
	extras.clear();


}

void cSprite::loadBody(std::string filename)
{
	if(body != NULL)
		delete body;
	body = new cActSpr();
	body->load(filename);
}

void cSprite::loadHead(std::string filename)
{
	if(head != NULL)
		delete head;
	head = new cActSpr();
	head->load(filename);
}

void cSprite::addExtra(std::string filename)
{
	cActSpr* spr = new cActSpr();
	spr->load(filename);
	extras.push_back(spr);

}

void cSprite::setExtra(unsigned int id, std::string filename)
{
	while(extras.size() <= id)
		extras.push_back(NULL);

	cActSpr* spr = new cActSpr();
	spr->load(filename);
	if(extras[id] != NULL)
		delete extras[id];
	extras[id] = spr;

}


cSprite::cActSpr::cActSpr()
{
	loaded = false;
}
cSprite::cActSpr::~cActSpr()
{
	unsigned int i;
	for(i = 0; i < frames.size(); i++)
		delete frames[i];
	for(i = 0; i < actions.size(); i++)
		delete actions[i];
}

void cSprite::cActSpr::load(std::string fname)
{
	unsigned long ticks = tickcount();
	fileName = fname;
	int i,ii,x,y;
	bEngine::util::cFileSystem::cReadFile* pFile = bEngine::util::cFileSystem::open(fileName + ".spr");
	if(pFile == NULL)
	{
		Log(2,0,"Error opening %s.spr", fileName.c_str());
		return;
	}
	pFile->get(); pFile->get();
	BYTE version1 = pFile->get();
	pFile->get();//	BYTE version2 = pFile->get();
	BYTE frameCount = pFile->get();
	pFile->get();
	pFile->get(); //BYTE frameCount32 = pFile->get();
	pFile->get();

	for(i = 0; i < frameCount; i++)
	{
		int width = pFile->readWord();
		int height = pFile->readWord();
		int framelen = width*height;
		if(version1 != 0)
			framelen = pFile->readWord();
		
		BYTE* data = new BYTE[width*height];
		int iii = 0;
		if (width > 1000 || height > 1000)
		{
			Log(2,0,"Invalid sprite height: %i x %i", width, height);
			return;
		}
		for(ii = 0; ii < framelen && iii < width*height;)
		{
			BYTE c = pFile->get();
			ii++;
			data[iii] = c;			iii++;
			if(iii >= width*height)
				break;
			if(c == 0 && version1 != 0)
			{
				BYTE d = pFile->get();
				ii++;
				if(d == 0)
				{
					data[iii] = d; iii++;
					if(iii >= width*height)
						break;
				}
				else
				{
					d--;
					if(iii+d > width*height)
						d = width*height-iii-1;
					memset(data+iii, c, d);
					iii+=d;
				}
			}
		}

		int oldIndex = pFile->tell();

	
//TODO: get pallette
		char* image = new char[SPRITESIZE*SPRITESIZE*4];

		for(y = 0; y < SPRITESIZE; y++)
		{
			for(x = 0; x < SPRITESIZE; x++)
			{
				int xx = (int)(x * (width / (float)SPRITESIZE));
				int yy = (int)(y * (height / (float)SPRITESIZE));

				BYTE index = data[xx+width*yy];

				pFile->seek(pFile->size()-1024+index*4, bEngine::util::cFileSystem::cReadFile::BEGIN);

				image[4*(x+SPRITESIZE*y)] = pFile->get();
				image[4*(x+SPRITESIZE*y)+1] = pFile->get();
				image[4*(x+SPRITESIZE*y)+2] = pFile->get();
				image[4*(x+SPRITESIZE*y)+3] = index==0 ? 0 : 255;
			}
		}
		delete[] data;
		pFile->seek(oldIndex, bEngine::util::cFileSystem::cReadFile::BEGIN);

		cFrame* f = new cFrame();
		f->tex = -1;
		f->loaded = false;
		f->data = image;
		f->h = height;
		f->w = width;
		glGenTextures(1, &f->tex);
		glBindTexture(GL_TEXTURE_2D, f->tex);
		glTexImage2D(GL_TEXTURE_2D,0,4,SPRITESIZE,SPRITESIZE,0,GL_RGBA,GL_UNSIGNED_BYTE,image);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		f->loaded = true;
		delete[] f->data;
		f->data = NULL;

		frames.push_back(f);

	}
	Log(3,0,"%ims for spr", tickcount()-ticks);
	ticks = tickcount();

	delete pFile;
// done reading sprite
	pFile = bEngine::util::cFileSystem::open(fileName + ".act");
	if(pFile == NULL)
	{
		Log(1,0,"Error opening %s.act", fileName.c_str());
		return;
	}
	pFile->get(); pFile->get();

	version1 = pFile->get();
	pFile->get();
	int actioncount = pFile->readWord();
	char buf[100];
	pFile->read(buf, 10);

	for(i = 0; i < actioncount && !pFile->eof(); i++)
	{
		cAction* action = new cAction();
		
		pFile->read((char*)&action->frameCount,4);
		for(ii = 0; ii < action->frameCount && !pFile->eof(); ii++)
		{
			pFile->read(buf, 32);
			cAction::cFrame* frame = new cAction::cFrame();
			
			pFile->read((char*)&frame->subFrameCount, 4);
			for(int iii = 0; iii < frame->subFrameCount && !pFile->eof(); iii++)
			{
				cAction::cFrame::cSubFrame* subframe = new cAction::cFrame::cSubFrame();
				pFile->read((char*)&subframe->offsetX,4);
				pFile->read((char*)&subframe->offsetY,4);
				pFile->read((char*)&subframe->image,4);
				pFile->read((char*)&subframe->direction,4);
				pFile->read((char*)&subframe->color,4);
				if(version1 >= 2)
					pFile->read((char*)&subframe->scaleX,4);
				if(version1 >= 4)
					pFile->read((char*)&subframe->scaleY,4);
				else
					subframe->scaleY = subframe->scaleX;
				if(version1 >= 2)
				{
					pFile->read(buf, 4);
					pFile->read((char*)&subframe->dontJump,4);
					//if(subframe->dontJump != 0)
					//	pFile->read(buf, 12);
					if(version1 >= 5)
					{
						pFile->read((char*)&subframe->sizeX,4);
						pFile->read((char*)&subframe->sizeY,4);
					}
				}

				frame->subframes.push_back(subframe);
			}

			pFile->read(buf,4);
			pFile->read((char*)&frame->extraInfo,4);
			if(frame->extraInfo != 0)
			{
				pFile->read((char*)&frame->extraRotation,4);
				pFile->read((char*)&frame->extraX,4);
				pFile->read((char*)&frame->extraY,4);
				pFile->read(buf,4);

			}
			else
			{
				frame->extraX = 0;
				frame->extraY = 0;
			}
			action->frames.push_back(frame);

		}
		actions.push_back(action);
	}
	Log(3,0,"%ims for act", tickcount()-ticks);



	delete pFile;

	Log(3,0,"Done loading sprite %s", fileName.c_str());
	loaded = true;

}


void cSprite::draw()
{
	if(body == NULL)
		return;
	unsigned int i;
	glPushMatrix();
	glTranslatef(5*pos.x, pos.y, cGraphics::world->height*10-5*pos.z);

	
	float modelview[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

	for(i = 0; i < 3; i++)
		for(unsigned int j = 0; j < 3; j++)
			modelview[i*4+j] = ((i == j) ? 1.0 : 0.0);
	glLoadMatrixf(modelview);
	glScalef(scale,scale,1);

	glTranslatef(0,0,5);

	if(!body->loaded)
	{
		glPopMatrix();
		return;
	}


	int id = (8*action+direction) % body->actions.size();

	while(cGraphics::worldContainer->camera.rot > 360)
		cGraphics::worldContainer->camera.rot-=360;
	while(cGraphics::worldContainer->camera.rot < 0)
		cGraphics::worldContainer->camera.rot+=360;

	if(scale != 1)
		id = (8*action+((int)(8+direction-((cGraphics::worldContainer->camera.rot/(bEngine::math::PI/180.0f)-22.5)/45))%8)) % body->actions.size();

	if(body->actions[id]->frameCount <= 0)
	{
		glPopMatrix();
		return;
	}
	if(body->actions[id]->frames[0]->subFrameCount == 0)
	{
		glPopMatrix();
		return;
	}

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);



	
	cActSpr::cAction::cFrame* bodyframe = body->actions[id]->frames[(tickcount()/100) % ((body->actions[id]->frameCount != 3 || (action != 0 && action != 2)) ? body->actions[id]->frameCount : 1)];
	cActSpr::cAction::cFrame::cSubFrame* subframe = bodyframe->subframes[0];
	int frame = subframe->image;
	int dir = subframe->direction;
	
	glBindTexture(GL_TEXTURE_2D, body->frames[frame]->texId());
	float width = body->frames[frame]->w;
	float height = body->frames[frame]->h;
//	float bodyheight = height;

	width/=2;
	height/=2;
	glTranslatef(-subframe->offsetX, -subframe->offsetY, 0);
	glBegin(GL_QUADS);
		glTexCoord2f(dir,0);		glVertex3f(width,height,0);
		glTexCoord2f(1-dir,0);		glVertex3f(-width,height,0);
		glTexCoord2f(1-dir,1);		glVertex3f(-width,-height,0);
		glTexCoord2f(dir,1);		glVertex3f(width,-height,0);
	glEnd();
	glTranslatef(subframe->offsetX, subframe->offsetY, 0);



	if(head == NULL)
	{
		glPopMatrix();
		return;
	}

	if(!head->loaded)
	{
		glPopMatrix();
		return;
	}




	subframe = head->actions[id]->frames[0]->subframes[1];
	cActSpr::cAction::cFrame* myframe = head->actions[id]->frames[0];
	frame = subframe->image;
	//frame = 1;
	dir = subframe->direction;
	
	glBindTexture(GL_TEXTURE_2D, head->frames[frame]->texId());
	width = head->frames[frame]->w;
	height = head->frames[frame]->h;

	width/=2;
	height/=2;

	glPushMatrix();
	glTranslatef(-(subframe->offsetX + bodyframe->extraX - myframe->extraX), -(subframe->offsetY + bodyframe->extraY - myframe->extraY), 0);
	glBegin(GL_QUADS);
		glTexCoord2f(dir,0);		glVertex3f(width,height,0);
		glTexCoord2f(1-dir,0);	glVertex3f(-width,height,0);
		glTexCoord2f(1-dir,1);	glVertex3f(-width,-height,0);
		glTexCoord2f(dir,1);		glVertex3f(width,-height,0);
	glEnd();
	glPopMatrix();

	for(i = 0; i < extras.size(); i++)
	{
		if(extras[i] == NULL)
			continue;
		if(!extras[i]->loaded)
			continue;
		subframe = extras[i]->actions[id]->frames[0]->subframes[0];
		cActSpr::cAction::cFrame* myframe = extras[i]->actions[id]->frames[0];
		frame = subframe->image;
		//frame = 1;
		dir = subframe->direction;
		
		glBindTexture(GL_TEXTURE_2D, extras[i]->frames[frame]->texId());
		width = extras[i]->frames[frame]->w;
		height = extras[i]->frames[frame]->h;

		width/=2;
		height/=2;

		glPushMatrix();
		glTranslatef(-(subframe->offsetX + bodyframe->extraX - myframe->extraX), -(subframe->offsetY + bodyframe->extraY - myframe->extraY), 0);
		glBegin(GL_QUADS);
			glTexCoord2f(dir,0);		glVertex3f(width,height,1);
			glTexCoord2f(1-dir,0);		glVertex3f(-width,height,1);
			glTexCoord2f(1-dir,1);		glVertex3f(-width,-height,1);
			glTexCoord2f(dir,1);		glVertex3f(width,-height,1);
		glEnd();
		glPopMatrix();

	}
	
	glPopMatrix();
}



cSprite::cActSpr::cFrame::~cFrame()
{
	if(data != NULL)
		delete[] data;
	data = NULL;

	if(loaded)
		glDeleteTextures(1, &tex);
	loaded = false;

}

cSprite::cActSpr::cAction::~cAction()
{
	for(unsigned int i = 0; i < frames.size(); i++)
		delete frames[i];
}

cSprite::cActSpr::cAction::cFrame::~cFrame()
{
	for(unsigned int i = 0; i < subframes.size(); i++)
		delete subframes[i];
}
