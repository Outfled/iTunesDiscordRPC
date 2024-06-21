#pragma once

#ifndef AFX_ITUNESEVENTHANDLER_H__42970B2C_41F2_4339_8DCC_9AC7D5550402__INCLUDED_
#define AFX_ITUNESEVENTHANDLER_H__42970B2C_41F2_4339_8DCC_9AC7D5550402__INCLUDED_

#include "include/iTunes/Interface.h"

/* Global Pointer to the Active iTunes-Application Interface Object */
extern IiTunes *g_ItunesConnection;

/* iTunes Application Event Class */
class CiTunesEvents : public CCmdTarget
{
public:
	CiTunesEvents();
	virtual ~CiTunesEvents();

	//
	// CCmdTarget Implementation
	//

	virtual VOID OnFinalRelease() {
		return CCmdTarget::OnFinalRelease();
	}
	virtual LPUNKNOWN GetInterfaceHook(LPCVOID ppIID) {
		return ((*((IID *)ppIID)) == DIID__IiTunesEvents) ? this->GetIDispatch(FALSE) : NULL;
	}


protected:
	//
	// IiTunes Event Methods
	//

DECLARE_MESSAGE_MAP()
	afx_msg VOID OnDatabaseChangedEvent( const VARIANT FAR &, const VARIANT FAR & );
	afx_msg VOID OnPlayerPlayEvent( const VARIANT FAR & );
	afx_msg VOID OnPlayerStopEvent( const VARIANT FAR & );
	afx_msg VOID OnPlayerPlayingTrackChangedEvent( const VARIANT FAR & );
	afx_msg VOID OnUserInterfaceEnabledEvent();
	afx_msg VOID OnCOMCallsDisabledEvent( SHORT );
	afx_msg VOID OnCOMCallsEnabled();
	afx_msg VOID OnQuittingEvent();
	afx_msg VOID OnAboutToPromptUserToQuitEvent();
	afx_msg VOID OnSoundVolumeChangedEvent( LONG );
DECLARE_DISPATCH_MAP()
DECLARE_INTERFACE_MAP()		
};

#endif//!AFX_ITUNESEVENTHANDLER_H__42970B2C_41F2_4339_8DCC_9AC7D5550402__INCLUDED_