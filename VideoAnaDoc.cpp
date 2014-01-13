// VideoAna Version 3.0\n\nVideo analyzing framework demonstration program.
// Copyright (C) 2006-2008 by Mingliang Zhu
// http://dev.mingliang.org/article/VideoAnaFramework.php
// developer[AT]mingliang[DOT]org

// VideoAnaDoc.cpp : implementation of the CVideoAnaDoc class
//

#include "stdafx.h"
#include "VideoAna.h"

#include "VideoAnaDoc.h"
#include "VideoAnaView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }

STDMETHODIMP SVideoProcessAdapter::BufferCB(double SampleTime, BYTE *pBuffer, long nBufferLen)
{
	return m_pVideoAnaDoc->ProcessFrame(SampleTime, pBuffer, nBufferLen);
}

// CVideoAnaDoc

IMPLEMENT_DYNCREATE(CVideoAnaDoc, CDocument)

BEGIN_MESSAGE_MAP(CVideoAnaDoc, CDocument)
	ON_UPDATE_COMMAND_UI(ID_ANA_START, &CVideoAnaDoc::OnUpdateAnaStart)
	ON_UPDATE_COMMAND_UI(ID_ANA_PAUSE, &CVideoAnaDoc::OnUpdateAnaPause)
	ON_UPDATE_COMMAND_UI(ID_ANA_STOP, &CVideoAnaDoc::OnUpdateAnaStop)
	ON_COMMAND(ID_ANA_START, &CVideoAnaDoc::OnAnaStart)
	ON_COMMAND(ID_ANA_PAUSE, &CVideoAnaDoc::OnAnaPause)
	ON_COMMAND(ID_ANA_STOP, &CVideoAnaDoc::OnAnaStop)
	ON_COMMAND(ID_ANA_FINISHED, &CVideoAnaDoc::OnAnaFinished)
END_MESSAGE_MAP()


// CVideoAnaDoc construction/destruction

CVideoAnaDoc::CVideoAnaDoc()
: m_pGraph(NULL)
, m_pGrabberFilter(NULL)
, m_pGrabber(NULL)
, m_pControl(NULL)
, m_pEvent(NULL)
, m_GraphStatus(GRAPH_NONE)
, m_pSrcFilter(NULL)
, m_pNullRenderer(NULL)
, m_VideoProcessAdapter(this)
, m_nCurFrame(0)
{
	// TODO: add one-time construction code here
	memset(&m_Bfh, 0, sizeof(m_Bfh));
	m_Bfh.bfType = 0x4d42;
	//m_Bfh.bfSize = sizeof(m_Bfh) + sizeof(BITMAPINFOHEADER) + cbBuffer;
	m_Bfh.bfOffBits = sizeof(m_Bfh) + sizeof(BITMAPINFOHEADER);

	m_hUpdateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	memset(&m_Bih, 0, sizeof(m_Bih));
}

CVideoAnaDoc::~CVideoAnaDoc()
{
	//ClearAll();
}

BOOL CVideoAnaDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CVideoAnaDoc serialization

void CVideoAnaDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CVideoAnaDoc diagnostics

#ifdef _DEBUG
void CVideoAnaDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CVideoAnaDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CVideoAnaDoc commands

BOOL CVideoAnaDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	// TODO:  Add your specialized creation code here
	m_GraphStatus = GRAPH_STOPPED;

	POSITION ViewPos = GetFirstViewPosition();
	m_pVideoAnaView = (CVideoAnaView *)GetNextView(ViewPos);
	m_pVideoAnaView->m_PreviewBox.SetDibBits(NULL, 0);

	m_pVideoAnaView->RedrawWindow();
	m_pVideoAnaView->ClearAll();

	return TRUE;
}

void CVideoAnaDoc::OnUpdateAnaStart(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if(GRAPH_STOPPED == m_GraphStatus)
		pCmdUI->Enable();
	else
		pCmdUI->Enable(FALSE);
}

void CVideoAnaDoc::OnUpdateAnaPause(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if(GRAPH_RUNNING == m_GraphStatus || GRAPH_PAUSED == m_GraphStatus)
		pCmdUI->Enable();
	else
		pCmdUI->Enable(FALSE);
}

void CVideoAnaDoc::OnUpdateAnaStop(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if(GRAPH_RUNNING == m_GraphStatus || GRAPH_PAUSED == m_GraphStatus)
		pCmdUI->Enable();
	else
		pCmdUI->Enable(FALSE);
}

