#ifndef CHATDIALOG_HPP
#define CHATDIALOG_HPP

#pragma once

#include "interface.h"

class cVideoDialog : public CContainerUI, public Renderer
{
public:
	cVideoDialog(UInt32 id);
	~cVideoDialog();

protected:
	virtual void DoInit();
	virtual void RenderImage(const BITMAPINFO& bmi, const UInt8 *image);
	
private:
	CControlUI *m_pVideo = nullptr;
	UInt32 m_nId;
};


#endif // CHARTDIALOG_HPP