/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Szabados, Kristof
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include <TTCN3.hh>

namespace cstr__content {

void us_log(const UNIVERSAL_CHARSTRING& us) {
	TTCN_Logger::begin_event(TTCN_USER);
	us.log();
	TTCN_Logger::end_event();
}

INTEGER tc__univ__char__cpp() {
	const universal_char uc_array[] = {
		{ 0, 0, 1, 40 },
		{ 0, 0, 1, 41 },
		{ 0, 0, 1, 42 },
		{ 0, 0, 1, 43 }
	};
	UNIVERSAL_CHARSTRING us(1, 1, 1, 1);
	universal_char uc = { 1, 1, 1, 1 };
	if (us != uc || uc != us)
		return __LINE__;
	UNIVERSAL_CHARSTRING us2;
	us2 = uc;
	us_log(us2);
	if (us2 != uc || uc != us2)
		return __LINE__;
	us = "a";
	us_log(us);
	if (us != "a" || "a" != us)
		return __LINE__;
	UNIVERSAL_CHARSTRING us3("abcd");
	us_log(us3);
	if (us3 != "abcd" || "abcd" != us3)
		return __LINE__;
	UNIVERSAL_CHARSTRING us4(uc);
	us_log(us4);
	if (us4 != uc || uc != us4)
		return __LINE__;
	UNIVERSAL_CHARSTRING us5(4, "abcd");
	us_log(us5);
	if (us5 != "abcd" || "abcd" != us5)
		return __LINE__;
	const universal_char* p_uc = static_cast<const universal_char*>(us5);
	UNIVERSAL_CHARSTRING us_puc(4, p_uc);
	us_log(us_puc);
	universal_char uc_a = { 0, 0, 0, 97 };
	if (us5[0] != uc_a || uc_a != us5[0] || us5[0] != "a" || "a" != us5[0])
		return __LINE__;
	us5[0] = "b";
	us_log(us5);
	if (us5[0] != "b" || "b" != us5[0] || us5[0] != us5[1] || us5 != "bbcd" || "bbcd" != us5)
		return __LINE__;
	us5[0] = uc_a;
	us_log(us5);
	if (us5[0] != "a" || "a" != us5[0] || us5 != "abcd" || "abcd" != us5)
		return __LINE__;
	CHARSTRING csx("x");
	us5[0] = csx;
	us_log(us5);
	if (us5[0] != "x" || "x" != us5[0] || us5 != "xbcd" || "xbcd" != us5)
		return __LINE__;
	us5[0] = "a";
	us5[0] = csx[0];
	us_log(us5);
	if (us5[0] != "x" || "x" != us5[0] || us5 != "xbcd" || "xbcd" != us5)
		return __LINE__;
	us5[0] = us2;
	us_log(us5);
	if (us5[0] != us2)
		return __LINE__;
	us5[0] = "a";
	us5[0] = us2[0];
	us_log(us5);
	if (us5[0] != us2)
		return __LINE__;

	// UNIVERSAL_CHARSTRING_ELEMENT operator+ tests
	universal_char uc_res[] = {
		{ 0, 0, 0, 97 },
		{ 0, 0, 0, 98 }
	};
	universal_char uc_res_abc[] = {
		{ 0, 0, 0, 97 },
		{ 0, 0, 0, 98 },
		{ 0, 0, 0, 99 }
	};
	universal_char uc_res_abX[] = {
		{ 0, 0, 0, 97 },
		{ 0, 0, 0, 98 },
		{ 1, 1, 1,  1 }
	};
	UNIVERSAL_CHARSTRING us_res_abc("abc");
	UNIVERSAL_CHARSTRING us_res_abc_uni(3, uc_res_abc);
	UNIVERSAL_CHARSTRING us_res_abX(3, uc_res_abX);
	UNIVERSAL_CHARSTRING use1("ab");
	UNIVERSAL_CHARSTRING use2(2, uc_res);
	universal_char uc_b = { 0, 0, 0, 98 };
	universal_char uc_c = { 0, 0, 0, 99 };
	UNIVERSAL_CHARSTRING us_a(uc_a);
	UNIVERSAL_CHARSTRING us_b(uc_b);
	UNIVERSAL_CHARSTRING uce_res(2, uc_res);
	CHARSTRING cse_res("ab");
	CHARSTRING cs_a("a");
	CHARSTRING cs_b("b");
	UNIVERSAL_CHARSTRING use;
	use = use1[0] + "b";
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = "a" + use1[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use2[0] + "b";
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = "a" + use2[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use1[0] + uc_b;
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = uc_a + use1[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use2[0] + uc_b;
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = uc_a + use2[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use1[0] + us_b;
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = us_a + use1[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use2[0] + us_b;
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = us_a + use2[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use1[0] + cs_b;
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = cs_a + use1[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use2[0] + cs_b;
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = cs_a + use2[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use1[0] + us_b;
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = us_a + use1[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use2[0] + us_b;
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = us_a + use2[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use1[0] + use1[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use2[0] + use2[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use1[0] + use2[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;
	use = use2[0] + use1[1];
	us_log(use);
	if (use != "ab" || "ab" != use || use != uce_res || uce_res != use || use != cse_res || cse_res != use)
		return __LINE__;

	// UNIVERSAL_CHARSTRING operator+
	use = use1 + uc_c;
	us_log(use);
	if (use != us_res_abc || use != us_res_abc_uni || use != "abc")
		return __LINE__;
	use = use2 + uc_c;
	us_log(use);
	if (use != us_res_abc || use != us_res_abc_uni || use != "abc")
		return __LINE__;
	use = uc_c + use1;
	us_log(use);
	if (use != "cab")
		return __LINE__;
	use = uc_c + use2;
	us_log(use);
	if (use != "cab")
		return __LINE__;
	use = use1 + uc;
	us_log(use);
	if (use != us_res_abX)
		return __LINE__;
	use = use2 + uc;
	us_log(use);
	if (use != us_res_abX)
		return __LINE__;
	use = use1 + cs_a;
	us_log(use);
	if (use != "aba")
		return __LINE__;
	use = use2 + cs_a;
	us_log(use);
	if (use != "aba")
		return __LINE__;

	UNIVERSAL_CHARSTRING us_from_uc(4, uc_array);
	for (int i = 0; i < 4; i++)
		if (us_from_uc[i] != uc_array[i] || uc_array[i] != us_from_uc[i])
			return __LINE__;
	CHARSTRING cs("abcd");
	UNIVERSAL_CHARSTRING us6(cs);
	us_log(us6);
	if (us6 != "abcd" || "abcd" != us6)
		return __LINE__;
	UNIVERSAL_CHARSTRING us7(cs[0]);
	us_log(us7);
	if (us7 != "a" || "a" != us7)
		return __LINE__;
	return 0;
}

}