void CVideoAnaDoc::OnAnaStart()
{
	// TODO: Add your command handler code here
	ASSERT(GRAPH_STOPPED == m_GraphStatus);
	
	// Create the graph builder
	HRESULT hr = ::CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder, (void**)(&m_pGraph));
	if (FAILED(hr))
	{
		AfxMessageBox("Failed creating DirectShow objects!");
		return;
	}

	// Create the Sample Grabber
	ASSERT(m_pGrabber == NULL);
	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, (void **)(&m_pGrabberFilter));
	hr = m_pGrabberFilter->QueryInterface(IID_ISampleGrabber,
		(void **)(&m_pGrabber));
	hr = m_pGraph->AddFilter(m_pGrabberFilter, L"SampleGrabber");

	// Set the media type
	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
	mt.formattype = FORMAT_VideoInfo; 
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_RGB24;	// only accept 24-bit bitmaps
	hr = m_pGrabber->SetMediaType(&mt);

	// Create the src filter
	wchar_t strFilename[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, m_strPathName, -1, strFilename, MAX_PATH);
	hr = m_pGraph->AddSourceFilter(strFilename, L"Source", &m_pSrcFilter);
	if(FAILED(hr))
	{
		AfxMessageBox("Unsupported media type!");
		return;
	}

	// Connect the src and grabber
	hr = ConnectFilters(m_pGraph, m_pSrcFilter, m_pGrabberFilter);
	if(FAILED(hr))
	{
		SAFE_RELEASE(m_pSrcFilter);
		SAFE_RELEASE(m_pGrabber);
		SAFE_RELEASE(m_pGrabberFilter);
		SAFE_RELEASE(m_pGraph);
		AfxMessageBox("Unsupported media type!");
		return;
	}

	// Create the NULL renderer and connect
	m_pNullRenderer = NULL;
	hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, (void **)(&m_pNullRenderer));
	hr = m_pGraph->AddFilter(m_pNullRenderer, L"NullRenderer");
	hr = ConnectFilters(m_pGraph, m_pGrabberFilter, m_pNullRenderer);

	m_nCurFrame = 0;

	// Set modes
	m_pGrabber->SetBufferSamples(FALSE);	// Buffer seems to be no use in callback mode
	m_pGrabber->SetCallback(&m_VideoProcessAdapter, 1);

	// Necessary interfaces for controlling
	m_pGraph->QueryInterface(IID_IMediaControl, (void **)(&m_pControl));
	m_pGraph->QueryInterface(IID_IMediaEventEx, (void **)(&m_pEvent));

	m_pEvent->SetNotifyWindow((OAHWND)m_pVideoAnaView->m_hWnd, WM_APP_GRAPHNOTIFY, 0);

	// Turn off the sync clock for max speed
	IMediaFilter *pMediaFilter = NULL;
	m_pGraph->QueryInterface(IID_IMediaFilter, reinterpret_cast<void**>(&pMediaFilter));
	pMediaFilter->SetSyncSource(NULL);
	SAFE_RELEASE(pMediaFilter);

	// Retrieve the actual media type
	ZeroMemory(&mt, sizeof(mt));
	hr = m_pGrabber->GetConnectedMediaType(&mt);
	VIDEOINFOHEADER *pVih;
	if (mt.formattype == FORMAT_VideoInfo) 
		pVih = reinterpret_cast<VIDEOINFOHEADER*>(mt.pbFormat);
	else 
	{
		SAFE_RELEASE(m_pControl);
		SAFE_RELEASE(m_pEvent);
		SAFE_RELEASE(m_pSrcFilter);
		SAFE_RELEASE(m_pNullRenderer);
		SAFE_RELEASE(m_pGrabber);
		SAFE_RELEASE(m_pGrabberFilter);
		SAFE_RELEASE(m_pGraph);
		AfxMessageBox("No video stream found!");
		return; // Something went wrong, perhaps not appropriate media type
	}

	// Save the video info header
	memcpy(&m_Bih, &pVih->bmiHeader, sizeof(m_Bih));
	m_Bfh.bfSize = sizeof(m_Bfh) + sizeof(BITMAPINFOHEADER) + m_Bih.biSizeImage;
	m_pVideoAnaView->m_PreviewBox.SetBih(&m_Bih);
	m_lTimeperFrame = pVih->AvgTimePerFrame;

	// Free the media type
	if (mt.cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mt.pbFormat);
		// Strictly unnecessary but tidier
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if (mt.pUnk != NULL)
	{
		// Unnecessary because pUnk should not be used, but safest.
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}

	// Get video info
	IMediaSeeking *pSeeking = NULL;
	m_pGraph->QueryInterface(IID_IMediaSeeking, (void **)(&pSeeking));
	pSeeking->GetDuration(&m_lDuration);
	if(FAILED(pSeeking->SetTimeFormat(&TIME_FORMAT_FRAME)))
		m_nTotalFrames = m_lDuration / m_lTimeperFrame;
	else
		pSeeking->GetDuration(&m_nTotalFrames);
	SAFE_RELEASE(pSeeking);

	m_pVideoAnaView->ClearAll();
	m_pVideoAnaView->m_AnaProgress.SetRange32(0, (int)m_nTotalFrames);

	// Setup the view
	m_pVideoAnaView->SetTimer(ID_TIMER_EVENT_UPDATE, 500, NULL);
	SetEvent(m_hUpdateEvent);

	frameRate=(int)ceil((m_nTotalFrames*10000000.0)/m_lDuration);
	fingerOutline = new bool[m_Bih.biHeight * m_Bih.biWidth];
	m_GraphStatus = GRAPH_RUNNING;
	m_pControl->Run(); // Run the graph to start the analyzing process!
	
}

