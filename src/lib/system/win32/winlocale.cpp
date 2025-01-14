/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#include "winlocale.h"
#include <clocale>
#include <cstdio>
#include <cctype>

#include "mint/system/utf8.h"

#define MAX_ELEM_LEN 64 /* Max length of country/language/CP string */
#define MAX_LOCALE_LENGTH 256

#define MSVCRT_malloc malloc
#define MSVCRT_free free

#define TRACE(__format, ...)
#define FIXME(__format, ...)
#define WARN(__format, ...)
#define ERR(__format, ...)

#define _MS 0x01
#define _MP 0x02
#define _M1 0x04
#define _M2 0x08

#define _SBUP 0x10
#define _SBLOW 0x20

#define _MBC_SINGLE 0
#define _MBC_LEAD 1
#define _MBC_TRAIL 2
#define _MBC_ILLEGAL -1

#define _MB_CP_SBCS 0
#define _MB_CP_OEM -2
#define _MB_CP_ANSI -3
#define _MB_CP_LOCALE -4

/* Friendly country strings & language names abbreviations. */
// clang-format off
static const char * const _country_synonyms[] = {
	"american", "enu",
	"american english", "enu",
	"american-english", "enu",
	"english-american", "enu",
	"english-us", "enu",
	"english-usa", "enu",
	"us", "enu",
	"usa", "enu",
	"australian", "ena",
	"english-aus", "ena",
	"belgian", "nlb",
	"french-belgian", "frb",
	"canadian", "enc",
	"english-can", "enc",
	"french-canadian", "frc",
	"chinese", "chs",
	"chinese-simplified", "chs",
	"chinese-traditional", "cht",
	"dutch-belgian", "nlb",
	"english-nz", "enz",
	"uk", "eng",
	"english-uk", "eng",
	"french-swiss", "frs",
	"swiss", "des",
	"german-swiss", "des",
	"italian-swiss", "its",
	"german-austrian", "dea",
	"portuguese", "ptb",
	"portuguese-brazil", "ptb",
	"spanish-mexican", "esm",
	"norwegian-bokmal", "nor",
	"norwegian-nynorsk", "non",
	"spanish-modern", "esn"
};
// clang-format on

/* INTERNAL: Map a synonym to an ISO code */
static void remap_synonym(char *name, size_t size) {
	unsigned int i;
	for (i = 0; i < sizeof(_country_synonyms) / sizeof(char *); i += 2) {
		if (!mint::utf8_compare_case_insensitive(_country_synonyms[i], name)) {
			TRACE(":Mapping synonym %s to %s\n", name, _country_synonyms[i + 1]);
			strncpy(name, _country_synonyms[i + 1], size);
			name[size - 1] = '\0';
			return;
		}
	}
}

typedef unsigned int MSVCRT_ulong;

typedef struct MSVCRT_tagLC_ID {
	unsigned short wLanguage;
	unsigned short wCountry;
	unsigned short wCodePage;
} MSVCRT_LC_ID, *MSVCRT_LPLC_ID;

struct MSVCRT_lconv {
	char *decimal_point;
	char *thousands_sep;
	char *grouping;
	char *int_curr_symbol;
	char *currency_symbol;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *mon_grouping;
	char *positive_sign;
	char *negative_sign;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
};

typedef struct __lc_time_data {
	union {
		char *str[43];

		struct {
			char *short_wday[7];
			char *wday[7];
			char *short_mon[12];
			char *mon[12];
			char *am;
			char *pm;
			char *short_date;
			char *date;
			char *time;
		} names;
	} str;

	LCID lcid;
	int unk[2];
	wchar_t *wstr[43];
	char data[1];
} MSVCRT___lc_time_data;

typedef struct MSVCRT_threadlocaleinfostruct {
	LONG refcount;
	unsigned int lc_codepage;
	unsigned int lc_collate_cp;
	MSVCRT_ulong lc_handle[6];
	MSVCRT_LC_ID lc_id[6];

	struct {
		char *locale;
		wchar_t *wlocale;
		int *refcount;
		int *wrefcount;
	} lc_category[6];

	int lc_clike;
	int mb_cur_max;
	int *lconv_intl_refcount;
	int *lconv_num_refcount;
	int *lconv_mon_refcount;
	MSVCRT_lconv *lconv;
	int *ctype1_refcount;
	unsigned short *ctype1;
	unsigned short *pctype;
	unsigned char *pclmap;
	unsigned char *pcumap;
	MSVCRT___lc_time_data *lc_time_curr;
} MSVCRT_threadlocinfo;

typedef struct MSVCRT_threadmbcinfostruct {
	LONG refcount;
	int mbcodepage;
	int ismbcodepage;
	int mblcid;
	unsigned short mbulinfo[6];
	unsigned char mbctype[257];
	unsigned char mbcasemap[256];
} MSVCRT_threadmbcinfo;

static MSVCRT_threadlocinfo *get_locinfo(MSVCRT__locale_t locale) {
	return ((MSVCRT_threadlocinfo *)locale->locinfo);
}

static MSVCRT_threadmbcinfo *get_mbcinfo(MSVCRT__locale_t locale) {
	return ((MSVCRT_threadmbcinfo *)locale->mbcinfo);
}

/* Note: Flags are weighted in order of matching importance */
#define FOUND_LANGUAGE 0x4
#define FOUND_COUNTRY 0x2
#define FOUND_CODEPAGE 0x1

typedef struct {
	char search_language[MAX_ELEM_LEN];
	char search_country[MAX_ELEM_LEN];
	char search_codepage[MAX_ELEM_LEN];
	char found_codepage[MAX_ELEM_LEN];
	unsigned int match_flags;
	LANGID found_lang_id;
} locale_search_t;

#define CONTINUE_LOOKING TRUE
#define STOP_LOOKING FALSE

/* INTERNAL: Get and compare locale info with a given string */
static int compare_info(LCID lcid, DWORD flags, char *buff, const char *cmp, BOOL exact) {

	if (!cmp[0]) {
		return 0;
	}

	buff[0] = 0;
	GetLocaleInfoA(lcid, flags | LOCALE_NOUSEROVERRIDE, buff, MAX_ELEM_LEN);
	if (!buff[0]) {
		return 0;
	}

	/* Partial matches are only allowed on language/country names */
	size_t len = mint::utf8_code_point_count(cmp);
	if (exact || len <= 3) {
		return !mint::utf8_compare_case_insensitive(cmp, buff);
	}
	else {
		return !mint::utf8_compare_substring_case_insensitive(cmp, buff, len);
	}
}

