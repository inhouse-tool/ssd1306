// FontabDlg.cpp : implementation file
//

#include "pch.h"
#include "FontabApp.h"
#include "FontabDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef	UNICODE
#define	atoi		_wtoi
#define	strtoul		wcstoul
#define	strcpy_s	wcscpy_s
#endif//UNICODE

#define	WM_PAINT_SUBWINDOW	(WM_APP+1)
#define	WM_CHAR_IN_TABLE	(WM_APP+2)

#define	TID_FONT_SIZE	1
#define	TID_CODE_CHANGE	2

#define	PIXWIDTH	128
#define	PIXHEIGHT	 64

///////////////////////////////////////////////////////////////////////////////////////
// Message Handlers ( of the Bitmap window )

BEGIN_MESSAGE_MAP( CBitmapWnd, CWnd )
	ON_WM_PAINT()
END_MESSAGE_MAP()

void
CBitmapWnd::OnPaint( void )
{
	PAINTSTRUCT ps;
	CDC*	pDC = BeginPaint( &ps );

	GetParent()->SendMessage( WM_PAINT_SUBWINDOW, (WPARAM)this, (LPARAM)pDC );

	EndPaint( &ps );
}

///////////////////////////////////////////////////////////////////////////////////////
// Message Handlers ( of the Table window )

BEGIN_MESSAGE_MAP( CTableWnd, CWnd )
	ON_WM_CHAR()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_PAINT()
END_MESSAGE_MAP()

void
CTableWnd::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	GetOwner()->PostMessage( WM_CHAR_IN_TABLE, nChar, 0 );

	Invalidate( TRUE );
}

void
CTableWnd::OnLButtonDblClk( UINT nFlags, CPoint point )
{
	CRect	rect;
	GetClientRect( &rect );
	int	cxColumn = rect.Width()  / (16+1);
	int	cyRow    = rect.Height() / (16+1);

	int	iU = point.x / cxColumn;
	int	iL = point.y / cyRow;
	if	( iU > 0 && iL > 0 ){
		iU--;
		iL--;
		int	iCode = (iU<<4) | (iL);
		CString	strCode;
		strCode.Format( _T("%02x"), iCode );
		GetOwner()->GetDlgItem( IDC_EDIT_CODE )->SetWindowText( strCode );
	}
	ShowWindow( SW_HIDE );
}

void
CTableWnd::OnPaint( void )
{
	PAINTSTRUCT ps;
	CDC*	pDC = BeginPaint( &ps );

	GetOwner()->SendMessage( WM_PAINT_SUBWINDOW, (WPARAM)this, (LPARAM)pDC );

	EndPaint( &ps );
}

///////////////////////////////////////////////////////////////////////////////////////
// Constructor

CFontabDlg::CFontabDlg( CWnd* pParent )
	: CDialog( IDD_FONTAB_DIALOG, pParent )
{
	m_hIcon = AfxGetApp()->LoadIcon( IDR_MAINFRAME );

	m_cyFont = 16;
	m_cxFont =  8;
	AllocateBitmap();
}

///////////////////////////////////////////////////////////////////////////////////////
// Overridden Functions

BOOL
CFontabDlg::OnInitDialog( void )
{
	CDialog::OnInitDialog();

	SetIcon( m_hIcon, TRUE );
	SetIcon( m_hIcon, FALSE );

	m_crBack = GetSysColor( COLOR_3DFACE );

	((CSpinButtonCtrl*)GetDlgItem( IDC_SPIN_HEIGHT ))->SetPos( m_cyFont );
	((CSpinButtonCtrl*)GetDlgItem( IDC_SPIN_WIDTH ) )->SetPos( m_cxFont );
	((CSpinButtonCtrl*)GetDlgItem( IDC_SPIN_HEIGHT ))->SetRange( 8, PIXHEIGHT );
	((CSpinButtonCtrl*)GetDlgItem( IDC_SPIN_WIDTH ) )->SetRange( 8, PIXWIDTH );

	CreatePreview();
	m_wndPreview.SubclassDlgItem( IDC_STATIC_PREVIEW, this );
	m_wndBitmap .SubclassDlgItem( IDC_STATIC_BITMAP,  this );
	CreateTable();
	GotoDlgCtrl( GetDlgItem( IDC_STATIC_BITMAP ) );

	LOGFONT	lf;
	memset( &lf, 0, sizeof( lf ) );
	strcpy_s( lf.lfFaceName, _T("Lucida Console") );
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfPitchAndFamily = FIXED_PITCH;
	lf.lfHeight  = 14;
	m_font.DeleteObject();
	m_font.CreateFontIndirect( &lf );
	GetDlgItem( IDC_EDIT_CODE  )->SetFont( &m_font );
	GetDlgItem( IDC_EDIT_CHAR  )->SetFont( &m_font );
	GetDlgItem( IDC_EDIT_BYTES )->SetFont( &m_font );

	GetDlgItem( IDC_EDIT_CODE )->SetWindowText( _T("41") );

	GetDlgItem( IDC_STATIC_PREVIEW )->SetWindowPos( &CWnd::wndTop, 0, 0, PIXWIDTH, PIXHEIGHT*4, SWP_NOMOVE );

	Gdiplus::GdiplusStartupInput	si = { 0 };
	si.GdiplusVersion = 1;
	Gdiplus::GdiplusStartup( &m_llGDI, &si, NULL );

	return	FALSE;
}

