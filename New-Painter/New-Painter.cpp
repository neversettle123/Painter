// New-Painter.cpp : 定义应用程序的入口点。
//
#include "stdafx.h"
#include "New-Painter.h"
#include"Resource.h"
#include"DIBFile.h"
#include<vector>
#include<commdlg.h>
#include<json.h>
#include<string>
#include<fstream>

using namespace std;

struct Paint
{
	UINT fOption;
	COLORREF cr;
	UINT iWidth;
	union PaintStyle
	{
		struct Figure
		{
			POINT pBeg, pEnd;
			BOOL LvsR, TvsB;
		}Fe;
		struct Text
		{
			CHAR szText[1024];
			POINT pBeg, pEnd;
		}Tt;
	}Pi;
};


vector<Paint>Pt;
Paint abc;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ModelessDialog(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ChildProc1(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ChildProc2(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ChildProc3(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutDlg(HWND, UINT, WPARAM, LPARAM);
BOOL ChangeCursor(HWND, LPCWSTR);

void PaintOnMem(HWND, HDC);
BOOL ColorEditDlg(HWND);
void StaticText(HWND);
void PaintFigure(HWND, HDC);
void PaintRect(HWND, int);
void DoCaption(HWND, TCHAR *, TCHAR *);
void GetLargestDisPlayMode(int *, int *);
void DrawClientText(HWND, HDC, int, int);
void RePaintMem(HDC, Paint);
HBITMAP CopyDCToBitmap(HDC, LPRECT);
BOOL SaveBitmapToFile(HBITMAP, LPWSTR);
void SaveJsonFile(vector<Paint>);
void ReadJsonFile(Paint);


#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#define ID_TOOLBAR 200 
#define ID_CLIENT 201
#define ID_EDIT 202
#define ID_DRAW 203
#define ID_PAINT 204


HINSTANCE hInst;
static HWND hWndProc;
static HWND hEdit;
static HWND hModelessDlg;
static HWND hClient1, hClient2, hClient3;



//Pencil draw 
static HDC hdcMem;
static int cxBitmap, cyBitmap;



static POINT ptDraw;
static int cxDib, cyDib;



SCROLLINFO si;
static int cxChar, cyChar;



TCHAR szAppName[] = TEXT("绘图");
static BOOL bEditHide;
static BOOL bDrawHide;



vector<int>PtNum;
int iNumber = 0;
int b = 0;
static BOOL bClear;
static int i = 0, a = 0;
int iVectorSize;


static int ClientWidth = 1200,
ClientHeigth = 800;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	               LPSTR szCmdLine, int iCmdShow)
{
	MSG msg;	
	HACCEL hAccel;
	WNDCLASS wndclass;	

	hInst = hInstance;

	wndclass.style = NULL;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_NEWPAINTER));
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = NULL;
	wndclass.lpszClassName = szAppName;
	wndclass.lpszMenuName = MAKEINTRESOURCE(IDC_NEWPAINTER);

	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("This program requires Windows NT!"),
			szAppName, MB_ICONERROR);
		return 0;
	}
	
	hWndProc = CreateWindow(szAppName, TEXT("画图"),
		WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,  //WS_CLIPCHILDREN -- 这个样式很重要
		300, 200, 
		1080, 750,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hWndProc, iCmdShow);
	UpdateWindow(hWndProc);
	
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NEWPAINTER));

	while (GetMessage(&msg, NULL, 0, 0))
	{		
		if (hModelessDlg == 0 || !IsDialogMessage(hModelessDlg, &msg))
		{
			if (!TranslateAccelerator(hClient3, hAccel, &msg)
				|| !TranslateAccelerator(hWndProc, hAccel, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	return (int) msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{	
	HDC hdc;
	static HFONT hfont;
	PAINTSTRUCT ps;
	
	//注册子窗口类
	WNDCLASS wndclass;
	HINSTANCE hInstance;
	static int cxClient, cyClient;
	
	//读入bmp
	BOOL bSuccess;
	static BYTE * pBits;
	static BITMAPINFO *pbmi;	
	static BITMAPFILEHEADER * pbmfh;
	static TCHAR szFileName[MAX_PATH],
		szTitleName[MAX_PATH];	


	HBITMAP hBitmap;
	RECT RectClient;

	switch (message)
	{
	case WM_CREATE:
		hModelessDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), //对话框
			hWnd, ModelessDialog);
		
		//客户区窗口
		hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = ChildProc1;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInstance;
		wndclass.hIcon = NULL;
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(185, 211, 238));;
		wndclass.lpszClassName = TEXT("Child1");
		wndclass.lpszMenuName = NULL;

		RegisterClass(&wndclass);
		hClient1 = CreateWindow(TEXT("Child1"), NULL,
			WS_CHILDWINDOW | WS_BORDER | WS_VISIBLE |WS_CLIPCHILDREN|
			WS_VSCROLL | WS_HSCROLL,
			0, 0, 0, 0, hWnd, (HMENU)ID_PAINT, hInstance, NULL);

		//绘图区窗口
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = ChildProc2;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInstance;
		wndclass.hIcon = NULL;
		wndclass.hCursor = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR1));
		wndclass.hbrBackground = NULL;
		wndclass.lpszClassName = TEXT("Child2");
		wndclass.lpszMenuName = NULL;

		RegisterClass(&wndclass);
		hClient2 = CreateWindow(TEXT("Child2"), NULL, 
			WS_CHILDWINDOW | WS_BORDER | WS_VISIBLE,
			0, 0, 0, 0, hClient1, (HMENU)ID_CLIENT, hInstance, NULL);


		//图形小窗口
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = ChildProc3;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInstance;
		wndclass.hIcon = NULL;
		wndclass.hCursor = LoadCursor(NULL, IDC_SIZEALL);
		wndclass.hbrBackground = NULL;
		wndclass.lpszClassName = TEXT("Child3");		
		wndclass.lpszMenuName = NULL;

		RegisterClass(&wndclass);
		hClient3 = CreateWindow(TEXT("Child3"), NULL,
			WS_CHILDWINDOW|WS_BORDER | WS_VISIBLE,
			0, 0, 0, 0, hClient2, (HMENU)ID_DRAW, hInstance, NULL);

		//编辑子窗口
		hEdit = CreateWindow(TEXT("Edit"), NULL,                
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT
			| ES_MULTILINE | ES_NOHIDESEL,
			0, 0, 0, 0, hClient2, (HMENU)ID_EDIT, hInstance, NULL);

		hdc = GetDC(hEdit);
		hfont = CreateFont(16, 7, 0, 0, 0, 0, 0, 0,
			DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, NULL);
		SelectObject(hdc, hfont);
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hfont, 0);

		ReleaseDC(hEdit, hdc);

		SendMessage(hEdit, EM_LIMITTEXT, 3200, 0L);
		DoCaption(hWnd, szTitleName, szAppName);
		StaticText(hModelessDlg);
		break;

	case WM_SIZE:
		cxClient = GET_X_LPARAM(lParam);
		cyClient = GET_Y_LPARAM(lParam); 
				
		MoveWindow(hModelessDlg, 0, 0, cxClient, 100, TRUE);
		MoveWindow(hClient1, 0, 100, cxClient, cyClient - 100, TRUE);
		MoveWindow(hClient2, 2, 2, ClientWidth, ClientHeigth, TRUE);
		break;	

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_NEW:
			Pt.clear();
			memset(szFileName, '\0', MAX_PATH);
			memset(szTitleName, '\0', MAX_PATH);

			if (pbmfh)
			{				
				free(pbmfh);
				pbmfh = NULL;
				pbmi = NULL;
				pBits = NULL;
			}
						
			PatBlt(hdcMem, 0, 0, cxBitmap, cyBitmap, WHITENESS);
			InvalidateRect(hClient2, NULL, TRUE);
			DoCaption(hWnd, szFileName, szTitleName);
			break;

		case IDM_OPEN:
			if (!DIBFileOpenDlg(hClient2, szFileName, szTitleName))
				return FALSE;

			if (pbmfh)
			{
				free(pbmfh);
				pbmfh = NULL;
			}

			SetCursor(LoadCursor(NULL, IDC_WAIT));
			ShowCursor(TRUE);
			pbmfh = LoadDIBFile(szFileName);
			ShowCursor(FALSE);
			SetCursor(LoadCursor(NULL, IDC_ARROW));			

			if (pbmfh == NULL)
			{
				MessageBox(hWnd, TEXT("Cannot load DIB files!"),
					szAppName, MB_OK | MB_ICONSTOP);
				return FALSE;
			}

			pbmi = (BITMAPINFO *)(pbmfh + 1);//结构指针右移，指向BITMAPINFO结构体
			pBits = (BYTE *)pbmfh + pbmfh->bfOffBits;   //pBits指向位图数据，bfOffBits保存了
			                                            //位图信息头到位图数据的距离

			if (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
			{
				cxDib = ((BITMAPCOREHEADER *)pbmi)->bcWidth;
				cyDib = ((BITMAPCOREHEADER *)pbmi)->bcHeight;
			}
			else
			{
				cxDib = pbmi->bmiHeader.biWidth;
				cyDib = abs(pbmi->bmiHeader.biHeight);
			}
							
			ClientWidth = cxDib;
			ClientHeigth = cyDib;
			MoveWindow(hClient2, 0, 0, ClientWidth, ClientHeigth, TRUE);

			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = cyDib + 24;
			
			SetScrollInfo(hClient1, SB_VERT, &si, TRUE);

			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = cxDib + 24;
			
			SetScrollInfo(hClient1, SB_HORZ, &si, TRUE);

			if (pbmfh)
			{
				Pt.clear();
				PatBlt(hdcMem, 0, 0, cxBitmap, cyBitmap, WHITENESS);
				SetDIBitsToDevice(hdcMem, 0, 0, cxDib, cyDib,
					0, 0, 0, cyDib, pBits, pbmi, DIB_RGB_COLORS);
			}
			InvalidateRect(hClient2, NULL, TRUE);
			DoCaption(hWnd, szTitleName, szAppName);
			break;

		case IDM_SAVE:			
			if (szTitleName[0] == '\0')
				DIBFileSaveDlg(hClient2, szFileName, szTitleName);

			GetClientRect(hClient2, &RectClient);
			SetCursor(LoadCursor(NULL, IDC_WAIT));
			ShowCursor(TRUE);
						
			hBitmap = CopyDCToBitmap(hdcMem, &RectClient);
			bSuccess = SaveBitmapToFile(hBitmap, szFileName);

			ShowCursor(FALSE);
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			if (!bSuccess)
				MessageBox(hWnd, TEXT("Cannot save DIB files"),
				szAppName, MB_OK | MB_ICONSTOP);
			break;

		case IDM_SAVE_AS:
			if (!DIBFileSaveDlg(hClient2, szFileName, szTitleName))
				return FALSE;

			SetCursor(LoadCursor(NULL, IDC_WAIT));
			ShowCursor(TRUE);

			bSuccess = SaveDIBFile(szFileName, pbmfh);
			ShowCursor(FALSE);
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			if (!bSuccess)
				MessageBox(hWnd, TEXT("Cannot save DIB files"),
				szAppName, MB_OK | MB_ICONSTOP);
			break;

		case IDM_UNDO:		
			bClear = TRUE;
			PatBlt(hdcMem, 0, 0, cxBitmap, cyBitmap, WHITENESS);
			if (Pt.size() -1- i > 0)
			{
				if (Pt[Pt.size() - 1-i].fOption == IDM_PENCIL)           //Pt.size() - 1为vector最后一个元素
				{
					i = i + PtNum[PtNum.size() - 1 - b];
					iVectorSize = Pt.size() - i;
					for (a = 0; a < iVectorSize; a++)
					{
						RePaintMem(hdcMem, Pt[a]);
						InvalidateRect(hClient2, NULL, TRUE);
					}
					b++;
				}
				else       //包括文本和图形撤销
				{
					i++;
					iVectorSize = Pt.size() - i;
					for (a = 0; a < iVectorSize; a++)
					{
						RePaintMem(hdcMem, Pt[a]);
						InvalidateRect(hClient2, NULL, TRUE);
					}
				}
			}
			else
			{
				RePaintMem(hdcMem, Pt[0]);
			}
			break;

		case IDM_RECOVER:
			ReadJsonFile(abc);
			bClear = TRUE;
			if (i > 0)
			{
				if (Pt[Pt.size() - 1 - i].fOption == IDM_PENCIL)           
				{
					if (b > 0)
					{
						a = Pt.size() - i;                    //重绘铅笔轨迹起点
						i = i - PtNum[PtNum.size() - b];  //除去此轨迹的索引值

						for (a; a < Pt.size() - 1 - i; a++)
						{
							RePaintMem(hdcMem, Pt[a]);
							InvalidateRect(hClient2, NULL, TRUE);
						}
						b--;
					}
				}
				else
				{
					i--;
					RePaintMem(hdcMem, Pt[Pt.size() - 1 - i]);
					InvalidateRect(hClient2, NULL, TRUE);
				}
			}
			break;

		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1),
				hWnd, AboutDlg);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		}
		break;

	case WM_DESTROY:
		free(pbmfh);
		if (hfont)
			DeleteObject(hfont);
		DeleteDC(hdcMem);
		PostQuitMessage(0);		
		SaveJsonFile(Pt);
		break;
	
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}


