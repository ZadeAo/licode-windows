#include "stdafx.h"
#include "interface.h"
#include "VideoDialog.h"
#include "resource.h"


const TCHAR* const kCtrlVideo = _T("ctrl_video");
const TCHAR* const kLabel_Name = _T("Label_Name");

cVideoDialog::cVideoDialog(UInt32 id)
	: m_nId(id)
{
	
}

cVideoDialog::~cVideoDialog()
{

}

void cVideoDialog::DoInit()
{
	SetFixedXY(CSize(362,270));

	CDialogBuilder builder;
	CContainerUI* pComputerExamine = static_cast<CContainerUI*>(builder.Create(_T("render.xml"), (UINT)0));
	if (pComputerExamine)
	{
		this->Add(pComputerExamine);

		m_pVideo = static_cast<CControlUI*>(m_pManager->FindControl(kCtrlVideo));

		CLabelUI *pLabel = static_cast<CLabelUI*>(m_pManager->FindControl(kLabel_Name));
		if (pLabel)
		{
			CString strID;
			strID.Format(_T("ID:%d"), m_nId);
			pLabel->SetText(strID);
		}
	}
	else
	{
		this->RemoveAll();
		return;
	}
}

void cVideoDialog::RenderImage(const BITMAPINFO& bmi, const UInt8 *image)
{
	if (!m_pVideo)
	{
		return;
	}

	if (image)
	{
		CDuiRect rc = m_pVideo->GetPos();
		int BitMapW = rc.GetWidth();
		int BitMapH = rc.GetHeight();
		//获取dc和内存位图
		HDC hdc = m_pManager->GetPaintDC();
		HDC dc_mem = ::CreateCompatibleDC(hdc);
		::SetStretchBltMode(dc_mem, HALFTONE);

		HBITMAP bmp_mem = ::CreateCompatibleBitmap(hdc, BitMapW, BitMapH);
		HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);

		int height = abs(bmi.bmiHeader.biHeight);
		int width = bmi.bmiHeader.biWidth;
		if (width*BitMapH > height*BitMapW)
		{
			int reHeight = (float)(BitMapW*height) / width;
			StretchDIBits(dc_mem, 0, (BitMapH - reHeight)*0.5, BitMapW, reHeight,
				0, 0, width, height, image, &bmi, DIB_RGB_COLORS, SRCCOPY);
		}
		else
		{
			int reWidth = (float)(BitMapH*width) / height;
			StretchDIBits(dc_mem, (BitMapW - reWidth)*0.5, 0, reWidth, BitMapH,
				0, 0, width, height, image, &bmi, DIB_RGB_COLORS, SRCCOPY);
		}


		BitBlt(hdc, rc.left, rc.top, BitMapW, BitMapH,
			dc_mem, 0, 0, SRCCOPY);

		// Cleanup.
		::SelectObject(dc_mem, bmp_old);
		::DeleteObject(bmp_mem);
		::DeleteDC(dc_mem);
	}
	else
	{
		m_pVideo->Invalidate();
	}
}

