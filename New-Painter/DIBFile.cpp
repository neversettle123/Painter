#include "stdafx.h"
#include "DIBFile.h"
#include<commdlg.h>


static OPENFILENAME ofn;

void DIBFileInitialize(HWND hWnd)
{
	static TCHAR szFilter[] = TEXT("Bitmap Files (*.BMP)\0*.bmp\0")
							  TEXT("All Files (*.*)\0*.*\0\0");

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = NULL;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = 0;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = TEXT("bmp");
	ofn.lCustData = 0;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
}


BOOL DIBFileOpenDlg(HWND hWnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = pstrFileName;
	ofn.lpstrFileTitle = pstrTitleName;
	ofn.Flags = 0;

	return GetOpenFileName(&ofn);
}


BITMAPFILEHEADER * LoadDIBFile(PTSTR pstrFileName)
{
	BOOL bSuccess;
	DWORD dwFileSize, dwHighSize, dwBytesRead;
	HANDLE hFile;
	PBITMAPFILEHEADER  pbmfh;

	hFile = CreateFile(pstrFileName, GENERIC_READ, 0,
		NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	dwFileSize = GetFileSize(hFile, &dwHighSize);
	if (dwHighSize)
	{
		CloseHandle(hFile);
		return NULL;
	}

	pbmfh = (PBITMAPFILEHEADER)malloc(dwFileSize);
	if (!pbmfh)
	{
		CloseHandle(hFile);
		return NULL;
	}

	bSuccess = ReadFile(hFile, pbmfh, dwFileSize, &dwBytesRead, NULL);
	CloseHandle(hFile);

	if (!bSuccess || (dwBytesRead != dwFileSize)
		|| (pbmfh->bfType != *(WORD *)"BM")
		|| (pbmfh->bfSize != dwFileSize))
	{
		free(pbmfh);
		return NULL;
	}

	return pbmfh;
}


BOOL DIBFileSaveDlg(HWND hWnd, PTSTR pstrFileName, PTSTR pstrTitleName)
{
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = pstrFileName;
	ofn.lpstrFileTitle = pstrTitleName;
	ofn.Flags = OFN_OVERWRITEPROMPT;

	return GetSaveFileName(&ofn);
}


BOOL SaveDIBFile(PTSTR pstrFileName, BITMAPFILEHEADER *pbmfh)
{
	BOOL bSuccess;
	DWORD dwBytesWritten;
	HANDLE hFile;

	hFile = CreateFile(pstrFileName, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	bSuccess = WriteFile(hFile, pbmfh, pbmfh->bfSize, &dwBytesWritten, NULL);

	if (!bSuccess || (dwBytesWritten != pbmfh->bfSize))
	{
		DeleteFile(pstrFileName);
		return FALSE;
	}
	return TRUE;
}

































