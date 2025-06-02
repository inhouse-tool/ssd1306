// FontabApp.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "FontabApp.h"
#include "FontabDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CFontabApp	theApp;

BOOL
CFontabApp::InitInstance( void )
{
	INITCOMMONCONTROLSEX	InitCtrls;
	InitCtrls.dwSize = sizeof( InitCtrls );
	InitCtrls.dwICC  = ICC_WIN95_CLASSES;
	InitCommonControlsEx( &InitCtrls );

	CWinApp::InitInstance();

	SetRegistryKey( _T("In-house Tool") );

	CFontabDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	return	FALSE;
}