static BOOL CALLBACK find_best_locale_proc(HMODULE hModule, LPCSTR type, LPCSTR name, WORD LangID, LONG_PTR lParam) {
	locale_search_t *res = (locale_search_t *)lParam;
	const LCID lcid = MAKELCID(LangID, SORT_DEFAULT);
	char buff[MAX_ELEM_LEN];
	unsigned int flags = 0;

	if (PRIMARYLANGID(LangID) == LANG_NEUTRAL) {
		return CONTINUE_LOOKING;
	}

	/* Check Language */
	if (compare_info(lcid, LOCALE_SISO639LANGNAME, buff, res->search_language, TRUE)
		|| compare_info(lcid, LOCALE_SABBREVLANGNAME, buff, res->search_language, TRUE)
		|| compare_info(lcid, LOCALE_SENGLANGUAGE, buff, res->search_language, FALSE)) {
		TRACE(":Found language: %s->%s\n", res->search_language, buff);
		flags |= FOUND_LANGUAGE;
	}
	else if (res->match_flags & FOUND_LANGUAGE) {
		return CONTINUE_LOOKING;
	}

	/* Check Country */
	if (compare_info(lcid, LOCALE_SISO3166CTRYNAME, buff, res->search_country, TRUE)
		|| compare_info(lcid, LOCALE_SABBREVCTRYNAME, buff, res->search_country, TRUE)
		|| compare_info(lcid, LOCALE_SENGCOUNTRY, buff, res->search_country, FALSE)) {
		TRACE("Found country:%s->%s\n", res->search_country, buff);
		flags |= FOUND_COUNTRY;
	}
	else if (!flags && (res->match_flags & FOUND_COUNTRY)) {
		return CONTINUE_LOOKING;
	}

	/* Check codepage */
	if (compare_info(lcid, LOCALE_IDEFAULTCODEPAGE, buff, res->search_codepage, TRUE)
		|| (compare_info(lcid, LOCALE_IDEFAULTANSICODEPAGE, buff, res->search_codepage, TRUE))) {
		TRACE("Found codepage:%s->%s\n", res->search_codepage, buff);
		flags |= FOUND_CODEPAGE;
		memcpy(res->found_codepage, res->search_codepage, MAX_ELEM_LEN);
	}
	else if (!flags && (res->match_flags & FOUND_CODEPAGE)) {
		return CONTINUE_LOOKING;
	}

	if (flags > res->match_flags) {
		/* Found a better match than previously */
		res->match_flags = flags;
		res->found_lang_id = LangID;
	}
	if ((flags & (FOUND_LANGUAGE | FOUND_COUNTRY | FOUND_CODEPAGE))
		== (FOUND_LANGUAGE | FOUND_COUNTRY | FOUND_CODEPAGE)) {
		TRACE(":found exact locale match\n");
		return STOP_LOOKING;
	}
	return CONTINUE_LOOKING;
}

/* INTERNAL: frees MSVCRT_pthreadlocinfo struct */
void free_locinfo(MSVCRT_pthreadlocinfo locinfo) {
	int i;

	if (!locinfo) {
		return;
	}

	if (InterlockedDecrement(&locinfo->refcount)) {
		return;
	}

	for (i = MSVCRT_LC_MIN + 1; i <= MSVCRT_LC_MAX; i++) {
		MSVCRT_free(locinfo->lc_category[i].locale);
		MSVCRT_free(locinfo->lc_category[i].refcount);
	}

	if (locinfo->lconv) {
		MSVCRT_free(locinfo->lconv->decimal_point);
		MSVCRT_free(locinfo->lconv->thousands_sep);
		MSVCRT_free(locinfo->lconv->grouping);
		MSVCRT_free(locinfo->lconv->int_curr_symbol);
		MSVCRT_free(locinfo->lconv->currency_symbol);
		MSVCRT_free(locinfo->lconv->mon_decimal_point);
		MSVCRT_free(locinfo->lconv->mon_thousands_sep);
		MSVCRT_free(locinfo->lconv->mon_grouping);
		MSVCRT_free(locinfo->lconv->positive_sign);
		MSVCRT_free(locinfo->lconv->negative_sign);
	}

	MSVCRT_free(locinfo->lconv_intl_refcount);
	MSVCRT_free(locinfo->lconv_num_refcount);
	MSVCRT_free(locinfo->lconv_mon_refcount);
	MSVCRT_free(locinfo->lconv);

	MSVCRT_free(locinfo->ctype1_refcount);
	MSVCRT_free(locinfo->ctype1);

	MSVCRT_free(locinfo->pclmap);
	MSVCRT_free(locinfo->pcumap);

	MSVCRT_free(locinfo->lc_time_curr);

	MSVCRT_free(locinfo);
}

/* INTERNAL: frees MSVCRT_pthreadmbcinfo struct */
void free_mbcinfo(MSVCRT_pthreadmbcinfo mbcinfo) {

	if (!mbcinfo) {
		return;
	}

	if (InterlockedDecrement(&mbcinfo->refcount)) {
		return;
	}

	MSVCRT_free(mbcinfo);
}

/* _free_locale - not exported in native msvcrt */
void CDECL MSVCRT__free_locale(MSVCRT__locale_t locale) {

	if (!locale) {
		return;
	}

	free_locinfo(locale->locinfo);
	free_mbcinfo(locale->mbcinfo);
	MSVCRT_free(locale);
}

/* Internal: Find the LCID for a locale specification */
LCID MSVCRT_locale_to_LCID(const char *locale, unsigned short *codepage) {

	LCID lcid;
	locale_search_t search;
	const char *cp, *region;

	memset(&search, 0, sizeof(locale_search_t));

	cp = strchr(locale, '.');
	region = strchr(locale, '_');

	lstrcpynA(search.search_language, locale, MAX_ELEM_LEN);
	if (region) {
		lstrcpynA(search.search_country, region + 1, MAX_ELEM_LEN);
		if (region - locale < MAX_ELEM_LEN) {
			search.search_language[region - locale] = '\0';
		}
	}
	else {
		search.search_country[0] = '\0';
	}

	if (cp) {
		lstrcpynA(search.search_codepage, cp + 1, MAX_ELEM_LEN);
		if (region && cp - region - 1 < MAX_ELEM_LEN) {
			search.search_country[cp - region - 1] = '\0';
		}
		if (cp - locale < MAX_ELEM_LEN) {
			search.search_language[cp - locale] = '\0';
		}
	}
	else {
		search.search_codepage[0] = '\0';
	}

	if (!search.search_country[0] && !search.search_codepage[0]) {
		remap_synonym(search.search_language, MAX_ELEM_LEN);
	}

	EnumResourceLanguagesA(GetModuleHandleA("KERNEL32"), (LPSTR)RT_STRING, (LPCSTR)LOCALE_ILANGUAGE,
						   find_best_locale_proc, (LONG_PTR)&search);

	if (!search.match_flags) {
		return -1;
	}

	/* If we were given something that didn't match, fail */
	if (search.search_country[0] && !(search.match_flags & FOUND_COUNTRY)) {
		return -1;
	}

	lcid = MAKELCID(search.found_lang_id, SORT_DEFAULT);

	/* Populate partial locale, translating LCID to locale string elements */
	if (!(search.match_flags & FOUND_CODEPAGE)) {
		/* Even if a codepage is not enumerated for a locale
		 * it can be set if valid */
		if (search.search_codepage[0]) {
			if (IsValidCodePage(atoi(search.search_codepage))) {
				memcpy(search.found_codepage, search.search_codepage, MAX_ELEM_LEN);
			}
			else {
				/* Special codepage values: OEM & ANSI */
				if (!mint::utf8_compare_case_insensitive(search.search_codepage, "OCP")) {
					GetLocaleInfoA(lcid, LOCALE_IDEFAULTCODEPAGE, search.found_codepage, MAX_ELEM_LEN);
				}
				else if (!mint::utf8_compare_case_insensitive(search.search_codepage, "ACP")) {
					GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, search.found_codepage, MAX_ELEM_LEN);
				}
				else {
					return -1;
				}

				if (!atoi(search.found_codepage)) {
					return -1;
				}
			}
		}
		else {
			/* Prefer ANSI codepages if present */
			GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, search.found_codepage, MAX_ELEM_LEN);
			if (!search.found_codepage[0] || !atoi(search.found_codepage)) {
				GetLocaleInfoA(lcid, LOCALE_IDEFAULTCODEPAGE, search.found_codepage, MAX_ELEM_LEN);
			}
		}
	}

	if (codepage) {
		*codepage = atoi(search.found_codepage);
	}

	return lcid;
}

