#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <string>
class cWindow;


class cBrowInterface
{
public:
	virtual void fixNormals() = 0;
	virtual cWindow* addXmlWindow(std::string) = 0;
	virtual int  getSelectedTextureIndex() = 0;
	virtual void render() = 0;
};


#endif