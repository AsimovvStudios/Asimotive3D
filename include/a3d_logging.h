#pragma once

/* ANSI colours */
#define A3D_ANSI_RESET     "\x1b[0m"
#define A3D_ANSI_FG_WHITE  "\x1b[37m"
#define A3D_ANSI_BG_CYAN   "\x1b[46m"
#define A3D_ANSI_BG_MAGENTA "\x1b[45m"
#define A3D_ANSI_BG_RED    "\x1b[41m"
#define A3D_ANSI_BG_YELLOW "\x1b[43m"

/* logging */
#define A3D_LOG(fmt, ...) \
	fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#define A3D_LOG_INFO(fmt, ...) \
	fprintf(stdout, A3D_ANSI_FG_WHITE A3D_ANSI_BG_CYAN "[INFO]" A3D_ANSI_RESET " " fmt "\n", ##__VA_ARGS__)

#define A3D_LOG_WARN(fmt, ...) \
	fprintf(stderr, A3D_ANSI_FG_WHITE A3D_ANSI_BG_YELLOW "[WARN]" A3D_ANSI_RESET " " fmt "\n", ##__VA_ARGS__)

#define A3D_LOG_ERROR(fmt, ...) \
	fprintf(stderr, A3D_ANSI_FG_WHITE A3D_ANSI_BG_RED "[ERROR]" A3D_ANSI_RESET " " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__)

#ifdef DEBUG
#define A3D_LOG_DEBUG(fmt, ...) \
	fprintf(stdout, A3D_ANSI_FG_WHITE A3D_ANSI_BG_MAGENTA "[DEBUG]" A3D_ANSI_RESET " " fmt "\n", ##__VA_ARGS__)
#else
#define A3D_LOG_DEBUG(fmt, ...) ((void)0)
#endif
