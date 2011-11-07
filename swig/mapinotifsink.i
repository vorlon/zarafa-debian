// Force constructor on MAPINotifSink even though we're not exposing all abstract
// methods
%feature("notabstract") MAPINotifSink;

class MAPINotifSink : public IMAPIAdviseSink {
public:
	HRESULT GetNotifications(ULONG *OUTPUT, LPNOTIFICATION *OUTPUT, BOOL fNonBlock, ULONG timeout_msec);
	%extend {
		MAPINotifSink() { return new MAPINotifSink(); };
		~MAPINotifSink() { delete self; }
	}
};
