/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include <cerrno>
#include <cstring>
#include <errno.h>

#ifdef MINT_OS_UNIX
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#endif

namespace {

mint::Reference mint_errno_setup(mint::Cursor& /*cursor*/, const mint::Reference& errno_enum) {

#define BIND_ERRNO_VALUE(_enum, _errno) \
	_enum.data<mint::Object>().metadata.find_global(#_errno)->value.data<mint::Number>().value = _errno
#define BIND_ERRNO_DISABLE(_enum, _errno) \
	_enum.data<mint::Object>().metadata.find_global(#_errno)->value.move_data(mint::create_none())

	BIND_ERRNO_VALUE(errno_enum, EPERM);
	BIND_ERRNO_VALUE(errno_enum, ENOENT);
	BIND_ERRNO_VALUE(errno_enum, ESRCH);
	BIND_ERRNO_VALUE(errno_enum, EINTR);
	BIND_ERRNO_VALUE(errno_enum, EIO);
	BIND_ERRNO_VALUE(errno_enum, ENXIO);
	BIND_ERRNO_VALUE(errno_enum, E2BIG);
	BIND_ERRNO_VALUE(errno_enum, ENOEXEC);
	BIND_ERRNO_VALUE(errno_enum, EBADF);
	BIND_ERRNO_VALUE(errno_enum, ECHILD);
	BIND_ERRNO_VALUE(errno_enum, EAGAIN);
	BIND_ERRNO_VALUE(errno_enum, ENOMEM);
	BIND_ERRNO_VALUE(errno_enum, EACCES);
	BIND_ERRNO_VALUE(errno_enum, EFAULT);
#ifdef ENOTBLK
	BIND_ERRNO_VALUE(errno_enum, ENOTBLK);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENOTBLK);
#endif
	BIND_ERRNO_VALUE(errno_enum, EBUSY);
	BIND_ERRNO_VALUE(errno_enum, EEXIST);
	BIND_ERRNO_VALUE(errno_enum, EXDEV);
	BIND_ERRNO_VALUE(errno_enum, ENODEV);
	BIND_ERRNO_VALUE(errno_enum, ENOTDIR);
	BIND_ERRNO_VALUE(errno_enum, EISDIR);
	BIND_ERRNO_VALUE(errno_enum, EINVAL);
	BIND_ERRNO_VALUE(errno_enum, ENFILE);
	BIND_ERRNO_VALUE(errno_enum, EMFILE);
	BIND_ERRNO_VALUE(errno_enum, ENOTTY);
	BIND_ERRNO_VALUE(errno_enum, ETXTBSY);
	BIND_ERRNO_VALUE(errno_enum, EFBIG);
	BIND_ERRNO_VALUE(errno_enum, ENOSPC);
	BIND_ERRNO_VALUE(errno_enum, ESPIPE);
	BIND_ERRNO_VALUE(errno_enum, EROFS);
	BIND_ERRNO_VALUE(errno_enum, EMLINK);
	BIND_ERRNO_VALUE(errno_enum, EPIPE);
	BIND_ERRNO_VALUE(errno_enum, EDOM);
	BIND_ERRNO_VALUE(errno_enum, ERANGE);
	BIND_ERRNO_VALUE(errno_enum, EDEADLK);
	BIND_ERRNO_VALUE(errno_enum, ENAMETOOLONG);
	BIND_ERRNO_VALUE(errno_enum, ENOLCK);
	BIND_ERRNO_VALUE(errno_enum, ENOSYS);
	BIND_ERRNO_VALUE(errno_enum, ENOTEMPTY);
	BIND_ERRNO_VALUE(errno_enum, ELOOP);
	BIND_ERRNO_VALUE(errno_enum, EWOULDBLOCK);
	BIND_ERRNO_VALUE(errno_enum, ENOMSG);
	BIND_ERRNO_VALUE(errno_enum, EIDRM);
#ifdef ECHRNG
	BIND_ERRNO_VALUE(errno_enum, ECHRNG);
#else
	BIND_ERRNO_DISABLE(errno_enum, ECHRNG);
#endif
#ifdef EL2NSYNC
	BIND_ERRNO_VALUE(errno_enum, EL2NSYNC);
#else
	BIND_ERRNO_DISABLE(errno_enum, EL2NSYNC);
#endif
#ifdef EL3HLT
	BIND_ERRNO_VALUE(errno_enum, EL3HLT);
#else
	BIND_ERRNO_DISABLE(errno_enum, EL3HLT);
#endif
#ifdef EL3RST
	BIND_ERRNO_VALUE(errno_enum, EL3RST);
#else
	BIND_ERRNO_DISABLE(errno_enum, EL3RST);
#endif
#ifdef ELNRNG
	BIND_ERRNO_VALUE(errno_enum, ELNRNG);
#else
	BIND_ERRNO_DISABLE(errno_enum, ELNRNG);
#endif
#ifdef EUNATCH
	BIND_ERRNO_VALUE(errno_enum, EUNATCH);
#else
	BIND_ERRNO_DISABLE(errno_enum, EUNATCH);
#endif
#ifdef ENOCSI
	BIND_ERRNO_VALUE(errno_enum, ENOCSI);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENOCSI);
#endif
#ifdef EL2HLT
	BIND_ERRNO_VALUE(errno_enum, EL2HLT);
#else
	BIND_ERRNO_DISABLE(errno_enum, EL2HLT);
#endif
#ifdef EBADE
	BIND_ERRNO_VALUE(errno_enum, EBADE);
#else
	BIND_ERRNO_DISABLE(errno_enum, EBADE);
#endif
#ifdef EBADR
	BIND_ERRNO_VALUE(errno_enum, EBADR);
#else
	BIND_ERRNO_DISABLE(errno_enum, EBADR);
#endif
#ifdef EXFULL
	BIND_ERRNO_VALUE(errno_enum, EXFULL);
#else
	BIND_ERRNO_DISABLE(errno_enum, EXFULL);
#endif
#ifdef ENOANO
	BIND_ERRNO_VALUE(errno_enum, ENOANO);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENOANO);
#endif
#ifdef EBADRQC
	BIND_ERRNO_VALUE(errno_enum, EBADRQC);
#else
	BIND_ERRNO_DISABLE(errno_enum, EBADRQC);
#endif
#ifdef EBADSLT
	BIND_ERRNO_VALUE(errno_enum, EBADSLT);
#else
	BIND_ERRNO_DISABLE(errno_enum, EBADSLT);
#endif
	BIND_ERRNO_VALUE(errno_enum, EDEADLOCK);
#ifdef EBFONT
	BIND_ERRNO_VALUE(errno_enum, EBFONT);
#else
	BIND_ERRNO_DISABLE(errno_enum, EBFONT);
#endif
	BIND_ERRNO_VALUE(errno_enum, ENOSTR);
	BIND_ERRNO_VALUE(errno_enum, ENODATA);
	BIND_ERRNO_VALUE(errno_enum, ETIME);
	BIND_ERRNO_VALUE(errno_enum, ENOSR);
#ifdef ENONET
	BIND_ERRNO_VALUE(errno_enum, ENONET);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENONET);
