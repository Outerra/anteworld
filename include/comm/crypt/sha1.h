/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is COID/comm module.
 *
 * The Initial Developer of the Original Code is
 * WIDE Project.
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996, 1997,
 * and 1998 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Jun-ichiro itojun Itoh
 * Brano Kemen
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#ifndef _NETINET6_SHA1_H_
#define _NETINET6_SHA1_H_

#include "../namespace.h"
#include "../commtypes.h"


COID_NAMESPACE_BEGIN

struct sha1_ctxt {
	union {
		uint8	b8[20];
		uint32	b32[5];
	} h;
	union {
		uint8	b8[8];
		uint64	b64[1];
	} c;
	union {
		uint8	b8[64];
		uint32	b32[16];
	} m;
	uint8	count;
};

extern void sha1_init(struct sha1_ctxt *);
extern void sha1_pad(struct sha1_ctxt *);
extern void sha1_loop(struct sha1_ctxt *, const void*, size_t);
extern void sha1_result(struct sha1_ctxt *, char* );

#define	SHA1_RESULTLEN	(160/8)


COID_NAMESPACE_END

#endif /*_NETINET6_SHA1_H_*/
