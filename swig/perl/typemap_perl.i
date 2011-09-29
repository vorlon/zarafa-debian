// PERL
%{
#include "conversion.h"
%}

%include "perl/wstrings.swg"

%fragment("SWIG_FromBytePtrAndSize","header",fragment="SWIG_FromCharPtrAndSize") {
SWIGINTERNINLINE SV *
SWIG_FromBytePtrAndSize(const unsigned char* carray, size_t size)
{
	return SWIG_FromCharPtrAndSize(reinterpret_cast<const char *>(carray), size);
}
}

// Input
%typemap(in) 				(ULONG, MAPIARRAY)
{
	STRLEN len;
	if(!SvOK($input)) {
		$2 = NULL;
		$1 = 0;
	} else {
		$2 = AV_to$2_mangle((AV *)SvRV($input), &len);
		$1 = len;
	}
}

%typemap(in)				MAPILIST
{
	if(!SvOK($input)) {
		$1 = NULL;
	} else {
		$1 = AV_to$mangle((AV *)SvRV($input));
	}
}

%typemap(in)				MAPISTRUCT
{
	if(!SvOK($input)) {
		$1 = NULL;
	} else {
		$1 = HV_to$mangle((HV *)SvRV($input));
	}
}

// Output
%typemap(argout)	MAPILIST *
{
  %append_output(sv_2mortal(newRV_noinc((SV *)AV_from$*mangle(*($1)))));
}

%typemap(argout)	MAPISTRUCT *
{
  %append_output(sv_2mortal(newRV_noinc((SV *)HV_from$*mangle(*($1)))));
}

%typemap(argout)	(ULONG *, MAPIARRAY *)
{
  %append_output(sv_2mortal(newRV_noinc((SV *)AV_from$*2_mangle(*($2),*($1)))));
}

////////////////////////////////
// HRESULT / exception handling
////////////////////////////////

/*
 * We override the standard exception handling by using Error::Simple
 * because it is much more useful and flexible than the standard exception
 * handling in perl. This means that any program using the MAPI module must
 * use Error::Simple for exception handling via try / catch
 */

%{
void Do_Exception(HRESULT hr)
{
  int count;
  SV *exp;

  // GO WEIRD PERL STACK FIDDLING
  dSP;	

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  XPUSHs(sv_2mortal(newSVpv("MAPI::Exception", 0)));
  XPUSHs(sv_2mortal(newSVuv(hr)));
  PUTBACK;
  count = perl_call_pv("MAPI::Exception::new", G_SCALAR);
  SPAGAIN;

  exp = POPs;

  PUSHMARK(SP);
  XPUSHs(exp);
  PUTBACK;
  perl_call_pv("Error::throw", G_DISCARD);

  FREETMPS;
  LEAVE;
}

%}

%typemap(out) HRESULT
{
  if(FAILED($1)) {
    Do_Exception($1);
  }
}

// Unicode

/////////////////////////////////
// LPTSTR
/////////////////////////////////

// Input

// Defer to 'CHECK' stage
%typemap(in) LPTSTR
{
  $1 = (LPTSTR)$input;
}

%typemap(check) LPTSTR (std::string strInput)
{
	SV *entry = (SV*)$1;
	if (!SvOK(entry))
		$1 = NULL;
	else {
		if (!SvUTF8(entry) && !(ulFlags & MAPI_UNICODE))
			$1 = (LPTSTR)SvPV_nolen(entry);
		
		else {
			const char *lpszFrom = "";
			if (SvUTF8(entry))
				lpszFrom = "UTF-8";
			const char *lpszTo = (ulFlags & MAPI_UNICODE ? "WCHAR_T" : "//TRANSLIT");
		
			STRLEN len = 0;
			const char *lpsz = SvPV(entry, len);
			strInput.assign(convert_to<std::string>(lpszTo, lpsz, len, lpszFrom));
		
			$1 = (LPTSTR)strInput.c_str();
		}
	}
}

%typemap(freearg) LPTSTR
{
}