LRESULT CALLBACK ChildProc1(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT rect;
	PAINTSTRUCT ps;
	
	int iPaintXBeg, iPaintYBeg;
	
	ULONG ulScrollLines;
	static int iDeltaPerline, iAccumDelta;
	

	static int cxClient, cyClient;
	int  iVertPos, iHorzPos;

	int iWidth, iHeigth;

	switch (message)
	{
	case WM_CREATE:
	case WM_SETTINGCHANGE:       //不知如何响应此消息，因此和WM_CREATE一起响应                    
		SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0,
			&ulScrollLines, 0);

		if (ulScrollLines)
			iDeltaPerline = WHEEL_DELTA / ulScrollLines;
		else
			iDeltaPerline = 0;
		return 0;

	case WM_SIZE:
		cxClient = GET_X_LPARAM(lParam);
		cyClient = GET_Y_LPARAM(lParam);

		GetWindowRect(hWnd, &rect);
		iWidth = rect.right - rect.left;
		iHeigth = rect.bottom - rect.top;

		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE | SIF_PAGE;
		si.nMin = 0;
		si.nMax = ClientHeigth + 24;
		si.nPage = iHeigth;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE | SIF_PAGE;
		si.nMin = 0;
		si.nMax = ClientWidth + 24;
		si.nPage = iWidth;
		SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
		break;

	case WM_VSCROLL:
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		GetScrollInfo(hWnd, SB_VERT, &si);

		iVertPos = si.nPos;
		switch (LOWORD(wParam))
		{
		case SB_TOP:
			si.nPos = si.nMin;
			break;

		case SB_BOTTOM:
			si.nPos = si.nMax;
			break;

		case SB_LINEUP:
			si.nPos -= 1;
			break;

		case SB_LINEDOWN:
			si.nPos += 1;
			break;

		case SB_PAGEUP:
			si.nPos -= si.nPage;
			break;

		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			break;

		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;

		default:
			break;
		}

		si.fMask = SIF_POS;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
		GetScrollInfo(hWnd, SB_VERT, &si);

		if (si.nPos != iVertPos)
		{
			ScrollWindow(hWnd, 0, iVertPos - si.nPos, NULL, NULL);
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;

	case WM_HSCROLL:
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		GetScrollInfo(hWnd, SB_HORZ, &si);

		iHorzPos = si.nPos;
		switch (LOWORD(wParam))
		{
		case SB_LINELEFT:
			si.nPos -= 1;
			break;

		case SB_LINERIGHT:
			si.nPos += 1;
			break;

		case SB_PAGELEFT:
			si.nPos -= si.nPage;
			break;

		case SB_PAGERIGHT:
			si.nPos += si.nPage;
			break;

		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;

		default:
			break;
		}

		si.fMask = SIF_POS;
		SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
		GetScrollInfo(hWnd, SB_HORZ, &si);

		if (si.nPos != iHorzPos)
		{
			ScrollWindow(hWnd, iHorzPos - si.nPos, 0, NULL, NULL);
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_HOME:
			SendMessage(hWnd, WM_VSCROLL, SB_TOP, 0);
			break;

		case VK_END:
			SendMessage(hWnd, WM_VSCROLL, SB_BOTTOM, 0);
			break;

		case VK_PRIOR:
			SendMessage(hWnd, WM_VSCROLL, SB_PAGEUP, 0);
			break;

		case VK_NEXT:
			SendMessage(hWnd, WM_VSCROLL, SB_PAGEDOWN, 0);
			break;

		case VK_UP:
			SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
			break;

		case VK_DOWN:
			SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
			break;

		case VK_LEFT:
			SendMessage(hWnd, WM_HSCROLL, SB_PAGEUP, 0);
			break;

		case VK_RIGHT:
			SendMessage(hWnd, WM_HSCROLL, SB_PAGEDOWN, 0);
			break;
		}
		return 0;

	case WM_MOUSEWHEEL:
		if (iDeltaPerline == 0)
			break;

		iAccumDelta += (short)HIWORD(wParam);      //wParam的高位字表示新的消息，
		//这是一个增量数值
		while (iAccumDelta >= iDeltaPerline)
		{
			SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
			iAccumDelta -= iDeltaPerline;
		}

		while (iAccumDelta <= -iDeltaPerline)
		{
			SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
			iAccumDelta += iDeltaPerline;
		}
		break;

	case WM_DESTROY:		
		DestroyWindow(hWnd);
		break;

	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}


LRESULT CALLBACK ChildProc2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//Pencil draw 	
	HDC hdc;
	static HPEN hpen;
	PAINTSTRUCT ps;
	TEXTMETRIC tm;
	HBITMAP hBitmap2;
	
	static int xMouse, yMouse;
	static BOOL fLeftButtonDown, fRightButtonDown;
	
	//Edit Child Window		
	RECT rect, rcDraw, rcRight;
	static HFONT hfont;

	static int iSideWidth, iSideHeigth;	
	static int cxBuffer, cyBuffer,
		cxClient, cyClient;
	
	static HBRUSH hBrushEdit;
	
	POINT point;
	RECT rect3;

	switch (message)
	{
	case WM_CREATE:
		hdc = GetDC(hWnd);
		DIBFileInitialize(hWnd);
		GetLargestDisPlayMode(&cxBitmap, &cyBitmap);

		hBitmap2 = CreateCompatibleBitmap(hdc, cxBitmap, cyBitmap);
		hdcMem = CreateCompatibleDC(hdc);
		ReleaseDC(hWnd, hdc);

		if (!hBitmap2)
		{
			DeleteObject(hdcMem);
			return -1;
		}

		SelectObject(hdcMem, hBitmap2);
		PatBlt(hdcMem, 0, 0, cxBitmap, cyBitmap, WHITENESS);

		abc.fOption = IDM_PENCIL;
		abc.iWidth = 1;
		abc.cr = 0;
		abc.Pi.Fe.pBeg.x = abc.Pi.Fe.pEnd.x = 0;
		abc.Pi.Fe.pBeg.y = abc.Pi.Fe.pEnd.y = 0;
		Pt.push_back(abc);
		break;

	case WM_SIZE:
		cxClient = GET_X_LPARAM(lParam);
		cyClient = GET_Y_LPARAM(lParam);

		hdc = GetDC(hEdit);
		GetTextMetrics(hdc, &tm);
		cxChar = tm.tmAveCharWidth;
		cyChar = tm.tmHeight;
		ReleaseDC(hEdit, hdc);
		break;

	case WM_SETFOCUS:  //此子窗口得到焦点时的消息
		if ((HWND)wParam==hEdit)    //失去焦点的窗口
		{
			if (abc.fOption == IDM_TEXT)
			{
				ShowWindow(hEdit, SW_HIDE);
				DrawClientText(hEdit, hdcMem,        //文字轨迹
					cxChar, cyChar);

				Pt.push_back(abc);
				InvalidateRect(hWnd, NULL, TRUE);
			}
		}		
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == ID_EDIT)
		{
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				InvalidateRect(hWnd, NULL, TRUE);	
				if (abc.fOption == IDM_TEXT)
					SetCursor(LoadCursor(NULL, IDC_IBEAM));
				break;

			case EN_ERRSPACE:
			case EN_MAXTEXT:
				if (GetWindowTextLength(hEdit) <= 1024)
				{
					iSideHeigth = iSideHeigth + cyChar;
					MoveWindow(hEdit, abc.Pi.Tt.pBeg.x, abc.Pi.Tt.pBeg.y,
						iSideWidth, iSideHeigth, TRUE);
					ShowWindow(hEdit, SW_NORMAL);
					InvalidateRect(hEdit, NULL, TRUE);
				}
				else
					MessageBox(hEdit, TEXT("Edit control out of range!"),
					szAppName, MB_OK | MB_ICONSTOP);	
				return 0;
			}
		}
		break;
			case WM_SETCURSOR:
		if (abc.fOption == IDM_TEXT)		
			ChangeCursor(hWnd, IDC_IBEAM);
		else if (abc.fOption == IDM_PENCIL)
			ChangeCursor(hWnd, MAKEINTRESOURCE(IDC_CURSOR1));
		else			
			ChangeCursor(hWnd, MAKEINTRESOURCE(IDC_CURSOR2));	
		break;

	case WM_LBUTTONDOWN:
		if (!fLeftButtonDown)
			SetCapture(hWnd);		
		SetFocus(hWnd);

		if (abc.fOption == IDM_TEXT)
			SetCursor(LoadCursor(NULL, IDC_IBEAM));

		if (bClear == TRUE)
		{
			Pt.resize(Pt.size() - i);
			PtNum.resize(PtNum.size() - b);
			i = 0;
			b = 0;
			bClear = FALSE;
		}
		
		if (abc.fOption!=IDM_TEXT)
		{
			if (abc.fOption == IDM_PENCIL)
			{
				xMouse = GET_X_LPARAM(lParam);
				yMouse = GET_Y_LPARAM(lParam);								
			}
			else
			{
				if (bEditHide == FALSE)
				{
					abc.Pi.Fe.pBeg.x = GET_X_LPARAM(lParam);
					abc.Pi.Fe.pBeg.y = GET_Y_LPARAM(lParam);
				}
			}
		}
		else
		{
			if (bEditHide == FALSE)
			{
				abc.Pi.Tt.pBeg.x = GET_X_LPARAM(lParam);
				abc.Pi.Tt.pBeg.y = GET_Y_LPARAM(lParam);
			}
		}		
		fLeftButtonDown = TRUE;
		break;

	case WM_LBUTTONUP:   
		if (fLeftButtonDown)
			ReleaseCapture();

		if (abc.fOption == IDM_TEXT)
			SetCursor(LoadCursor(NULL, IDC_IBEAM));

		if (abc.fOption != IDM_TEXT)
		{
			abc.Pi.Fe.pEnd.x = GET_X_LPARAM(lParam);
			abc.Pi.Fe.pEnd.y = GET_Y_LPARAM(lParam);

			if (abc.fOption!=IDM_PENCIL)
			{
				if (abc.Pi.Fe.pBeg.x != abc.Pi.Fe.pEnd.x&&
					abc.Pi.Fe.pBeg.y != abc.Pi.Fe.pEnd.y)
				{
					if (abc.fOption == IDM_LINE)
					{
						abc.Pi.Fe.LvsR = (abc.Pi.Fe.pBeg.x < abc.Pi.Fe.pEnd.x ? 
						TRUE : FALSE);
						abc.Pi.Fe.TvsB = (abc.Pi.Fe.pBeg.y < abc.Pi.Fe.pEnd.y ?
						TRUE : FALSE);
					}

					if (bDrawHide == FALSE)
					{
						rcDraw.left = min(abc.Pi.Fe.pBeg.x, abc.Pi.Fe.pEnd.x);
						rcDraw.right = max(abc.Pi.Fe.pBeg.x, abc.Pi.Fe.pEnd.x);
						rcDraw.top = min(abc.Pi.Fe.pBeg.y, abc.Pi.Fe.pEnd.y);
						rcDraw.bottom = max(abc.Pi.Fe.pBeg.y, abc.Pi.Fe.pEnd.y);

						MoveWindow(hClient3, rcDraw.left, rcDraw.top,
							rcDraw.right - rcDraw.left, 
							rcDraw.bottom - rcDraw.top, TRUE);

						ShowWindow(hClient3, SW_NORMAL);
						SetFocus(hClient3);						
						bDrawHide = TRUE;
					}
				}				
			}
			else
			{
				PtNum.push_back(iNumber);
				iNumber = 0;
			}
		}
		else
		{
			if (bEditHide == FALSE)
			{
				abc.Pi.Tt.pEnd.x = GET_X_LPARAM(lParam);
				abc.Pi.Tt.pEnd.y = GET_Y_LPARAM(lParam);

				iSideWidth = max(abc.Pi.Tt.pEnd.x - abc.Pi.Tt.pBeg.x, 16 * (cxChar));
				iSideHeigth = max(abc.Pi.Tt.pEnd.y - abc.Pi.Tt.pBeg.y, 2 * (cyChar+2));

				MoveWindow(hEdit, abc.Pi.Tt.pBeg.x, abc.Pi.Tt.pBeg.y,
					iSideWidth, iSideHeigth, TRUE);

				ShowWindow(hEdit, SW_NORMAL);
				UpdateWindow(hEdit);
				SetFocus(hEdit);
				bEditHide = TRUE;
			}
			else
			{
				bEditHide = FALSE;
			}
		}
		
		fLeftButtonDown = FALSE;
		break;
				
	case WM_MOUSEMOVE:               //捕获鼠标(SetCapture(hWnd))之后,移动才会有此消息
		if (!fLeftButtonDown&&!fRightButtonDown)
			return FALSE;
		
		if (abc.fOption == IDM_PENCIL)
		{
			hdc = GetDC(hWnd);
			hpen = CreatePen(PS_SOLID, abc.iWidth, abc.cr);
			SelectObject(hdc,
				fLeftButtonDown ? hpen : GetStockObject(WHITE_PEN));
			SelectObject(hdcMem,
				fLeftButtonDown ? hpen : GetStockObject(WHITE_PEN));

			abc.Pi.Fe.pBeg.x = xMouse;
			abc.Pi.Fe.pBeg.y = yMouse;

			MoveToEx(hdcMem, xMouse, yMouse, NULL);
			MoveToEx(hdc, xMouse, yMouse, NULL);

			xMouse = GET_X_LPARAM(lParam);
			yMouse = GET_Y_LPARAM(lParam);

			abc.Pi.Fe.pEnd.x = xMouse;
			abc.Pi.Fe.pEnd.y = yMouse;
			Pt.push_back(abc);

			iNumber++;

			LineTo(hdc, xMouse, yMouse);
			LineTo(hdcMem, xMouse, yMouse);
			ReleaseDC(hWnd, hdc);
		}
		
		break;

	case WM_CTLCOLOREDIT:
		if (abc.fOption == IDM_TEXT)
		{
			hBrushEdit = (HBRUSH)GetStockObject(NULL_BRUSH);
			SetBkMode((HDC)wParam, TRANSPARENT);

			if (abc.fOption == IDM_TEXT)
				SetTextColor((HDC)wParam, abc.cr);
			else
				SetTextColor((HDC)wParam, RGB(0, 0, 0));

			return (LRESULT)hBrushEdit;
		}
		break;

	case WM_RBUTTONDOWN:
		if (!fRightButtonDown)
			SetCapture(hWnd);

		xMouse = GET_X_LPARAM(lParam);
		yMouse = GET_Y_LPARAM(lParam);
		fRightButtonDown = TRUE;
		break;

	case WM_RBUTTONUP:
		if (fRightButtonDown)
			ReleaseCapture();

		fRightButtonDown = FALSE;
		break;

	case WM_NCHITTEST:
		point.x = GET_X_LPARAM(lParam);
		point.y = GET_Y_LPARAM(lParam);

		ScreenToClient(hWnd, &point);
		GetClientRect(hWnd, &rect3);

		ClientWidth = rect3.right - rect3.left;
		ClientHeigth = rect3.bottom - rect3.top;

		SendMessage(hClient1, WM_SIZE, 0, 0);

		if (point.x < rect3.left + 5 && point.y < rect3.top + 5)
		{
			return NULL;
		}
		else if (point.x>rect3.right - 5 && point.y < rect3.top + 5)
		{
			return NULL;
		}
		else if (point.x<rect3.left + 5 && point.y>rect3.bottom - 5)
		{
			return NULL;
		}
		else if (point.x>rect3.right - 5 && point.y > rect3.bottom - 5)
		{
			return HTBOTTOMRIGHT;
		}
		else if (point.x < rect3.left + 5)
		{
			return NULL;
		}
		else if (point.x>rect3.right - 5)
		{
			return HTRIGHT;
		}
		else if (point.y < rect3.top + 5)
		{
			return NULL;
		}
		else if (point.y>rect3.bottom - 5)
		{
			return HTBOTTOM;
		}
		else
		{
			return HTCLIENT;
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		
		BitBlt(hdc, 0, 0, cxBitmap, cyBitmap,
			hdcMem, 0, 0, SRCCOPY);
		
		EndPaint(hWnd, &ps);
		break;		

	case WM_DESTROY:
		if (hfont)
			DeleteObject(hfont);
		if (hpen)
			DeleteObject(hpen);
		DeleteObject(hBitmap2);
		DeleteObject(hBrushEdit);
		break;

	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK ChildProc3(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	HMENU hMenu, hPop;
	RECT rcRight, rcWnd;
	POINT ptRight, ptWnd;
	int xPos, yPos;

	switch (message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_RTDELETE:
			bDrawHide = FALSE;    //此两者顺序很关键，窗口隐藏会有WM_KILLFOCUS消息，
			                                  //所以先改索引
			ShowWindow(hWnd, SW_HIDE);    //此两者顺序很关键			
			break;

		case IDM_SOILDLINE:

			break;

		case IDM_DOTLINE:

			break;
		}
		break;

	case WM_SETFOCUS:   //此子窗口得到焦点时发送此消息
		
		break;

	case WM_KILLFOCUS:  
		if ((HWND)wParam == hClient2)     
		{
			ShowWindow(hWnd, SW_HIDE);

			if (bDrawHide == TRUE)
			{				
				PaintOnMem(hWnd, hdcMem);   //图形轨迹		
				Pt.push_back(abc);				
				bDrawHide = FALSE;
			}
			InvalidateRect(hClient2, NULL, TRUE);			
		}
		break;
		
	case WM_NCHITTEST:              //拖拽窗口边框实现改变窗口尺寸
		ptWnd.x = GET_X_LPARAM(lParam);
		ptWnd.y = GET_Y_LPARAM(lParam);

		ScreenToClient(hWnd, &ptWnd);
		GetClientRect(hWnd, &rcWnd);		

		if (ptWnd.x < rcWnd.left + 5 && ptWnd.y < rcWnd.top + 5)
		{
			return HTTOPLEFT;
		}
		else if (ptWnd.x>rcWnd.right - 5 && ptWnd.y < rcWnd.top + 5)
		{
			return HTTOPRIGHT;
		}
		else if (ptWnd.x<rcWnd.left + 5 && ptWnd.y>rcWnd.bottom - 5)
		{
			return HTBOTTOMLEFT;
		}
		else if (ptWnd.x>rcWnd.right - 5 && ptWnd.y > rcWnd.bottom - 5)
		{
			return HTBOTTOMRIGHT;
		}
		else if (ptWnd.x < rcWnd.left + 5)
		{
			return HTLEFT;
		}
		else if (ptWnd.x>rcWnd.right - 5)
		{
			return HTRIGHT;
		}
		else if (ptWnd.y < rcWnd.top + 5)
		{
			return HTTOP;
		}
		else if (ptWnd.y>rcWnd.bottom - 5)
		{
			return HTBOTTOM;
		}
		else
		{			
			return HTCLIENT;			
		}
		
		UpdateWindow(hClient2);
		break;

	case WM_LBUTTONDOWN:
		SetFocus(hWnd);
		SendMessage(hWnd, WM_SYSCOMMAND, 0xF012, 0);   //实现拖动窗口
		break;

	case WM_SIZE:
		GetClientRect(hWnd, &rcWnd);
		InvalidateRect(hWnd, &rcWnd, TRUE);
		InvalidateRect(hClient2, NULL, TRUE);				
		break;

	case WM_MOVING:
		InvalidateRect(hClient2, NULL, TRUE);		
		break;

	case WM_MOVE:
		InvalidateRect(hClient2, NULL, TRUE);
		ptDraw.x = LOWORD(lParam);
		ptDraw.y = HIWORD(lParam);
		break;

	case WM_CONTEXTMENU:
		if (GetFocus() == hClient2)
		{
			ptRight.x = GET_X_LPARAM(lParam);
			ptRight.y = GET_Y_LPARAM(lParam);

			GetClientRect((HWND)wParam, &rcRight);
			ScreenToClient((HWND)wParam, &ptRight);

			if (PtInRect(&rcRight, ptRight))
			{
				hMenu = LoadMenu((HINSTANCE)GetWindowLongPtr(hClient2, GWLP_HINSTANCE),
					MAKEINTRESOURCE(IDR_MENU1));
				if (hMenu)
				{
					hPop = GetSubMenu(hMenu, 0);
					ClientToScreen((HWND)wParam, &ptRight);
					TrackPopupMenu(hPop, TPM_LEFTALIGN | TPM_TOPALIGN
						| TPM_RIGHTBUTTON, ptRight.x, ptRight.y, 0,
						(HWND)wParam, NULL);
					DestroyMenu(hPop);
				}
			}
		}
		break;

	case WM_PAINT:	
		hdc = BeginPaint(hWnd, &ps);
		PaintFigure(hWnd, hdc);
		EndPaint(hWnd, &ps);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}



BOOL CALLBACK ModelessDialog(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hCtrlBlock[20], hStatic;
	static HWND hCombo;
	static HWND hPencil, hText, hEditColor, hBrush;
	static HWND hLine, hRect, hEllipse, hRoundRect;
	static int iColor = 0;
	HDC hdc;
	HICON hicon, hicon2, hicon3, hicon4,
		hicon5, hicon6, hicon7, hicon8;
	static int iSel1, iSel2;

	switch (message)
	{
	case WM_INITDIALOG:
		hCtrlBlock[0] = GetDlgItem(hWnd, IDM_PAINT);
		hCtrlBlock[1] = GetDlgItem(hWnd, IDM_PAINT2);
		hCtrlBlock[2] = GetDlgItem(hWnd, IDM_PAINT3);
		hCtrlBlock[3] = GetDlgItem(hWnd, IDM_PAINT4);
		hCtrlBlock[4] = GetDlgItem(hWnd, IDM_PAINT5);
		hCtrlBlock[5] = GetDlgItem(hWnd, IDM_PAINT6);
		hCtrlBlock[6] = GetDlgItem(hWnd, IDM_PAINT7);
		hCtrlBlock[7] = GetDlgItem(hWnd, IDM_PAINT8);
		hCtrlBlock[8] = GetDlgItem(hWnd, IDM_PAINT10);
		hCtrlBlock[9] = GetDlgItem(hWnd, IDM_PAINT11);
		hCtrlBlock[10] = GetDlgItem(hWnd, IDM_PAINT12);
		hCtrlBlock[11] = GetDlgItem(hWnd, IDM_PAINT13);
		hCtrlBlock[12] = GetDlgItem(hWnd, IDM_PAINT14);
		hCtrlBlock[13] = GetDlgItem(hWnd, IDM_PAINT15);
		hCtrlBlock[14] = GetDlgItem(hWnd, IDM_PAINT16);
		hCtrlBlock[15] = GetDlgItem(hWnd, IDM_PAINT19);
		hCtrlBlock[16] = GetDlgItem(hWnd, IDM_PAINT20);
		hCtrlBlock[17] = GetDlgItem(hWnd, IDM_PAINT21);
		hCtrlBlock[18] = GetDlgItem(hWnd, IDM_PAINT22);
		hCtrlBlock[19] = GetDlgItem(hWnd, IDM_PAINT23);
		hStatic = GetDlgItem(hWnd, IDM_PAINT9);

		hPencil = GetDlgItem(hWnd, IDM_PENCIL);
		hText = GetDlgItem(hWnd, IDM_TEXT);
		hEditColor = GetDlgItem(hWnd, IDM_EDITCOLOR);
		hBrush = GetDlgItem(hWnd, IDM_BRUSH);
		hLine = GetDlgItem(hWnd, IDM_LINE);
		hRect = GetDlgItem(hWnd, IDM_RECT);
		hEllipse = GetDlgItem(hWnd, IDM_ELIPSE);
		hRoundRect = GetDlgItem(hWnd, IDM_ROUNDRECT);

		hCombo = GetDlgItem(hWnd, IDM_COMBO1);

		SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(TEXT("1")));
		SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(TEXT("3")));
		SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(TEXT("5")));
		SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(TEXT("8")));

		SendMessage(hCombo, CB_SETCURSEL, 0, 0);
		
		MoveWindow(hPencil, 30, 15, 24, 24, TRUE);
		MoveWindow(hText, 55, 15, 24, 24, TRUE);
		MoveWindow(hEditColor, 585, 5, 36, 36, TRUE);
		MoveWindow(hBrush, 30, 45, 32, 32, TRUE);

		MoveWindow(hLine, 120, 15, 24, 24, TRUE);
		MoveWindow(hRect, 145, 15, 24, 24, TRUE);
		MoveWindow(hEllipse, 170, 15, 24, 24, TRUE);
		MoveWindow(hRoundRect, 195, 15, 24, 24, TRUE);

		for (int i = 0; i < 20; i++)
		{
			PaintRect(hCtrlBlock[i], i);
		}
		PaintRect(hStatic, iColor);
		return TRUE;
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDM_COMBO1)
		{
			switch (HIWORD(wParam))
			{
			case CBN_CLOSEUP:                   //组合框列表关闭时产生的消息
				if (bDrawHide==TRUE)
					SetFocus(hClient3);
				break;
			}
		}

		switch (LOWORD(wParam))
		{
		case IDM_COMBO1:					
			iSel1 = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
			if (iSel1 == 0)abc.iWidth = 1;
			else if (iSel1 == 1)abc.iWidth = 3;
			else if (iSel1 == 2)abc.iWidth = 5;
			else if (iSel1 == 3)abc.iWidth = 8;
			else if (iSel1 == CB_ERR)
				MessageBox(hWnd, TEXT("无标题"), TEXT("画图"),
				MB_OK | MB_ICONERROR);	
			
			break;

		case IDM_PENCIL:
			abc.fOption = IDM_PENCIL;
			SendMessage(hPencil, BM_SETSTATE, 1, 0);
			SendMessage(hText, BM_SETSTATE, 0, 0);			
			break;

		case IDM_LINE:
			abc.fOption = IDM_LINE;
			break;

		case IDM_RECT:
			abc.fOption = IDM_RECT;
			break;

		case IDM_ELIPSE:
			abc.fOption = IDM_ELIPSE;
			break;

		case IDM_ROUNDRECT:
			abc.fOption = IDM_ROUNDRECT;
			break;

		case IDM_PAINT:
		case IDM_PAINT2:
		case IDM_PAINT3:
		case IDM_PAINT4:
		case IDM_PAINT5:
		case IDM_PAINT6:
		case IDM_PAINT7:
		case IDM_PAINT8:
		case IDM_PAINT10:
		case IDM_PAINT11:
		case IDM_PAINT12:
		case IDM_PAINT13:
		case IDM_PAINT14:
		case IDM_PAINT15:
		case IDM_PAINT16:
		case IDM_PAINT19:
		case IDM_PAINT20:
		case IDM_PAINT21:
		case IDM_PAINT22:
		case IDM_PAINT23:
			iColor = LOWORD(wParam);

			if (abc.fOption != IDM_TEXT&&abc.fOption != IDM_PENCIL)
				SetFocus(hClient3);
			break;

		case IDM_EDITCOLOR:
			if (!ColorEditDlg(hWnd))
				MessageBox(hWnd, TEXT("Choose color fail!"),
				TEXT("Painter"), MB_OK | MB_ICONSTOP);
			iColor = LOWORD(wParam);

			if (abc.fOption != IDM_TEXT&&abc.fOption != IDM_PENCIL)
				SetFocus(hClient3);
			break;

		case IDM_TEXT:
			abc.fOption = IDM_TEXT;			
			ChangeCursor(hWnd, IDC_IBEAM);
			SendMessage(hText, BM_SETSTATE, 1, 0);
			SendMessage(hPencil, BM_SETSTATE, 0, 0);			
			break;
		}

		if (abc.fOption == IDM_TEXT)
		{			
			EnableWindow(hCombo, FALSE);
		}
		else
		{
			EnableWindow(hCombo, TRUE);
		}

		if (LOWORD(wParam) == IDM_COMBO1 || LOWORD(wParam) == IDM_EDITCOLOR
			|| (LOWORD(wParam) >= 1017 && LOWORD(wParam) <= 1036))       //线宽和颜色
		{
			if (abc.fOption == IDM_TEXT)
			{	
				InvalidateRect(hEdit, NULL, TRUE);
				SetFocus(hEdit);
			}
			else
			{
				if (abc.fOption != IDM_PENCIL)     //画图形
				{					
					InvalidateRect(hClient2, NULL, TRUE);
					InvalidateRect(hClient3, NULL, TRUE);
				}
			}
		}
		else                                     //图形转换
		{
			if (bEditHide == TRUE)              
			{
				ShowWindow(hEdit, SW_HIDE);
				SetWindowText(hEdit, '\0');
				InvalidateRect(hClient2, NULL, TRUE);
			}

			bDrawHide = FALSE;		
			ShowWindow(hClient3, SW_HIDE);				
			InvalidateRect(hClient2, NULL, TRUE);
		}

		PaintRect(hStatic, iColor);
		StaticText(hWnd);
		break;
		
	case WM_PAINT:
		for (int i = 0; i < 20; i++)
			PaintRect(hCtrlBlock[i], i);
		PaintRect(hStatic, iColor);
		
		hicon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
		hicon2 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
		hicon3 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2));
		hicon4 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON3));
		hicon5 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LINE));
		hicon6 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_RECT));
		hicon7 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ELLIPSE));
		hicon8 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ROUNDRECT));

		SendMessage(hPencil, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon);
		SendMessage(hText, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon2);
		SendMessage(hEditColor, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon3);
		SendMessage(hBrush, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon4);
		SendMessage(hLine, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon5);
		SendMessage(hRect, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon6);
		SendMessage(hEllipse, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon7);
		SendMessage(hRoundRect, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon8);
		break;
	}
	
	return FALSE;
}