#endif
#ifdef ENOPKG
	BIND_ERRNO_VALUE(errno_enum, ENOPKG);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENOPKG);
#endif
#ifdef EREMOTE
	BIND_ERRNO_VALUE(errno_enum, EREMOTE);
#else
	BIND_ERRNO_DISABLE(errno_enum, EREMOTE);
#endif
	BIND_ERRNO_VALUE(errno_enum, ENOLINK);
#ifdef EADV
	BIND_ERRNO_VALUE(errno_enum, EADV);
#else
	BIND_ERRNO_DISABLE(errno_enum, EADV);
#endif
#ifdef ESRMNT
	BIND_ERRNO_VALUE(errno_enum, ESRMNT);
#else
	BIND_ERRNO_DISABLE(errno_enum, ESRMNT);
#endif
#ifdef ECOMM
	BIND_ERRNO_VALUE(errno_enum, ECOMM);
#else
	BIND_ERRNO_DISABLE(errno_enum, ECOMM);
#endif
	BIND_ERRNO_VALUE(errno_enum, EPROTO);
#ifdef EMULTIHOP
	BIND_ERRNO_VALUE(errno_enum, EMULTIHOP);
#else
	BIND_ERRNO_DISABLE(errno_enum, EMULTIHOP);
#endif
#ifdef EDOTDOT
	BIND_ERRNO_VALUE(errno_enum, EDOTDOT);
