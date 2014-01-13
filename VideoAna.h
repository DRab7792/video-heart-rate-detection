// VideoAna.h : main header file for the VideoAna application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CVideoAnaApp:
// See VideoAna.cpp for the implementation of this class
//

class CVideoAnaApp : public CWinApp
{
public:
	CVideoAnaApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
public:
	virtual int ExitInstance();
};

extern CVideoAnaApp theApp;