/* It seems that the data about valid trail bytes is not available from kernel32
 * so we have to store is here. The format is the same as for lead bytes in CPINFO */
struct cp_extra_info_t {
	int cp;
	BYTE TrailBytes[MAX_LEADBYTES];
};

static struct cp_extra_info_t g_cpextrainfo[] = {
	{932, {0x40, 0x7e, 0x80, 0xfc, 0, 0}},
	{936, {0x40, 0xfe, 0, 0}},
	{949, {0x41, 0xfe, 0, 0}},
	{950, {0x40, 0x7e, 0xa1, 0xfe, 0, 0}},
	{1361, {0x31, 0x7e, 0x81, 0xfe, 0, 0}},
	{20932, {1, 255, 0, 0}}, /* seems to give different results on different systems */
	{0, {1, 255, 0, 0}}		 /* match all with FIXME */
};

/*********************************************************************
 * INTERNAL: _setmbcp_l
 */
int _setmbcp_l(int cp, LCID lcid, MSVCRT_pthreadmbcinfo mbcinfo) {
	const char format[] = ".%d";

	int newcp;
	CPINFO cpi;
	BYTE *bytes;
	WORD chartypes[256];
	char bufA[256];
	WCHAR bufW[256];
	int charcount;
	int ret;
	int i;

	switch (cp) {
	case _MB_CP_ANSI:
		newcp = GetACP();
		break;
	case _MB_CP_OEM:
		newcp = GetOEMCP();
		break;
	case _MB_CP_LOCALE:
		newcp = ___lc_codepage_func();
		if (newcp) {
			break;
		}
	/* fall through (C locale) */
	case _MB_CP_SBCS:
		newcp = 20127; /* ASCII */
		break;
	default:
		newcp = cp;
		break;
	}

	if (lcid == -1) {
		sprintf(bufA, format, newcp);
		mbcinfo->mblcid = MSVCRT_locale_to_LCID(bufA, NULL);
	}
	else {
		mbcinfo->mblcid = lcid;
	}

	if (mbcinfo->mblcid == -1) {
		WARN("Can't assign LCID to codepage (%d)\n", mbcinfo->mblcid);
		mbcinfo->mblcid = 0;
	}

	if (!GetCPInfo(newcp, &cpi)) {
		WARN("Codepage %d not found\n", newcp);
		*_errno() = EINVAL;
		return -1;
	}

	/* setup the _mbctype */
	memset(mbcinfo->mbctype, 0, sizeof(unsigned char[257]));
	memset(mbcinfo->mbcasemap, 0, sizeof(unsigned char[256]));

	bytes = cpi.LeadByte;
	while (bytes[0] || bytes[1]) {
		for (i = bytes[0]; i <= bytes[1]; i++) {
			mbcinfo->mbctype[i + 1] |= _M1;
		}
		bytes += 2;
	}

	if (cpi.MaxCharSize > 1) {
		/* trail bytes not available through kernel32 but stored in a structure in msvcrt */
		struct cp_extra_info_t *cpextra = g_cpextrainfo;

		mbcinfo->ismbcodepage = 1;
		while (TRUE) {
			if (cpextra->cp == 0 || cpextra->cp == newcp) {
				if (cpextra->cp == 0) {
					FIXME("trail bytes data not available for DBCS codepage %d - assuming all bytes\n", newcp);
				}

				bytes = cpextra->TrailBytes;
				while (bytes[0] || bytes[1]) {
					for (i = bytes[0]; i <= bytes[1]; i++) {
						mbcinfo->mbctype[i + 1] |= _M2;
					}
					bytes += 2;
				}
				break;
			}
			cpextra++;
		}
	}
	else {
		mbcinfo->ismbcodepage = 0;
	}

	/* we can't use GetStringTypeA directly because we don't have a locale - only a code page
	 */
	charcount = 0;
	for (i = 0; i < 256; i++) {
		if (!(mbcinfo->mbctype[i + 1] & _M1)) {
			bufA[charcount++] = i;
		}
	}

	ret = MultiByteToWideChar(newcp, 0, bufA, charcount, bufW, charcount);
	if (ret != charcount) {
		ERR("MultiByteToWideChar of chars failed for cp %d, ret=%d (exp %d), error=%d\n", newcp, ret, charcount,
			GetLastError());
	}

	GetStringTypeW(CT_CTYPE1, bufW, charcount, chartypes);

	charcount = 0;
	for (i = 0; i < 256; i++) {
		if (!(mbcinfo->mbctype[i + 1] & _M1)) {
			if (chartypes[charcount] & C1_UPPER) {
				mbcinfo->mbctype[i + 1] |= _SBUP;
				bufW[charcount] = towlower(bufW[charcount]);
			}
			else if (chartypes[charcount] & C1_LOWER) {
				mbcinfo->mbctype[i + 1] |= _SBLOW;
				bufW[charcount] = towupper(bufW[charcount]);
			}
			charcount++;
		}
	}

	ret = WideCharToMultiByte(newcp, 0, bufW, charcount, bufA, charcount, NULL, NULL);
	if (ret != charcount) {
		ERR("WideCharToMultiByte failed for cp %d, ret=%d (exp %d), error=%d\n", newcp, ret, charcount, GetLastError());
	}

	charcount = 0;
	for (i = 0; i < 256; i++) {
		if (!(mbcinfo->mbctype[i + 1] & _M1)) {
			if (mbcinfo->mbctype[i] & (C1_UPPER | C1_LOWER)) {
				mbcinfo->mbcasemap[i] = bufA[charcount];
			}
			charcount++;
		}
	}

	if (newcp == 932) { /* CP932 only - set _MP and _MS */
		/* On Windows it's possible to calculate the _MP and _MS from CT_CTYPE1
		 * and CT_CTYPE3. But as of Wine 0.9.43 we return wrong values what makes
		 * it hard. As this is set only for codepage 932 we hardcode it what gives
		 * also faster execution.
		 */
		for (i = 161; i <= 165; i++) {
			mbcinfo->mbctype[i + 1] |= _MP;
		}
		for (i = 166; i <= 223; i++) {
			mbcinfo->mbctype[i + 1] |= _MS;
		}
	}

	mbcinfo->mbcodepage = newcp;
	return 0;
}