void CVideoAnaDoc::OnAnaPause()
{
	// TODO: Add your command handler code here
	ASSERT(GRAPH_RUNNING == m_GraphStatus || GRAPH_PAUSED == m_GraphStatus);
	if(GRAPH_RUNNING == m_GraphStatus)
	{
		if(S_FALSE == m_pControl->Pause())
		{
			OAFilterState oState;
			m_pControl->GetState(INFINITE, &oState);
		}
		m_GraphStatus = GRAPH_PAUSED;
	}
	else
	{
		m_pControl->Run();
		m_GraphStatus = GRAPH_RUNNING;
	}

}

void CVideoAnaDoc::OnAnaStop()
{
	// TODO: Add your command handler code here
	ASSERT(GRAPH_RUNNING == m_GraphStatus || GRAPH_PAUSED == m_GraphStatus);

	if(GRAPH_RUNNING == m_GraphStatus && S_FALSE == m_pControl->Pause())
	{
		OAFilterState oState;
		m_pControl->GetState(INFINITE, &oState);	// Wait until pause finished
		TRACE("Wait to paused\n");
	}
	m_pControl->Stop();
	m_GraphStatus = GRAPH_STOPPED;

	OnAnaFinished();
}

void CVideoAnaDoc::OnAnaFinished()
{
	// TODO: Add your command handler code here
	ASSERT(GRAPH_STOPPED == m_GraphStatus || GRAPH_RUNNING == m_GraphStatus);

	m_pVideoAnaView->KillTimer(ID_TIMER_EVENT_UPDATE);
	m_GraphStatus = GRAPH_STOPPED;
	// Free DirectShow resources when finished analyzing
	SAFE_RELEASE(m_pControl);
	SAFE_RELEASE(m_pEvent);
	SAFE_RELEASE(m_pSrcFilter);
	SAFE_RELEASE(m_pGraph);
	SAFE_RELEASE(m_pNullRenderer);
	SAFE_RELEASE(m_pGrabber);
	SAFE_RELEASE(m_pGrabberFilter);

	CString strMsg;
	vector<double>::iterator it = min_element(hues.begin(), hues.end());
	//vector<int> squared;
	int *hueResults = performAnalysis(hues);
	int *intensityResults = performAnalysis(intensities);
	int *satResults = performAnalysis(saturations);
	int bpm = 0;
	int total = 0;
	int avgDistance = 0;
	string method = "";
	int maxBeats = max(hueResults[1],max(intensityResults[1], satResults[1]));
	if (intensityResults[1] == maxBeats){
		bpm = intensityResults[0];
		total = intensityResults[1];
		avgDistance = intensityResults[2];
		method = "intensity";
	}else if (hueResults[1] == maxBeats){
		bpm = hueResults[0];
		total = hueResults[1];
		avgDistance = hueResults[2];
		method = "hue";
	}else if (satResults[1] == maxBeats){
		bpm = satResults[0];
		total = satResults[1];
		avgDistance = satResults[2];
		method = "saturation";
	}
	strMsg.Format("Process finished. %d frames total. The method of detection used is %s. The frame rate is %d frames per second and the average time between beats was %d frames over %d detected beats. Therefore, your heart rate is rougly %d bpm.", m_nCurFrame, method.c_str(), frameRate, avgDistance, total,  bpm);
	AfxMessageBox(strMsg);
}