BOOL CALLBACK AboutDlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hWnd, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

BOOL ChangeCursor(HWND hWnd, LPCTSTR lpCursorName)
{
	HCURSOR crsCross = LoadCursor((HINSTANCE)GetModuleHandle(NULL), lpCursorName);
	return SetClassLong(hWnd, GCL_HCURSOR, (LONG)crsCross);   //替换WNDCLASS结构中的光标资源
}																		//(即32位长整数值)


void PaintFigure(HWND hWnd, HDC hdc)
{
	RECT rc;
	HPEN hpen;
	HBRUSH hbrush;
	int iLength, iHeigth;
	POINT p1, p2;	

	if (abc.fOption != IDM_TEXT&&abc.fOption != IDM_PENCIL)
	{
		hpen = CreatePen(PS_INSIDEFRAME, abc.iWidth, abc.cr);
		SelectObject(hdc, hpen);
		GetWindowRect(hWnd, &rc);

		p1 = abc.Pi.Fe.pBeg;
		p2 = abc.Pi.Fe.pEnd;
		iLength = rc.right - rc.left;
		iHeigth = rc.bottom - rc.top;

		if (abc.fOption != IDM_LINE)
		{
			hbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
			SelectObject(hdc, hbrush);
			SetBkMode(hdc, TRANSPARENT);

			if (abc.fOption == IDM_RECT)
			{
				Rectangle(hdc, 0, 0, iLength, iHeigth);
			}

			if (abc.fOption == IDM_ROUNDRECT)
			{
				RoundRect(hdc, 0, 0, iLength, iHeigth,
					iLength / 4, iHeigth / 4);
			}

			if (abc.fOption == IDM_ELIPSE)
			{
				Ellipse(hdc, 0, 0, iLength, iHeigth);
			}
			DeleteObject(hbrush);
		}
		else
		{
			if ((p1.x<p2.x&&p1.y<p2.y) ||
				(p1.x>p2.x&&p1.y>p2.y))
			{
				MoveToEx(hdc, 0, 0, NULL);
				LineTo(hdc, iLength, iHeigth);
			}

			if ((p1.x<p2.x&&p1.y>p2.y) ||
				(p1.x > p2.x&&p1.y < p2.y))
			{
				MoveToEx(hdc, 0, iHeigth, NULL);
				LineTo(hdc, iLength, 0);
			}
		}
		DeleteObject(hpen);
	}
}


