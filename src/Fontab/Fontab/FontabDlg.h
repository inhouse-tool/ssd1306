// FontabDlg.h : header file
//

#pragma once

class CBitmapWnd : public CWnd
{
protected:
	afx_msg	void	OnPaint( void );
	DECLARE_MESSAGE_MAP()
};

class CTableWnd : public CWnd
{
protected:
	afx_msg	void	OnChar( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg	void	OnLButtonDblClk( UINT nFlags, CPoint point );
	afx_msg	void	OnPaint( void );
	DECLARE_MESSAGE_MAP()
};


class CFontabDlg : public CDialog
{
public:
	CFontabDlg( CWnd* pParent = nullptr );

protected:
		HICON	m_hIcon;

	    COLORREF	m_crBack;
		CFont	m_font;
		CBitmap	m_bmPreview;
		CDC	m_dcPreview;
		BYTE*	m_pbPreview;
	    CBitmapWnd	m_wndBitmap;
	    CBitmapWnd	m_wndPreview;
	    CTableWnd	m_wndTable;
		int	m_cyFont,
			m_cxFont;
		int	m_cbY,
			m_cbX;
		BYTE*	m_pbFont;
	       CStringA	m_strTable;

	    ULONG_PTR	m_llGDI;

	virtual	BOOL	OnInitDialog( void );
	virtual	BOOL	DestroyWindow( void );

	afx_msg	void	OnLButtonDblClk( UINT nFlags, CPoint point );
	afx_msg	void	OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg	BOOL	OnMouseWheel( UINT nFlags, short zDelta, CPoint pt );
	afx_msg	void	OnTimer( UINT_PTR nIDEvent );
	afx_msg	void	OnChangeFontSize( void );
	afx_msg	void	OnChangeCode( UINT uID );
	afx_msg	void	OnLoad( void );
	afx_msg	void	OnSave( void );
	afx_msg LRESULT	OnPaintSubwindow( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT	OnCharInTable(    WPARAM wParam, LPARAM lParam );
	DECLARE_MESSAGE_MAP()

		void	OnFontSize( void );
		void	CreatePreview( void );
		void	CreateTable( void );
		void	DrawBitmap(  CDC* pDC, CRect* pRect );
		void	DrawPreview( CDC* pDC, CRect* pRect );
		void	DrawTable(   CDC* pDC, CRect* pRect );
		CRect	DrawFont( CDC* pDC, CRect* pRect, int chCode );
		void	FlipBit( int x, int y );
		void	AllocateBitmap( void );
		void	LoadFile( CString strPath );
		void	SaveFile( CString strPath );
	      CStringA	DumpBytes( int chCode );
	      CString	DecBin( int iValue, int nDigit );
};