//Somsook - 72 bpm
//Ricky - 78 bpm
//Somsook - active - 126 bpm
//Baby - 148bpm

int *CVideoAnaDoc::performAnalysis(vector<double> data){
	vector<int> firstDeriv;
	vector<int> secondDeriv;
	int i=0;
	/*for (vector<double>::iterator it=hues.begin();it!=hues.end();it++){
		squared.push_back((int)pow(*it,2));
	}*/
	firstDeriv.push_back(0);
	for (i=1; i<data.size();i++){
			firstDeriv.push_back((int)ceil(data[i] - data[i-1]));
	}
	secondDeriv.push_back(0);
	float sum = 0;
	float sum_sqrd = 0;
	for (i=1; i<firstDeriv.size();i++){
		secondDeriv.push_back(firstDeriv[i] - firstDeriv[i-1]);
		secondDeriv[i] *= abs(secondDeriv[i]);
		sum += secondDeriv[i];
		sum_sqrd += secondDeriv[i]*secondDeriv[i];
	}
	float mean = sum/(float)secondDeriv.size();
	float dev = sqrt((sum_sqrd - sum*mean)/(secondDeriv.size() - 1));
	int threshold = mean - (2 * dev);
	vector<int> beatFrames;
	for (i=0;i<secondDeriv.size();i++){
		if (secondDeriv[i]<threshold){
			beatFrames.push_back(i);
		}
	}
	int bpm =0;
	int medianDist = 0;
	int total = 0;
	int avgDistance = 0;
	bool uncertain = false;
	vector<int> distances;
	if (beatFrames.size()>1){
		vector<int> adjustedBeats;
		for (int i=1; i<beatFrames.size();i++){
			int dist = beatFrames[i] - beatFrames[i-1];
			if (dist>(frameRate/3)){
				int adjustedVal = min(beatFrames[i],beatFrames[i-1]);
				adjustedBeats.push_back(adjustedVal);
			}else {
				adjustedBeats.push_back(beatFrames[i]);
			}
		}
		
		
		if(adjustedBeats.size()>1){
			for (int i=1;i<adjustedBeats.size();i++){
				int dist = adjustedBeats[i] - adjustedBeats[i-1];
				if (dist>(frameRate/3) && dist<(frameRate*1.2)){
					avgDistance +=dist;
					distances.push_back(dist);
					total++;
				}
			}
			sort(distances.begin(), distances.end());
		}
		if (total>0){
			avgDistance = avgDistance/total;
			medianDist = distances[distances.size()/2];
			uncertain = (distances.back() - distances[0])>((2.0/3.0)*frameRate) || total<4;
			if (!uncertain) {
				bpm = (frameRate* 60)/medianDist;
			}else{
				bpm = (frameRate* 60)/avgDistance;
			}
		}
	}
	int *res = new int[3];
	res[0] = bpm;
	res[1] = total;
	if (!uncertain){
		res[2] = medianDist;
	}else{
		res[2] = avgDistance;
	}
	return res;
}


// Helper functions:
HRESULT CVideoAnaDoc::GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
	IEnumPins  *pEnum;
	IPin       *pPin;
	pFilter->EnumPins(&pEnum);
	while(pEnum->Next(1, &pPin, 0) == S_OK)
	{
		PIN_DIRECTION PinDirThis;
		pPin->QueryDirection(&PinDirThis);
		if (PinDir == PinDirThis)
		{
			pEnum->Release();
			*ppPin = pPin;
			return S_OK;
		}
		pPin->Release();
	}
	pEnum->Release();
	return E_FAIL;  
}

