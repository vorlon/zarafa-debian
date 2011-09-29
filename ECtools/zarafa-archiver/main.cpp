/*
 * Copyright 2005 - 2011  Zarafa B.V.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3, 
 * as published by the Free Software Foundation with the following additional 
 * term according to sec. 7:
 *  
 * According to sec. 7 of the GNU Affero General Public License, version
 * 3, the terms of the AGPL are supplemented with the following terms:
 * 
 * "Zarafa" is a registered trademark of Zarafa B.V. The licensing of
 * the Program under the AGPL does not imply a trademark license.
 * Therefore any rights, title and interest in our trademarks remain
 * entirely with us.
 * 
 * However, if you propagate an unmodified version of the Program you are
 * allowed to use the term "Zarafa" to indicate that you distribute the
 * Program. Furthermore you may use our trademarks where it is necessary
 * to indicate the intended purpose of a product or service provided you
 * use it in accordance with honest practices in industrial or commercial
 * matters.  If you want to propagate modified versions of the Program
 * under the name "Zarafa" or "Zarafa Server", you may only do so if you
 * have a written permission by Zarafa B.V. (to acquire a permission
 * please contact Zarafa at trademark@zarafa.com).
 * 
 * The interactive user interface of the software displays an attribution
 * notice containing the term "Zarafa" and/or the logo of Zarafa.
 * Interactive user interfaces of unmodified and modified versions must
 * display Appropriate Legal Notices according to sec. 5 of the GNU
 * Affero General Public License, version 3, when you propagate
 * unmodified or modified versions of the Program. In accordance with
 * sec. 7 b) of the GNU Affero General Public License, version 3, these
 * Appropriate Legal Notices must retain the logo of Zarafa or display
 * the words "Initial Development by Zarafa" if the display of the logo
 * is not reasonably feasible for technical reasons. The use of the logo
 * of Zarafa in Legal Notices is allowed for unmodified and modified
 * versions of the software.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include "platform.h"
#include "my_getopt.h"
#include "stringutil.h"
#include "ECConfig.h"
#include <locale.h>
#include <iostream>
using namespace std;

#include <initguid.h>
#include "archiver.h"

enum modes {
	MODE_INVALID = 0, 
	MODE_ATTACH, 
	MODE_DETACH,
	MODE_DETACH_IDX,
	MODE_LIST,
	MODE_LIST_ARCHUSER,
	MODE_ARCHIVE,
	MODE_CLEANUP
};

/**
 * Print a help message.
 *
 * @param[in]	ostr
 *					std::ostream where the message is written to.
 * @param[in]	name
 *					The name of the application to use in the output.
 */
void print_help(ostream &ostr, const char *name) 
{
	ostr << endl;
	ostr << "Usage:" << endl;
	ostr << name << " [options]" << endl << endl;
	ostr << "Options:" << endl;
	ostr << "  -u <name>                        : Select user" << endl;
	ostr << "  -l|--list                        : List archives for the specified user" << endl;
	ostr << "  -L|--list-archiveusers           : List users that have an archived attached" << endl;
	ostr << "  -A|--archive                     : Perform archive operation" << endl;
	ostr << "                                     If no user is specified all user stores will" << endl;
	ostr << "                                     be archived." << endl;
	ostr << "  -C|--cleanup                     : Perform a cleanup of the archive stores attached" << endl;
	ostr << "                                     to the user specified with -u. If no user is" << endl;
	ostr << "                                     specified, all archives are cleanedup." << endl;
	ostr << "     --local-only                  : Archive or cleanup only those users that have" << endl;
	ostr << "                                     their store on the server on which the archiver" << endl;
	ostr << "                                     is invoked." << endl;
	ostr << "  -a|--attach-to <archive store>   : Attach an archive to the specified user." << endl;
	ostr << "                                     By default a subfolder will be created with" << endl;
	ostr << "                                     the same name as the specified user. This" << endl;
	ostr << "                                     folder will be the root of the archive." << endl;
	ostr << "  -d|--detach-from <archive store> : Detach an archive from the specified user." << endl;
	ostr << "                                     If a user has multiple archives in the same" << endl;
	ostr << "                                     archive store, the folder needs to be" << endl;
	ostr << "                                     specified with --archive-folder." << endl;
	ostr << "  -D|--detach <archive no>         : Detach the archive specified by archive no. This" << endl;
	ostr << "                                     number can be found by running zarafa-archiver -l" << endl;
	ostr << "  -f|--archive-folder <name>       : Specify an alternate name for the subfolder" << endl;
	ostr << "                                     that acts as the root of the archive." << endl;
	ostr << "  -s|--archive-server <path>       : Specify the server on which the archive should" << endl;
	ostr << "                                     be found." << endl;
	ostr << "  -N|--no-folder                   : Don't use a subfolder that acts as the root" << endl;
	ostr << "                                     of the archive. This implies that only one" << endl;
	ostr << "                                     archive can be made in the specified archive" << endl;
	ostr << "                                     store." << endl;
	ostr << "  -w|--writable                    : Grant write permissions on the archive." << endl;
	ostr << "  -c|--config                      : Use alternate config file." << endl;
	ostr << "                                     Default: archiver.cfg" << endl;
	ostr << "     --help                        : Show this help message." << endl;
	ostr << endl;
}