BOOL
CFontabDlg::DestroyWindow( void )
{
	if	( m_pbFont )
		delete[]	m_pbFont;
	if	( m_pbPreview )
		delete[]	m_pbPreview;

	m_wndTable.DestroyWindow();

	Gdiplus::GdiplusShutdown( m_llGDI );

	return	CDialog::DestroyWindow();
}

///////////////////////////////////////////////////////////////////////////////////////
// Message Handlers

BEGIN_MESSAGE_MAP( CFontabDlg, CDialog )
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_TIMER()
	ON_EN_CHANGE(  IDC_EDIT_HEIGHT,    OnChangeFontSize )
	ON_EN_CHANGE(  IDC_EDIT_WIDTH,     OnChangeFontSize )
	ON_CONTROL_RANGE( EN_CHANGE,
		       IDC_EDIT_CODE,
		       IDC_EDIT_CHAR,      OnChangeCode )
	ON_BN_CLICKED( IDC_BUTTON_LOAD,    OnLoad )
	ON_BN_CLICKED( IDC_BUTTON_SAVE,    OnSave )
	ON_MESSAGE(    WM_PAINT_SUBWINDOW, OnPaintSubwindow )
	ON_MESSAGE(    WM_CHAR_IN_TABLE,   OnCharInTable )
END_MESSAGE_MAP()

void
CFontabDlg::OnLButtonDblClk( UINT nFlags, CPoint point )
{
	CRect	rectPreview;
	m_wndPreview.GetWindowRect( &rectPreview );
	ScreenToClient( &rectPreview );

	if	( rectPreview.PtInRect( point ) )
		m_wndTable.ShowWindow( m_wndTable.IsWindowVisible()? SW_HIDE: SW_SHOW );
}

void
CFontabDlg::OnLButtonDown( UINT nFlags, CPoint point )
{
	CRect	rectBitmap;
	m_wndBitmap.GetWindowRect( &rectBitmap );
	ScreenToClient( &rectBitmap );

	if	( rectBitmap.PtInRect( point ) ){
		int	xDiv = rectBitmap.Width()  / m_cxFont;
		int	yDiv = rectBitmap.Height() / m_cyFont;
		int	nDiv = ( xDiv > yDiv )? yDiv: xDiv;

		int	cy = nDiv * m_cyFont;
		int	cx = nDiv * m_cxFont;
		int	dy = rectBitmap.Height() - cy;
		int	dx = rectBitmap.Width()  - cx;
		dy /= 2;
		dx /= 2;

		int	x = point.x-rectBitmap.left;
		int	y = point.y-rectBitmap.top;
		x -= dx;
		y -= dy;
		if	( x >= 0 && y >= 0 && x <= cx && y <= cy ){
			x /= nDiv;
			y /= nDiv;
			FlipBit( x, y );
			CreatePreview();
			m_wndBitmap .Invalidate( FALSE );
			m_wndPreview.Invalidate( FALSE );
		}
	}
}

BOOL
CFontabDlg::OnMouseWheel( UINT nFlags, short zDelta, CPoint pt )
{
	CString	strCode;
	GetDlgItem( IDC_EDIT_CODE )->GetWindowText( strCode );

	BYTE	b = (BYTE)strtoul( strCode, NULL, 16 );
	if	( zDelta < 0 )
		b++;
	else
		b--;

	strCode.Format( _T("%02x"), b );
	GetDlgItem( IDC_EDIT_CODE )->SetWindowText( strCode );

	return	TRUE;
}

