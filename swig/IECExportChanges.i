%{
#include "IECExportChanges.h"
%}

class IECExportChanges : public IUnknown{
public:
	virtual HRESULT GetChangeCount(ULONG *lpcChanges) = 0;
	virtual HRESULT SetMessageInterface(IID refiid) = 0;
#ifndef WIN32
	virtual HRESULT SetLogger(ECLogger *lpLogger) = 0;
#endif
	%extend {
		~IECExportChanges() { self->Release(); };
	}
};