#else
	BIND_ERRNO_DISABLE(errno_enum, EDOTDOT);
#endif
	BIND_ERRNO_VALUE(errno_enum, EBADMSG);
	BIND_ERRNO_VALUE(errno_enum, EOVERFLOW);
#ifdef ENOTUNIQ
	BIND_ERRNO_VALUE(errno_enum, ENOTUNIQ);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENOTUNIQ);
#endif
#ifdef EBADFD
	BIND_ERRNO_VALUE(errno_enum, EBADFD);
#else
	BIND_ERRNO_DISABLE(errno_enum, EBADFD);
#endif
#ifdef EREMCHG
	BIND_ERRNO_VALUE(errno_enum, EREMCHG);
#else
	BIND_ERRNO_DISABLE(errno_enum, EREMCHG);
#endif
#ifdef ELIBACC
	BIND_ERRNO_VALUE(errno_enum, ELIBACC);
#else
	BIND_ERRNO_DISABLE(errno_enum, ELIBACC);
#endif
#ifdef ELIBBAD
	BIND_ERRNO_VALUE(errno_enum, ELIBBAD);
#else
	BIND_ERRNO_DISABLE(errno_enum, ELIBBAD);
#endif
#ifdef ELIBSCN
	BIND_ERRNO_VALUE(errno_enum, ELIBSCN);
#else
	BIND_ERRNO_DISABLE(errno_enum, ELIBSCN);
#endif
#ifdef ELIBMAX
	BIND_ERRNO_VALUE(errno_enum, ELIBMAX);
#else
	BIND_ERRNO_DISABLE(errno_enum, ELIBMAX);
#endif
#ifdef ELIBEXEC
	BIND_ERRNO_VALUE(errno_enum, ELIBEXEC);
#else
	BIND_ERRNO_DISABLE(errno_enum, ELIBEXEC);
#endif
	BIND_ERRNO_VALUE(errno_enum, EILSEQ);
#ifdef ERESTART
	BIND_ERRNO_VALUE(errno_enum, ERESTART);
#else
	BIND_ERRNO_DISABLE(errno_enum, ERESTART);
#endif
#ifdef ESTRPIPE
	BIND_ERRNO_VALUE(errno_enum, ESTRPIPE);
#else
	BIND_ERRNO_DISABLE(errno_enum, ESTRPIPE);
#endif
#ifdef EUSERS
	BIND_ERRNO_VALUE(errno_enum, EUSERS);
#else
	BIND_ERRNO_DISABLE(errno_enum, EUSERS);
#endif
	BIND_ERRNO_VALUE(errno_enum, ENOTSOCK);
	BIND_ERRNO_VALUE(errno_enum, EDESTADDRREQ);
	BIND_ERRNO_VALUE(errno_enum, EMSGSIZE);
	BIND_ERRNO_VALUE(errno_enum, EPROTOTYPE);
	BIND_ERRNO_VALUE(errno_enum, ENOPROTOOPT);
	BIND_ERRNO_VALUE(errno_enum, EPROTONOSUPPORT);
#ifdef ESOCKTNOSUPPORT
	BIND_ERRNO_VALUE(errno_enum, ESOCKTNOSUPPORT);
#else
	BIND_ERRNO_DISABLE(errno_enum, ESOCKTNOSUPPORT);
#endif
	BIND_ERRNO_VALUE(errno_enum, EOPNOTSUPP);
#ifdef EPFNOSUPPORT
	BIND_ERRNO_VALUE(errno_enum, EPFNOSUPPORT);
#else
	BIND_ERRNO_DISABLE(errno_enum, EPFNOSUPPORT);
#endif
	BIND_ERRNO_VALUE(errno_enum, EAFNOSUPPORT);
	BIND_ERRNO_VALUE(errno_enum, EADDRINUSE);
	BIND_ERRNO_VALUE(errno_enum, EADDRNOTAVAIL);
	BIND_ERRNO_VALUE(errno_enum, ENETDOWN);
	BIND_ERRNO_VALUE(errno_enum, ENETUNREACH);
	BIND_ERRNO_VALUE(errno_enum, ENETRESET);
	BIND_ERRNO_VALUE(errno_enum, ECONNABORTED);
	BIND_ERRNO_VALUE(errno_enum, ECONNRESET);
	BIND_ERRNO_VALUE(errno_enum, ENOBUFS);
	BIND_ERRNO_VALUE(errno_enum, EISCONN);
	BIND_ERRNO_VALUE(errno_enum, ENOTCONN);
