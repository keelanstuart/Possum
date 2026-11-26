#include "stdafx.h"

#include "CPicture.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CPicture::CPicture()
{
	m_TextAlign = ta_none;
	m_TextColor = RGB(0,0,0);
	pgpPicture = nullptr;
	m_Imagelist = 0;
	m_Text = _T("");
	noOfGifs = 0;
	m_TextFont.CreatePointFont(90, _T("Arial"), NULL);
}

CPicture::~CPicture()
{
	if (pgpPicture != nullptr)
		pgpPicture->Release();
}


#define HIMETRIC_INCH 2540

void CPicture::SetImageList(int size)
{
	m_Imagelist = size;
	CB = CBitmap::FromHandle(GetBitmapHandle());

	long hmHeight, hmWidth;

	pgpPicture->get_Height(&hmHeight);
	pgpPicture->get_Width(&hmWidth);

	int width = MulDiv(hmWidth, 96, HIMETRIC_INCH);
	int height = MulDiv(hmHeight, 96, HIMETRIC_INCH);

	il.Create(size, height, ILC_COLOR8, width / size, 0);

	il.Add(CB, RGB(255, 0, 255));
}


bool CPicture::LoadPicture(const TCHAR *filename)
{		
    // open file
    HANDLE hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
		CString strUrl;
		strUrl.Format(_T("http://www.someurl.com/installer/%s"), filename);

		HRESULT hr = URLDownloadToFile(NULL, strUrl, filename, NULL, NULL);
		if (hr == S_OK)
		{
			hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0,NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				return false;
			}
		}		        
    }

    // get file size
    DWORD dwFileSize = GetFileSize(hFile, NULL);

    if (dwFileSize == (DWORD)-1)
    {
        CloseHandle(hFile);
        return false;
    }

    LPVOID pvData = NULL;

    // alloc memory based on file size
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwFileSize);

    if (hGlobal == NULL)
    {
        CloseHandle(hFile);
        return false;
    }

    pvData = GlobalLock(hGlobal);

    if (pvData == NULL)
    {
        GlobalUnlock(hGlobal);
        CloseHandle(hFile);
        return false;
    }

    DWORD dwBytesRead = 0;

    // read file and store in global memory
    BOOL bRead = ReadFile(hFile, pvData, dwFileSize, &dwBytesRead, NULL);

    GlobalUnlock(hGlobal);
    CloseHandle(hFile);

    if (!bRead)
    {
        return false;
    }

    LPSTREAM pstm = NULL;

    // create IStream* from global memory
    HRESULT hr = CreateStreamOnHGlobal(hGlobal, true, &pstm);

    if (!(SUCCEEDED(hr)))
    {
        if (pstm != NULL)
            pstm->Release();
            
        return false;
    }
    else if (pstm == NULL)
    {
        return false;
    }

	// Create IPicture from image file
	if (pgpPicture) pgpPicture->Release();

    hr = ::OleLoadPicture(pstm, dwFileSize, false, IID_IPicture, (LPVOID *)&(pgpPicture));

    if (!(SUCCEEDED(hr)))
    {
    	pstm->Release();
        return false;
    }
    else if (pgpPicture == NULL)
    {
    	pstm->Release();
        return false;
    }

    pstm->Release();

    return true;  //Made it ...!
}