/**
 * Print an error message when multiple operational modes are selected by the user.
 *
 * @param[in]	modeSet
 *					The currently set mode.
 * @param[in]	modeReq
 *					The requested mode.
 * @param[in]	name
 *					The name of the application.
 *
 * @todo	Make a nicer message about what went wrong based on modeSet and modeReq.
 */
void print_mode_error(modes modeSet, modes modeReq, const char *name)
{
	cerr << "Cannot select more than one mode!" << endl;
	print_help(cerr, name);
}

enum cmdOptions {
	OPT_USER = 129,
	OPT_ATTACH,
	OPT_DETACH,
	OPT_DETACH_IDX,
	OPT_FOLDER,
	OPT_ASERVER,
	OPT_NOFOLDER,
	OPT_LIST,
	OPT_LIST_ARCHUSER,
	OPT_ARCHIVE,
	OPT_CLEANUP,
	OPT_CONFIG,
	OPT_LOCAL,
	OPT_WRITABLE,
	OPT_HELP
};

struct option long_options[] = {
		{ "user", 			required_argument,	NULL, OPT_USER		},
		{ "attach-to",		required_argument,	NULL, OPT_ATTACH	},
		{ "detach-from",	required_argument,	NULL, OPT_DETACH	},
		{ "detach",			required_argument,	NULL, OPT_DETACH_IDX },
		{ "archive-folder",	required_argument,	NULL, OPT_FOLDER	},
		{ "archive-server", required_argument,  NULL, OPT_ASERVER	},
		{ "no-folder",		no_argument,		NULL, OPT_NOFOLDER	},
		{ "list",			no_argument,		NULL, OPT_LIST		},
		{ "list-archiveusers", no_argument,		NULL, OPT_LIST_ARCHUSER },
		{ "archive",		no_argument,		NULL, OPT_ARCHIVE	},
		{ "cleanup",		no_argument,		NULL, OPT_CLEANUP	},
		{ "local-only",		no_argument,		NULL, OPT_LOCAL		},
		{ "help", 			no_argument, 		NULL, OPT_HELP		},
		{ "config", 		required_argument, 	NULL, OPT_CONFIG 	},
		{ "writable",		no_argument,		NULL, OPT_WRITABLE	},
		{ NULL, 			no_argument, 		NULL, 0				}
};

/**
 * In some programming languages, the main function is where a program starts execution.
 *
 * It is generally the first user-written function run when a program starts (some system-specific software generally runs
 * before the main function), though some languages (notably C++ with global objects that have constructors) can execute user-written
 * functions before main runs. The main function usually organizes at a high level the functionality of the rest of the program. The
 * main function typically has access to the command arguments given to the program at the command-line interface.
 */
