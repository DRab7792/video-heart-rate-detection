// VideoAnaView.h : interface of the CVideoAnaView class
//


#pragma once
#include "afxwin.h"
#include "PicBox.h"
#include "afxcmn.h"


class CVideoAnaDoc;
class CVideoAnaView : public CFormView
{
	friend class CVideoAnaDoc;
protected: // create from serialization only
	CVideoAnaView();
	DECLARE_DYNCREATE(CVideoAnaView)

public:
	enum{ IDD = IDD_VIDEOANA_FORM };

// Attributes
public:
	CVideoAnaDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate(); // called first time after construct

// Implementation
public:
	virtual ~CVideoAnaView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void ClearAll();

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
public:
	CString m_strAnaStatus;
	CProgressCtrl m_AnaProgress;
	SPicBox m_PreviewBox;

};

#ifndef _DEBUG  // debug version in VideoAnaView.cpp
inline CVideoAnaDoc* CVideoAnaView::GetDocument() const
   { return reinterpret_cast<CVideoAnaDoc*>(m_pDocument); }
#endif