#ifdef ESHUTDOWN
	BIND_ERRNO_VALUE(errno_enum, ESHUTDOWN);
#else
	BIND_ERRNO_DISABLE(errno_enum, ESHUTDOWN);
#endif
#ifdef ETOOMANYREFS
	BIND_ERRNO_VALUE(errno_enum, ETOOMANYREFS);
#else
	BIND_ERRNO_DISABLE(errno_enum, ETOOMANYREFS);
#endif
	BIND_ERRNO_VALUE(errno_enum, ETIMEDOUT);
	BIND_ERRNO_VALUE(errno_enum, ECONNREFUSED);
#ifdef EHOSTDOWN
	BIND_ERRNO_VALUE(errno_enum, EHOSTDOWN);
#else
	BIND_ERRNO_DISABLE(errno_enum, EHOSTDOWN);
#endif
	BIND_ERRNO_VALUE(errno_enum, EHOSTUNREACH);
	BIND_ERRNO_VALUE(errno_enum, EALREADY);
	BIND_ERRNO_VALUE(errno_enum, EINPROGRESS);
#ifdef ESTALE
	BIND_ERRNO_VALUE(errno_enum, ESTALE);
#else
	BIND_ERRNO_DISABLE(errno_enum, ESTALE);
#endif
#ifdef EUCLEAN
	BIND_ERRNO_VALUE(errno_enum, EUCLEAN);
#else
	BIND_ERRNO_DISABLE(errno_enum, EUCLEAN);
#endif
#ifdef ENOTNAM
	BIND_ERRNO_VALUE(errno_enum, ENOTNAM);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENOTNAM);
#endif
#ifdef ENAVAIL
	BIND_ERRNO_VALUE(errno_enum, ENAVAIL);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENAVAIL);
#endif
#ifdef EISNAM
	BIND_ERRNO_VALUE(errno_enum, EISNAM);
#else
	BIND_ERRNO_DISABLE(errno_enum, EISNAM);
#endif
#ifdef EREMOTEIO
	BIND_ERRNO_VALUE(errno_enum, EREMOTEIO);
#else
	BIND_ERRNO_DISABLE(errno_enum, EREMOTEIO);
#endif
#ifdef EDQUOT
	BIND_ERRNO_VALUE(errno_enum, EDQUOT);
#else
	BIND_ERRNO_DISABLE(errno_enum, EDQUOT);
#endif
#ifdef ENOMEDIUM
	BIND_ERRNO_VALUE(errno_enum, ENOMEDIUM);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENOMEDIUM);
#endif
#ifdef EMEDIUMTYPE
	BIND_ERRNO_VALUE(errno_enum, EMEDIUMTYPE);
#else
	BIND_ERRNO_DISABLE(errno_enum, EMEDIUMTYPE);
#endif
	BIND_ERRNO_VALUE(errno_enum, ECANCELED);
#ifdef ENOKEY
	BIND_ERRNO_VALUE(errno_enum, ENOKEY);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENOKEY);
#endif
#ifdef EKEYEXPIRED
	BIND_ERRNO_VALUE(errno_enum, EKEYEXPIRED);
#else
	BIND_ERRNO_DISABLE(errno_enum, EKEYEXPIRED);
#endif
#ifdef EKEYREVOKED
	BIND_ERRNO_VALUE(errno_enum, EKEYREVOKED);
#else
	BIND_ERRNO_DISABLE(errno_enum, EKEYREVOKED);
#endif
#ifdef EKEYREJECTED
	BIND_ERRNO_VALUE(errno_enum, EKEYREJECTED);
#else
	BIND_ERRNO_DISABLE(errno_enum, EKEYREJECTED);
#endif
	BIND_ERRNO_VALUE(errno_enum, EOWNERDEAD);
	BIND_ERRNO_VALUE(errno_enum, ENOTRECOVERABLE);
