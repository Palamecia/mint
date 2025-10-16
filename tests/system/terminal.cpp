#include <gtest/gtest.h>
#include "mint/system/terminal.h"

TEST(terminal, mint_term_opt) {
	EXPECT_STREQ("\033[m", MINT_TERM_OPT());
	EXPECT_STREQ("\033[0m", MINT_TERM_OPT("0"));
	EXPECT_STREQ("\033[0;1m", MINT_TERM_OPT("0", "1"));
	EXPECT_STREQ("\033[0;1;2m", MINT_TERM_OPT("0", "1", "2"));
	EXPECT_STREQ("\033[0;1;2;3m", MINT_TERM_OPT("0", "1", "2", "3"));
	EXPECT_STREQ("\033[0;1;2;3;4m", MINT_TERM_OPT("0", "1", "2", "3", "4"));
	EXPECT_STREQ("\033[0;1;2;3;4;5m", MINT_TERM_OPT("0", "1", "2", "3", "4", "5"));
	EXPECT_STREQ("\033[0;1;2;3;4;5;6m", MINT_TERM_OPT("0", "1", "2", "3", "4", "5", "6"));
	EXPECT_STREQ("\033[0;1;2;3;4;5;6;7m", MINT_TERM_OPT("0", "1", "2", "3", "4", "5", "6", "7"));
	EXPECT_STREQ("\033[0;1;2;3;4;5;6;7;8m", MINT_TERM_OPT("0", "1", "2", "3", "4", "5", "6", "7", "8"));
	EXPECT_STREQ("\033[0;1;2;3;4;5;6;7;8;9m", MINT_TERM_OPT("0", "1", "2", "3", "4", "5", "6", "7", "8", "9"));
	EXPECT_STREQ("\033[0;1;2;3;4;5;6;7;8;9;Am", MINT_TERM_OPT("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A"));
	EXPECT_STREQ("\033[0;1;2;3;4;5;6;7;8;9;A;Bm",
	    MINT_TERM_OPT("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B"));
	EXPECT_STREQ("\033[0;1;2;3;4;5;6;7;8;9;A;B;Cm",
	    MINT_TERM_OPT("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C"));
	EXPECT_STREQ("\033[0;1;2;3;4;5;6;7;8;9;A;B;C;Dm",
	    MINT_TERM_OPT("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D"));
	EXPECT_STREQ("\033[0;1;2;3;4;5;6;7;8;9;A;B;C;D;Em",
	    MINT_TERM_OPT("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E"));
	EXPECT_STREQ("\033[0;1;2;3;4;5;6;7;8;9;A;B;C;D;E;Fm",
	    MINT_TERM_OPT("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"));
}
