#pragma once

void DIBFileInitialize(HWND);
BOOL DIBFileOpenDlg(HWND, PTSTR, PTSTR);
BOOL DIBFileSaveDlg(HWND, PTSTR, PTSTR);
BITMAPFILEHEADER * LoadDIBFile(PTSTR);
BOOL SaveDIBFile(PTSTR, BITMAPFILEHEADER *);