bool CPicture::LoadPicture2(const TCHAR *filename)
{
	// open file
	HANDLE hFile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	// get file size
	DWORD dwFileSize = GetFileSize(hFile, NULL);

	if (dwFileSize == (DWORD)-1)
	{
		CloseHandle(hFile);
		return false;
	}

	LPVOID pvData = NULL;

	// alloc memory based on file size
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwFileSize);

	if (hGlobal == NULL)
	{
		CloseHandle(hFile);
		return false;
	}

	pvData = GlobalLock(hGlobal);

	if (pvData == NULL)
	{
		GlobalUnlock(hGlobal);
		CloseHandle(hFile);
		return false;
	}

	DWORD dwBytesRead = 0;

	// read file and store in global memory
	BOOL bRead = ReadFile(hFile, pvData, dwFileSize, &dwBytesRead, NULL);

	GlobalUnlock(hGlobal);
	CloseHandle(hFile);

	if (!bRead)
	{
		return false;
	}

	LPSTREAM pstm = NULL;

	// create IStream* from global memory
	HRESULT hr = CreateStreamOnHGlobal(hGlobal, true, &pstm);

	if (!(SUCCEEDED(hr)))
	{
		if (pstm != NULL)
			pstm->Release();

		return false;
	}

	else if (pstm == NULL)
	{
		return false;
	}

	hr = ::OleLoadPicture(pstm, dwFileSize, false, IID_IPicture, (LPVOID *)&(pictureArray[noOfGifs]));

	if (!(SUCCEEDED(hr)))
	{
		pstm->Release();
		return false;
	}

	else if (pictureArray[noOfGifs] == NULL)
	{
		pstm->Release();
		return false;
	}

	pstm->Release();

	noOfGifs++; // increment the index number of the number of files we have
	return true;  //Made it ...!
}

#define SafeRelease(_obj) { if (_obj != nullptr) _obj->Release(); _obj = nullptr; }

HRESULT CPicture::LoadPictureFromMemory(IWICImagingFactory *pIWICFactory, void *pData, size_t szData)
{
	IWICBitmapDecoder *pDecoder = NULL;
	IWICBitmapFrameDecode *pSource = NULL;
	IWICStream *pStream = NULL;
	IWICFormatConverter *pConverter = NULL;
	IWICBitmapScaler *pScaler = NULL;

	HRESULT hr;

#if 0
	HRSRC imageResHandle = NULL;
	HGLOBAL imageResDataHandle = NULL;
	void *pImageFile = NULL;
	DWORD imageFileSize = 0;

	// Locate the resource.
	imageResHandle = FindResourceW(HINST_THISCOMPONENT, resourceName, resourceType);
	HRESULT hr = imageResHandle ? S_OK : E_FAIL;
	if (SUCCEEDED(hr))
	{
		// Load the resource.
		imageResDataHandle = LoadResource(HINST_THISCOMPONENT, imageResHandle);

		hr = imageResDataHandle ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		// Lock it to get a system memory pointer.
		pImageFile = LockResource(imageResDataHandle);

		hr = pImageFile ? S_OK : E_FAIL;
	}

	if (SUCCEEDED(hr))
	{
		// Calculate the size.
		imageFileSize = SizeofResource(HINST_THISCOMPONENT, imageResHandle);

		hr = imageFileSize ? S_OK : E_FAIL;
	}
#endif

	// Create a WIC stream to map onto the memory.
	hr = pIWICFactory->CreateStream(&pStream);

	if (SUCCEEDED(hr))
	{
		// Initialize the stream with the memory pointer and size.
		hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pData), szData);
	}

	if (SUCCEEDED(hr))
	{
		// Create a decoder for the stream.
		hr = pIWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnLoad, &pDecoder);
	}

	if (SUCCEEDED(hr))
	{
		// Create the initial frame.
		hr = pDecoder->GetFrame(0, &pSource);
	}

	if (SUCCEEDED(hr))
	{
			// Convert the image format to 32bppPBGRA
			// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
			hr = pIWICFactory->CreateFormatConverter(&pConverter);
	}

	if (SUCCEEDED(hr))
	{
	}

	SafeRelease(pDecoder);
	SafeRelease(pSource);
	SafeRelease(pStream);
	SafeRelease(pConverter);
	SafeRelease(pScaler);
}


bool CPicture::Draw(CDC *pDC, int x, int y, int w, int h)
{
	if (pgpPicture == nullptr)
		return false;

	//Get width and height of picture
    long hmWidth = 0;
    long hmHeight = 0;

	CRect rectImage(x, y, w, h);
	if (pgpPicture == 0)
	{
		pDC->FillSolidRect(rectImage, RGB(0, 0, 0));
		return false;
	}
	
    pgpPicture->get_Width(&hmWidth);
    pgpPicture->get_Height(&hmHeight);

	CRect rectI = rectImage;
    rectI.NormalizeRect();

    int nWidth = rectI.Width();
    int nHeight= rectI.Height() * -1;

	CRect rc(x, y, w, h);

	HRESULT hrP = pgpPicture->Render(pDC->m_hDC, x, y, nWidth, -nHeight, 0, hmHeight, hmWidth, -hmHeight, &rc);

    if (SUCCEEDED(hrP))
        return true;

	return true;
}