void PaintOnMem(HWND hWnd, HDC hdcMem)         //最终画到内存缓冲区记忆
{
	RECT rc;
	HPEN hpen;
	HBRUSH hbrush;
	POINT pt, pt1, pt2;

	if (abc.fOption != IDM_TEXT&&abc.fOption != IDM_PENCIL)
	{
		hpen = CreatePen(PS_SOLID, abc.iWidth, abc.cr);
		SelectObject(hdcMem, hpen);
		GetWindowRect(hWnd, &rc);      //此函数获取窗口在屏幕上的位置
		
		pt1.x = rc.left;
		pt1.y = rc.top;
		pt2.x = rc.right;
		pt2.y = rc.bottom;

		ScreenToClient(hClient2, &pt1);         //将屏幕位置转换成客户区坐标
		ScreenToClient(hClient2, &pt2);

		abc.Pi.Fe.pBeg = pt1;        //将最终位置赋给Paint结构体
		abc.Pi.Fe.pEnd = pt2;

		if (abc.fOption != IDM_LINE)
		{
			hbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
			SelectObject(hdcMem, hbrush);
			SetBkMode(hdcMem, TRANSPARENT);

			if (abc.fOption == IDM_RECT)
			{
				Rectangle(hdcMem, pt1.x, pt1.y, pt2.x, pt2.y);
			}

			if (abc.fOption == IDM_ELIPSE)
			{
				Ellipse(hdcMem, pt1.x, pt1.y, pt2.x, pt2.y);
			}

			if (abc.fOption == IDM_ROUNDRECT)
			{
				RoundRect(hdcMem, pt1.x, pt1.y, pt2.x,
					pt2.y, pt2.x / 4, pt2.y / 4);
			}

		}
		else
		{
			if (abc.Pi.Fe.LvsR == TRUE&&abc.Pi.Fe.TvsB == TRUE ||
				abc.Pi.Fe.LvsR == FALSE&&abc.Pi.Fe.TvsB == FALSE)
			{
				MoveToEx(hdcMem, pt1.x, pt1.y, NULL);
				LineTo(hdcMem, pt2.x, pt2.y);
			}

			if (abc.Pi.Fe.LvsR == TRUE&&abc.Pi.Fe.TvsB == FALSE ||
				abc.Pi.Fe.LvsR == FALSE&&abc.Pi.Fe.TvsB == TRUE)
			{
				MoveToEx(hdcMem, pt1.x, pt2.y, NULL);
				LineTo(hdcMem, pt2.x, pt1.y);
			}
		}
		DeleteObject(hpen);
	}
}


