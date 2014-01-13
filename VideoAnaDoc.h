// VideoAna Version 3.0\n\nVideo analyzing framework demonstration program.
// Copyright (C) 2006-2008 by Mingliang Zhu
// http://dev.mingliang.org/article/VideoAnaFramework.php
// developer[AT]mingliang[DOT]org

// VideoAnaDoc.h : interface of the CVideoAnaDoc class
//


#pragma once

#include <atlbase.h>

#include "dshow.h"
#include "qedit.h"
#include "stdio.h"
#include <vector>
#include <algorithm>
using namespace std;

// Some code in "vector" generate this warning and is bothering :-(
#pragma warning(disable : 4995)
#include "vector"
#pragma warning(default : 4995)

#define FRAME_DIFF_BUFFER_SIZE 50

class CVideoAnaDoc;

// SVideoProcessAdapter is only for providing a callback for ISampleGrabber.
class SVideoProcessAdapter : public ISampleGrabberCB
{
public:
	SVideoProcessAdapter(CVideoAnaDoc *pVideoAnaDoc):m_pVideoAnaDoc(pVideoAnaDoc){}
public:
	virtual ~SVideoProcessAdapter(void){}

public:
	STDMETHOD(BufferCB)(double SampleTime, BYTE *pBuffer, long nBufferLen);

	STDMETHOD(SampleCB)(double, IMediaSample *){return S_OK;}

	STDMETHOD( QueryInterface )( REFIID iid, LPVOID *ppv )
	{
		if( iid == IID_ISampleGrabberCB || iid == IID_IUnknown )
		{ 
			*ppv = (void *) static_cast<ISampleGrabberCB*>( this );
			return NOERROR;
		} 
		return E_NOINTERFACE;
	}

	STDMETHOD_( ULONG, AddRef )(){return 2;}
	STDMETHOD_( ULONG, Release)(){return 1;}

protected:
	CVideoAnaDoc *m_pVideoAnaDoc;
};

class CVideoAnaView;

class CVideoAnaDoc : public CDocument
{
	friend class CVideoAnaView;

	// SVideoProcessAdapter is only for providing a callback for DirectShow.
	// All data are actually stored and processed in CVideoAnaDoc,
	// so make SVideoProcessAdapter a friend for convenience
	friend class SVideoProcessAdapter;

protected: // create from serialization only
	CVideoAnaDoc();
	DECLARE_DYNCREATE(CVideoAnaDoc)

// Attributes
public:
	enum GraphStutas
	{
		GRAPH_NONE,
		GRAPH_STOPPED,
		GRAPH_RUNNING,
		GRAPH_PAUSED,
		GRAPH_PAUSEPENDING,	// Pause action has fired but not finished yet
		GRAPH_STOPPENDING	// Stop action has fired but not finished yet
	};

protected:
	CVideoAnaView *m_pVideoAnaView;
	HANDLE m_hUpdateEvent;

	// DirectShow objects needed to extract frames
	IGraphBuilder *m_pGraph;
	IBaseFilter *m_pGrabberFilter;
	ISampleGrabber *m_pGrabber;
	IBaseFilter *m_pSrcFilter;
	IBaseFilter *m_pNullRenderer;
	IMediaControl *m_pControl;
	IMediaEventEx *m_pEvent;
	SVideoProcessAdapter m_VideoProcessAdapter;

	// The processing status
	GraphStutas m_GraphStatus;

	// Data for processing frames
	REFERENCE_TIME m_lDuration;
	REFERENCE_TIME m_lTimeperFrame;
	REFERENCE_TIME m_nTotalFrames;
	unsigned int m_nCurFrame;	// Frame no that is currently processing
	int frameRate;
	bool *fingerOutline;
	vector<double> intensities;
	vector<double> saturations;
	BITMAPINFOHEADER m_Bih;	// info header of frames of the video
	BITMAPFILEHEADER m_Bfh;	// File header of frames of the video
	vector<double> hues;

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CVideoAnaDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	// This function is called when a frame is extracted by DirectShow.
	// Note that the function will be called from another thread, 
	// so pay special attention to cooperations among threads!
	HRESULT ProcessFrame(double SampleTime, BYTE *pBuffer, long nBufferLen);

	//// Wait until analyzing ends or pauses
	//static UINT __cdecl WaitProc(CVideoAnaDoc * pThis);

	// Helpers building the graph
	HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pFirst, IBaseFilter *pSecond);
	HRESULT GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);

	// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	afx_msg void OnUpdateAnaStart(CCmdUI *pCmdUI);
	afx_msg void OnUpdateAnaPause(CCmdUI *pCmdUI);
	afx_msg void OnUpdateAnaStop(CCmdUI *pCmdUI);
	afx_msg void OnAnaStart();
	afx_msg void OnAnaPause();
	afx_msg void OnAnaStop();
	afx_msg void OnAnaFinished();
	int *performAnalysis(vector<double> data);
	void OnGraphNotify();
};


