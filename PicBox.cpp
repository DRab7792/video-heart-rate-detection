// PicBox.cpp : implementation file
//

#include "stdafx.h"
#include "VideoAna.h"
#include "PicBox.h"
#include "stdio.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// SPicBox

IMPLEMENT_DYNAMIC(SPicBox, CStatic)

//SPicBox::SPicBox()
//{
//
//}
//
SPicBox::~SPicBox()
{
	delete[] m_pDibBits;
}


BEGIN_MESSAGE_MAP(SPicBox, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()



// SPicBox message handlers

void SPicBox::PaintDIB(const CDC *pDC)
{
	int x = 0, y = 0;
	CRect ClientRect;
	GetClientRect(ClientRect);

	// Stretch the Image to fit the window and center it ( Reduce the image size only, do not enlarge)
	if(m_Bih.biWidth > ClientRect.Width() || m_Bih.biHeight > ClientRect.Height())	// Image is larger
	{
		// Calculate the height/width ratio to decide how to reduce
		if((double)m_Bih.biHeight / m_Bih.biWidth > (double)ClientRect.Height() / ClientRect.Width())	// Image is higher
		{
			ClientRect.left = (ClientRect.right - (LONG)((double)m_Bih.biWidth / m_Bih.biHeight * ClientRect.bottom)) / 2;
			ClientRect.right -= ClientRect.left;
		}
		else	// Image is wider
		{
			ClientRect.top = (ClientRect.bottom - (LONG)((double)m_Bih.biHeight / m_Bih.biWidth * ClientRect.right)) / 2;
			ClientRect.bottom -= ClientRect.top;
		}
	}
	else	// Image is smaller, center only
	{
		ClientRect.left = (ClientRect.Width() - m_Bih.biWidth) / 2;
		ClientRect.right = ClientRect.left + m_Bih.biWidth;
		ClientRect.top = (ClientRect.Height() - m_Bih.biHeight) / 2;
		ClientRect.bottom = ClientRect.top + m_Bih.biHeight;
	}

	::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);
	StretchDIBits(pDC->m_hDC, ClientRect.left, ClientRect.top, ClientRect.Width(), ClientRect.Height(),
		0, 0, m_Bih.biWidth, m_Bih.biHeight, m_pDibBits, (LPBITMAPINFO)&m_Bih,
		DIB_RGB_COLORS, SRCCOPY);
}

void SPicBox::PaintDIB(void)
{
	CDC *pDC = GetDC();
	PaintDIB(pDC);
	ReleaseDC(pDC);
}

bool SPicBox::SetBih(LPBITMAPINFOHEADER pBih)
{
	memcpy(&m_Bih, pBih, sizeof(BITMAPINFOHEADER));

	SetDibBits(NULL, 0);

	return true;
}

bool SPicBox::SetDibBits(LPBYTE pDibBits, int nSize)
{
	if(NULL == pDibBits)
	{
		delete[] m_pDibBits;
		m_pDibBits = NULL;
		RedrawWindow();
		return true;
	}

	int nSizeRequired = (m_Bih.biWidth * 24 + 31) / 32 * 4 * m_Bih.biHeight;
	ASSERT(nSize == nSizeRequired);

	if(nSize != nSizeRequired)
		return false;

	delete[] m_pDibBits;
	m_pDibBits = new BYTE[nSizeRequired];
	memcpy(m_pDibBits, pDibBits, nSizeRequired);

	return true;
}

void SPicBox::OnPaint()
{
	if(!m_pDibBits)
	{
		CStatic::OnPaint();
		return;
	}

	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CStatic::OnPaint() for painting messages
	PaintDIB(&dc);
}