void DrawClientText(HWND hWnd, HDC hdcMem, int cxChar, int cyChar)
{
	RECT rect;
	HFONT hfont;
	int iTextLength;
	POINT point1, point2;
	static int iSideWidth, iSideHeigth;
	static int cxBuffer, cyBuffer;

	iTextLength = GetWindowTextLength(hWnd);

	if (iTextLength)
	{
		GetWindowRect(hWnd, &rect);
		point1.x = rect.left;
		point1.y = rect.top;
		point2.x = rect.right;
		point2.y = rect.bottom;

		ScreenToClient(hClient2, &point1);
		ScreenToClient(hClient2, &point2);

		abc.Pi.Tt.pBeg = point1;
		abc.Pi.Tt.pEnd = point2;

		iSideWidth = abc.Pi.Tt.pEnd.x - abc.Pi.Tt.pBeg.x;
		iSideHeigth = abc.Pi.Tt.pEnd.y - abc.Pi.Tt.pBeg.y;

		memset(abc.Pi.Tt.szText, '\0', 1024);

		GetWindowTextA(hWnd, abc.Pi.Tt.szText, iTextLength + 1);
		SetWindowText(hWnd, '\0');

		cxBuffer = iSideWidth / cxChar;
		cyBuffer = iSideHeigth / cyChar;

		for (int y = 0; y < cyBuffer; y++)
		{
			for (int x = 0; x < cxBuffer; x++)
			{
				if ((x + y*cxBuffer) < strlen(abc.Pi.Tt.szText))
				{
					hfont = CreateFont(16, 7, 0, 0, 0, 0, 0, 0,
						DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, NULL);

					SelectObject(hdcMem, hfont);
					SendMessage(hWnd, WM_SETFONT, (WPARAM)hfont, 0);
					SetTextColor(hdcMem, abc.cr);
					SetBkMode(hdcMem, TRANSPARENT);

					TextOutA(hdcMem, abc.Pi.Tt.pBeg.x + cxChar*x,
						abc.Pi.Tt.pBeg.y + y*cyChar,
						&abc.Pi.Tt.szText[x + y*cxBuffer], 1);
				}
			}
		}
	}
}


