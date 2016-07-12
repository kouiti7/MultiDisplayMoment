
#pragma once
#define __TASKTRAYICON_H__

#include <windows.h>
#include <shellapi.h>

#define WM_APP_TASKTRAY ( WM_APP + 1 )

class CTaskTrayIcon
{
public:
			CTaskTrayIcon( HWND hwnd = NULL, UINT id = 0, UINT msg = 0, HICON icon = NULL, LPCTSTR tip = NULL );
			~CTaskTrayIcon();
	bool	Create( HWND hwnd, UINT id, UINT msg = 0, HICON icon = NULL, LPCTSTR tip = NULL );
	bool	Add();
	bool	Restore();
	bool	Delete();
	bool	Modify();
	bool	SetMessage( UINT msg, BOOL refresh = TRUE );
	HICON	SetIcon( HICON icon, BOOL refresh = TRUE );
	bool	SetTip( LPCTSTR tip, BOOL refresh = TRUE );
	UINT	GetRestartMessage() { return m_show ? m_restartMsg : 0; }
	void	ExplorerRestartCheck( UINT message );

protected:
	NOTIFYICONDATA	m_data;
	bool			m_show;
	UINT			m_restartMsg;
};
