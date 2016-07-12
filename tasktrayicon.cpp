
#include <windows.h>
#include "tasktrayicon.h"

CTaskTrayIcon::CTaskTrayIcon( HWND hwnd, UINT id, UINT msg, HICON icon, LPCTSTR tip )
{
	m_restartMsg = 0;
	ZeroMemory( &m_data, sizeof( NOTIFYICONDATA ) );
	m_data.cbSize = sizeof( NOTIFYICONDATA );
	m_show = FALSE;
	if( hwnd != NULL )
	{
		Create( hwnd, id, msg, icon, tip );
		Add();
	}
}

CTaskTrayIcon::~CTaskTrayIcon()
{
	if( m_show )
		Delete();
}

bool
CTaskTrayIcon::Create( HWND hwnd, UINT id, UINT msg, HICON icon, LPCTSTR tip )
{
	if( m_show )
		Delete();

	m_data.hWnd = hwnd;
	m_data.uID = id;
	m_data.uFlags = ((msg != 0) ? NIF_MESSAGE : 0) | ((icon != NULL) ? NIF_ICON : 0) | ((tip != NULL) ? NIF_TIP : 0);
	m_data.uCallbackMessage = msg;
	m_data.hIcon = icon;
	if( tip != NULL )
		strcpy_s( m_data.szTip, tip );
	else
		strcpy_s( m_data.szTip, "" );
	m_restartMsg = RegisterWindowMessage( "TaskTrayCreated" );

	return true;
}

bool
CTaskTrayIcon::Restore()
{
	Delete();
	return Add();
}

bool
CTaskTrayIcon::Add()
{
	if( m_show )
		return Modify();

	if( !Shell_NotifyIcon( NIM_ADD, &m_data ) )
		return false;

	m_show = true;
	return true;
}

bool
CTaskTrayIcon::Delete()
{
	m_show = FALSE;
	return Shell_NotifyIcon( NIM_DELETE, &m_data ) != FALSE;
}

bool
CTaskTrayIcon::Modify()
{
	if( m_show )
		return Shell_NotifyIcon( NIM_MODIFY, &m_data ) != FALSE;
	else
		return Add();
}

bool
CTaskTrayIcon::SetMessage( UINT msg, BOOL refresh )
{
	if( msg == 0 )
		m_data.uFlags = m_data.uFlags & ( !NIF_MESSAGE );
	else
	{
		m_data.uFlags |= NIF_MESSAGE;
		m_data.uCallbackMessage = msg;
	}
	if( refresh )
		Modify();
	return true;
}

HICON
CTaskTrayIcon::SetIcon( HICON icon,BOOL refresh )
{
	HICON old = m_data.hIcon;
	if( icon == NULL )
		m_data.uFlags = m_data.uFlags & ( !NIF_ICON );
	else
	{
		m_data.uFlags |= NIF_ICON;
		m_data.hIcon = icon;
	}

	if( refresh )
		Modify();

	return old;
}

bool
CTaskTrayIcon::SetTip(LPCTSTR tip,BOOL refresh)
{
	if( tip == NULL )
		m_data.uFlags = m_data.uFlags & ( !NIF_TIP );
	else
	{
		m_data.uFlags |= NIF_TIP;
		lstrcpy( m_data.szTip, tip );
	}

	if( refresh )
		Modify();

	return true;
}

void
CTaskTrayIcon::ExplorerRestartCheck( UINT message )
{
	if( message == GetRestartMessage() )
		Restore();
}

///////////////////////////////////////////////////////////////////////////
// helper

void
HideMainWnd( HWND hwnd )
{
	LONG exstyle = GetWindowLong( hwnd, GWL_EXSTYLE );
	exstyle = ( exstyle & ~WS_EX_APPWINDOW ) | WS_EX_TOOLWINDOW;
	SetWindowLong( hwnd, GWL_EXSTYLE, exstyle );
}