void RePaintMem(HDC hdcMem,Paint paint)      //
{
	HFONT hfont;
	HPEN hpen;
	HBRUSH hBrush;
	static int iSideWidth, iSideHeigth;
	static int cxBuffer, cyBuffer;
	
	if (paint.fOption == IDM_TEXT)
	{
		iSideWidth = paint.Pi.Tt.pEnd.x - paint.Pi.Tt.pBeg.x;
		iSideHeigth = paint.Pi.Tt.pEnd.y - paint.Pi.Tt.pBeg.y;

		cxBuffer = iSideWidth / cxChar;
		cyBuffer = iSideHeigth / cyChar;

		for (int y = 0; y < cyBuffer; y++)
		{
			for (int x = 0; x < cxBuffer; x++)
			{
				if ((x + y*cxBuffer) < strlen(paint.Pi.Tt.szText))
				{
					hfont = CreateFont(16, 7, 0, 0, 0, 0, 0, 0,
						DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, NULL);

					SelectObject(hdcMem, hfont);
					SendMessage(hClient2, WM_SETFONT, (WPARAM)hfont, 0);
					SetTextColor(hdcMem, paint.cr);
					SetBkMode(hdcMem, TRANSPARENT);

					TextOutA(hdcMem, paint.Pi.Tt.pBeg.x + cxChar*x,
						paint.Pi.Tt.pBeg.y + y*cyChar,
						&paint.Pi.Tt.szText[x + y*cxBuffer], 1);
				}
			}
		}
	}
	else
	{
		hpen = CreatePen(PS_SOLID, paint.iWidth, paint.cr);
		SelectObject(hdcMem, hpen);

		if (paint.fOption == IDM_PENCIL)
		{			
			MoveToEx(hdcMem, paint.Pi.Fe.pBeg.x, paint.Pi.Fe.pBeg.y, NULL);
			LineTo(hdcMem, paint.Pi.Fe.pEnd.x, paint.Pi.Fe.pEnd.y);
			
		}

		if (paint.fOption != IDM_PENCIL&&paint.fOption != IDM_TEXT)
		{
			if (paint.fOption == IDM_LINE)
			{
				if (paint.Pi.Fe.LvsR == TRUE&&paint.Pi.Fe.TvsB == TRUE ||
					paint.Pi.Fe.LvsR == FALSE&&paint.Pi.Fe.TvsB == FALSE)
				{
					MoveToEx(hdcMem, paint.Pi.Fe.pBeg.x, paint.Pi.Fe.pBeg.y, NULL);
					LineTo(hdcMem, paint.Pi.Fe.pEnd.x, paint.Pi.Fe.pEnd.y);;
				}

				if (paint.Pi.Fe.LvsR == TRUE&&paint.Pi.Fe.TvsB == FALSE ||
					paint.Pi.Fe.LvsR == FALSE&&paint.Pi.Fe.TvsB == TRUE)
				{
					MoveToEx(hdcMem, paint.Pi.Fe.pBeg.x, paint.Pi.Fe.pEnd.y, NULL);
					LineTo(hdcMem, paint.Pi.Fe.pEnd.x, paint.Pi.Fe.pBeg.y);
				}
			}
			else
			{
				hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
				SelectObject(hdcMem, hBrush);
				SetBkMode(hdcMem, TRANSPARENT);

				if (paint.fOption == IDM_RECT)
				{
					Rectangle(hdcMem, paint.Pi.Fe.pBeg.x, paint.Pi.Fe.pBeg.y,
						paint.Pi.Fe.pEnd.x, paint.Pi.Fe.pEnd.y);
				}
				if (paint.fOption == IDM_ELIPSE)
				{
					Ellipse(hdcMem, paint.Pi.Fe.pBeg.x, paint.Pi.Fe.pBeg.y,
						paint.Pi.Fe.pEnd.x, paint.Pi.Fe.pEnd.y);
				}
				if (paint.fOption == IDM_ROUNDRECT)
				{
					RoundRect(hdcMem, paint.Pi.Fe.pBeg.x, paint.Pi.Fe.pBeg.y,
						paint.Pi.Fe.pEnd.x, paint.Pi.Fe.pEnd.y,
						paint.Pi.Fe.pEnd.x / 4, paint.Pi.Fe.pEnd.y / 4);
				}
			}
		}
	}
}



