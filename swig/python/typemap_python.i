// Python
%{
#include "conversion.h"
#include "director_util.h"

#define SWIG_SHADOW 0
#define SWIG_OWNER SWIG_POINTER_OWN

#define DIRECTORARGOUT(_arg) (__tupleIndex == -1 ? (PyObject*)(_arg) : PyTuple_GetItem((_arg), __tupleIndex++))
%}

%init %{
	Init();
%}

%fragment("SWIG_FromBytePtrAndSize","header",fragment="SWIG_FromCharPtrAndSize") {
SWIGINTERNINLINE PyObject *
SWIG_FromBytePtrAndSize(const unsigned char* carray, size_t size)
{
	return SWIG_FromCharPtrAndSize(reinterpret_cast<const char *>(carray), size);
}
}

// Exceptions
%include "exception.i"
%typemap(out) HRESULT
{
  $result = Py_None;
  Py_INCREF(Py_None);
  if(FAILED($1)) {
	DoException($1);
	SWIG_fail;
  }
}

// Input
%typemap(in) 				(ULONG, MAPIARRAY) ($1_type cArray = 0, $2_type lpArray = NULL)
{
	ULONG len;
	$2 = List_to$2_mangle($input, &len);
	$1 = len;
	if(PyErr_Occurred()) goto fail;
}

%typemap(in)				MAPILIST *INPUT ($*type tmp)
{
	tmp = List_to$*mangle($input);
	if(PyErr_Occurred()) goto fail;
	$1 = &tmp;
}

%typemap(in)				MAPILIST
{
	$1 = List_to$mangle($input);
	if(PyErr_Occurred()) goto fail;
}

%typemap(in)				MAPISTRUCT
{
	$1 = Object_to$mangle($input);
	if(PyErr_Occurred()) goto fail;
}

%typemap(in)				MAPISTRUCT_W_FLAGS
{
	$1 = ($1_type)$input;
}

%typemap(check)				MAPISTRUCT_W_FLAGS
{
	$1 = Object_to$mangle((PyObject*)$1, ulFlags);
	if(PyErr_Occurred()) {
		%argument_fail(SWIG_ERROR,"$type",$symname, $argnum);
	}
}

// Output
%typemap(argout)	MAPILIST *
{
    %append_output(List_from_$basetype(*($1)));
	if(PyErr_Occurred()) goto fail;
}

%typemap(argout)	MAPILIST INOUT
{
	%append_output(List_from_$basetype($1));
	if(PyErr_Occurred()) goto fail;
}

%typemap(argout)	MAPISTRUCT *
{
    %append_output(Object_from_$basetype(*($1)));
	if(PyErr_Occurred()) goto fail;
}

