#ifndef __FILEWINDOW_H__
#define __FILEWINDOW_H__

#include <wm/window.h>

#include <wm/windowroundbutton.h>
#include <wm/windowbutton.h>
#include <wm/windowinputbox.h>

#include <vector>

class cFileWindow : public cWindow
{

	class cOkButton : public cWindowButton
	{
	public:
		void (*callback)(std::string);
		cOkButton(cWindow* parent, void (*pCallback)(std::string), TiXmlDocument* skin = &cWM::skin);
		void onClick();
	};
public:
	class cWindowFilterBox : public cWindowInputBox
	{
	public:
		cWindowFilterBox(cWindow* parent, TiXmlDocument* skin = &cWM::skin);
		void onChange();
	};

	std::vector<std::string> mapNames;

	cFileWindow(void (*pCallback)(std::string));
	void resizeTo(int ww, int hh);
};

#endif
