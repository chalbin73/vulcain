#pragma once

#define ANSI_ESC     "\033"

#define ANSI_CLS_SCR ANSI_ESC "[2J"

#define T_CLEAR      ANSI_ESC "[0m"
#define T_BOLD       ANSI_ESC "[1m"
#define T_DIM        ANSI_ESC "[2m"
#define T_ITAL       ANSI_ESC "[3m"
#define T_UNDER      ANSI_ESC "[4m"
#define T_BLNK       ANSI_ESC "[5m"
#define T_REV        ANSI_ESC "[7m"
#define T_HIDN       ANSI_ESC "[8m"
#define T_STRK       ANSI_ESC "[9m"

// Colors
#define T_BLACK      ANSI_ESC "[30m"
#define T_RED        ANSI_ESC "[31m"
#define T_GREEN      ANSI_ESC "[32m"
#define T_YELLOW     ANSI_ESC "[33m"
#define T_BLUE       ANSI_ESC "[34m"
#define T_MAGENTA    ANSI_ESC "[35m"
#define T_CYAN       ANSI_ESC "[36m"
#define T_WHITE      ANSI_ESC "[37m"
#define T_BBLACK     ANSI_ESC "[90m"
#define T_BRED       ANSI_ESC "[91m"
#define T_BGREEN     ANSI_ESC "[92m"
#define T_BYELLOW    ANSI_ESC "[93m"
#define T_BBLUE      ANSI_ESC "[94m"
#define T_BMAGENTA   ANSI_ESC "[95m"
#define T_BCYAN      ANSI_ESC "[96m"
#define T_BWHITE     ANSI_ESC "[97m"

// Background
#define TB_BLACK     ANSI_ESC "[40m"
#define TB_RED       ANSI_ESC "[41m"
#define TB_GREEN     ANSI_ESC "[42m"
#define TB_YELLOW    ANSI_ESC "[43m"
#define TB_BLUE      ANSI_ESC "[44m"
#define TB_MAGENTA   ANSI_ESC "[45m"
#define TB_CYAN      ANSI_ESC "[46m"
#define TB_WHITE     ANSI_ESC "[47m"
#define TB_BBLACK    ANSI_ESC "[100m"
#define TB_BRED      ANSI_ESC "[101m"
#define TB_BGREEN    ANSI_ESC "[102m"
#define TB_BYELLOW   ANSI_ESC "[103m"
#define TB_BBLUE     ANSI_ESC "[104m"
#define TB_BMAGENTA  ANSI_ESC "[105m"
#define TB_BCYAN     ANSI_ESC "[106m"
#define TB_BWHITE    ANSI_ESC "[107m"
#define TB_DEFAULT   ANSI_ESC "[49m"
#define T_DEFAULT    ANSI_ESC "[39m"
#define T_COL_RES    ANSI_ESC "[0m"

#define T_RGB(r, g, b)  ANSI_ESC "[38;2;" #r ";" #g ";" #b "m"
#define TB_RGB(r, g, b) ANSI_ESC "[48;2;" #r ";" #g ";" #b "m"