%typemap(argout)	(ULONG *, MAPIARRAY *)
{
    %append_output(List_from_$2_basetype(*($2),*($1)));
	if(PyErr_Occurred()) goto fail;
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

%typemap(check) LPTSTR (std::string strInput, std::wstring wstrInput)
{
  PyObject *o = (PyObject *)$1;
  if(o == Py_None)
    $1 = NULL;
  else {
    if(ulFlags & MAPI_UNICODE) {
	  if(PyUnicode_Check(o)) {
        wstrInput.assign((wchar_t*)PyUnicode_AsUnicode(o), PyUnicode_GetSize(o));
        $1 = (LPTSTR)wstrInput.c_str();
      } else {
        PyErr_SetString(PyExc_RuntimeError, "MAPI_UNICODE flag passed but passed parameter is not a unicode string");
      }
    } else {
      if(PyUnicode_Check(o)) {
        PyErr_SetString(PyExc_RuntimeError, "MAPI_UNICODE flag not passed but passed parameter is a unicode string");
      }
      char *input;
      Py_ssize_t size;

      PyString_AsStringAndSize(o, &input, &size);
      strInput.assign(input, size);

      $1 = (LPTSTR)strInput.c_str();
    }
  }

  if(PyErr_Occurred()) {
    %argument_fail(SWIG_ERROR,"$type",$symname, $argnum);
  }

}

%typemap(freearg) LPTSTR
{
}


// Director stuff
%feature("director:except") {
  if ($error != NULL) {
    HRESULT hr;
    if (GetExceptionError($error, &hr) == 1) {
        PyErr_Clear();
        return hr;	// Early return
    } else {
		if (check_call_from_python() == true)
			throw Swig::DirectorMethodException();	// Let the calling python interpreter handle the exception
		else {
			PyErr_Print();
			PyErr_Clear();
			return MAPI_E_CALL_FAILED;
		}
    }
  }
}

%typemap(directorin)	(ULONG, MAPIARRAY)
{
  $input = List_from_$2_basetype($2_name, $1_name);
  if(PyErr_Occurred())
    %dirout_fail(0, "$type");
}

%typemap(directorin)	MAPILIST
{
  $input = List_from_$1_basetype($1_name);
  if(PyErr_Occurred())
    %dirout_fail(0, "$type");
}

%typemap(directorin,noblock=1) const IID& USE_IID_FOR_OUTPUT
{
  const IID* __iid = &$1_name;
  $input = SWIG_FromCharPtrAndSize((const char *)__iid, sizeof(IID));
}

%typemap(directorargout) void **OUTPUT_USE_IID
{
  int swig_res = SWIG_ConvertPtr(DIRECTORARGOUT($input), $result, TypeFromIID(*__iid), SWIG_POINTER_DISOWN);
  if (!SWIG_IsOK(swig_res)) {
    %dirout_fail(swig_res, "$type");
  }
}

%typemap(directorargout)	MAPICLASS *
{
  int swig_res = SWIG_ConvertPtr(DIRECTORARGOUT($input), (void**)$result, $*1_descriptor, SWIG_POINTER_DISOWN);
  if (!SWIG_IsOK(swig_res)) {
    %dirout_fail(swig_res, "$type");
  }
}

%typemap(directorin)	MAPICLASS
{
  $input = SWIG_NewPointerObj($1_name, $1_descriptor, SWIG_SHADOW | SWIG_OWNER);
}

%typemap(directorin)	(ULONG, BYTE*)
{
  if ($1_name > 0 && $2_name != NULL)
    $input = SWIG_FromCharPtrAndSize((char*)$2_name, $1_name);
}
%apply (ULONG, BYTE*) {(ULONG cbSourceKeySrcFolder, BYTE *pbSourceKeySrcFolder)}
%apply (ULONG, BYTE*) {(ULONG cbSourceKeySrcMessage, BYTE *pbSourceKeySrcMessage)}
%apply (ULONG, BYTE*) {(ULONG cbPCLMessage, BYTE *pbPCLMessage)}
%apply (ULONG, BYTE*) {(ULONG cbSourceKeyDestMessage, BYTE *pbSourceKeyDestMessage)}
%apply (ULONG, BYTE*) {(ULONG cbChangeNumDestMessage, BYTE * pbChangeNumDestMessage)}

%typemap(directorargout)	MAPISTRUCT *
{
  *$result = Object_to_$basetype(DIRECTORARGOUT($input));
  if(PyErr_Occurred()) {
    %dirout_fail(0, "$type");
  }
}

%typemap(directorout,fragment=SWIG_AsVal_frag(long))	HRESULT (Py_ssize_t __tupleIndex)
{
  __tupleIndex = (PyTuple_Check($input) == 0 ? -1 : 0);
  $result = hrSuccess;
}

%apply (ULONG, MAPIARRAY) {(ULONG cElements, LPREADSTATE lpReadState)};
%apply (ULONG, MAPIARRAY) {(ULONG cNotif, LPNOTIFICATION lpNotifications)};
%apply MAPICLASS {IMAPISession *, IProfAdmin *, IMsgServiceAdmin *, IMAPITable *, IMsgStore *, IMAPIFolder *, IMAPITable *, IStream *, IMessage *, IAttach *, IAddrBook *}

%typemap(in) (unsigned int, char **) {
  /* Check if is a list */
  if (PyList_Check($input)) {
    int i;
    $1 = PyList_Size($input);
    $2 = (char **) malloc(($1+1)*sizeof(char *));
    for (i = 0; i < $1; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
	$2[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
	PyErr_SetString(PyExc_TypeError,"list must contain strings");
	free($2);
	return NULL;
      }
    }
    $2[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}

%typemap(freearg) (unsigned int, char **) {
	free((void *)$2);
}
