#pragma once


// SPicBox

class SPicBox : public CStatic
{
	DECLARE_DYNAMIC(SPicBox)

public:
	SPicBox():m_pDibBits(NULL){};
	virtual ~SPicBox();

	bool SetDibBits(LPBYTE pDibBits, int nSize);
	bool SetBih(LPBITMAPINFOHEADER pBih);
	void PaintDIB(const CDC *pDC);
	void PaintDIB(void);

protected:
	LPBYTE m_pDibBits;
	BITMAPINFOHEADER m_Bih;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};


