/* File : archiver.i */
%module archiver

%{
	#include "../../ECtools/zarafa-archiver/archiver.h"

	class ArchiverError : public std::runtime_error {
	public:
		ArchiverError(eResult e, const std::string &message)
		: std::runtime_error(message)
		, m_e(e)
		{ }

		eResult result() const { return m_e; }

	private:
		eResult m_e;
	};
%}

%include "exception.i"
%exception
{
    try {
   		$action
    } catch (const ArchiverError &ae) {
		SWIG_exception(SWIG_RuntimeError, ae.what());
	}
}

#if SWIGPYTHON
%include "python/archiver_python.i"
#endif

class ArchiveControl {
public:
	%extend {
		void ArchiveAll(bool bLocalOnly) {
			eResult e = Success;

			e = self->ArchiveAll(bLocalOnly);
			if (e != Success)
				throw ArchiverError(e, "Method returned an error!");
		}
		
		void Archive(const char *lpszUser) {
			eResult e = Success;

			e = self->Archive(lpszUser);
			if (e != Success)
				throw ArchiverError(e, "Method returned an error!");
		}

		void CleanupAll(bool bLocalOnly) {
			eResult e = Success;

			e = self->CleanupAll(bLocalOnly);
			if (e != Success)
				throw ArchiverError(e, "Method returned an error!");
		}
		
		void Cleanup(const char *lpszUser) {
			eResult e = Success;

			e = self->Cleanup(lpszUser);
			if (e != Success)
				throw ArchiverError(e, "Method returned an error!");
		}
	}

private:
	ArchiveControl();
};




struct ArchiveEntry {
	std::string StoreName;
	std::string FolderName;
	std::string StoreOwner;
	unsigned Rights;
};
typedef std::list<ArchiveEntry> ArchiveList;

struct UserEntry {
	std::string UserName;
};
typedef std::list<UserEntry> UserList;


class ArchiveManage {
public:
	enum {
		UseIpmSubtree = 1,
		Writable = 2
	};

	%extend {
		void AttachTo(const char *lpszArchiveServer, const char *lpszArchive, const char *lpszFolder, unsigned int ulFlags) {
			eResult e = Success;

			e = self->AttachTo(lpszArchiveServer, lpszArchive, lpszFolder, ulFlags);
			if (e != Success)
				throw ArchiverError(e, "Method returned an error!");
		}
		
		void DetachFrom(const char *lpszArchiveServer, const char *lpszArchive, const char *lpszFolder) {
			eResult e = Success;

			e = self->DetachFrom(lpszArchiveServer, lpszArchive, lpszFolder);
			if (e != Success)
				throw ArchiverError(e, "Method returned an error!");
		}

		void DetachFrom(unsigned int ulArchive) {
			eResult e = Success;

			e = self->DetachFrom(ulArchive);
			if (e != Success)
				throw ArchiverError(e, "Method returned an error!");
		}

		ArchiveList ListArchives(const char *lpszIpmSubtreeSubstitude = NULL) {
			eResult e = Success;
			ArchiveList lst;

			e = self->ListArchives(&lst);
			if (e != Success)
				throw ArchiverError(e, "Method returned an error!");

			return lst;
		}

		UserList ListAttachedUsers() {
			eResult e = Success;
			UserList lst;

			e = self->ListAttachedUsers(&lst);
			if (e != Success)
				throw ArchiverError(e, "Method returned an error!");

			return lst;
		}
	}

private:
	ArchiveManage();
};



class Archiver {
public:
	enum {
		RequireConfig		= 0x00000001,
		AttachStdErr		= 0x00000002,
		InhibitErrorLogging	= 0x40000000
	};

	%extend {
		static Archiver *Create(const char *lpszAppName, const char *lpszConfig, unsigned int ulFlags = 0) {
			eResult r = Success;
			ArchiverPtr ptr;

			r = Archiver::Create(&ptr);
			if (r != Success)
				throw ArchiverError(r, "Failed to instantiate object!");

			r = ptr->Init(lpszAppName, lpszConfig, NULL, ulFlags);
			if (r != Success)
				throw ArchiverError(r, "Failed to initialize object!");

			return ptr.release();
		}

		ArchiveControl *GetControl() {
			eResult r = Success;
			ArchiveControlPtr ptr;

			r = self->GetControl(&ptr);
			if (r != Success)
				throw ArchiverError(r, "Failed to get object!");

			return ptr.release();
		}
		
		ArchiveManage *GetManage(const char *lpszUser) {
			eResult r = Success;
			ArchiveManagePtr ptr;

			r = self->GetManage(lpszUser, &ptr);
			if (r != Success)
				throw ArchiverError(r, "Failed to get object!");

			return ptr.release();
		}
    }

private:
	Archiver();
};
