/*
 * Copyright 2005 - 2014  Zarafa B.V.
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
#include <string>
#include "Archiver.h"
#include "ECConfig.h"
#include "ArchiverImpl.h"

const char* Archiver::GetConfigPath()
{
	static std::string s_strConfigPath;

	if (s_strConfigPath.empty()) {
		const char *lpszConfigPath = getenv("ZARAFA_ARCHIVER_CONF");
		if (!lpszConfigPath || lpszConfigPath[0] == '\0')
			s_strConfigPath = ECConfig::GetDefaultPath("archiver.cfg");
		else
			s_strConfigPath = lpszConfigPath;
	}

	return s_strConfigPath.c_str();
}

const configsetting_t* Archiver::GetConfigDefaults()
{
	static const configsetting_t s_lpDefaults[] = {
		// Connect settings
		{ "server_socket",	"" },
		{ "sslkey_file",	"" },
		{ "sslkey_pass",	"", CONFIGSETTING_EXACT },

		// Archive settings
		{ "archive_enable",	"yes" },
		{ "archive_after", 	"30" },

		{ "stub_enable",	"no" },
		{ "stub_unread",	"no" },
		{ "stub_after", 	"0" },

		{ "delete_enable",	"no" },
		{ "delete_unread",	"no" },
		{ "delete_after", 	"0" },

		{ "purge_enable",	"no" },
		{ "purge_after", 	"2555" },

		{ "track_history",	"no" },
		{ "cleanup_action",	"store" },
		{ "cleanup_follow_purge_after",	"no" },
		{ "enable_auto_attach",	"no" },
		{ "auto_attach_writable",	"yes" },

		// Log options
		{ "log_method",		"file" },
		{ "log_file",		"-" },
		{ "log_level",		"2", CONFIGSETTING_RELOADABLE },
		{ "log_timestamp",	"yes" },

		{ "mysql_host",		"localhost" },
		{ "mysql_port",		"3306" },
		{ "mysql_user",		"root" },
		{ "mysql_password",	"",	CONFIGSETTING_EXACT },
		{ "mysql_database",	"zarafa-archiver" },
		{ "mysql_socket",	"" },
		{ "purge-soft-deleted", "no" },

		{ NULL, NULL },
	};

	return s_lpDefaults;
}

eResult Archiver::Create(auto_ptr_type *lpptrArchiver)
{
	eResult r = Success;
	auto_ptr_type ptrArchiver;

	if (lpptrArchiver == NULL) {
		r = InvalidParameter;
		goto exit;
	}

	try {
		ptrArchiver.reset(new ArchiverImpl());
	} catch (std::bad_alloc &) {
		r = OutOfMemory;
		goto exit;
	}

	*lpptrArchiver = ptrArchiver;

exit:
	return r;
}
