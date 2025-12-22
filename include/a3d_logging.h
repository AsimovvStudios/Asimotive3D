#pragma once

#include <stdio.h>

/* ANSI colours */
#define ANSI_RESET     "\x1b[0m"
#define ANSI_FG_WHITE  "\x1b[37m"
#define ANSI_BG_CYAN   "\x1b[46m"
#define ANSI_BG_MAGENTA "\x1b[45m"
#define ANSI_BG_RED    "\x1b[41m"
#define ANSI_BG_YELLOW "\x1b[43m"

/* logging */
#define A3D_LOG(fmt, ...) \
	fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#define A3D_LOG_INFO(fmt, ...) \
	fprintf(stdout, ANSI_FG_WHITE ANSI_BG_CYAN " INFO  " ANSI_RESET " " fmt "\n", ##__VA_ARGS__)

#define A3D_LOG_WARN(fmt, ...) \
	fprintf(stderr, ANSI_FG_WHITE ANSI_BG_YELLOW " WARN  " ANSI_RESET " " fmt "\n", ##__VA_ARGS__)

#define A3D_LOG_ERROR(fmt, ...) \
	fprintf(stderr, ANSI_FG_WHITE ANSI_BG_RED " ERROR " ANSI_RESET " %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#if !defined(NDEBUG)
#define A3D_LOG_DEBUG(fmt, ...) \
	fprintf(stdout, ANSI_FG_WHITE ANSI_BG_MAGENTA " DEBUG " ANSI_RESET " " fmt "\n", ##__VA_ARGS__)
#else
#define A3D_LOG_DEBUG(fmt, ...) ((void)0)
#endif
