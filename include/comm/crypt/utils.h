#ifndef _CRYPT_UTILS_H_
#define _CRYPT_UTILS_H_

#include "sha1.h"
#include "../str.h"

COID_NAMESPACE_BEGIN

static void sha1(coid::charstr &out, const uint8* in, uint in_length)
{
    char digest[SHA1_RESULTLEN];
    coid::sha1_ctxt hash;
    coid::sha1_init(&hash);
    coid::sha1_loop(&hash, in, (size_t)in_length);
    coid::sha1_result(&hash, digest);

    char *out_str = out.get_buf(SHA1_RESULTLEN * 2);
    coid::charstrconv::bin2hex(digest, out_str, SHA1_RESULTLEN, 1, 0);
}

COID_NAMESPACE_END

#endif /*_CRYPT_UTILS_H_*/