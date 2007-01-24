#include "windowroundbutton.h"
#include "window.h"
#include <math.h>

#include <GL/gl.h>												// Header File For The OpenGL32 Library
#include <GL/glu.h>												// Header File For The GLu32 Library
#include "../graphics.h"
extern cGraphics Graphics;

extern float mousex, mousey;

void cWindowRoundButton::draw()
{
	GLfloat colors[4];
	glGetFloatv(GL_CURRENT_COLOR, colors);
	if (this->inobject())
		glColor3f(0.75f, 0.75f, 1.0f);
	glTranslatef((float)realx(), (float)realy2(), 0);
	glBindTexture(GL_TEXTURE_2D, parent->texture->texid());
	glBegin(GL_QUADS);
		glTexCoord2f(258.0f/512.0f,	211.0f/512.0f);		glVertex2d(0,0);
		glTexCoord2f(386.0f/512.0f,	211.0f/512.0f);		glVertex2d(w,0);
		glTexCoord2f(386.0f/512.0f,	339.0f/512.0f);		glVertex2d(w,h);
		glTexCoord2f(258.0f/512.0f,	339.0f/512.0f);		glVertex2d(0,h);		
	glEnd();
	glTranslatef(-(float)realx(), -(float)realy2(), 0);
	glColor4fv(colors);
}


void cWindowRoundButton::click()
{
	Log(1,0,"You clicked the button!");
}

bool cWindowRoundButton::inobject()
{
	int xx=mousex-parent->px();
	int yy=(Graphics.h()-mousey)-parent->py();
	int xxx = realx() + (w/2);
	int yyy = realy() + (h/2);
	if (((xx - xxx)*(xx - xxx))+((yy - yyy)*(yy - yyy)) < (w*w)/2)
		return true;
	return false;
}
