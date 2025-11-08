#pragma once

/* ANSI foreground colours */
#define A3D_ANSI_RESET    "\x1b[0m"
#define A3D_ANSI_BLACK    "\x1b[30m"
#define A3D_ANSI_RED      "\x1b[31m"
#define A3D_ANSI_GREEN    "\x1b[32m"
#define A3D_ANSI_YELLOW   "\x1b[33m"
#define A3D_ANSI_BLUE     "\x1b[34m"
#define A3D_ANSI_MAGENTA  "\x1b[35m"
#define A3D_ANSI_CYAN     "\x1b[36m"
#define A3D_ANSI_WHITE    "\x1b[37m"

/* ANSI background colours */
#define A3D_BG_BLACK      "\x1b[40m"
#define A3D_BG_RED        "\x1b[41m"
#define A3D_BG_GREEN      "\x1b[42m"
#define A3D_BG_YELLOW     "\x1b[43m"
#define A3D_BG_BLUE       "\x1b[44m"
#define A3D_BG_MAGENTA    "\x1b[45m"
#define A3D_BG_CYAN       "\x1b[46m"
#define A3D_BG_WHITE      "\x1b[47m"

/* ANSI attributes */
#define A3D_ANSI_BOLD     "\x1b[1m"
#define A3D_ANSI_DIM      "\x1b[2m"
#define A3D_ANSI_UNDERLINE "\x1b[4m"

/* logging */

#define A3D_LOG_INFO(fmt, ...) \
	fprintf(stdout, A3D_ANSI_CYAN "[INFO]" A3D_ANSI_RESET "  " fmt "\n", ##__VA_ARGS__)

#define A3D_LOG_WARN(fmt, ...) \
	fprintf(stderr, A3D_ANSI_YELLOW "[WARN]" A3D_ANSI_RESET "  " fmt "\n", ##__VA_ARGS__)

#define A3D_LOG_ERROR(fmt, ...) \
	fprintf(stderr, A3D_ANSI_RED "[ERROR]" A3D_ANSI_RESET "  " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__)

#ifdef DEBUG
#define A3D_LOG_DEBUG(fmt, ...) \
	fprintf(stdout, A3D_ANSI_MAGENTA "[DEBUG]" A3D_ANSI_RESET "  " fmt "\n", ##__VA_ARGS__)

#else
#define A3D_LOG_DEBUG(fmt, ...) ((void)0)

#endif
