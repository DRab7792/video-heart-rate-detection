// VideoAna Version 3.0\n\nVideo analyzing framework demonstration program.
// Copyright (C) 2006-2008 by Mingliang Zhu
// http://dev.mingliang.org/article/VideoAnaFramework.php
// developer[AT]mingliang[DOT]org

// VideoAnaView.cpp : implementation of the CVideoAnaView class
//

#include "stdafx.h"
#include "VideoAna.h"

#include "VideoAnaDoc.h"
#include "VideoAnaView.h"
#include "stdio.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVideoAnaView

IMPLEMENT_DYNCREATE(CVideoAnaView, CFormView)

BEGIN_MESSAGE_MAP(CVideoAnaView, CFormView)
	ON_WM_TIMER()
END_MESSAGE_MAP()

// CVideoAnaView construction/destruction

CVideoAnaView::CVideoAnaView()
	: CFormView(CVideoAnaView::IDD)
	, m_strAnaStatus(_T("Not analyzing. Open a video file and push \"start\"."))
{
	// TODO: add construction code here

}

CVideoAnaView::~CVideoAnaView()
{
}

void CVideoAnaView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PREVIEWBOX, m_PreviewBox);
	DDX_Text(pDX, IDC_ANA_STATUS, m_strAnaStatus);
	DDX_Control(pDX, IDC_ANA_PROGRESS, m_AnaProgress);
}

BOOL CVideoAnaView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

void CVideoAnaView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();
}


// CVideoAnaView diagnostics

#ifdef _DEBUG
void CVideoAnaView::AssertValid() const
{
	CFormView::AssertValid();
}

void CVideoAnaView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CVideoAnaDoc* CVideoAnaView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVideoAnaDoc)));
	return (CVideoAnaDoc*)m_pDocument;
}
#endif //_DEBUG


// CVideoAnaView message handlers

LRESULT CVideoAnaView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: Add your specialized code here and/or call the base class
	switch(message)
	{
	case WM_USER_PREVIEW_FRAME:
		{
			// Preview image
			m_PreviewBox.SetDibBits((LPBYTE)wParam, (int)lParam);
			m_PreviewBox.PaintDIB();

			return 0;
			break;
		}
	case WM_USER_UPDATE_PROGRESS:
		{
			// Ana status
			m_AnaProgress.SetPos((int)wParam);
			m_strAnaStatus.Format("Analyzing... Frame: %d/%d", GetDocument()->hues.size(), GetDocument()->m_nTotalFrames);
			UpdateData(FALSE);
			return 0;
			break;
		}
	case WM_APP_GRAPHNOTIFY:
		{
			GetDocument()->OnGraphNotify();
			return 0;
		}

	default:
		return CFormView::WindowProc(message, wParam, lParam);
	    break;
	}

}

void CVideoAnaView::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if(ID_TIMER_EVENT_UPDATE == nIDEvent)
		SetEvent(GetDocument()->m_hUpdateEvent);

	CFormView::OnTimer(nIDEvent);
}

void CVideoAnaView::ClearAll()
{
	m_AnaProgress.SetPos(0);
	m_PreviewBox.SetDibBits(NULL, 0);

	m_strAnaStatus = "Not analyzing. Open a video file and push \"start\".";

	UpdateData(FALSE);
}

