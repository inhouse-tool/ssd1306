// FontabApp.h : main header file for the Fontab application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"

class CFontabApp : public CWinApp
{
protected:
	virtual	BOOL	InitInstance( void );
};

extern	CFontabApp	theApp;
