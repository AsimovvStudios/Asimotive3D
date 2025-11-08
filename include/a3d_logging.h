#pragma once

/* ANSI foreground colours */
#define A3D_ANSI_CYAN     "\x1b[36m"
#define A3D_ANSI_MAGENTA  "\x1b[35m"
#define A3D_ANSI_RED      "\x1b[31m"
#define A3D_ANSI_RESET    "\x1b[0m"
#define A3D_ANSI_YELLOW   "\x1b[33m"

/* logging */
#define A3D_LOG_INFO(fmt, ...) \
	fprintf(stdout, A3D_ANSI_CYAN "[INFO]" A3D_ANSI_RESET " " fmt "\n", ##__VA_ARGS__)

#define A3D_LOG_WARN(fmt, ...) \
	fprintf(stderr, A3D_ANSI_YELLOW "[WARN]" A3D_ANSI_RESET " " fmt "\n", ##__VA_ARGS__)

#define A3D_LOG_ERROR(fmt, ...) \
	fprintf(stderr, A3D_ANSI_RED "[ERROR]" A3D_ANSI_RESET " " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__)

#ifdef DEBUG
#define A3D_LOG_DEBUG(fmt, ...) \
	fprintf(stdout, A3D_ANSI_MAGENTA "[DEBUG]" A3D_ANSI_RESET " " fmt "\n", ##__VA_ARGS__)

#else
#define A3D_LOG_DEBUG(fmt, ...) ((void)0)

#endif