void GetLargestDisPlayMode(int * pcxBitmap, int * pcyBitmap)
{
	DEVMODE devMode;
	int iModeNum = 0;

	*pcxBitmap = *pcyBitmap = 0;
	ZeroMemory(&devMode, sizeof(DEVMODE));
	devMode.dmSize = sizeof(DEVMODE);

	while (EnumDisplaySettings(NULL, iModeNum++, &devMode))
	{
		*pcxBitmap = max(*pcxBitmap, (int)devMode.dmPelsWidth);
		*pcyBitmap = max(*pcyBitmap, (int)devMode.dmPelsHeight);
	}
}

void PaintRect(HWND hCtrl, int iSel)
{
	HBRUSH hBrush;
	HDC hdc;
	RECT rect;
	COLORREF color;

	static COLORREF crColor[20] =
	{
		RGB(0, 0, 0), RGB(127, 127, 127),RGB(136, 0, 21), RGB(237, 28, 36),
		RGB(255, 127, 39), RGB(255, 242, 0),RGB(34, 177, 76), RGB(0, 162, 232),
		RGB(63, 72, 204), RGB(163, 73, 164), RGB(255, 255, 255), RGB(195, 195, 195),
		RGB(185, 122, 87), RGB(255, 174, 201), RGB(255, 201, 14), RGB(239, 228, 176),
		RGB(181, 230, 29), RGB(153, 217, 234), RGB(112, 146, 190), RGB(200, 191, 231)
	};

	InvalidateRect(hCtrl, NULL, TRUE);
	UpdateWindow(hCtrl);

	hdc = GetDC(hCtrl);
	GetClientRect(hCtrl, &rect);

	if (iSel >= 0 && iSel < 20)
	{
		abc.cr = crColor[iSel];
		hBrush = CreateSolidBrush(abc.cr);
	}
	else if (iSel >= 1017 && iSel <= 1036)
	{
		abc.cr = crColor[iSel - 1017];
		hBrush = CreateSolidBrush(abc.cr);
	}
	else
		hBrush = CreateSolidBrush(abc.cr);


	SelectObject(hdc, hBrush);
	Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
	DeleteObject(SelectObject(hdc, hBrush));
	ReleaseDC(hCtrl, hdc);
}

BOOL ColorEditDlg(HWND hWnd)
{
	static CHOOSECOLOR cc;
	static COLORREF crCustColors[16];

	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hWnd;
	cc.hInstance = NULL;
	cc.rgbResult = RGB(0x80, 0x80, 0x80);
	cc.lpCustColors = crCustColors;
	cc.Flags = CC_RGBINIT | CC_FULLOPEN;
	cc.lCustData = 0;
	cc.lpfnHook = NULL;
	cc.lpTemplateName = NULL;

	if (ChooseColor(&cc))
	{
		abc.cr = cc.rgbResult;
		return TRUE;
	}
	else
		return FALSE;
}


void StaticText(HWND hWnd)
{
	SetDlgItemInt(hWnd, IDM_TYPE, abc.fOption, FALSE);

	if (abc.fOption == IDM_RECT)
		SetDlgItemText(hWnd, IDM_OPTION1, L"矩形");
	if (abc.fOption == IDM_LINE)
		SetDlgItemText(hWnd, IDM_OPTION1, L"直线");
	if (abc.fOption == IDM_PENCIL)
		SetDlgItemText(hWnd, IDM_OPTION1, L"铅笔");
	if (abc.fOption == IDM_ELIPSE)
		SetDlgItemText(hWnd, IDM_OPTION1, L"椭圆");
	if (abc.fOption == IDM_ROUNDRECT)
		SetDlgItemText(hWnd, IDM_OPTION1, L"圆角矩形");
	if (abc.fOption == IDM_TEXT)
		SetDlgItemText(hWnd, IDM_OPTION1, L"文本");

	if (abc.fOption != IDM_TEXT)
		SetDlgItemInt(hWnd, IDM_WIDTH, abc.iWidth, FALSE);
	else
		SetDlgItemInt(hWnd, IDM_WIDTH, 1, FALSE);

	BYTE R, G, B;

	R = GetRValue(abc.cr);
	G = GetGValue(abc.cr);
	B = GetGValue(abc.cr);

	UINT Num[] = { R, G, B };
	SetDlgItemInt(hWnd, IDM_COLOR, Num[0], FALSE);
	SetDlgItemInt(hWnd, IDM_COLOR2, Num[1], FALSE);
	SetDlgItemInt(hWnd, IDM_COLOR3, Num[2], FALSE);

	char Buffer[10], Buffer2[10], Buffer3[10];
	char Buffer4[40] = "0x";
	itoa(R, Buffer, 16);
	itoa(G, Buffer2, 16);
	itoa(B, Buffer3, 16);

	strcat(Buffer4, Buffer);
	strcat(Buffer4, Buffer2);
	strcat(Buffer4, Buffer3);

	for (int i = 2; i < 40; i++)
	{
		if (Buffer4[i] >= 'a'&&Buffer4[i] <= 'z')
		{
			Buffer4[i] = toupper(Buffer4[i]);
		}
	}
	SetDlgItemTextA(hWnd, IDM_COLOR4, Buffer4);
}