/* INTERNAL: Set lc_handle, lc_id and lc_category in threadlocinfo struct */
static BOOL update_threadlocinfo_category(LCID lcid, unsigned short cp, MSVCRT__locale_t loc, int category) {

	char buf[256], *p;

	if (GetLocaleInfoA(lcid, LOCALE_ILANGUAGE | LOCALE_NOUSEROVERRIDE, buf, 256)) {
		p = buf;

		loc->locinfo->lc_id[category].wLanguage = 0;
		while (*p) {
			loc->locinfo->lc_id[category].wLanguage *= 16;

			if (*p <= '9') {
				loc->locinfo->lc_id[category].wLanguage += *p - '0';
			}
			else {
				loc->locinfo->lc_id[category].wLanguage += *p - 'a' + 10;
			}

			p++;
		}

		loc->locinfo->lc_id[category].wCountry = loc->locinfo->lc_id[category].wLanguage;
	}

	loc->locinfo->lc_id[category].wCodePage = cp;

	loc->locinfo->lc_handle[category] = lcid;

	int len = 0;
	len += GetLocaleInfoA(lcid, LOCALE_SENGLANGUAGE | LOCALE_NOUSEROVERRIDE, buf, 256);
	buf[len - 1] = '_';
	len += GetLocaleInfoA(lcid, LOCALE_SENGCOUNTRY | LOCALE_NOUSEROVERRIDE, &buf[len], 256 - len);
	buf[len - 1] = '.';
	sprintf(buf + len, "%u", cp);
	len += static_cast<int>(strlen(buf + len)) + 1;

	loc->locinfo->lc_category[category].locale = (char *)MSVCRT_malloc(len);
	loc->locinfo->lc_category[category].refcount = (int *)MSVCRT_malloc(sizeof(int));
	if (!loc->locinfo->lc_category[category].locale || !loc->locinfo->lc_category[category].refcount) {
		MSVCRT_free(loc->locinfo->lc_category[category].locale);
		MSVCRT_free(loc->locinfo->lc_category[category].refcount);
		loc->locinfo->lc_category[category].locale = NULL;
		loc->locinfo->lc_category[category].refcount = NULL;
		return TRUE;
	}
	memcpy(loc->locinfo->lc_category[category].locale, buf, len);
	*loc->locinfo->lc_category[category].refcount = 1;

	return FALSE;
}

static const DWORD _time_data[] = {LOCALE_SABBREVDAYNAME7,
								   LOCALE_SABBREVDAYNAME1,
								   LOCALE_SABBREVDAYNAME2,
								   LOCALE_SABBREVDAYNAME3,
								   LOCALE_SABBREVDAYNAME4,
								   LOCALE_SABBREVDAYNAME5,
								   LOCALE_SABBREVDAYNAME6,
								   LOCALE_SDAYNAME7,
								   LOCALE_SDAYNAME1,
								   LOCALE_SDAYNAME2,
								   LOCALE_SDAYNAME3,
								   LOCALE_SDAYNAME4,
								   LOCALE_SDAYNAME5,
								   LOCALE_SDAYNAME6,
								   LOCALE_SABBREVMONTHNAME1,
								   LOCALE_SABBREVMONTHNAME2,
								   LOCALE_SABBREVMONTHNAME3,
								   LOCALE_SABBREVMONTHNAME4,
								   LOCALE_SABBREVMONTHNAME5,
								   LOCALE_SABBREVMONTHNAME6,
								   LOCALE_SABBREVMONTHNAME7,
								   LOCALE_SABBREVMONTHNAME8,
								   LOCALE_SABBREVMONTHNAME9,
								   LOCALE_SABBREVMONTHNAME10,
								   LOCALE_SABBREVMONTHNAME11,
								   LOCALE_SABBREVMONTHNAME12,
								   LOCALE_SMONTHNAME1,
								   LOCALE_SMONTHNAME2,
								   LOCALE_SMONTHNAME3,
								   LOCALE_SMONTHNAME4,
								   LOCALE_SMONTHNAME5,
								   LOCALE_SMONTHNAME6,
								   LOCALE_SMONTHNAME7,
								   LOCALE_SMONTHNAME8,
								   LOCALE_SMONTHNAME9,
								   LOCALE_SMONTHNAME10,
								   LOCALE_SMONTHNAME11,
								   LOCALE_SMONTHNAME12,
								   LOCALE_S1159,
								   LOCALE_S2359,
								   LOCALE_SSHORTDATE,
								   LOCALE_SLONGDATE,
								   LOCALE_STIMEFORMAT};

