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

#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include <cerrno>

using namespace mint;

MINT_FUNCTION(mint_errno_setup, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &Errno = helper.pop_parameter();

#define BIND_ERRNO_VALUE(_enum, _errno) \
	_enum.data<Object>()->metadata->globals()[#_errno]->value.data<Number>()->value = _errno
#define BIND_ERRNO_DISABLE(_enum, _errno) \
	_enum.data<Object>()->metadata->globals()[#_errno]->value.move_data(WeakReference::create<None>())

	BIND_ERRNO_VALUE(Errno, EPERM);
	BIND_ERRNO_VALUE(Errno, ENOENT);
	BIND_ERRNO_VALUE(Errno, ESRCH);
	BIND_ERRNO_VALUE(Errno, EINTR);
	BIND_ERRNO_VALUE(Errno, EIO);
	BIND_ERRNO_VALUE(Errno, ENXIO);
	BIND_ERRNO_VALUE(Errno, E2BIG);
	BIND_ERRNO_VALUE(Errno, ENOEXEC);
	BIND_ERRNO_VALUE(Errno, EBADF);
	BIND_ERRNO_VALUE(Errno, ECHILD);
	BIND_ERRNO_VALUE(Errno, EAGAIN);
	BIND_ERRNO_VALUE(Errno, ENOMEM);
	BIND_ERRNO_VALUE(Errno, EACCES);
	BIND_ERRNO_VALUE(Errno, EFAULT);
#ifdef ENOTBLK
	BIND_ERRNO_VALUE(Errno, ENOTBLK);
#else
	BIND_ERRNO_DISABLE(Errno, ENOTBLK);
#endif
	BIND_ERRNO_VALUE(Errno, EBUSY);
	BIND_ERRNO_VALUE(Errno, EEXIST);
	BIND_ERRNO_VALUE(Errno, EXDEV);
	BIND_ERRNO_VALUE(Errno, ENODEV);
	BIND_ERRNO_VALUE(Errno, ENOTDIR);
	BIND_ERRNO_VALUE(Errno, EISDIR);
	BIND_ERRNO_VALUE(Errno, EINVAL);
	BIND_ERRNO_VALUE(Errno, ENFILE);
	BIND_ERRNO_VALUE(Errno, EMFILE);
	BIND_ERRNO_VALUE(Errno, ENOTTY);
	BIND_ERRNO_VALUE(Errno, ETXTBSY);
	BIND_ERRNO_VALUE(Errno, EFBIG);
	BIND_ERRNO_VALUE(Errno, ENOSPC);
	BIND_ERRNO_VALUE(Errno, ESPIPE);
	BIND_ERRNO_VALUE(Errno, EROFS);
	BIND_ERRNO_VALUE(Errno, EMLINK);
	BIND_ERRNO_VALUE(Errno, EPIPE);
	BIND_ERRNO_VALUE(Errno, EDOM);
	BIND_ERRNO_VALUE(Errno, ERANGE);
	BIND_ERRNO_VALUE(Errno, EDEADLK);
	BIND_ERRNO_VALUE(Errno, ENAMETOOLONG);
	BIND_ERRNO_VALUE(Errno, ENOLCK);
	BIND_ERRNO_VALUE(Errno, ENOSYS);
	BIND_ERRNO_VALUE(Errno, ENOTEMPTY);
	BIND_ERRNO_VALUE(Errno, ELOOP);
	BIND_ERRNO_VALUE(Errno, EWOULDBLOCK);
	BIND_ERRNO_VALUE(Errno, ENOMSG);
	BIND_ERRNO_VALUE(Errno, EIDRM);
#ifdef ECHRNG
	BIND_ERRNO_VALUE(Errno, ECHRNG);
#else
	BIND_ERRNO_DISABLE(Errno, ECHRNG);
#endif
#ifdef EL2NSYNC
	BIND_ERRNO_VALUE(Errno, EL2NSYNC);
#else
	BIND_ERRNO_DISABLE(Errno, EL2NSYNC);
#endif
#ifdef EL3HLT
	BIND_ERRNO_VALUE(Errno, EL3HLT);
#else
	BIND_ERRNO_DISABLE(Errno, EL3HLT);
#endif
#ifdef EL3RST
	BIND_ERRNO_VALUE(Errno, EL3RST);
#else
	BIND_ERRNO_DISABLE(Errno, EL3RST);
#endif
#ifdef ELNRNG
	BIND_ERRNO_VALUE(Errno, ELNRNG);
#else
	BIND_ERRNO_DISABLE(Errno, ELNRNG);
#endif
#ifdef EUNATCH
	BIND_ERRNO_VALUE(Errno, EUNATCH);
#else
	BIND_ERRNO_DISABLE(Errno, EUNATCH);
#endif
#ifdef ENOCSI
	BIND_ERRNO_VALUE(Errno, ENOCSI);
#else
	BIND_ERRNO_DISABLE(Errno, ENOCSI);
#endif
#ifdef EL2HLT
	BIND_ERRNO_VALUE(Errno, EL2HLT);
#else
	BIND_ERRNO_DISABLE(Errno, EL2HLT);
#endif
#ifdef EBADE
	BIND_ERRNO_VALUE(Errno, EBADE);
#else
	BIND_ERRNO_DISABLE(Errno, EBADE);
#endif
#ifdef EBADR
	BIND_ERRNO_VALUE(Errno, EBADR);
#else
	BIND_ERRNO_DISABLE(Errno, EBADR);
#endif
#ifdef EXFULL
	BIND_ERRNO_VALUE(Errno, EXFULL);
#else
	BIND_ERRNO_DISABLE(Errno, EXFULL);
#endif
#ifdef ENOANO
	BIND_ERRNO_VALUE(Errno, ENOANO);
#else
	BIND_ERRNO_DISABLE(Errno, ENOANO);
#endif
#ifdef EBADRQC
	BIND_ERRNO_VALUE(Errno, EBADRQC);
#else
	BIND_ERRNO_DISABLE(Errno, EBADRQC);
#endif
#ifdef EBADSLT
	BIND_ERRNO_VALUE(Errno, EBADSLT);
#else
	BIND_ERRNO_DISABLE(Errno, EBADSLT);
#endif
	BIND_ERRNO_VALUE(Errno, EDEADLOCK);
#ifdef EBFONT
	BIND_ERRNO_VALUE(Errno, EBFONT);
#else
	BIND_ERRNO_DISABLE(Errno, EBFONT);
#endif
	BIND_ERRNO_VALUE(Errno, ENOSTR);
	BIND_ERRNO_VALUE(Errno, ENODATA);
	BIND_ERRNO_VALUE(Errno, ETIME);
	BIND_ERRNO_VALUE(Errno, ENOSR);
#ifdef ENONET
	BIND_ERRNO_VALUE(Errno, ENONET);
#else
	BIND_ERRNO_DISABLE(Errno, ENONET);
#endif
#ifdef ENOPKG
	BIND_ERRNO_VALUE(Errno, ENOPKG);
#else
	BIND_ERRNO_DISABLE(Errno, ENOPKG);
#endif
#ifdef EREMOTE
	BIND_ERRNO_VALUE(Errno, EREMOTE);
#else
	BIND_ERRNO_DISABLE(Errno, EREMOTE);
#endif
	BIND_ERRNO_VALUE(Errno, ENOLINK);
#ifdef EADV
	BIND_ERRNO_VALUE(Errno, EADV);
#else
	BIND_ERRNO_DISABLE(Errno, EADV);
#endif
#ifdef ESRMNT
	BIND_ERRNO_VALUE(Errno, ESRMNT);
#else
	BIND_ERRNO_DISABLE(Errno, ESRMNT);
#endif
#ifdef ECOMM
	BIND_ERRNO_VALUE(Errno, ECOMM);
#else
	BIND_ERRNO_DISABLE(Errno, ECOMM);
#endif
	BIND_ERRNO_VALUE(Errno, EPROTO);
#ifdef EMULTIHOP
	BIND_ERRNO_VALUE(Errno, EMULTIHOP);
#else
	BIND_ERRNO_DISABLE(Errno, EMULTIHOP);
#endif
#ifdef EDOTDOT
	BIND_ERRNO_VALUE(Errno, EDOTDOT);
#else
	BIND_ERRNO_DISABLE(Errno, EDOTDOT);
#endif
	BIND_ERRNO_VALUE(Errno, EBADMSG);
	BIND_ERRNO_VALUE(Errno, EOVERFLOW);
#ifdef ENOTUNIQ
	BIND_ERRNO_VALUE(Errno, ENOTUNIQ);
#else
	BIND_ERRNO_DISABLE(Errno, ENOTUNIQ);
#endif
#ifdef EBADFD
	BIND_ERRNO_VALUE(Errno, EBADFD);
#else
	BIND_ERRNO_DISABLE(Errno, EBADFD);
#endif
#ifdef EREMCHG
	BIND_ERRNO_VALUE(Errno, EREMCHG);
#else
	BIND_ERRNO_DISABLE(Errno, EREMCHG);
#endif
#ifdef ELIBACC
	BIND_ERRNO_VALUE(Errno, ELIBACC);
#else
	BIND_ERRNO_DISABLE(Errno, ELIBACC);
#endif
#ifdef ELIBBAD
	BIND_ERRNO_VALUE(Errno, ELIBBAD);
#else
	BIND_ERRNO_DISABLE(Errno, ELIBBAD);
#endif
#ifdef ELIBSCN
	BIND_ERRNO_VALUE(Errno, ELIBSCN);
#else
	BIND_ERRNO_DISABLE(Errno, ELIBSCN);
#endif
#ifdef ELIBMAX
	BIND_ERRNO_VALUE(Errno, ELIBMAX);
#else
	BIND_ERRNO_DISABLE(Errno, ELIBMAX);
#endif
#ifdef ELIBEXEC
	BIND_ERRNO_VALUE(Errno, ELIBEXEC);
#else
	BIND_ERRNO_DISABLE(Errno, ELIBEXEC);
#endif
	BIND_ERRNO_VALUE(Errno, EILSEQ);
#ifdef ERESTART
	BIND_ERRNO_VALUE(Errno, ERESTART);
#else
	BIND_ERRNO_DISABLE(Errno, ERESTART);
#endif
#ifdef ESTRPIPE
	BIND_ERRNO_VALUE(Errno, ESTRPIPE);
#else
	BIND_ERRNO_DISABLE(Errno, ESTRPIPE);
#endif
#ifdef EUSERS
	BIND_ERRNO_VALUE(Errno, EUSERS);
#else
	BIND_ERRNO_DISABLE(Errno, EUSERS);
#endif
	BIND_ERRNO_VALUE(Errno, ENOTSOCK);
	BIND_ERRNO_VALUE(Errno, EDESTADDRREQ);
	BIND_ERRNO_VALUE(Errno, EMSGSIZE);
	BIND_ERRNO_VALUE(Errno, EPROTOTYPE);
	BIND_ERRNO_VALUE(Errno, ENOPROTOOPT);
	BIND_ERRNO_VALUE(Errno, EPROTONOSUPPORT);
#ifdef ESOCKTNOSUPPORT
	BIND_ERRNO_VALUE(Errno, ESOCKTNOSUPPORT);
#else
	BIND_ERRNO_DISABLE(Errno, ESOCKTNOSUPPORT);
#endif
	BIND_ERRNO_VALUE(Errno, EOPNOTSUPP);
#ifdef EPFNOSUPPORT
	BIND_ERRNO_VALUE(Errno, EPFNOSUPPORT);
#else
	BIND_ERRNO_DISABLE(Errno, EPFNOSUPPORT);
#endif
	BIND_ERRNO_VALUE(Errno, EAFNOSUPPORT);
	BIND_ERRNO_VALUE(Errno, EADDRINUSE);
	BIND_ERRNO_VALUE(Errno, EADDRNOTAVAIL);
	BIND_ERRNO_VALUE(Errno, ENETDOWN);
	BIND_ERRNO_VALUE(Errno, ENETUNREACH);
	BIND_ERRNO_VALUE(Errno, ENETRESET);
	BIND_ERRNO_VALUE(Errno, ECONNABORTED);
	BIND_ERRNO_VALUE(Errno, ECONNRESET);
	BIND_ERRNO_VALUE(Errno, ENOBUFS);
	BIND_ERRNO_VALUE(Errno, EISCONN);
	BIND_ERRNO_VALUE(Errno, ENOTCONN);
#ifdef ESHUTDOWN
	BIND_ERRNO_VALUE(Errno, ESHUTDOWN);
#else
	BIND_ERRNO_DISABLE(Errno, ESHUTDOWN);
#endif
#ifdef ETOOMANYREFS
	BIND_ERRNO_VALUE(Errno, ETOOMANYREFS);
#else
	BIND_ERRNO_DISABLE(Errno, ETOOMANYREFS);
#endif
	BIND_ERRNO_VALUE(Errno, ETIMEDOUT);
	BIND_ERRNO_VALUE(Errno, ECONNREFUSED);
#ifdef EHOSTDOWN
	BIND_ERRNO_VALUE(Errno, EHOSTDOWN);
#else
	BIND_ERRNO_DISABLE(Errno, EHOSTDOWN);
#endif
	BIND_ERRNO_VALUE(Errno, EHOSTUNREACH);
	BIND_ERRNO_VALUE(Errno, EALREADY);
	BIND_ERRNO_VALUE(Errno, EINPROGRESS);
#ifdef ESTALE
	BIND_ERRNO_VALUE(Errno, ESTALE);
#else
	BIND_ERRNO_DISABLE(Errno, ESTALE);
#endif
#ifdef EUCLEAN
	BIND_ERRNO_VALUE(Errno, EUCLEAN);
#else
	BIND_ERRNO_DISABLE(Errno, EUCLEAN);
#endif
#ifdef ENOTNAM
	BIND_ERRNO_VALUE(Errno, ENOTNAM);
#else
	BIND_ERRNO_DISABLE(Errno, ENOTNAM);
#endif
#ifdef ENAVAIL
	BIND_ERRNO_VALUE(Errno, ENAVAIL);
#else
	BIND_ERRNO_DISABLE(Errno, ENAVAIL);
#endif
#ifdef EISNAM
	BIND_ERRNO_VALUE(Errno, EISNAM);
#else
	BIND_ERRNO_DISABLE(Errno, EISNAM);
#endif
#ifdef EREMOTEIO
	BIND_ERRNO_VALUE(Errno, EREMOTEIO);
#else
	BIND_ERRNO_DISABLE(Errno, EREMOTEIO);
#endif
#ifdef EDQUOT
	BIND_ERRNO_VALUE(Errno, EDQUOT);
#else
	BIND_ERRNO_DISABLE(Errno, EDQUOT);
#endif
#ifdef ENOMEDIUM
	BIND_ERRNO_VALUE(Errno, ENOMEDIUM);
#else
	BIND_ERRNO_DISABLE(Errno, ENOMEDIUM);
#endif
#ifdef EMEDIUMTYPE
	BIND_ERRNO_VALUE(Errno, EMEDIUMTYPE);
#else
	BIND_ERRNO_DISABLE(Errno, EMEDIUMTYPE);
#endif
	BIND_ERRNO_VALUE(Errno, ECANCELED);
#ifdef ENOKEY
	BIND_ERRNO_VALUE(Errno, ENOKEY);
#else
	BIND_ERRNO_DISABLE(Errno, ENOKEY);
#endif
#ifdef EKEYEXPIRED
	BIND_ERRNO_VALUE(Errno, EKEYEXPIRED);
#else
	BIND_ERRNO_DISABLE(Errno, EKEYEXPIRED);
#endif
#ifdef EKEYREVOKED
	BIND_ERRNO_VALUE(Errno, EKEYREVOKED);
#else
	BIND_ERRNO_DISABLE(Errno, EKEYREVOKED);
#endif
#ifdef EKEYREJECTED
	BIND_ERRNO_VALUE(Errno, EKEYREJECTED);
#else
	BIND_ERRNO_DISABLE(Errno, EKEYREJECTED);
#endif
	BIND_ERRNO_VALUE(Errno, EOWNERDEAD);
	BIND_ERRNO_VALUE(Errno, ENOTRECOVERABLE);
#ifdef ERFKILL
	BIND_ERRNO_VALUE(Errno, ERFKILL);
#else
	BIND_ERRNO_DISABLE(Errno, ERFKILL);
#endif
#ifdef EHWPOISON
	BIND_ERRNO_VALUE(Errno, EHWPOISON);
#else
	BIND_ERRNO_DISABLE(Errno, EHWPOISON);
#endif
#ifdef ERESTARTSYS
	BIND_ERRNO_VALUE(Errno, ERESTARTSYS);
#else
	BIND_ERRNO_DISABLE(Errno, ERESTARTSYS);
#endif
#ifdef ERESTARTNOINTR
	BIND_ERRNO_VALUE(Errno, ERESTARTNOINTR);
#else
	BIND_ERRNO_DISABLE(Errno, ERESTARTNOINTR);
#endif
#ifdef ERESTARTNOHAND
	BIND_ERRNO_VALUE(Errno, ERESTARTNOHAND);
#else
	BIND_ERRNO_DISABLE(Errno, ERESTARTNOHAND);
#endif
#ifdef ENOIOCTLCMD
	BIND_ERRNO_VALUE(Errno, ENOIOCTLCMD);
#else
	BIND_ERRNO_DISABLE(Errno, ENOIOCTLCMD);
#endif
#ifdef ERESTART_RESTARTBLOCK
	BIND_ERRNO_VALUE(Errno, ERESTART_RESTARTBLOCK);
#else
	BIND_ERRNO_DISABLE(Errno, ERESTART_RESTARTBLOCK);
#endif
#ifdef EBADHANDLE
	BIND_ERRNO_VALUE(Errno, EBADHANDLE);
#else
	BIND_ERRNO_DISABLE(Errno, EBADHANDLE);
#endif
#ifdef ENOTSYNC
	BIND_ERRNO_VALUE(Errno, ENOTSYNC);
#else
	BIND_ERRNO_DISABLE(Errno, ENOTSYNC);
#endif
#ifdef EBADCOOKIE
	BIND_ERRNO_VALUE(Errno, EBADCOOKIE);
#else
	BIND_ERRNO_DISABLE(Errno, EBADCOOKIE);
#endif
#ifdef ENOTSUPP
	BIND_ERRNO_VALUE(Errno, ENOTSUPP);
#else
	BIND_ERRNO_DISABLE(Errno, ENOTSUPP);
#endif
#ifdef ETOOSMALL
	BIND_ERRNO_VALUE(Errno, ETOOSMALL);
#else
	BIND_ERRNO_DISABLE(Errno, ETOOSMALL);
#endif
#ifdef ESERVERFAULT
	BIND_ERRNO_VALUE(Errno, ESERVERFAULT);
#else
	BIND_ERRNO_DISABLE(Errno, ESERVERFAULT);
#endif
#ifdef EBADTYPE
	BIND_ERRNO_VALUE(Errno, EBADTYPE);
#else
	BIND_ERRNO_DISABLE(Errno, EBADTYPE);
#endif
#ifdef EJUKEBOX
	BIND_ERRNO_VALUE(Errno, EJUKEBOX);
#else
	BIND_ERRNO_DISABLE(Errno, EJUKEBOX);
#endif
#ifdef EIOCBQUEUED
	BIND_ERRNO_VALUE(Errno, EIOCBQUEUED);
#else
	BIND_ERRNO_DISABLE(Errno, EIOCBQUEUED);
#endif
#ifdef EIOCBRETRY
	BIND_ERRNO_VALUE(Errno, EIOCBRETRY);
#else
	BIND_ERRNO_DISABLE(Errno, EIOCBRETRY);
#endif
}

MINT_FUNCTION(mint_errno_get, 0, cursor) {
	FunctionHelper(cursor, 0).return_value(create_number(errno));
}

MINT_FUNCTION(mint_errno_strerror, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Reference &error = helper.pop_parameter();
	helper.return_value(create_string(strerror(to_integer(cursor, error))));
}