#ifdef ERFKILL
	BIND_ERRNO_VALUE(errno_enum, ERFKILL);
#else
	BIND_ERRNO_DISABLE(errno_enum, ERFKILL);
#endif
#ifdef EHWPOISON
	BIND_ERRNO_VALUE(errno_enum, EHWPOISON);
#else
	BIND_ERRNO_DISABLE(errno_enum, EHWPOISON);
#endif
#ifdef ERESTARTSYS
	BIND_ERRNO_VALUE(errno_enum, ERESTARTSYS);
#else
	BIND_ERRNO_DISABLE(errno_enum, ERESTARTSYS);
#endif
#ifdef ERESTARTNOINTR
	BIND_ERRNO_VALUE(errno_enum, ERESTARTNOINTR);
#else
	BIND_ERRNO_DISABLE(errno_enum, ERESTARTNOINTR);
#endif
#ifdef ERESTARTNOHAND
	BIND_ERRNO_VALUE(errno_enum, ERESTARTNOHAND);
#else
	BIND_ERRNO_DISABLE(errno_enum, ERESTARTNOHAND);
#endif
#ifdef ENOIOCTLCMD
	BIND_ERRNO_VALUE(errno_enum, ENOIOCTLCMD);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENOIOCTLCMD);
#endif
#ifdef ERESTART_RESTARTBLOCK
	BIND_ERRNO_VALUE(errno_enum, ERESTART_RESTARTBLOCK);
#else
	BIND_ERRNO_DISABLE(errno_enum, ERESTART_RESTARTBLOCK);
#endif
#ifdef EBADHANDLE
	BIND_ERRNO_VALUE(errno_enum, EBADHANDLE);
#else
	BIND_ERRNO_DISABLE(errno_enum, EBADHANDLE);
#endif
#ifdef ENOTSYNC
	BIND_ERRNO_VALUE(errno_enum, ENOTSYNC);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENOTSYNC);
#endif
#ifdef EBADCOOKIE
	BIND_ERRNO_VALUE(errno_enum, EBADCOOKIE);
#else
	BIND_ERRNO_DISABLE(errno_enum, EBADCOOKIE);
#endif
#ifdef ENOTSUPP
	BIND_ERRNO_VALUE(errno_enum, ENOTSUPP);
#else
	BIND_ERRNO_DISABLE(errno_enum, ENOTSUPP);
#endif
#ifdef ETOOSMALL
	BIND_ERRNO_VALUE(errno_enum, ETOOSMALL);
#else
	BIND_ERRNO_DISABLE(errno_enum, ETOOSMALL);
#endif
#ifdef ESERVERFAULT
	BIND_ERRNO_VALUE(errno_enum, ESERVERFAULT);
#else
	BIND_ERRNO_DISABLE(errno_enum, ESERVERFAULT);
#endif
#ifdef EBADTYPE
	BIND_ERRNO_VALUE(errno_enum, EBADTYPE);
#else
	BIND_ERRNO_DISABLE(errno_enum, EBADTYPE);
#endif
#ifdef EJUKEBOX
	BIND_ERRNO_VALUE(errno_enum, EJUKEBOX);
#else
	BIND_ERRNO_DISABLE(errno_enum, EJUKEBOX);
#endif
#ifdef EIOCBQUEUED
	BIND_ERRNO_VALUE(errno_enum, EIOCBQUEUED);
#else
	BIND_ERRNO_DISABLE(errno_enum, EIOCBQUEUED);
#endif
#ifdef EIOCBRETRY
	BIND_ERRNO_VALUE(errno_enum, EIOCBRETRY);
#else
	BIND_ERRNO_DISABLE(errno_enum, EIOCBRETRY);
#endif

	return {};
}

mint::Reference mint_errno_strerror(mint::Cursor& cursor, const mint::Reference& error) {
	return mint::create_string(cursor.ast(), strerror(mint::to_integer<int>(cursor, error)));
}

}

MINT_EXPORT_FUNCTION(mint_errno_setup, 1)
MINT_EXPORT_FUNCTION(mint_errno_strerror, 1)

MINT_RAW_FUNCTION(mint_errno_get, 0, cursor) {
	cursor.stack().emplace_back(mint::create_number(errno));
}