/* _create_locale - not exported in native msvcrt */
MSVCRT__locale_t CDECL MSVCRT__create_locale(int category, const char *locale) {

	static const char collate[] = "COLLATE=";
	static const char ctype[] = "CTYPE=";
	static const char monetary[] = "MONETARY=";
	static const char numeric[] = "NUMERIC=";
	static const char time[] = "TIME=";
	static const char cloc_short_date[] = "MM/dd/yy";
	static const wchar_t cloc_short_dateW[] = {'M', 'M', '/', 'd', 'd', '/', 'y', 'y', 0};
	static const char cloc_long_date[] = "dddd, MMMM dd, yyyy";
	static const wchar_t cloc_long_dateW[] = {'d', 'd', 'd', 'd', ',', ' ', 'M', 'M', 'M', 'M',
											  ' ', 'd', 'd', ',', ' ', 'y', 'y', 'y', 'y', 0};
	static const char cloc_time[] = "HH:mm:ss";
	static const wchar_t cloc_timeW[] = {'H', 'H', ':', 'm', 'm', ':', 's', 's', 0};

	MSVCRT__locale_t loc;
	LCID lcid[6] = {0}, lcid_tmp;
	unsigned short cp[6] = {0};
	char buf[256];
	int i, ret, size;

	TRACE("(%d %s)\n", category, locale);

	if (category < MSVCRT_LC_MIN || category > MSVCRT_LC_MAX || !locale) {
		return NULL;
	}

	if (locale[0] == 'C' && !locale[1]) {
		lcid[0] = 0;
		cp[0] = CP_ACP;
	}
	else if (!locale[0]) {
		lcid[0] = GetSystemDefaultLCID();
		GetLocaleInfoA(lcid[0], LOCALE_IDEFAULTANSICODEPAGE | LOCALE_NOUSEROVERRIDE, buf, sizeof(buf));
		cp[0] = atoi(buf);

		for (i = 1; i < 6; i++) {
			lcid[i] = lcid[0];
			cp[i] = cp[0];
		}
	}
	else if (locale[0] == 'L' && locale[1] == 'C' && locale[2] == '_') {
		const char *p;

		while (1) {
			locale += 3; /* LC_ */
			if (!memcmp(locale, collate, sizeof(collate) - 1)) {
				i = MSVCRT_LC_COLLATE;
				locale += sizeof(collate) - 1;
			}
			else if (!memcmp(locale, ctype, sizeof(ctype) - 1)) {
				i = MSVCRT_LC_CTYPE;
				locale += sizeof(ctype) - 1;
			}
			else if (!memcmp(locale, monetary, sizeof(monetary) - 1)) {
				i = MSVCRT_LC_MONETARY;
				locale += sizeof(monetary) - 1;
			}
			else if (!memcmp(locale, numeric, sizeof(numeric) - 1)) {
				i = MSVCRT_LC_NUMERIC;
				locale += sizeof(numeric) - 1;
			}
			else if (!memcmp(locale, time, sizeof(time) - 1)) {
				i = MSVCRT_LC_TIME;
				locale += sizeof(time) - 1;
			}
			else {
				return NULL;
			}

			p = strchr(locale, ';');
			if (locale[0] == 'C' && (locale[1] == ';' || locale[1] == '\0')) {
				lcid[i] = 0;
				cp[i] = CP_ACP;
			}
			else if (p) {
				memcpy(buf, locale, p - locale);
				buf[p - locale] = '\0';
				lcid[i] = MSVCRT_locale_to_LCID(buf, &cp[i]);
			}
			else {
				lcid[i] = MSVCRT_locale_to_LCID(locale, &cp[i]);
			}

			if (lcid[i] == -1) {
				return NULL;
			}

			if (!p || *(p + 1) != 'L' || *(p + 2) != 'C' || *(p + 3) != '_') {
				break;
			}

			locale = p + 1;
		}
	}
	else {
		lcid[0] = MSVCRT_locale_to_LCID(locale, &cp[0]);
		if (lcid[0] == -1) {
			return NULL;
		}

		for (i = 1; i < 6; i++) {
			lcid[i] = lcid[0];
			cp[i] = cp[0];
		}
	}

	loc = (MSVCRT__locale_t)MSVCRT_malloc(sizeof(MSVCRT__locale_tstruct));
	if (!loc) {
		return NULL;
	}

	loc->locinfo = (MSVCRT_pthreadlocinfo)MSVCRT_malloc(sizeof(MSVCRT_threadlocinfo));
	if (!loc->locinfo) {
		MSVCRT_free(loc);
		return NULL;
	}

	loc->mbcinfo = (MSVCRT_pthreadmbcinfo)MSVCRT_malloc(sizeof(MSVCRT_threadmbcinfo));
	if (!loc->mbcinfo) {
		MSVCRT_free(loc->locinfo);
		MSVCRT_free(loc);
		return NULL;
	}

	memset(loc->locinfo, 0, sizeof(MSVCRT_threadlocinfo));
	loc->locinfo->refcount = 1;
	loc->mbcinfo->refcount = 1;

	loc->locinfo->lconv = (MSVCRT_lconv *)MSVCRT_malloc(sizeof(struct MSVCRT_lconv));
	if (!loc->locinfo->lconv) {
		MSVCRT__free_locale(loc);
		return NULL;
	}
	memset(loc->locinfo->lconv, 0, sizeof(struct MSVCRT_lconv));

	loc->locinfo->pclmap = static_cast<byte *>(MSVCRT_malloc(sizeof(byte[256])));
	loc->locinfo->pcumap = static_cast<byte *>(MSVCRT_malloc(sizeof(byte[256])));
	if (!loc->locinfo->pclmap || !loc->locinfo->pcumap) {
		MSVCRT__free_locale(loc);
		return NULL;
	}

	if (lcid[MSVCRT_LC_COLLATE] && (category == MSVCRT_LC_ALL || category == MSVCRT_LC_COLLATE)) {
		if (update_threadlocinfo_category(lcid[MSVCRT_LC_COLLATE], cp[MSVCRT_LC_COLLATE], loc, MSVCRT_LC_COLLATE)) {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		loc->locinfo->lc_collate_cp = loc->locinfo->lc_id[MSVCRT_LC_COLLATE].wCodePage;
	}
	else {
		loc->locinfo->lc_category[MSVCRT_LC_COLLATE].locale = _strdup("C");
	}

	if (lcid[MSVCRT_LC_CTYPE] && (category == MSVCRT_LC_ALL || category == MSVCRT_LC_CTYPE)) {
		CPINFO cp_info;
		int j;

		if (update_threadlocinfo_category(lcid[MSVCRT_LC_CTYPE], cp[MSVCRT_LC_CTYPE], loc, MSVCRT_LC_CTYPE)) {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		loc->locinfo->lc_codepage = loc->locinfo->lc_id[MSVCRT_LC_CTYPE].wCodePage;
		loc->locinfo->lc_clike = 1;
		if (!GetCPInfo(loc->locinfo->lc_codepage, &cp_info)) {
			MSVCRT__free_locale(loc);
			return NULL;
		}
		loc->locinfo->mb_cur_max = cp_info.MaxCharSize;

		loc->locinfo->ctype1_refcount = static_cast<int *>(MSVCRT_malloc(sizeof(int)));
		loc->locinfo->ctype1 = static_cast<unsigned short *>(MSVCRT_malloc(sizeof(unsigned short[257])));
		if (!loc->locinfo->ctype1_refcount || !loc->locinfo->ctype1) {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		*loc->locinfo->ctype1_refcount = 1;
		loc->locinfo->ctype1[0] = 0;
		loc->locinfo->pctype = loc->locinfo->ctype1 + 1;

		buf[1] = buf[2] = '\0';
		for (i = 1; i < 257; i++) {
			buf[0] = i - 1;

			/* builtin GetStringTypeA doesn't set output to 0 on invalid input */
			loc->locinfo->ctype1[i] = 0;

			GetStringTypeA(lcid[MSVCRT_LC_CTYPE], CT_CTYPE1, buf, 1, loc->locinfo->ctype1 + i);
		}

		for (i = 0; cp_info.LeadByte[i + 1] != 0; i += 2) {
			for (j = cp_info.LeadByte[i]; j <= cp_info.LeadByte[i + 1]; j++) {
				loc->locinfo->ctype1[j + 1] |= _LEADBYTE;
			}
		}
	}
	else {
		loc->locinfo->lc_clike = 1;
		loc->locinfo->mb_cur_max = 1;
		loc->locinfo->pctype = (unsigned short *)__acrt_get_locale_data_prefix(_get_current_locale())->_locale_pctype
							   + 1;
		loc->locinfo->lc_category[MSVCRT_LC_CTYPE].locale = _strdup("C");
	}

	for (i = 0; i < 256; i++) {
		if (loc->locinfo->pctype[i] & _LEADBYTE) {
			buf[i] = ' ';
		}
		else {
			buf[i] = i;
		}
	}

	if (lcid[MSVCRT_LC_CTYPE]) {
		LCMapStringA(lcid[MSVCRT_LC_CTYPE], LCMAP_LOWERCASE, buf, 256, (char *)loc->locinfo->pclmap, 256);
		LCMapStringA(lcid[MSVCRT_LC_CTYPE], LCMAP_UPPERCASE, buf, 256, (char *)loc->locinfo->pcumap, 256);
	}
	else {
		for (i = 0; i < 256; i++) {
			loc->locinfo->pclmap[i] = (i >= 'A' && i <= 'Z' ? i - 'A' + 'a' : i);
			loc->locinfo->pcumap[i] = (i >= 'a' && i <= 'z' ? i - 'a' + 'A' : i);
		}
	}

	_setmbcp_l(loc->locinfo->lc_id[MSVCRT_LC_CTYPE].wCodePage, lcid[MSVCRT_LC_CTYPE], loc->mbcinfo);

	if (lcid[MSVCRT_LC_MONETARY] && (category == MSVCRT_LC_ALL || category == MSVCRT_LC_MONETARY)) {
		if (update_threadlocinfo_category(lcid[MSVCRT_LC_MONETARY], cp[MSVCRT_LC_MONETARY], loc, MSVCRT_LC_MONETARY)) {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		loc->locinfo->lconv_intl_refcount = (int *)MSVCRT_malloc(sizeof(int));
		loc->locinfo->lconv_mon_refcount = (int *)MSVCRT_malloc(sizeof(int));
		if (!loc->locinfo->lconv_intl_refcount || !loc->locinfo->lconv_mon_refcount) {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		*loc->locinfo->lconv_intl_refcount = 1;
		*loc->locinfo->lconv_mon_refcount = 1;

		i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SINTLSYMBOL | LOCALE_NOUSEROVERRIDE, buf, 256);
		if (i && (loc->locinfo->lconv->int_curr_symbol = (char *)MSVCRT_malloc(i))) {
			memcpy(loc->locinfo->lconv->int_curr_symbol, buf, i);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SCURRENCY | LOCALE_NOUSEROVERRIDE, buf, 256);
		if (i && (loc->locinfo->lconv->currency_symbol = (char *)MSVCRT_malloc(i))) {
			memcpy(loc->locinfo->lconv->currency_symbol, buf, i);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SMONDECIMALSEP | LOCALE_NOUSEROVERRIDE, buf, 256);
		if (i && (loc->locinfo->lconv->mon_decimal_point = (char *)MSVCRT_malloc(i))) {
			memcpy(loc->locinfo->lconv->mon_decimal_point, buf, i);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SMONTHOUSANDSEP | LOCALE_NOUSEROVERRIDE, buf, 256);
		if (i && (loc->locinfo->lconv->mon_thousands_sep = (char *)MSVCRT_malloc(i))) {
			memcpy(loc->locinfo->lconv->mon_thousands_sep, buf, i);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SMONGROUPING | LOCALE_NOUSEROVERRIDE, buf, 256);
		if (i > 1) {
			i = i / 2 + (buf[i - 2] == '0' ? 0 : 1);
		}
		if (i && (loc->locinfo->lconv->mon_grouping = (char *)MSVCRT_malloc(i))) {
			for (i = 0; buf[i + 1] == ';'; i += 2) {
				loc->locinfo->lconv->mon_grouping[i / 2] = buf[i] - '0';
			}
			loc->locinfo->lconv->mon_grouping[i / 2] = buf[i] - '0';
			if (buf[i] != '0') {
				loc->locinfo->lconv->mon_grouping[i / 2 + 1] = 127;
			}
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SPOSITIVESIGN | LOCALE_NOUSEROVERRIDE, buf, 256);
		if (i && (loc->locinfo->lconv->positive_sign = (char *)MSVCRT_malloc(i))) {
			memcpy(loc->locinfo->lconv->positive_sign, buf, i);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		i = GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_SNEGATIVESIGN | LOCALE_NOUSEROVERRIDE, buf, 256);
		if (i && (loc->locinfo->lconv->negative_sign = (char *)MSVCRT_malloc(i))) {
			memcpy(loc->locinfo->lconv->negative_sign, buf, i);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		if (GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IINTLCURRDIGITS | LOCALE_NOUSEROVERRIDE, buf, 256)) {
			loc->locinfo->lconv->int_frac_digits = atoi(buf);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		if (GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_ICURRDIGITS | LOCALE_NOUSEROVERRIDE, buf, 256)) {
			loc->locinfo->lconv->frac_digits = atoi(buf);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		if (GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IPOSSYMPRECEDES | LOCALE_NOUSEROVERRIDE, buf, 256)) {
			loc->locinfo->lconv->p_cs_precedes = atoi(buf);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		if (GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IPOSSEPBYSPACE | LOCALE_NOUSEROVERRIDE, buf, 256)) {
			loc->locinfo->lconv->p_sep_by_space = atoi(buf);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		if (GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_INEGSYMPRECEDES | LOCALE_NOUSEROVERRIDE, buf, 256)) {
			loc->locinfo->lconv->n_cs_precedes = atoi(buf);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		if (GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_INEGSEPBYSPACE | LOCALE_NOUSEROVERRIDE, buf, 256)) {
			loc->locinfo->lconv->n_sep_by_space = atoi(buf);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		if (GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_IPOSSIGNPOSN | LOCALE_NOUSEROVERRIDE, buf, 256)) {
			loc->locinfo->lconv->p_sign_posn = atoi(buf);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		if (GetLocaleInfoA(lcid[MSVCRT_LC_MONETARY], LOCALE_INEGSIGNPOSN | LOCALE_NOUSEROVERRIDE, buf, 256)) {
			loc->locinfo->lconv->n_sign_posn = atoi(buf);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}
	}
	else {
		loc->locinfo->lconv->int_curr_symbol = (char *)MSVCRT_malloc(sizeof(char));
		loc->locinfo->lconv->currency_symbol = (char *)MSVCRT_malloc(sizeof(char));
		loc->locinfo->lconv->mon_decimal_point = (char *)MSVCRT_malloc(sizeof(char));
		loc->locinfo->lconv->mon_thousands_sep = (char *)MSVCRT_malloc(sizeof(char));
		loc->locinfo->lconv->mon_grouping = (char *)MSVCRT_malloc(sizeof(char));
		loc->locinfo->lconv->positive_sign = (char *)MSVCRT_malloc(sizeof(char));
		loc->locinfo->lconv->negative_sign = (char *)MSVCRT_malloc(sizeof(char));

		if (!loc->locinfo->lconv->int_curr_symbol || !loc->locinfo->lconv->currency_symbol
			|| !loc->locinfo->lconv->mon_decimal_point || !loc->locinfo->lconv->mon_thousands_sep
			|| !loc->locinfo->lconv->mon_grouping || !loc->locinfo->lconv->positive_sign
			|| !loc->locinfo->lconv->negative_sign) {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		loc->locinfo->lconv->int_curr_symbol[0] = '\0';
		loc->locinfo->lconv->currency_symbol[0] = '\0';
		loc->locinfo->lconv->mon_decimal_point[0] = '\0';
		loc->locinfo->lconv->mon_thousands_sep[0] = '\0';
		loc->locinfo->lconv->mon_grouping[0] = '\0';
		loc->locinfo->lconv->positive_sign[0] = '\0';
		loc->locinfo->lconv->negative_sign[0] = '\0';
		loc->locinfo->lconv->int_frac_digits = 127;
		loc->locinfo->lconv->frac_digits = 127;
		loc->locinfo->lconv->p_cs_precedes = 127;
		loc->locinfo->lconv->p_sep_by_space = 127;
		loc->locinfo->lconv->n_cs_precedes = 127;
		loc->locinfo->lconv->n_sep_by_space = 127;
		loc->locinfo->lconv->p_sign_posn = 127;
		loc->locinfo->lconv->n_sign_posn = 127;

		loc->locinfo->lc_category[MSVCRT_LC_MONETARY].locale = _strdup("C");
	}

	if (lcid[MSVCRT_LC_NUMERIC] && (category == MSVCRT_LC_ALL || category == MSVCRT_LC_NUMERIC)) {
		if (update_threadlocinfo_category(lcid[MSVCRT_LC_NUMERIC], cp[MSVCRT_LC_NUMERIC], loc, MSVCRT_LC_NUMERIC)) {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		if (!loc->locinfo->lconv_intl_refcount) {
			loc->locinfo->lconv_intl_refcount = (int *)MSVCRT_malloc(sizeof(int));
		}
		loc->locinfo->lconv_num_refcount = (int *)MSVCRT_malloc(sizeof(int));
		if (!loc->locinfo->lconv_intl_refcount || !loc->locinfo->lconv_num_refcount) {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		*loc->locinfo->lconv_intl_refcount = 1;
		*loc->locinfo->lconv_num_refcount = 1;

		i = GetLocaleInfoA(lcid[MSVCRT_LC_NUMERIC], LOCALE_SDECIMAL | LOCALE_NOUSEROVERRIDE, buf, 256);
		if (i && (loc->locinfo->lconv->decimal_point = (char *)MSVCRT_malloc(i))) {
			memcpy(loc->locinfo->lconv->decimal_point, buf, i);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		i = GetLocaleInfoA(lcid[MSVCRT_LC_NUMERIC], LOCALE_STHOUSAND | LOCALE_NOUSEROVERRIDE, buf, 256);
		if (i && (loc->locinfo->lconv->thousands_sep = (char *)MSVCRT_malloc(i))) {
			memcpy(loc->locinfo->lconv->thousands_sep, buf, i);
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		i = GetLocaleInfoA(lcid[MSVCRT_LC_NUMERIC], LOCALE_SGROUPING | LOCALE_NOUSEROVERRIDE, buf, 256);
		if (i > 1) {
			i = i / 2 + (buf[i - 2] == '0' ? 0 : 1);
		}
		if (i && (loc->locinfo->lconv->grouping = (char *)MSVCRT_malloc(i))) {
			for (i = 0; buf[i + 1] == ';'; i += 2) {
				loc->locinfo->lconv->grouping[i / 2] = buf[i] - '0';
			}
			loc->locinfo->lconv->grouping[i / 2] = buf[i] - '0';
			if (buf[i] != '0') {
				loc->locinfo->lconv->grouping[i / 2 + 1] = 127;
			}
		}
		else {
			MSVCRT__free_locale(loc);
			return NULL;
		}
	}
	else {
		loc->locinfo->lconv->decimal_point = (char *)MSVCRT_malloc(sizeof(char[2]));
		loc->locinfo->lconv->thousands_sep = (char *)MSVCRT_malloc(sizeof(char));
		loc->locinfo->lconv->grouping = (char *)MSVCRT_malloc(sizeof(char));
		if (!loc->locinfo->lconv->decimal_point || !loc->locinfo->lconv->thousands_sep
			|| !loc->locinfo->lconv->grouping) {
			MSVCRT__free_locale(loc);
			return NULL;
		}

		loc->locinfo->lconv->decimal_point[0] = '.';
		loc->locinfo->lconv->decimal_point[1] = '\0';
		loc->locinfo->lconv->thousands_sep[0] = '\0';
		loc->locinfo->lconv->grouping[0] = '\0';

		loc->locinfo->lc_category[MSVCRT_LC_NUMERIC].locale = _strdup("C");
	}

	if (lcid[MSVCRT_LC_TIME] && (category == MSVCRT_LC_ALL || category == MSVCRT_LC_TIME)) {
		if (update_threadlocinfo_category(lcid[MSVCRT_LC_TIME], cp[MSVCRT_LC_TIME], loc, MSVCRT_LC_TIME)) {
			MSVCRT__free_locale(loc);
			return NULL;
		}
	}
	else {
		loc->locinfo->lc_category[MSVCRT_LC_TIME].locale = _strdup("C");
	}

	size = sizeof(MSVCRT___lc_time_data);
	lcid_tmp = lcid[MSVCRT_LC_TIME] ? lcid[MSVCRT_LC_TIME] : MAKELCID(LANG_ENGLISH, SORT_DEFAULT);
	for (i = 0; i < sizeof(_time_data) / sizeof(_time_data[0]); i++) {
		if (_time_data[i] == LOCALE_SSHORTDATE && !lcid[MSVCRT_LC_TIME]) {
			size += sizeof(cloc_short_date) + sizeof(cloc_short_dateW);
		}
		else if (_time_data[i] == LOCALE_SLONGDATE && !lcid[MSVCRT_LC_TIME]) {
			size += sizeof(cloc_long_date) + sizeof(cloc_long_dateW);
		}
		else {
			ret = GetLocaleInfoA(lcid_tmp, _time_data[i] | LOCALE_NOUSEROVERRIDE, NULL, 0);
			if (!ret) {
				MSVCRT__free_locale(loc);
				return NULL;
			}
			size += ret;

			ret = GetLocaleInfoW(lcid_tmp, _time_data[i] | LOCALE_NOUSEROVERRIDE, NULL, 0);
			if (!ret) {
				MSVCRT__free_locale(loc);
				return NULL;
			}
			size += ret * sizeof(wchar_t);
		}
	}

	loc->locinfo->lc_time_curr = (MSVCRT___lc_time_data *)MSVCRT_malloc(size);
	if (!loc->locinfo->lc_time_curr) {
		MSVCRT__free_locale(loc);
		return NULL;
	}

	ret = 0;
	for (i = 0; i < sizeof(_time_data) / sizeof(_time_data[0]); i++) {
		loc->locinfo->lc_time_curr->str.str[i] = &loc->locinfo->lc_time_curr->data[ret];
		if (_time_data[i] == LOCALE_SSHORTDATE && !lcid[MSVCRT_LC_TIME]) {
			memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_short_date, sizeof(cloc_short_date));
			ret += sizeof(cloc_short_date);
		}
		else if (_time_data[i] == LOCALE_SLONGDATE && !lcid[MSVCRT_LC_TIME]) {
			memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_long_date, sizeof(cloc_long_date));
			ret += sizeof(cloc_long_date);
		}
		else if (_time_data[i] == LOCALE_STIMEFORMAT && !lcid[MSVCRT_LC_TIME]) {
			memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_time, sizeof(cloc_time));
			ret += sizeof(cloc_time);
		}
		else {
			ret += GetLocaleInfoA(lcid_tmp, _time_data[i] | LOCALE_NOUSEROVERRIDE,
								  &loc->locinfo->lc_time_curr->data[ret], size - ret);
		}
	}
	for (i = 0; i < sizeof(_time_data) / sizeof(_time_data[0]); i++) {
		loc->locinfo->lc_time_curr->wstr[i] = (wchar_t *)&loc->locinfo->lc_time_curr->data[ret];
		if (_time_data[i] == LOCALE_SSHORTDATE && !lcid[MSVCRT_LC_TIME]) {
			memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_short_dateW, sizeof(cloc_short_dateW));
			ret += sizeof(cloc_short_dateW);
		}
		else if (_time_data[i] == LOCALE_SLONGDATE && !lcid[MSVCRT_LC_TIME]) {
			memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_long_dateW, sizeof(cloc_long_dateW));
			ret += sizeof(cloc_long_dateW);
		}
		else if (_time_data[i] == LOCALE_STIMEFORMAT && !lcid[MSVCRT_LC_TIME]) {
			memcpy(&loc->locinfo->lc_time_curr->data[ret], cloc_timeW, sizeof(cloc_timeW));
			ret += sizeof(cloc_timeW);
		}
		else {
			ret += GetLocaleInfoW(lcid_tmp, _time_data[i] | LOCALE_NOUSEROVERRIDE,
								  (wchar_t *)&loc->locinfo->lc_time_curr->data[ret], size - ret)
				   * sizeof(wchar_t);
		}
	}
	loc->locinfo->lc_time_curr->lcid = lcid[MSVCRT_LC_TIME];

	return loc;
}

char *_windows_to_strftime(const char *format, BOOL am_pm) {

	static char g_buffer[MAX_ELEM_LEN];
	char *cur = g_buffer;

	static auto count = [](const char **cptr) -> int {
		int i = 1;

		while ((*cptr)[i] && ((*cptr)[i] == **cptr)) {
			++i;
		}

		*cptr = *cptr + (i - 1);
		return i;
	};

	for (const char *cptr = format; *cptr; ++cptr) {
		switch (*cptr) {
		case 'd':
			switch (count(&cptr)) {
			case 1:
			case 2:
				*(cur++) = '%';
				*(cur++) = 'd';
				break;
			case 3:
				*(cur++) = '%';
				*(cur++) = 'a';
				break;
			case 4:
				*(cur++) = '%';
				*(cur++) = 'A';
				break;
			default:
				break;
			}
			break;
		case 'h':
			switch (count(&cptr)) {
			case 1:
			case 2:
				*(cur++) = '%';
				*(cur++) = 'I';
				break;
			default:
				break;
			}
			break;
		case 'H':
			switch (count(&cptr)) {
			case 1:
			case 2:
				*(cur++) = '%';
				*(cur++) = 'H';
				break;
			default:
				break;
			}
			break;
		case 'm':
			switch (count(&cptr)) {
			case 1:
			case 2:
				*(cur++) = '%';
				*(cur++) = 'M';
				break;
			default:
				break;
			}
			break;
		case 'M':
			switch (count(&cptr)) {
			case 1:
			case 2:
				*(cur++) = '%';
				*(cur++) = 'm';
				break;
			case 3:
				*(cur++) = '%';
				*(cur++) = 'b';
				break;
			case 4:
				*(cur++) = '%';
				*(cur++) = 'B';
				break;
			default:
				break;
			}
			break;
		case 's':
			switch (count(&cptr)) {
			case 1:
			case 2:
				*(cur++) = '%';
				*(cur++) = 'S';
				break;
			default:
				break;
			}
			break;
		case 't':
			switch (count(&cptr)) {
			case 1:
			case 2:
				*(cur++) = '%';
				*(cur++) = 'p';
				break;
			default:
				break;
			}
			break;
		case 'y':
			switch (count(&cptr)) {
			case 1:
			case 2:
				*(cur++) = '%';
				*(cur++) = 'y';
				break;
			case 3:
			case 4:
				*(cur++) = '%';
				*(cur++) = 'Y';
				break;
			default:
				break;
			}
			break;
		case 'k':
			*(cur++) = '%';
			*(cur++) = 'Z';
			break;
		case 'z':
			*(cur++) = '%';
			*(cur++) = 'Z';
			break;
		case 'f':
			break;
		default:
			*(cur++) = *cptr;
			break;
		}
	}

	*(cur++) = '\0';
	return g_buffer;
}

char *_nl_langinfo_time(int index, MSVCRT_threadlocinfo *locinfo) {
	switch (index) {
	case _NL_ITEM_INDEX(ABDAY_1):
	case _NL_ITEM_INDEX(ABDAY_2):
	case _NL_ITEM_INDEX(ABDAY_3):
	case _NL_ITEM_INDEX(ABDAY_4):
	case _NL_ITEM_INDEX(ABDAY_5):
	case _NL_ITEM_INDEX(ABDAY_6):
	case _NL_ITEM_INDEX(ABDAY_7):
		return locinfo->lc_time_curr->str.names.short_wday[index - _NL_ITEM_INDEX(ABDAY_1)];
	case _NL_ITEM_INDEX(DAY_1):
	case _NL_ITEM_INDEX(DAY_2):
	case _NL_ITEM_INDEX(DAY_3):
	case _NL_ITEM_INDEX(DAY_4):
	case _NL_ITEM_INDEX(DAY_5):
	case _NL_ITEM_INDEX(DAY_6):
	case _NL_ITEM_INDEX(DAY_7):
		return locinfo->lc_time_curr->str.names.wday[index - _NL_ITEM_INDEX(DAY_1)];
	case _NL_ITEM_INDEX(ABMON_1):
	case _NL_ITEM_INDEX(ABMON_2):
	case _NL_ITEM_INDEX(ABMON_3):
	case _NL_ITEM_INDEX(ABMON_4):
	case _NL_ITEM_INDEX(ABMON_5):
	case _NL_ITEM_INDEX(ABMON_6):
	case _NL_ITEM_INDEX(ABMON_7):
	case _NL_ITEM_INDEX(ABMON_8):
	case _NL_ITEM_INDEX(ABMON_9):
	case _NL_ITEM_INDEX(ABMON_10):
	case _NL_ITEM_INDEX(ABMON_11):
	case _NL_ITEM_INDEX(ABMON_12):
		return locinfo->lc_time_curr->str.names.short_mon[index - _NL_ITEM_INDEX(ABMON_1)];
	case _NL_ITEM_INDEX(MON_1):
	case _NL_ITEM_INDEX(MON_2):
	case _NL_ITEM_INDEX(MON_3):
	case _NL_ITEM_INDEX(MON_4):
	case _NL_ITEM_INDEX(MON_5):
	case _NL_ITEM_INDEX(MON_6):
	case _NL_ITEM_INDEX(MON_7):
	case _NL_ITEM_INDEX(MON_8):
	case _NL_ITEM_INDEX(MON_9):
	case _NL_ITEM_INDEX(MON_10):
	case _NL_ITEM_INDEX(MON_11):
	case _NL_ITEM_INDEX(MON_12):
		return locinfo->lc_time_curr->str.names.mon[index - _NL_ITEM_INDEX(MON_1)];
	case _NL_ITEM_INDEX(AM_STR):
		return locinfo->lc_time_curr->str.names.am;
	case _NL_ITEM_INDEX(PM_STR):
		return locinfo->lc_time_curr->str.names.pm;
	case _NL_ITEM_INDEX(D_T_FMT):
		return _windows_to_strftime(locinfo->lc_time_curr->str.names.date, FALSE);
	case _NL_ITEM_INDEX(D_FMT):
		return _windows_to_strftime(locinfo->lc_time_curr->str.names.short_date, FALSE);
	case _NL_ITEM_INDEX(T_FMT):
		return _windows_to_strftime(locinfo->lc_time_curr->str.names.time, FALSE);
	case _NL_ITEM_INDEX(T_FMT_AMPM):
		return _windows_to_strftime(locinfo->lc_time_curr->str.names.time, TRUE);
	}

	return nullptr;
}

char *CDECL nl_langinfo_l(nl_item item, MSVCRT__locale_t locale) {

	switch (_NL_ITEM_CATEGORY(item)) {
	case MSVCRT_LC_CTYPE:
		return nullptr;
	case MSVCRT_LC_NUMERIC:
		return nullptr;
	case MSVCRT_LC_TIME:
		return _nl_langinfo_time(_NL_ITEM_INDEX(item), get_locinfo(locale));
	case MSVCRT_LC_COLLATE:
		return nullptr;
	case MSVCRT_LC_MONETARY:
		return nullptr;
	case MSVCRT_LC_ALL:
		return nullptr;
#if 0
	case MSVCRT_LC_MESSAGES:
		return nullptr;
	case MSVCRT_LC_PAPER:
		return nullptr;
	case MSVCRT_LC_NAME:
		return nullptr;
	case MSVCRT_LC_ADDRESS:
		return nullptr;
	case MSVCRT_LC_TELEPHONE:
		return nullptr;
	case MSVCRT_LC_MEASUREMENT:
		return nullptr;
	case MSVCRT_LC_IDENTIFICATION:
		return nullptr;
#endif
	}

	return nullptr;
}
