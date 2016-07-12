
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
static POINT		offsetPt;	// monitor rect over value
static int			margin = 300;
static CTaskTrayIcon ticon;

#define PtAdd( a, b ) a.x += b.x; a.y += b.y;

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
	PtAdd( virtualPt, offsetPt );
	PtClipRect( allRect, virtualPt );
	if( PtInRect( &rc, virtualPt ) )
	{
		memset( &offsetPt, 0, sizeof( offsetPt ) );
		return ::CallNextHookEx( llmHook, nCode, wParam, lParam );
	}

	// rect over calc
	if( virtualPt.x < rc.left )
		offsetPt.x += mll.pt.x - rc.left;
	else if( virtualPt.x > rc.right - 1 )
		offsetPt.x += mll.pt.x - ( rc.right - 1 );

	if( virtualPt.y < rc.top )
		offsetPt.y += mll.pt.y - rc.top;
	else if( virtualPt.y > rc.bottom - 1 )
		offsetPt.y += mll.pt.y - ( rc.bottom - 1 );

	// threshold over
	if( abs( offsetPt.x ) > margin  ||  abs( offsetPt.y ) > margin )
	{
		lastMon = MonitorFromPoint( mll.pt, MONITOR_DEFAULTTONEAREST );
		memset( &offsetPt, 0, sizeof( offsetPt ) );
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