int main(int argc, char *argv[])
{
	eResult	r = Success;

	modes mode = MODE_INVALID;
	const char *lpszUser = NULL;
	const char *lpszArchive = NULL;
	unsigned int ulArchive = 0;
	const char *lpszFolder = NULL;
	const char *lpszArchiveServer = NULL;
	bool bLocalOnly = false;
	unsigned ulAttachFlags = 0;
	ArchiverPtr ptrArchiver;
	
    ULONG ulFlags = 0;
    
	const char *lpszConfig = Archiver::GetConfigPath();

	setlocale(LC_CTYPE, "");

	int c;
	while (1) {
		c = my_getopt_long(argc, argv, "u:c:lLACwa:d:D:f:s:N", long_options, NULL);
		if (c == -1)
			break;
		switch (c) {
		case 'u':
		case OPT_USER:
			lpszUser = my_optarg;
			break;

		case 'a':
		case OPT_ATTACH:
			if (mode != MODE_INVALID) {
				print_mode_error(mode, MODE_ATTACH, argv[0]);
				return 1;
			}
			mode = MODE_ATTACH;
			lpszArchive = my_optarg;
			break;

		case 'd':
		case OPT_DETACH:
			if (mode != MODE_INVALID) {
				print_mode_error(mode, MODE_DETACH, argv[0]);
				return 1;
			}
			mode = MODE_DETACH;
			lpszArchive = my_optarg;
			break;

		case 'D':
		case OPT_DETACH_IDX: {
			char *res = NULL;
			
			if (mode != MODE_INVALID) {
				print_mode_error(mode, MODE_DETACH_IDX, argv[0]);
				return 1;
			}
			mode = MODE_DETACH_IDX;
			ulArchive = strtoul(my_optarg, &res, 10);
			if (!res || *res != '\0') {
				cerr << "Please specify a valid archive number." << endl;
				return 1;
			}
		} break;

		case 'f':
		case OPT_FOLDER:
			if ((ulAttachFlags & ArchiveManage::UseIpmSubtree)) {
				cerr << "You cannot mix --archive-folder and --nofolder." << endl;
				print_help(cerr, argv[0]);
				return 1;
			}
			lpszFolder = my_optarg;
			break;

		case 's':
		case OPT_ASERVER:
			lpszArchiveServer = my_optarg;
			break;

		case 'N':
		case OPT_NOFOLDER:
			if (lpszFolder) {
				cerr << "You cannot mix --archive-folder and --nofolder." << endl;
				print_help(cerr, argv[0]);
				return 1;
			}
			ulAttachFlags |= ArchiveManage::UseIpmSubtree;
			break;
			
		case 'l':
		case OPT_LIST:
			if (mode != MODE_INVALID) {
				print_mode_error(mode, MODE_ATTACH, argv[0]);
				return 1;
			}
			mode = MODE_LIST;
			break;

		case 'L':
		case OPT_LIST_ARCHUSER:
			if (mode != MODE_INVALID) {
				print_mode_error(mode, MODE_ATTACH, argv[0]);
				return 1;
			}
			mode = MODE_LIST_ARCHUSER;
			break;

		case 'A':
		case OPT_ARCHIVE:
			if (mode != MODE_INVALID) {
				print_mode_error(mode, MODE_ATTACH, argv[0]);
				return 1;
			}
			mode = MODE_ARCHIVE;
			break;

		case 'C':
		case OPT_CLEANUP:
			if (mode != MODE_INVALID) {
				print_mode_error(mode, MODE_CLEANUP, argv[0]);
				return 1;
			}
			mode = MODE_CLEANUP;
			break;

		case OPT_LOCAL:
			bLocalOnly = true;
			break;
			
		case 'c':
		case OPT_CONFIG:
			lpszConfig = my_optarg;
			ulFlags |= Archiver::RequireConfig;
			break;
			
		case 'w':
		case OPT_WRITABLE:
			ulAttachFlags |= ArchiveManage::Writable;
			break;
			
		case OPT_HELP:
			print_help(cout, argv[0]);
			return 1;

		// error handling?
		case '?':
			break;
		default:
			break;
		};
	}	
	
	if (mode == MODE_INVALID) {
		cerr << "Nothing to do!" << endl;
		print_help(cerr, argv[0]);
		return 1;
	}
	
	else if (mode == MODE_ATTACH) {
		if (lpszUser == NULL || *lpszUser == '\0') {
			cerr << "Username cannot be empty" << endl;
			print_help(cerr, argv[0]);
			return 1;
		}
		
		if (lpszFolder != NULL && *lpszFolder == '\0')
			lpszFolder = NULL;
	}
	
	else if (mode == MODE_DETACH) {
		if (lpszUser == NULL || *lpszUser == '\0') {
			cerr << "Username cannot be empty" << endl;
			print_help(cerr, argv[0]);
			return 1;
		}
		
		if (lpszFolder != NULL && *lpszFolder == '\0')
			lpszFolder = NULL;
	}
	
	else if (mode == MODE_LIST) {
		if (lpszUser == NULL || *lpszUser == '\0') {
			cerr << "Username cannot be empty" << endl;
			print_help(cerr, argv[0]);
			return 1;
		}
	}
	
	else if (mode == MODE_ARCHIVE) {
		// Needs no arguments
	}


	r = Archiver::Create(&ptrArchiver);
	if (r != Success) {
		cerr << "Failed to instantiate archiver object" << endl;
		return 1;
	}

	if (mode != MODE_ARCHIVE)
		ulFlags |= Archiver::AttachStdErr;
		
	r = ptrArchiver->Init(argv[0], lpszConfig, ulFlags);
	if (r == FileNotFound) {
		cerr << "Unable to open configuration file " << lpszConfig << endl;
		return 1;
	}
	else if (r != Success) {
		cerr << "Failed to initialize" << endl;
		return 1;
	}
	
	
	switch (mode) {
	case MODE_ATTACH: {
		ArchiveManagePtr ptr;
		r = ptrArchiver->GetManage(lpszUser, &ptr);
		if (r != Success)
			goto exit;
			
		r = ptr->AttachTo(lpszArchiveServer, lpszArchive, lpszFolder, ulAttachFlags);
	} break;

	case MODE_DETACH_IDX:
	case MODE_DETACH: {
		ArchiveManagePtr ptr;
		r = ptrArchiver->GetManage(lpszUser, &ptr);
		if (r != Success)
			goto exit;

		if (mode == MODE_DETACH_IDX)
			r = ptr->DetachFrom(ulArchive);
		else
			r = ptr->DetachFrom(lpszArchiveServer, lpszArchive, lpszFolder);
	} break;
	
	case MODE_LIST: {
		ArchiveManagePtr ptr;
		r = ptrArchiver->GetManage(lpszUser, &ptr);
		if (r != Success)
			goto exit;
			
		r = ptr->ListArchives(cout);
	} break;

	case MODE_LIST_ARCHUSER: {
		ArchiveManagePtr ptr;
		r = ptrArchiver->GetManage("SYSTEM", &ptr);
		if (r != Success)
			goto exit;
			
		r = ptr->ListAttachedUsers(cout);
	} break;
	
	case MODE_ARCHIVE: {
		ArchiveControlPtr ptr;
		r = ptrArchiver->GetControl(&ptr);
		if (r != Success)
			goto exit;
			
		if (lpszUser)
			r = ptr->Archive(lpszUser);
		else
			r = ptr->ArchiveAll(bLocalOnly);
	} break;
		
	case MODE_CLEANUP: {
		ArchiveControlPtr ptr;
		r = ptrArchiver->GetControl(&ptr);
		if (r != Success)
			goto exit;
			
		if (lpszUser)
			r = ptr->Cleanup(lpszUser);
		else
			r = ptr->CleanupAll(bLocalOnly);
	} break;
		
	case MODE_INVALID:
		break;
	}

exit:
	return (r == Success ? 0 : 1);
}
