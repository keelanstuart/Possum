#pragma once

#include <afxctl.h>
#include <afx.h>
#include <afxcmn.h>

class CPicture  : public CObject
{
	public:
		CPicture();
		virtual ~CPicture();

		enum TextAlign
		{
			ta_center = 1,
			ta_right  = 2,
			ta_left   = 3,
			ta_none   = 0
		};

		enum ButtonAlign
		{
			ba_left = 1,
			ba_right = 2
		};

	public:		
		int GetHeight();
		int GetWidth();
		
		int noOfGifs;

		HBITMAP GetBitmapHandle();
		HBITMAP GetBitmapHandle2(int index);
		
		// Draw a bitmap
		bool Draw(CDC *, int x, int y, int w, int h);

		// If this bitmap is an image list, set the size, and draw
		// a specified item.  RGB(255, 0, 255) is the background mask
		// and truecolor images are supported.
		void SetImageList(int size);		
		void Draw(CDC *pDC, int x, int y, int item, bool bCenter = true);
		
		//  Load from a file
		bool LoadPicture(const TCHAR *filename);	

		HRESULT LoadPictureFromMemory(IWICImagingFactory *pIWICFactory, void *pData, size_t szData);

		// Functions that allow you to load multiple images
		// and return an image list from those pictures.
		// These functions are specialized
		bool LoadPicture2(const TCHAR *filename);
		CImageList*  CreateCImageList();						
		LPPICTURE	pictureArray[5];

		COLORREF	m_TextColor;
		CFont		m_TextFont;
		CString		m_Text;			// "Alt" text
		LPPICTURE	pgpPicture;		// Picture object

	public:		
		int			m_Imagelist;	// Size in pixels of each item in the list
		CImageList 	il;				// Image list
		CBitmap*	CB;				// Bitmap after the file is loaded

		// set the picture as text for debugging
		bool				m_isText;
		CPicture::TextAlign m_TextAlign;
};