void CPicture::Draw(CDC *pDC, int x, int y, int item, bool bCenter)
{
	if (pgpPicture == nullptr)
		return;

	if (m_Text.GetLength() > 0)
	{
		CRect rect(x, y, x + item, y + 24);

		pDC->SetTextColor(m_TextColor);		
		pDC->SelectObject(&m_TextFont);

		if (m_TextAlign == ta_none)
		{
			if (bCenter)
				pDC->DrawText(m_Text, rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
			else
				pDC->DrawText(m_Text, rect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
		}
		else
		{
			switch (m_TextAlign)
			{
				case ta_center:
					pDC->DrawText(m_Text, rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
					break;

				case ta_left:
					pDC->DrawText(m_Text, rect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
					break;

				case ta_right:
					pDC->DrawText(m_Text, rect, DT_SINGLELINE | DT_VCENTER | DT_RIGHT);
					break;
			}
		}
	}
	else 
	{
		int size = m_Imagelist;

		if (size == 0)
			size = GetWidth();

		if ((x == -1) || (y == -1))
			return;

		il.Draw(pDC, item, CPoint(x, y), ILD_MASK);
	}
}

HBITMAP CPicture::GetBitmapHandle()
{
	HBITMAP hBitmap;

	if (pgpPicture == 0)
		return 0;

	pgpPicture->get_Handle((OLE_HANDLE FAR *) &hBitmap);

	return hBitmap;
}

int CPicture::GetWidth()
{
	long hmWidth;
	if (pgpPicture == 0)
		return 0;

	pgpPicture->get_Width(&hmWidth);	    

	int width  = MulDiv(hmWidth, 96, HIMETRIC_INCH);

	return width;
}

int CPicture::GetHeight()
{
	long hmHeight;
	if (pgpPicture == 0)
		return 0;

	pgpPicture->get_Height(&hmHeight);			

	int height = MulDiv(hmHeight, 96, HIMETRIC_INCH);

	return height;
}

CImageList *CPicture:: CreateCImageList()
{
	long hmWidth, hmHeight, tempWidth, tempHeight;
	
	int width, height;
	
	// the following functions finds the largest width among the given gifs
	for(int i = 0; i < (noOfGifs - 1); i++)
	{
		pictureArray[i]->get_Width(&hmWidth);
		pictureArray[i + 1]->get_Width(&tempWidth);
		
		if(hmWidth>tempWidth)
		{
			pictureArray[i]->get_Width(&hmWidth);
		}
		else
		{
			pictureArray[i + 1]->get_Width(&hmWidth);
		}
	}

	width = MulDiv(hmWidth, 96, HIMETRIC_INCH);

	//the following function finds the largest height among the given gifs
	
	for(int i = 0; i < (noOfGifs - 1); i++)
	{
		pictureArray[i]->get_Height(&hmHeight);
		pictureArray[i + 1]->get_Width(&tempHeight);
		
		if(hmHeight>tempHeight)
		{
			pictureArray[i]->get_Width(&hmHeight);
		}
		else
		{
			pictureArray[i + 1]->get_Width(&hmHeight);
		}
	}

	height = MulDiv(hmHeight, 96, HIMETRIC_INCH);

	il.Create(width, height, ILC_COLOR32 | ILC_MASK, noOfGifs, 1);
	
	for(int i = 0; i < noOfGifs; i++)
	{
		CB = CBitmap::FromHandle(GetBitmapHandle2(i));
		il.Add(CB, RGB(255, 255, 255));
	}

	return &il;
}

HBITMAP CPicture::GetBitmapHandle2(int index)
{
	HBITMAP hBitmap;
	pictureArray[index]->get_Handle((OLE_HANDLE FAR *) &hBitmap);
	return hBitmap;
}