void
CFontabDlg::OnTimer( UINT_PTR nIDEvent )
{
	if	( nIDEvent == TID_FONT_SIZE ){
		KillTimer( nIDEvent );
		OnFontSize();
	}
	else if	( nIDEvent == TID_CODE_CHANGE ){
		KillTimer( nIDEvent );
		m_wndBitmap.Invalidate( TRUE );
	}
	else
		CDialog::OnTimer( nIDEvent );
}

void
CFontabDlg::OnChangeFontSize( void )
{
	SetTimer( TID_FONT_SIZE, 0, NULL );
}

void
CFontabDlg::OnChangeCode( UINT uID )
{
	bool	bDone = false;
	int	chCode = 0x00;

	if	( uID == IDC_EDIT_CODE ){
		CString	strNew, strOld;
		GetDlgItem( IDC_EDIT_CODE )->GetWindowText( strNew );
		if	( strNew.GetLength() == 2 ){
			chCode = (BYTE)strtoul( strNew, NULL, 16 );
			if	( chCode >= 0x20 && chCode <= 0x7e )
				strNew.Format( _T("%c"), chCode );
			else
				strNew.Format( _T("\\x%02x"), chCode );
			GetDlgItem( IDC_EDIT_CHAR )->GetWindowText( strOld );
			if	( strNew != strOld ){
				GetDlgItem( IDC_EDIT_CHAR )->SetWindowText( strNew );
				bDone = true;
			}
		}
	}
	else if	( uID == IDC_EDIT_CHAR ){
		CString	strNew, strOld;
		GetDlgItem( IDC_EDIT_CHAR )->GetWindowText( strNew );
		if	( strNew.GetLength() == 1 ){
			chCode = (BYTE)strNew[0];
			strNew.Format( _T("%02x"), chCode );
			GetDlgItem( IDC_EDIT_CODE )->SetWindowText( strNew );
			bDone = true;
		}
		else if	( strNew.Left( 2 ) == _T("\\x") &&
			  strNew.GetLength() == 4 ){
			chCode = (BYTE)strtoul( strNew.Mid( 2 ), NULL, 16 );
			strNew.Format( _T("%02x"), chCode );
			GetDlgItem( IDC_EDIT_CODE )->GetWindowText( strOld );
			if	( strNew != strOld ){
				GetDlgItem( IDC_EDIT_CODE )->SetWindowText( strNew );
				bDone = true;
			}
		}
	}

	if	( bDone ){
		CString	strBytes = (CString)DumpBytes( chCode );
		GetDlgItem( IDC_EDIT_BYTES )->SetWindowText( strBytes );
		SetTimer( TID_CODE_CHANGE, 0, NULL );
	}
}

void
CFontabDlg::OnLoad( void )
{
	CString	strFile;
	TCHAR	szFile[MAX_PATH];
	CString	strTitle;
	strTitle.Format( _T("Load a font table") );

	CFileDialog	dlg( TRUE, _T("c") );
	dlg.m_ofn.lpstrFilter     = _T("C language source file\0*.c\0Any file\0*.*\0");
	dlg.m_ofn.lpstrFile       = szFile;
	dlg.m_ofn.lpstrInitialDir = NULL;
	dlg.m_ofn.lpstrTitle      = strTitle.GetBuffer();
	*szFile = '\0';

	if	( dlg.DoModal() == IDOK )
		LoadFile( dlg.GetPathName() );
}

void
CFontabDlg::OnSave( void )
{
	CString	strFile;
	TCHAR	szFile[MAX_PATH];
	CString	strTitle;
	strTitle.Format( _T("Save the font table") );

	CFileDialog	dlg( FALSE, _T("c") );
	dlg.m_ofn.lpstrFilter     = _T("C language source file\0*.c\0Any file\0*.*\0");
	dlg.m_ofn.lpstrFile       = szFile;
	dlg.m_ofn.lpstrInitialDir = NULL;
	dlg.m_ofn.lpstrTitle      = strTitle.GetBuffer();
	*szFile = '\0';

	if	( dlg.DoModal() == IDOK )
		SaveFile( dlg.GetPathName() );
}

