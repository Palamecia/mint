#ifndef LIBMINT_SYSTEM_WIN32_GLOBALSID_H
#define LIBMINT_SYSTEM_WIN32_GLOBALSID_H

#include <windows.h>
#include <AclAPI.h>

namespace mint {

struct GlobalSid {
	TRUSTEE_W currentUserTrusteeW;
	TRUSTEE_W worldTrusteeW;
	PSID currentUserSID = NULL;
	PSID worldSID = NULL;
	HANDLE currentUserImpersonatedToken = nullptr;

	static GlobalSid g_instance;

	GlobalSid();
	~GlobalSid();
    
	GlobalSid(GlobalSid &&) = delete;
	GlobalSid(const GlobalSid &other) = delete;
	
	GlobalSid &operator=(GlobalSid &&) = delete;
	GlobalSid &operator=(const GlobalSid &other) = delete;
};

}

#endif // LIBMINT_SYSTEM_WIN32_GLOBALSID_H
