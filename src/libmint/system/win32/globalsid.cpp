/**
 * Copyright (c) 2025 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "globalsid.h"

using namespace mint;

GlobalSid GlobalSid::g_instance;

GlobalSid::GlobalSid() {
	{
		{
			// Create TRUSTEE for current user
			HANDLE hnd = GetCurrentProcess();
			HANDLE token = 0;
			if (OpenProcessToken(hnd, TOKEN_QUERY, &token)) {
				DWORD retsize = 0;
				// GetTokenInformation requires a buffer big enough for the TOKEN_USER struct and
				// the SID struct. Since the SID struct can have variable number of subauthorities
				// tacked at the end, its size is variable. Obtain the required size by first
				// doing a dummy GetTokenInformation call.
				GetTokenInformation(token, TokenUser, 0, 0, &retsize);
				if (retsize) {
					void *tokenBuffer = malloc(retsize);
					if (GetTokenInformation(token, TokenUser, tokenBuffer, retsize, &retsize)) {
						PSID tokenSid = reinterpret_cast<PTOKEN_USER>(tokenBuffer)->User.Sid;
						DWORD sidLen = ::GetLengthSid(tokenSid);
						currentUserSID = reinterpret_cast<PSID>(malloc(sidLen));
						if (CopySid(sidLen, currentUserSID, tokenSid)) {
							BuildTrusteeWithSidW(&currentUserTrusteeW, currentUserSID);
						}
					}
					free(tokenBuffer);
				}
				CloseHandle(token);
			}
			token = nullptr;
			if (OpenProcessToken(hnd, TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE | STANDARD_RIGHTS_READ,
								 &token)) {
				DuplicateToken(token, SecurityImpersonation, &currentUserImpersonatedToken);
				CloseHandle(token);
			}
			{
				// Create TRUSTEE for Everyone (World)
				SID_IDENTIFIER_AUTHORITY worldAuth = {SECURITY_WORLD_SID_AUTHORITY};
				if (AllocateAndInitializeSid(&worldAuth, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &worldSID)) {
					BuildTrusteeWithSidW(&worldTrusteeW, worldSID);
				}
			}
		}
	}
}

GlobalSid::~GlobalSid() {
	free(currentUserSID);
	currentUserSID = 0;
	// worldSID was allocated with AllocateAndInitializeSid so it needs to be freed with FreeSid
	if (worldSID) {
		FreeSid(worldSID);
		worldSID = 0;
	}
	if (currentUserImpersonatedToken) {
		CloseHandle(currentUserImpersonatedToken);
		currentUserImpersonatedToken = nullptr;
	}
}