HRESULT CVideoAnaDoc::ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pFirst, IBaseFilter *pSecond)
{
	IPin *pOut = NULL, *pIn = NULL;
	HRESULT hr = GetPin(pSecond, PINDIR_INPUT, &pIn);
	if (FAILED(hr)) return hr;
	// The previous filter may have multiple outputs, so try each one!
	IEnumPins  *pEnum;
	pFirst->EnumPins(&pEnum);
	while(pEnum->Next(1, &pOut, 0) == S_OK)
	{
		PIN_DIRECTION PinDirThis;
		pOut->QueryDirection(&PinDirThis);
		if (PINDIR_OUTPUT == PinDirThis)
		{
			hr = pGraph->Connect(pOut, pIn);
			if(!FAILED(hr))
			{
				break;
			}
		}
		SAFE_RELEASE(pOut);
	}
	SAFE_RELEASE(pOut);
	SAFE_RELEASE(pEnum);
	SAFE_RELEASE(pIn);
	return hr;
}

HRESULT CVideoAnaDoc::ProcessFrame(double SampleTime, BYTE *pBuffer, long nBufferLen)
{
	// TODO: Put the frame processing code here
	// Keep in mind that code here is executed within another thread,
	// so do consider the data access problem among threads

	// Here just do nothing but send a preview image and update progress view every 0.5 seconds
	// Comment the following "if" line if you want to see each frame :)
	if(WAIT_OBJECT_0 == WaitForSingleObject(m_hUpdateEvent, 0) || m_nCurFrame == m_nTotalFrames - 1)
	{
		m_pVideoAnaView->SendMessage(WM_USER_PREVIEW_FRAME, (WPARAM)pBuffer, nBufferLen);
		m_pVideoAnaView->SendMessage(WM_USER_UPDATE_PROGRESS, (WPARAM)m_nCurFrame + 1);
	}
	int iHeight = m_Bih.biHeight;
	int iWidth = m_Bih.biWidth;
	//// The following code demonstrates how to save a snapshot to BMP file every 10 frames
	if(0 == m_nCurFrame % (1*frameRate))
	{
		int *grayscale = new int[iHeight * iWidth];
		int nLineBytes = (m_Bih.biWidth * 24 + 31) / 32 * 4;	// # of bytes per line
		for(int i=0; i<iHeight; i++)
		for(int j=0; j<iWidth; j++)
		{
			BYTE *pLine = pBuffer + (m_Bih.biHeight - i - 1) * nLineBytes;
			BYTE *pPixel = pLine + 3 * j;
			BYTE B = *pPixel;
			BYTE G = *(pPixel + 1);
			BYTE R = *(pPixel + 2);
			grayscale[i*iWidth+j] = (int)((B + G + R)/3);
		}
		int threshold = 127;
		int newThreshold = 0;
		int thresholdDiff = 10;
		while (thresholdDiff>1)
		{
			int darkerMean = 0;
			int darkerPxs = 0;
			int lighterMean = 0;
			int lighterPxs = 0;
			for(int i=0; i<iHeight; i++)
			for(int j=0; j<iWidth; j++)
			{
				if (grayscale[i*iWidth+j]<threshold){
					darkerPxs ++;
					darkerMean += grayscale[i*iWidth+j];
				}else{
					lighterPxs ++;
					lighterMean += grayscale[i*iWidth+j];
				}
			}
			if (darkerPxs > 0) darkerMean = darkerMean / darkerPxs;
			if (lighterPxs > 0) lighterMean = lighterMean / lighterPxs;
			newThreshold = (darkerMean + lighterMean)/2;
			if (newThreshold > threshold){
				thresholdDiff = newThreshold - threshold;
			}else{
				thresholdDiff = threshold - newThreshold;
			}
			threshold = newThreshold;
		}
		bool *binary = new bool[iHeight * iWidth];
		for(int i=0; i<iHeight; i++)
		for(int j=0; j<iWidth; j++)
		{
			if (grayscale[i*iWidth+j] > threshold){
				binary[i*iWidth+j] = true;
			}else{
				binary[i*iWidth+j] = false;
			}
		}
		for(int i=0; i<iHeight; i++)
		for(int j=0; j<iWidth; j++)
		{
			fingerOutline[i*iWidth+j] = true;
		}
		int sizeB = 3;
		for(int i=sizeB; i<(iHeight-sizeB); i++)
		for(int j=sizeB; j<(iWidth-sizeB); j++)
		{
			bool cur = true;
			for (int n= -sizeB; n <= sizeB; n++)
			for (int m= -sizeB; m <= sizeB; m++)
			{
				fingerOutline[i*iWidth+j] = (fingerOutline[i*iWidth+j] && (binary[(i+n)*iWidth+j+m]));
			}
		}
		BYTE *erosBuffer = new BYTE[nBufferLen];
		for(int i=0; i<iHeight; i++)
		for(int j=0; j<iWidth; j++)
		{
			BYTE *pLine = erosBuffer + (m_Bih.biHeight - i - 1) * nLineBytes;
			BYTE *pPixel = pLine + 3 * j;
			int color = 0;
			if (fingerOutline[i*iWidth+j]) color = 255;
			*(pPixel) = color;
			*(pPixel + 1) = color;
			*(pPixel + 2) = color;
		}
		//CString strFilename;
		//strFilename.Format("C:\\Users\\Public\\Videos\\Sample Videos\\Snap%d.bmp", m_nCurFrame / frameRate);
		//FILE *pfSnap = fopen(strFilename, "wb");
		//fwrite(&m_Bfh, sizeof(m_Bfh), 1, pfSnap);	// BITMAPFILEHEADER
		//fwrite(&m_Bih, sizeof(m_Bih), 1, pfSnap);	// BITMAPINFOHEADER
		//fwrite(erosBuffer, nBufferLen, 1, pfSnap);	// DIBits
		//fclose(pfSnap);
		delete grayscale, binary, erosBuffer;
	}

	//// The following code demonstrates how to get rgb values of a specified pixel
	//// You can write a loop to examine all pixels
	//// Keep in mind the pixel data is stored from bottom to top in pBuffer
	double sum = 0;
	long int total = 0;
	float hue = 0;
	float intensity = 0;
	float saturation = 0;
	float pi = 3.141592653;
	for (int y=0;y<iHeight;y++)
	for (int x=0;x<iWidth;x++){
		int nLineBytes = (m_Bih.biWidth * 24 + 31) / 32 * 4;	// # of bytes per line
		BYTE *pLine = pBuffer + (m_Bih.biHeight - y - 1) * nLineBytes;
		BYTE *pPixel = pLine + 3 * x;
		int B = (int)*pPixel;
		int G = (int)*(pPixel + 1);
		int R = (int)*(pPixel + 2);
		int curIntensity = (B+G+R)/3;
		if (fingerOutline[y*iWidth + x]){
			sum+= R;
			float curHue  = 0;
			float num = sqrt((float)(R - G + R - B));
			float denom = sqrt((float)((R-G) * (R-G) + (R-B) * (G-B)));
			float theta = acos(num/denom);
			theta = theta * (180/pi);
			if (B > G){
				curHue = 360 - theta;
			}else{
				curHue = theta;
			}
			float min = (float)min(R,min(G,B));
			float curSat = 1 - (3.0/(float)(R+G+B) * min);
			curSat *= 360;
			if (curHue>0 && curHue<360){
				hue += curHue;
				intensity += curIntensity;
				saturation += curSat;
				total ++;
			}
		}
	}
	float averageHue = hue/(float)total;
	double conversion = (double)averageHue;
	hues.push_back(conversion);
	float averageIntensity = intensity/(float)total;
	conversion = (double)averageIntensity;
	intensities.push_back(conversion);
	float averageSat = saturation/(float)total;
	conversion = (double)averageSat;
	saturations.push_back(conversion);
	m_nCurFrame++;	// m_nCurFrame indicates which frame is being processed
	return S_OK;
}

void CVideoAnaDoc::OnGraphNotify()
{
	if(!m_pEvent)
		return;

	long lEventCode;
	LONG_PTR lParam1;
	LONG_PTR lParam2;

	while(S_OK == m_pEvent->GetEvent(&lEventCode, &lParam1, &lParam2, 0))
	{
		TRACE("%d\n", lEventCode);
		//if(EC_PAUSED == lEventCode && GRAPH_RUNNING == m_GraphStatus)
		//{
		//}
		if(EC_COMPLETE == lEventCode)	// All data has been rendered.
		{
			if(S_FALSE == m_pControl->Pause())
			{
				OAFilterState oState;
				m_pControl->GetState(INFINITE, &oState);
			}
			m_pControl->Stop();
			//AfxMessageBox("Graph complete");
			AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_ANA_FINISHED);
		}

		m_pEvent->FreeEventParams(lEventCode, lParam1, lParam2);
	}
	
}