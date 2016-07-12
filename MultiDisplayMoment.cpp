
#include <windows.h>
#include <map>

#include "tasktrayicon.h"
#include "resource.h"

using namespace std;

#define WINDOW_CLASS "MultiDisplayMoment"

static HINSTANCE	inst = NULL;
static HWND			hwnd = NULL;
static RECT			allRect;
static HMONITOR		lastMon = NULL;
static HHOOK		llmHook = NULL;
static map< HMONITOR, RECT > monitors;
//static POINT		offsetPt;	// monitor rect over value
static double		offsetX, offsetY;
static int			margin = 300;
static double		accadj = 1.0;
static CTaskTrayIcon ticon;

void
PtClipRect( RECT &rc, POINT &pt )
{
	pt.x = __min( __max( pt.x, rc.left ), rc.right - 1 );
	pt.y = __min( __max( pt.y, rc.top ), rc.bottom - 1 );
}

LRESULT CALLBACK
LowLevelMouseProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	if( nCode != HC_ACTION  ||  wParam != WM_MOUSEMOVE )
		return ::CallNextHookEx( llmHook, nCode, wParam, lParam );

	MSLLHOOKSTRUCT &mll = *(MSLLHOOKSTRUCT *)lParam;
	RECT &rc = monitors[ lastMon ];

	POINT virtualPt = mll.pt;
	virtualPt.x += (int)offsetX;
	virtualPt.y += (int)offsetY;
	PtClipRect( allRect, virtualPt );
	if( PtInRect( &rc, virtualPt ) )
	{
		offsetX = offsetY = 0;
		return ::CallNextHookEx( llmHook, nCode, wParam, lParam );
	}

	// rect over calc
#if 0
	if( virtualPt.x < rc.left )
		offsetX += mll.pt.x - rc.left;
	else if( virtualPt.x > rc.right - 1 )
		offsetX += mll.pt.x - ( rc.right - 1 );

	if( virtualPt.y < rc.top )
		offsetY += mll.pt.y - rc.top;
	else if( virtualPt.y > rc.bottom - 1 )
		offsetY += mll.pt.y - ( rc.bottom - 1 );
#elif 0

	int n;
	
	if( virtualPt.x < rc.left )
	{
		n = mll.pt.x - rc.left;
		if( n < -1 )
			n = -(int)sqrt( -n );
		offsetX += n;
	}
	else if( virtualPt.x > rc.right - 1 )
	{
		n = mll.pt.x - ( rc.right - 1 );
		if( n > 1 )
			n = (int)sqrt( n );
		offsetX += n;
	}

	if( virtualPt.y < rc.top )
	{
		n = mll.pt.y - rc.top;
		if( n < -1 )
			n = -(int)sqrt( -n );
		offsetY += n;
	}
	else if( virtualPt.y > rc.bottom - 1 )
	{
		n = mll.pt.y - ( rc.bottom - 1 );
		if( n < -1 )
			n = -(int)sqrt( -n );
		offsetY += n;
	}
#else

#define adj( a, b ) __max( 0, pow( (a), (b) ) )
//#define adj( pos, acc ) __max( 0, log(acc) / log( __max(0,pos) * 2.0 ) )

	if( mll.pt.x <= rc.left )
		offsetX -= adj( rc.left - mll.pt.x, accadj );
	else if( mll.pt.x >= rc.right - 1 )
		offsetX += adj( mll.pt.x + rc.right + 1, accadj );
	else
		offsetX = 0;

	if( mll.pt.y <= rc.top )
		offsetY -= adj( rc.top - mll.pt.y, accadj );
	else if( mll.pt.y >= rc.bottom - 1 )
		offsetY += adj( mll.pt.y + rc.bottom + 1, accadj );
	else
		offsetY = 0;

#endif

	// threshold over
	if( abs( offsetX ) > margin  ||  abs( offsetY ) > margin )
	{
		lastMon = MonitorFromPoint( mll.pt, MONITOR_DEFAULTTONEAREST );
		offsetX = offsetY = 0;
		return ::CallNextHookEx( llmHook, nCode, wParam, lParam );
	}

	// cursor position clip
	PtClipRect( rc, virtualPt );
	SetCursorPos( virtualPt.x, virtualPt.y );

	return TRUE; // mouse move event cancel
}

BOOL CALLBACK
MonitorEnumProc( HMONITOR h, HDC hdc, LPRECT rc, LPARAM lp )
{
	monitors[ h ] = *rc;
	UnionRect( &allRect, &allRect, rc );
	return TRUE;
}

void
LoadIni()
{
	char inipath[MAX_PATH];
	GetModuleFileName( NULL, inipath, MAX_PATH );
	strrchr( inipath, '\\' )[1] = 0;
	strcat_s( inipath, "setting.ini" );

	margin = GetPrivateProfileInt( "global", "margin", 300, inipath );

	char buf[256];
	GetPrivateProfileString( "global", "accelerated_adjustment", "1.0", buf, 256, inipath );
	accadj = atof( buf );
}

void
OnContextMenu()
{
	POINT pt;
	GetCursorPos( &pt );

	SetForegroundWindow( hwnd );
	HMENU menu, submenu;
	menu = LoadMenu( inst, MAKEINTRESOURCE( IDR_MENU ) );
	submenu = GetSubMenu( menu, 0 );
	TrackPopupMenu( submenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL );
	DestroyMenu( menu );
}

LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int c = 0;

	switch( message ) 
	{
	case WM_COMMAND:
		if( LOWORD(wParam) == ID_APP_EXIT )
		{
			DestroyWindow( hwnd );
			return 0;
		}
		break;

	case WM_APP_TASKTRAY:
		OnContextMenu();
		return 0;

	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;
   }

	return DefWindowProc( hWnd, message, wParam, lParam );
}

bool
CreateDummyWindow()
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= inst;
	wcex.hIcon			= LoadIcon(inst, (LPCTSTR)IDI_APP);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= "";
	wcex.lpszClassName	= WINDOW_CLASS;
	wcex.hIconSm		= LoadIcon(inst, (LPCTSTR)IDI_APP);
	RegisterClassEx( &wcex );

	hwnd = CreateWindowEx( WS_EX_TOOLWINDOW, WINDOW_CLASS, WINDOW_CLASS, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, inst, NULL );
	return hwnd != NULL;
}

int WINAPI
WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	inst = hInstance;
	if( !CreateDummyWindow() )
		return 1;

	LoadIni();
	ticon.Create( hwnd, 0, WM_APP_TASKTRAY, LoadIcon(inst, (LPCTSTR)IDI_APP), WINDOW_CLASS );
	ticon.Add();

	EnumDisplayMonitors( NULL, NULL, MonitorEnumProc, 0 );
	POINT pt;
	GetCursorPos( &pt );
	lastMon = MonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST );

	llmHook = SetWindowsHookEx( WH_MOUSE_LL, (HOOKPROC)LowLevelMouseProc, hInstance, 0 );



	MSG msg;
	while( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	return 0;
}