LRESULT
CFontabDlg::OnPaintSubwindow( WPARAM wParam, LPARAM lParam )
{
	if	( !m_cxFont || !m_cyFont )
		return	FALSE;

	CDC*	pDC  = (CDC*)lParam;
	CWnd*	pwnd = (CWnd*)wParam;
	CRect	rect;
	pwnd->GetClientRect( rect );

	if	( pwnd == &m_wndBitmap )
		DrawBitmap( pDC, &rect );

	else if	( pwnd == &m_wndPreview )
		DrawPreview( pDC, &rect );

	else if	( pwnd == &m_wndTable )
		DrawTable( pDC, &rect );

	return	TRUE;
}

LRESULT
CFontabDlg::OnCharInTable( WPARAM wParam, LPARAM lParam )
{
	int	nChar = (int)wParam;
	if	( nChar == '\b' ){
		if	( !m_strTable.IsEmpty() )
			m_strTable.Delete( m_strTable.GetLength()-1, 1 );
	}
	else if	( nChar == '\x1b' )
		m_strTable.Empty();
	else
		m_strTable += (char)nChar;

	return	TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////
// Specific Functions

void
CFontabDlg::OnFontSize( void )
{
	if	( GetDlgItem( IDC_EDIT_HEIGHT )->IsWindowEnabled() ){
		CString	str;

		GetDlgItem( IDC_EDIT_HEIGHT )->GetWindowText( str );
		m_cyFont = atoi( str );
		GetDlgItem( IDC_EDIT_WIDTH  )->GetWindowText( str );
		m_cxFont = atoi( str );

		AllocateBitmap();

		GetDlgItem( IDC_EDIT_CHAR )->SetWindowText( _T("") );
		OnChangeCode( IDC_EDIT_CODE );

		m_wndBitmap .Invalidate( TRUE );
		m_wndPreview.Invalidate( TRUE );
	}
}

void
CFontabDlg::CreatePreview( void )
{
	if	( m_pbPreview )
		delete[]	m_pbPreview;

	int	cbFont = m_cbY * m_cbX;
	DWORD	cbPreview = cbFont * 256;
	m_pbPreview = new BYTE[cbPreview];
	memset( m_pbPreview, 0x00, cbPreview );

	int	nCol = PIXWIDTH / m_cxFont;

	for	( int chCode = 0x00; chCode <= 0xff; chCode++ ){
		DWORD	iFont = chCode * cbFont;
		BYTE*	pFont = &m_pbFont[iFont];
		DWORD	iPreview = ( ( chCode / nCol ) * (m_cyFont/8) * (PIXWIDTH) )
				 + ( ( chCode % nCol ) * (m_cxFont/8) );
		BYTE*	pbPreview = &m_pbPreview[iPreview];
		for	( int ix = 0; ix < m_cxFont; ix++ ){
			for	( int iy = 0; iy < m_cyFont; iy++ ){
				if	( pFont[iy/8] & ( 0x01 << iy%8 ) ){
					int	i = ( PIXWIDTH*iy ) / 8;
					i += ix / 8;
					pbPreview[i] |= 0x80 >> (ix%8);
				}
			}
			pFont += m_cbY;
		}
	}

	m_bmPreview.DeleteObject();
	m_bmPreview.CreateBitmap( PIXWIDTH, 256/nCol*m_cyFont, 1, 1, m_pbPreview );

	m_dcPreview.DeleteDC();
	m_dcPreview.CreateCompatibleDC( NULL );
	m_dcPreview.SelectObject( &m_bmPreview );
}

#pragma comment( lib, "Dwmapi.lib" )

void
CFontabDlg::CreateTable( void )
{
	{
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc   = AfxWndProc;
		wc.hInstance     = AfxGetInstanceHandle();
		wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
		wc.style         = CS_DBLCLKS | CS_SAVEBITS;
		wc.lpszClassName = _T("fontabTableWnd");
		AfxRegisterClass( &wc );

		m_wndTable.CreateEx( 0, wc.lpszClassName,
				_T("Table"), WS_OVERLAPPED, 0, 0, 0, 0, NULL, 0, NULL );
	}

	DWORD	dwValue = DWMWCP_DONOTROUND;
	DwmSetWindowAttribute( m_wndTable.m_hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &dwValue, sizeof( dwValue ) );

	int	cx = 50*(16+1) +2;
	int	cy = 0;
	int	x  = 0;
	{
		HMONITOR hMonitor = MonitorFromPoint( CPoint( 0, 0 ), MONITOR_DEFAULTTONEAREST );
		MONITORINFO mi = { 0 };
		mi.cbSize = sizeof( mi );
		GetMonitorInfo( hMonitor, &mi );
		cy = mi.rcWork.bottom;
		cy /= (16+1);
		cy *= (16+1);
		cy += 2;
		x = mi.rcWork.right;
		x -= cx;
		x /= 2;
		x += 2;
	}

	m_wndTable.SetWindowPos( NULL, x, 0, cx, cy, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE );

	{
		DWORD	dwStyle   = GetWindowLong( m_wndTable.m_hWnd, GWL_STYLE );
		dwStyle &= ~WS_CAPTION;
		SetWindowLong( m_wndTable.m_hWnd, GWL_STYLE, dwStyle );

		DWORD dwExStyle = GetWindowLong( m_wndTable.m_hWnd, GWL_EXSTYLE );
		dwExStyle &= ~WS_EX_CLIENTEDGE;
		SetWindowLong( m_wndTable.m_hWnd, GWL_EXSTYLE, dwExStyle );
	}

	m_wndTable.SetOwner( this );
}

void
CFontabDlg::DrawBitmap( CDC* pDC, CRect* pRect )
{
	// Get the character code to draw.

	CString	strCode;
	GetDlgItem( IDC_EDIT_CODE )->GetWindowText( strCode );
	int	chCode = (BYTE)strtoul( strCode, NULL, 16 );

	// Draw the code in bitmap.

	CRect	rectFont = DrawFont( pDC, pRect, chCode );

	// Fill around the bitmap.

	CRect	rect;
	rect = *pRect;
	rect.right = rectFont.left;
	pDC->FillSolidRect( &rect, m_crBack );
	rect = *pRect;
	rect.left = rectFont.right;
	pDC->FillSolidRect( &rect, m_crBack );
	rect = *pRect;
	rect.bottom = rectFont.top;
	pDC->FillSolidRect( &rect, m_crBack );
	rect = *pRect;
	rect.top = rectFont.bottom;
	pDC->FillSolidRect( &rect, m_crBack );
}

void
CFontabDlg::DrawPreview( CDC* pDC, CRect* pRect )
{
	// Fill the preview rectangle with font's color.

	pDC->FillSolidRect( pRect, RGB( 255, 255, 255 ) );

	// Mask the preview rectangle with black of 'black & white' bitmap.

	int	cy = pRect->Height();
	pDC->StretchBlt( 0, 0, PIXWIDTH, cy, &m_dcPreview,
		         0, 0, PIXWIDTH, cy, SRCCOPY );

	// Fill the left area with dialog's coloer.

	BITMAP	bm;
	m_bmPreview.GetBitmap( &bm );
	if	( bm.bmHeight < pRect->bottom ){
		CRect	rect = *pRect;
		rect.top    =   bm.bmHeight;
		pDC->FillSolidRect( &rect, m_crBack );
	}
}

void
CFontabDlg::DrawTable( CDC* pDC, CRect* pRect )
{
	bool	bDark = true;
	COLORREF	crFore = bDark? RGB( 255, 255, 255 ): RGB(   0,   0,   0 );
	COLORREF	crBack = bDark? RGB(  13,  17,  23 ): RGB( 255, 255, 255 );

	pDC->FillSolidRect( pRect, crBack );

	int	cxColumn = pRect->Width()  / ( 16+1 );
	int	cyRow    = pRect->Height() / ( 16+1 );

	CFont	font;

	LOGFONT	lf;
	memset( &lf, 0, sizeof( lf ) );
	strcpy_s( lf.lfFaceName, _T("Arial") );
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfPitchAndFamily = VARIABLE_PITCH;

	CRect	rect;

	if	( !m_strTable.IsEmpty() ){
		pDC->FillSolidRect( pRect, RGB( 0, 0, 0 ) );
		GetClientRect( &rect );
		int	nMagnify = 6;
		int	cx = m_cxFont * nMagnify;
		int	cy = m_cyFont * nMagnify;
		rect.left   = 1;
		rect.right  = rect.left + cx;
		rect.top    = 1;
		rect.bottom = rect.top  + cy;
		for	( int i = 0; i < m_strTable.GetLength(); i++ ){
			BYTE	ch = m_strTable[i];
			if	( ch == '\r' ){
				rect.left   = 1;
				rect.right  = rect.left + cx;
				rect.top    = rect.bottom;
				rect.bottom = rect.top  + cy;
				continue;
			}
			else if	( ch == '\t' ){
				CStringA strCode = m_strTable.Mid( ++i, 2 );
				ch = (char)strtol( strCode, NULL, 16 );
				i++;
			}
			{
				DrawFont( pDC, &rect, ch );
				rect.left  += cx;
				rect.right += cx;
			}
		}
		return;
	}

	rect.left   = 0;
	rect.right  = cxColumn;
	rect.top    = 0;
	rect.bottom = cyRow;

	pDC->SetTextColor( crFore );
	{
		font.DeleteObject();
		lf.lfHeight  = 15;
		font.CreateFontIndirect( &lf );
		pDC->SelectObject( &font );
		pDC->DrawText( _T("Upper "), &rect, DT_SINGLELINE | DT_RIGHT | DT_TOP );
		pDC->DrawText( _T(" Lower"), &rect, DT_SINGLELINE | DT_LEFT  | DT_BOTTOM );
	}
	{
#if	1
		Gdiplus::Graphics	g( pDC->GetSafeHdc() );
		g.SetSmoothingMode( Gdiplus::SmoothingModeAntiAlias );
		g.SetPageUnit( Gdiplus::UnitPixel );

		Gdiplus::Color	cFore;
		cFore.SetFromCOLORREF( crFore );
		int	w = 1;
		Gdiplus::Pen	pen( cFore, (Gdiplus::REAL)w );

		g.DrawLine( &pen, rect.left,  rect.top, rect.right, rect.bottom );
#else
		CPen	pen( PS_SOLID, 1, crFore );
		pDC->SelectObject( &pen );
		pDC->MoveTo( rect.left,  rect.top );
		pDC->LineTo( rect.right, rect.bottom );
#endif
	}
	{
		font.DeleteObject();
		lf.lfHeight  = 14;
		strcpy_s( lf.lfFaceName, _T("Lucida Console") );
		lf.lfCharSet = ANSI_CHARSET;
		lf.lfPitchAndFamily = FIXED_PITCH;
		font.CreateFontIndirect( &lf );
		pDC->SelectObject( &font );

		for	( int d = 0; d < 16; d++ ){
			CString	str = DecBin( d, 4 );
			rect.left  += cxColumn;
			rect.right += cxColumn;
			pDC->DrawText( str, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER );
		}

		rect.left   = 0;
		rect.right  = cxColumn;
		rect.top    = 0;
		rect.bottom = cyRow;

		for	( int d = 0; d < 16; d++ ){
			CString	str = DecBin( d, 4 );
			rect.top    += cyRow;
			rect.bottom += cyRow;
			pDC->DrawText( str, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER );
		}

		rect.left   = cxColumn;
		rect.right  = cxColumn + rect.left;
		rect.top    = cyRow;
		rect.bottom = cyRow + rect.top;

		for	( int iCode = 0x00; iCode <= 0xff; iCode++ ){

			DrawFont( pDC, &rect, iCode );

			if	( iCode % 16 == 15 ){
				rect.top     = cyRow;
				rect.bottom  = cyRow + rect.top;
				rect.left   += cxColumn;
				rect.right  += cxColumn;
			}
			else{
				rect.top     += cyRow;
				rect.bottom  += cyRow;
			}
		}
	}

	// Draw a frame.
	{
		CPen	pen( PS_SOLID, 3, crFore );
		pDC->SelectObject( &pen );
		pDC->MoveTo( pRect->left,    pRect->top );
		pDC->LineTo( pRect->right-1, pRect->top );
		pDC->LineTo( pRect->right-1, pRect->bottom-1 );
		pDC->LineTo( pRect->left,    pRect->bottom-1 );
		pDC->LineTo( pRect->left,    pRect->top );
	}
	// Draw grids.
	{
		CPen	pen( PS_SOLID, 1, crFore );
		pDC->SelectObject( &pen );
		int	x = pRect->left;
		for	( int ix = 0; ix <= (16+1); ix++ ){
			pDC->MoveTo( x, pRect->top );
			pDC->LineTo( x, pRect->bottom );
			x += cxColumn;
		}
		int	y = pRect->top;
		for	( int iy = 0; iy <= (16+1); iy++ ){
			pDC->MoveTo( pRect->left,  y );
			pDC->LineTo( pRect->right, y );
			y += cyRow;
		}
	}

	font.DeleteObject();
}

CRect
CFontabDlg::DrawFont( CDC* pDC, CRect* pRect, int chCode )
{
	pDC->FillSolidRect( pRect, RGB( 0, 0, 0 ) );

	int	x = pRect->left;
	int	y = pRect->top;
	int	xDiv = pRect->Width()  / m_cxFont;
	int	yDiv = pRect->Height() / m_cyFont;
	int	nDiv = ( xDiv > yDiv )? yDiv: xDiv;

	int	cy = nDiv * m_cyFont;
	int	cx = nDiv * m_cxFont;
	int	dy = pRect->Height() - cy;
	int	dx = pRect->Width()  - cx;
	dy /= 2;
	dx /= 2;

	int	cbChar = m_cbY * m_cbX;
	DWORD	iFont = chCode * cbChar;

	for	( int ix = 0; ix < m_cbX; ix++ ){
		CRect	rect = *pRect;
		rect.left   += ( nDiv * ix ) + dx;
		rect.right   = rect.left + nDiv;
		rect.top     = rect.top + dy;
		rect.bottom  = rect.top  + nDiv;
		for	( int iy = 0; iy < m_cbY; iy++ ){
			BYTE	d = m_pbFont[iFont++];
			for	( int ib = 0; ib < 8; ib++ ){
				if	( ( iy * 8 ) + ib >= m_cyFont )
					break;
				COLORREF	cr =  ( d & (0x01<<ib) )?
						RGB( 255, 255, 255 ): RGB( 0, 0, 0 );
				pDC->FillSolidRect( &rect, cr );
				rect.top    += nDiv;
				rect.bottom += nDiv;
			}
		}
	}

	CPen	penGrid( PS_SOLID, 1, RGB( 100, 100, 100 ) );
	pDC->SelectObject( &penGrid );

	if	( nDiv > 2 ){
		for	( int i = 0; i <= m_cyFont; i++ ){
			pDC->MoveTo( x + dx +  0, y + dy + ( nDiv * i ) );
			pDC->LineTo( x + dx + cx, y + dy + ( nDiv * i ) );
		}
		for	( int i = 0; i <= m_cxFont; i++ ){
			pDC->MoveTo( x + dx + ( nDiv * i ), y + dy  );
			pDC->LineTo( x + dx + ( nDiv * i ), y + dy + cy );
		}
	}

	CRect	rect;
	rect.left   = pRect->left + dx;
	rect.top    = pRect->top  + dy;
	rect.right  = rect.left   + cx;
	rect.bottom = rect.top    + cy;
	
	return	rect;
}

void
CFontabDlg::FlipBit( int x, int y )
{
	CString	strCode;
	GetDlgItem( IDC_EDIT_CODE )->GetWindowText( strCode );
	int	chCode = (BYTE)strtoul( strCode, NULL, 16 );
	int	cbChar = m_cbY * m_cbX;
	DWORD	iFont = chCode * cbChar;
	DWORD	ib = iFont;
	ib += ( x * m_cbY );
	ib += ( y / 8 );

	m_pbFont[ib] ^= 0x01 << ( y % 8 );

	CString	strBytes = (CString)DumpBytes( chCode );
	GetDlgItem( IDC_EDIT_BYTES )->SetWindowText( strBytes );
}

void
CFontabDlg::AllocateBitmap( void )
{
	// Calculate byte counts.

	m_cbY = m_cyFont/8;
	if	( m_cyFont%8 )
		m_cbY++;
	m_cbX = m_cxFont;

	// Allocate the buffer.

	if	( m_pbFont )
		delete[]	m_pbFont;
	DWORD	cbFont = m_cbX * m_cbY * 256;
	m_pbFont = new BYTE[cbFont];
	memset( m_pbFont, 0x00, cbFont );
}

void
CFontabDlg::LoadFile( CString strPath )
{
	CFile	f;
	if	( !f.Open( strPath, CFile::modeRead | CFile::shareDenyNone ) )
		return;

	CString	str;
	int	cyFont = 0,
		cxFont = 0;
	GetDlgItem( IDC_EDIT_HEIGHT )->GetWindowText( str );
	cyFont = atoi( str );
	GetDlgItem( IDC_EDIT_WIDTH  )->GetWindowText( str );
	cxFont = atoi( str );

	QWORD	qwSize = f.GetLength();
	char*	pchTab = new char[qwSize+1];
	f.Read( pchTab, (UINT)qwSize );
	pchTab[qwSize] = '\0';

	char*	pch = pchTab;
	do{
		// Take 'Width' of font.

		for	( ; *pch; pch++ )
			if	( !strncmp( pch, "fontWidth", 9 ) )
				break;
		if	( pch ){
			while	( *pch != '=' )
				pch++;
			pch++;
			int	n = strtol( pch, &pch, 10 );
			if	( n >= 8 && n <= PIXWIDTH )
				m_cxFont = n;
		}
		else
			break;

		// Take 'Height' of font.

		for	( ; *pch; pch++ )
			if	( !strncmp( pch, "fontHeight", 10 ) )
				break;
		if	( pch ){
			while	( *pch != '=' )
				pch++;
			pch++;
			int	n = strtol( pch, &pch, 10 );
			if	( n >= 8 && n <= PIXHEIGHT )
				m_cyFont = n;
		}
		else
			break;

		// Check the font size.

		bool	bStretch = false;
		if	( m_cyFont * 2 == cyFont &&
			  m_cxFont * 2 == cxFont )
			bStretch = true;

		// Set the font size.

		if	( bStretch ){
			m_cyFont = cyFont;
			m_cxFont = cxFont;
		}
		else{
			((CSpinButtonCtrl*)GetDlgItem( IDC_SPIN_HEIGHT ))->SetPos( m_cyFont );
			((CSpinButtonCtrl*)GetDlgItem( IDC_SPIN_WIDTH ) )->SetPos( m_cxFont );

			AllocateBitmap();
		}

		m_wndBitmap .Invalidate( TRUE );
		m_wndPreview.Invalidate( TRUE );

		// Load the font bitmap.

		int	cbFont = cyFont * cxFont / 8;
		int	cbFontMag = cbFont * 4;
		int	iByte = 0x00;
		for	( ; *pch; pch++ )
			if	( pch[0] == '0' && pch[1] == 'x' ){
				pch += 2;
				BYTE	b = (BYTE)strtol( pch, &pch, 16 );
				if	( bStretch ){
					{
						int	iBase = ( iByte / cbFontMag ) * cbFontMag;
						int	iOffset = iByte % cbFontMag;

						int	i0 = ( iByte%2 == 0 )? iByte*4: ((iByte-1)*4) +2;
						int	i1 = i0 + (m_cyFont/8);

						for	( int nShift = 0; nShift < 4; nShift++ ){
							if	( b & (0x01 << nShift) )
								m_pbFont[i0+0] |= 0x03 << (nShift*2);
						}
						for	( int nShift = 0; nShift < 4; nShift++ ){
							if	( b & (0x10 << nShift) )
								m_pbFont[i0+1] |= 0x03 << (nShift*2);
						}
						m_pbFont[i1+0] = m_pbFont[i0+0];
						m_pbFont[i1+1] = m_pbFont[i0+1];
						iByte++;
					}
				}
				else
					m_pbFont[iByte++] = b;
			}

		GetDlgItem( IDC_EDIT_CHAR )->SetWindowText( _T("") );
		OnChangeCode( IDC_EDIT_CODE );
		CreatePreview();

	}while	( false );

	delete[]	pchTab;
}

void
CFontabDlg::SaveFile( CString strPath )
{
	CStringA strLines;
	CStringA str;

	str.Format( "\tstatic\tint\tfontWidth  = %2d;\n", m_cxFont );
	strLines += str;
	str.Format( "\tstatic\tint\tfontHeight = %2d;\n", m_cyFont );
	strLines += str;
	str.Format( "\tstatic\tBYTE\tfontab[] ={\n" );
	strLines += str;

	for	( int chCode = 0x00; chCode <= 0xff; chCode++ ){
		strLines += "\t";
		strLines += DumpBytes( chCode );
		strLines += "\n";
	}
	strLines += "\t};\n";

	CFile	f;
	if	( f.Open( strPath, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyNone ) )
		f.Write( strLines.GetBuffer(), strLines.GetLength() );
}

CStringA
CFontabDlg::DumpBytes( int chCode )
{
	CStringA str, strLines;

	int	cbChar = m_cbY * m_cbX;
	DWORD	iFont = chCode * cbChar;
	for	( int x = 0; x < cbChar; x++ ){
		str.Format( "0x%02x,", m_pbFont[iFont] );
		strLines += str;
		iFont++;
	}
	strLines += "//";
	if	( chCode >= 0x20 && chCode <= 0x7e )
		str.Format( "'%c'", (char)chCode );
	else
		str.Format( "\\x%02x", chCode );
	strLines += str;

	return	strLines;
}

CString
CFontabDlg::DecBin( int iValue, int nDigit )
{
	CString	strBin;
	for	( --nDigit; nDigit >= 0; nDigit-- )
		strBin += ( iValue & ( 0x01 << nDigit ) )? _T("1"): _T("0");

	return	strBin;
}
