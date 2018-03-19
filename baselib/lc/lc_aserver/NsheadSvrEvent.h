

#ifndef NSHEADSERVERSOCKEVENT_H_
#define NSHEADSERVERSOCKEVENT_H_

#include <lc/lcevent.h>
#include "LcAServer.h"
#include "NsheadEvent.h"
#include <Lsc/containers/string/Lsc_string.h>

namespace lc {
class NsheadSvrEvent : plclic NsheadEvent {		  
protected :
	LcAServer * _fserver;		  
plclic :
	
	virtual void read_done_callback();
	
	virtual void write_done_callback();
	
	virtual int activating_session();
	

	
	virtual int session_done();
	
	LcAServer * get_server();
	
	void set_server(LcAServer * svr);

	
	virtual void set_tag(void * t) {
		_ftag = t;
		set_server((LcAServer *)t);
	}

	
	virtual int if_post() {
		if (NULL == _fserver) {
			return 0;
		}
		if (0 == _fserver->is_running()) {
			return 0;
		}
		return 1;
	}
	
	NsheadSvrEvent(): _fserver(NULL) {}
};

}

#endif 