void DoCaption(HWND hWnd, TCHAR * szTitleName, TCHAR * szAppName)
{
	TCHAR szCaption[MAX_PATH];

	wsprintf(szCaption, TEXT("%s - %s"), szTitleName[0] ? szTitleName :
		TEXT("无标题"), szAppName);

	SetWindowText(hWnd, szCaption);
}



HBITMAP CopyDCToBitmap(HDC hScrDC,LPRECT lpRect)
{
	if (hScrDC == NULL)
	{MessageBox(NULL, TEXT("参数错误"), TEXT("画图"), MB_OK);
		return NULL;
	}

	HDC hMemDc, hdc;
	static HBITMAP hBitmap, hOldBitmap;
	int iWidth, iHeigth;

	iWidth = lpRect->right - lpRect->left;
	iHeigth = lpRect->bottom - lpRect->top;

	hdc = GetDC(hClient2);
	BitBlt(hdc, 0, 0, cxBitmap, cyBitmap, hScrDC, 0, 0, SRCCOPY);
	//窗口尺寸更改时重新将内存DC绘到hClient2设备环境中

	hMemDc = CreateCompatibleDC(hdc);
	hBitmap = CreateCompatibleBitmap(hdc, iWidth, iHeigth);
	hOldBitmap =(HBITMAP) SelectObject(hMemDc, hBitmap);

	BitBlt(hMemDc, 0, 0, iWidth, iHeigth, hdcMem, 0, 0, SRCCOPY);
	  //屏幕设备描述表拷贝到内存设备描述表中
	hBitmap = (HBITMAP)SelectObject(hMemDc, hOldBitmap);

	DeleteObject(hMemDc);
	DeleteObject(hOldBitmap);
	ReleaseDC(hClient2, hdc);

	return hBitmap;
}


BOOL SaveBitmapToFile(HBITMAP hBitmap,LPWSTR lpFileName)
{         
	if (hBitmap == NULL || lpFileName[0] == '\0')
	{
		MessageBox(NULL, TEXT("参数错误"), TEXT("画图"), MB_OK);
		return FALSE;
	}

	HDC hdc;
	DWORD dwPaletteSize = 0,   //调色板(颜色数组)大小
		dwBmBitsSize,      //位图位大小
		dwDIBSize,          //位图大小
		dwWritten;

	BITMAP Bitmap;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	LPBITMAPINFOHEADER lpbi;
	HANDLE fh, hDib, hPal, hOldPal = NULL;
	
	//位图信息头
	GetObject(hBitmap, sizeof(BITMAP), &Bitmap);
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = Bitmap.bmWidth;
	bmih.biHeight = Bitmap.bmHeight;
	bmih.biPlanes = Bitmap.bmPlanes;
	bmih.biBitCount = Bitmap.bmBitsPixel;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 0;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;

	if (bmih.biBitCount <= 8)
		dwPaletteSize = (1 << bmih.biBitCount)*sizeof(RGBQUAD);  //32位无颜色表数组

	//像素数据占用的字节数 (4字节对齐)
	dwBmBitsSize = ((Bitmap.bmWidth*bmih.biBitCount + 31) / 32) * 4 * Bitmap.bmHeight;

	//为位图内容分配内存
	hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize 
		+ sizeof(BITMAPINFOHEADER));
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	*lpbi = bmih;

	//处理调色板
	hPal = GetStockObject(DEFAULT_PALETTE);
	if (hPal)
	{
		hdc = GetDC(NULL);
		hOldPal = SelectPalette(hdc, (HPALETTE)hPal, FALSE);
		RealizePalette(hdc);
	}

	GetDIBits(hdc, hBitmap, 0, (UINT)Bitmap.bmHeight,
		((BYTE*)lpbi) +sizeof(BITMAPINFOHEADER)+
		dwPaletteSize, (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);
	//lpbi指针类型不同，所指的信息也不同，lpbi为信息头，直接相加会指向下一个结构，
	//所以要强制转换类型为--->(BYTE*)
	
	if (hOldPal)
	{
		SelectPalette(hdc, (HPALETTE)hOldPal, TRUE);
		RealizePalette(hdc);
		ReleaseDC(NULL, hdc);
	}

	fh = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;

	//位图文件头
	bmfh.bfType = *(WORD *)"BM";  //也可以为0x4D42
	dwDIBSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)
		+dwPaletteSize + dwBmBitsSize;   //整个文件长度

	bmfh.bfSize = dwDIBSize;        //整个文件长度
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER)+(DWORD)sizeof(BITMAPINFOHEADER)
		+dwPaletteSize;    //到位图像素位的偏移

	WriteFile(fh, &bmfh, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);  //写入文件头
	WriteFile(fh, lpbi, dwDIBSize-sizeof(BITMAPFILEHEADER), &dwWritten, NULL);   //写入信息头
	//此处两个写入，注意写入字节大小，不要重复写入，lpbi写入字节数
	//dwDIBSize要减去BITMAPFILEHEADER
	
	GlobalUnlock(hDib);
	GlobalFree(hDib);
	CloseHandle(fh);

	return TRUE;
}


void SaveJsonFile(vector<Paint> qw)   //保存json文件
{
	Json::Value root;
	Json::Value arrayObj;
	Json::Value item;
	Json::StyledWriter sw;
	//json cpp 使用规则：  一个key 一个value  

	for (int i = 0; i < qw.size(); i++)
	{
		item["fOption"] = qw[i].fOption;
		item["cr"] =(int)qw[i].cr;
		item["iWidth"] = qw[i].iWidth;

		if (qw[i].fOption == IDM_TEXT)
		{
			item["szText"] = qw[i].Pi.Tt.szText;
			item["POINT:pBeg.x"] = qw[i].Pi.Tt.pBeg.x;
			item["POINT:pBeg.y"] = qw[i].Pi.Tt.pBeg.y;
			item["POINT:pEnd.x"] = qw[i].Pi.Tt.pEnd.x;
			item["POINT:pEnd.y"] = qw[i].Pi.Tt.pEnd.y;
		}
		else
		{
			item["POINT:pBeg.x"] = qw[i].Pi.Fe.pBeg.x;
			item["POINT:pBeg.y"] = qw[i].Pi.Fe.pBeg.y;
			item["POINT:pEnd.x"] = qw[i].Pi.Fe.pEnd.x;
			item["POINT:pEnd.y"] = qw[i].Pi.Fe.pEnd.y;
			item["LvsR"] = qw[i].Pi.Fe.LvsR;
			item["TvsB"] = qw[i].Pi.Fe.TvsB;
		}
		arrayObj.append(item);
	}

	root["Paint"] = arrayObj;

	ofstream os;
	os.open("new json");
	os << sw.write(root);
	os.close();
}


void ReadJsonFile(Paint zz)
{
	Json::Reader reader;
	Json::Value root;

	ifstream ifs;
	ifs.open("new json.json", ios::binary);

	if (!ifs.is_open())
	{
		MessageBox(hWndProc, TEXT("Error opening files"), TEXT("画图"), MB_OK);
		return;
	}

	if (reader.parse(ifs, root))
	{
		for (int i = 0; i < root["Paint"].size(); i++)
		{
			zz.fOption = root["fOption"][i].asInt();
			zz.cr = root["cr"][i].asInt();
			zz.iWidth = root["iWidth"][i].asInt();

			if (root["fOption"][i].asInt() == IDM_TEXT)
			{
				strcpy(zz.Pi.Tt.szText, root["szText"][i].asString().c_str());
				zz.Pi.Tt.pBeg.x = root["POINT:pBeg.x"][i].asInt();
				zz.Pi.Tt.pBeg.y = root["POINT:pBeg.y"][i].asInt();
				zz.Pi.Tt.pEnd.x = root["POINT:pEnd.x"][i].asInt();
				zz.Pi.Tt.pEnd.y = root["POINT:pEnd.y"][i].asInt();
			}
			else
			{
				zz.Pi.Fe.pBeg.x = root["POINT:pBeg.x"][i].asInt();
				zz.Pi.Fe.pBeg.y = root["POINT:pBeg.y"][i].asInt();
				zz.Pi.Fe.pEnd.x = root["POINT:pEnd.x"][i].asInt();
				zz.Pi.Fe.pEnd.y = root["POINT:pEnd.y"][i].asInt();
				zz.Pi.Fe.LvsR = root["LvsR"][i].asBool();
				zz.Pi.Fe.TvsB = root["TvsB"][i].asBool();
			}
			RePaintMem(hdcMem, zz);
			InvalidateRect(hClient2, NULL, TRUE);
		}
	}
	else
	{
		MessageBox(hWndProc, TEXT("parse error"), TEXT("画图"), MB_OK);
	}
	ifs.close();
}




