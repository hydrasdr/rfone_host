/**
 * Copyright 2025 Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 * @file hydrasdr_async_rx.c
 * @brief Cross-platform asynchronous streaming example for HydraSDR RFOne.
 *
 * @details
 * This application demonstrates how to:
 * 1. Open a HydraSDR device.
 * 2. Configure RF parameters (Frequency, Linearity Gain, Bias-T).
 * 3. Configure Data parameters (Sample Rate, Sample Type).
 * 4. Perform non-blocking (async) streaming to a file.
 * 5. Deduce sample buffer sizes dynamically in the callback.
 * 6. Monitor Real-time statistics (Time, Rate, MSPS, Drops).
 *
 * Usage:
 * ./hydrasdr_async_rx [-f freq_hz] [-s rate_sps] [-t sample_type] [-g gain] [-b bias_on_off] [-o filename]
 *
 * Defaults:
 * - Frequency: 100 MHz
 * - Sample Rate: 2.5 MSPS
 * - Gain: 10 (Linearity Profile)
 * - Bias Tee: Off
 * - Sample Type: INT16 IQ
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

/* Include the HydraSDR API definition */
#include "hydrasdr.h"

#define HYDRASDR_ASYNC_RX_VERSION "1.0.0"

// Cross-platform utilities
#ifdef _WIN32
	#include <windows.h>
	#define SLEEP_MS(ms) Sleep(ms)
	typedef volatile long sig_atomic_bool_t;

	static double get_time_sec(void)
	{
		LARGE_INTEGER t, f;
		QueryPerformanceCounter(&t);
		QueryPerformanceFrequency(&f);
		return (double)t.QuadPart / (double)f.QuadPart;
	}
#else /* Linux / macOS */
	#include <unistd.h>
	#include <pthread.h>
	#include <sys/time.h>

	#define SLEEP_MS(ms) usleep((ms) * 1000)
	typedef volatile sig_atomic_t sig_atomic_bool_t;

	static double get_time_sec(void)
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return ts.tv_sec + (ts.tv_nsec / 1e9);
	}
#endif

//  Default settings
#define DEFAULT_FREQ_HZ      100000000ULL   /* 100 MHz */
#define DEFAULT_SAMPLERATE   2500000U       /* 2.5 MSPS */
#define DEFAULT_SAMPLETYPE   HYDRASDR_SAMPLE_INT16_IQ
#define DEFAULT_GAIN         10             /* 0-21 */
#define DEFAULT_FILENAME     "capture.bin"

// Global Application State
static struct hydrasdr_device *g_dev = NULL;
static FILE *g_out = NULL;

static sig_atomic_bool_t g_exit_requested = 0;

static volatile uint64_t g_total_bytes = 0;
static volatile uint64_t g_total_dropped_samples = 0;

// Signal Handling
#ifdef _WIN32
BOOL WINAPI
windows_signal_handler(DWORD signum)
{
	if (!g_exit_requested)
		fprintf(stderr, "\nCaught signal %lu\n", (unsigned long)signum);

	g_exit_requested = 1;
	return TRUE;
}
#else
static void posix_signal_handler(int signum)
{
	if (!g_exit_requested)
		fprintf(stderr, "\nCaught signal %d\n", signum);

	g_exit_requested = 1;
}
#endif

/**
 * @brief Helper to calculate bytes per sample based on API enum.
 *
 * @param type The hydrasdr_sample_type enum from the transfer struct.
 * @return size_t Number of bytes per single sample (I+Q combined if applicable).
 */
static inline size_t bytes_per_sample(enum hydrasdr_sample_type type)
{
	switch (type) {
		case HYDRASDR_SAMPLE_FLOAT32_IQ:   return 8; // 4 bytes I + 4 bytes Q
		case HYDRASDR_SAMPLE_FLOAT32_REAL: return 4;
		case HYDRASDR_SAMPLE_INT16_IQ:     return 4; // 2 bytes I + 2 bytes Q
		case HYDRASDR_SAMPLE_INT16_REAL:   return 2;
		case HYDRASDR_SAMPLE_UINT16_REAL:  return 2;
		case HYDRASDR_SAMPLE_RAW:          return 2; /* Raw device stream */
		default:                           return 2;
	}
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [options]\n", prog);
	printf("\nOptions:\n");
	printf(" -f <Hz>   Set RF frequency (default: %llu Hz)\n", DEFAULT_FREQ_HZ);
	printf(" -s <SPS>  Set sample rate  (default: %u)\n", DEFAULT_SAMPLERATE);
	printf(" -t <type> Set sample type  (default: %d = Int16 IQ)\n", DEFAULT_SAMPLETYPE);
	printf("           0=FloatIQ, 1=FloatReal, 2=Int16IQ, 3=Int16Real, 5=Raw\n");
	printf(" -g <0-21> Linearity gain (default: %d)\n", DEFAULT_GAIN);
	printf(" -b <0/1>  Bias-T off/on (default: 0)\n");
	printf(" -o <file> Output file (default: %s)\n", DEFAULT_FILENAME);
	printf(" -h        Show help\n");
}

/**
 * @brief Asynchronous Callback Function.
 *
 * @details This function is invoked by the HydraSDR library thread.
 * Critical Section: Execution time must be minimized.
 *
 * @param transfer Pointer to the transfer structure containing data and metadata.
 * @return int 0 to continue streaming, non-zero to request stop (internally).
 */
int rx_callback(hydrasdr_transfer_t *t)
{
	if (g_exit_requested)
		return 0;

	if (t->dropped_samples)
		g_total_dropped_samples += t->dropped_samples;

	const size_t bps = bytes_per_sample(t->sample_type);
	const size_t chunk_bytes = (size_t)t->sample_count * bps;

	if (g_out && chunk_bytes > 0) {
		const size_t w = fwrite(t->samples, 1, chunk_bytes, g_out);
		if (w != chunk_bytes)
			fprintf(stderr, "Disk write error\n");
	}

	g_total_bytes += chunk_bytes;

	return 0;
}

/**
 * @brief Main Entry Point
 */

int main(int argc, char **argv)
{
	uint64_t freq_hz = DEFAULT_FREQ_HZ;
	uint32_t samplerate = DEFAULT_SAMPLERATE;
	uint8_t gain = DEFAULT_GAIN;
	uint8_t bias = 0;
	int sample_type = DEFAULT_SAMPLETYPE;
	const char *filename = DEFAULT_FILENAME;

	// 1. Parse Command Line Arguments (Manual parsing for no deps)
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-f") && i + 1 < argc) {
			freq_hz = strtoull(argv[++i], NULL, 10);
		}
		else if (!strcmp(argv[i], "-s") && i + 1 < argc) {
			samplerate = (uint32_t)strtoul(argv[++i], NULL, 10);
		}
		else if (!strcmp(argv[i], "-t") && i + 1 < argc) {
			sample_type = atoi(argv[++i]);
		}
		else if (!strcmp(argv[i], "-g") && i + 1 < argc) {
			gain = (uint8_t)atoi(argv[++i]);
		}
		else if (!strcmp(argv[i], "-b") && i + 1 < argc) {
			bias = (uint8_t)atoi(argv[++i]);
		}
		else if (!strcmp(argv[i], "-o") && i + 1 < argc) {
			filename = argv[++i];
		}
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_usage(argv[0]);
			return 0;
		}
		else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			print_usage(argv[0]);
			return 1;
		}
	}

	// 2. Setup Signal Handling (Ctrl+C)
#ifdef _WIN32
	SetConsoleCtrlHandler(windows_signal_handler, TRUE);
#else
	signal(SIGINT, posix_signal_handler);
	signal(SIGTERM, posix_signal_handler);
	signal(SIGABRT, posix_signal_handler);
#endif

	printf("HydraSDR Async RX Tool v%s\n", HYDRASDR_ASYNC_RX_VERSION);

	// 3. Open Device
	// Note: hydrasdr_open opens the first available device
	int ret = hydrasdr_open(&g_dev);
	if (ret != HYDRASDR_SUCCESS) {
		fprintf(stderr, "ERROR: hydrasdr_open failed: %s\n",
				hydrasdr_error_name(ret));
		return EXIT_FAILURE;
	}
	printf("[INFO] Device opened.\n");

	// 4. Print supported samplerates
	uint32_t count = 0;
	hydrasdr_get_samplerates(g_dev, &count, 0);

	if (count > 0) {
		uint32_t *rates = malloc(count * sizeof(uint32_t));
		if (rates) {
			hydrasdr_get_samplerates(g_dev, rates, count);
			printf("Available sample rates:\n");
			for (uint32_t i = 0; i < count; i++)
				printf("  %u (%.3f MSPS)\n", rates[i], rates[i] / 1e6);
			free(rates);
		}
	}
	printf("\n");

	// 5. Apply Configuration
	
	// A. Frequency
	printf("[CONF] Frequency:   %llu Hz\n", (unsigned long long)freq_hz);
	ret = hydrasdr_set_freq(g_dev, freq_hz);
	if (ret != HYDRASDR_SUCCESS) {
		fprintf(stderr, "[ERROR] Failed set frequency: %s\n", hydrasdr_error_name(ret));
		goto error;
	}

	// B. Sample Rate
	printf("[CONF] Samplerate:  %u SPS\n", samplerate);
	ret = hydrasdr_set_samplerate(g_dev, samplerate);
	if (ret != HYDRASDR_SUCCESS) {
		fprintf(stderr, "[ERROR] Failed set sample rate: %s\n", hydrasdr_error_name(ret));
		goto error;
	}

	// C. Sample Type
	if (sample_type < 0 || sample_type >= HYDRASDR_SAMPLE_END) {
		fprintf(stderr, "ERROR: Invalid sample type %d\n", sample_type);
		goto error;
	}
	printf("[CONF] Sample type: %d\n", sample_type);
	ret = hydrasdr_set_sample_type(g_dev, (enum hydrasdr_sample_type)sample_type);
	if (ret != HYDRASDR_SUCCESS) {
		fprintf(stderr, "[ERROR] Failed set sample type: %s\n", hydrasdr_error_name(ret));
		goto error;
	}

	// D. Gain (Linearity Mode)
	printf("[CONF] Gain:        %u\n", gain);
	ret = hydrasdr_set_linearity_gain(g_dev, gain);
	if (ret != HYDRASDR_SUCCESS) {
		fprintf(stderr, "[ERROR] Failed set linearity gain: %s\n", hydrasdr_error_name(ret));
		goto error;
	}

	// E. Bias Tee
	printf("[CONF] Bias-T:      %u\n", bias);
	ret = hydrasdr_set_rf_bias(g_dev, bias);
	if (ret != HYDRASDR_SUCCESS) {
		fprintf(stderr, "[ERROR] Failed set Bias Tee: %s\n", hydrasdr_error_name(ret));
		goto error;
	}
	if (bias)
		printf("[WARN] Bias-T ENABLED.\n");

	// 6. Open Output File
	g_out = fopen(filename, "wb");
	if (!g_out) {
		fprintf(stderr, "ERROR: Cannot open '%s': %s\n",
				filename, strerror(errno));
		goto error;
	}
	printf("[INFO] Writing to '%s'\n", filename);

	// 7. Start Streaming (Async)
	printf("[INFO] Starting stream... (Press Ctrl+C to stop)\n");

	// Pass 'NULL' as user context (last arg) since we use global vars for this simple example
	ret = hydrasdr_start_rx(g_dev, rx_callback, NULL);
	if (ret) {
		fprintf(stderr, "ERROR: start_rx failed: %s\n",
				hydrasdr_error_name(ret));
		goto error;
	}

	// 8. Monitoring Loop
	const double t_start = get_time_sec();
	double t_last = t_start;
	uint64_t last_bytes = 0;

	const size_t bps = bytes_per_sample((enum hydrasdr_sample_type)sample_type);

	while (!g_exit_requested) {
		SLEEP_MS(1000);

		if (hydrasdr_is_streaming(g_dev) != HYDRASDR_TRUE) {
			fprintf(stderr, "\n[ERROR] Device stopped streaming.\n");
			g_exit_requested = 1;
			break;
		}

		double t_now = get_time_sec();
		double dt = t_now - t_last;

		if (dt >= 1.0 && !g_exit_requested) {
			const uint64_t bytes_now = g_total_bytes;
			const uint64_t dbytes = bytes_now - last_bytes;

			const double inst_msps = (dbytes / (double)bps) / (dt * 1e6);
			const double avg_msps  = (bytes_now / (double)bps) / ((t_now - t_start) * 1e6);

			printf("Time %4.0fs | Inst %5.2f MSPS | Avg %5.2f MSPS | Vol %7.2f MB | Drops %llu\r",
				   t_now - t_start,
				   inst_msps,
				   avg_msps,
				   bytes_now / (1024.0 * 1024.0),
				   (unsigned long long)g_total_dropped_samples);
			fflush(stdout);

			last_bytes = bytes_now;
			t_last = t_now;
		}
	}

	printf("\n\n[INFO] Stopping ...\n");

error:
	if (g_dev) {
		hydrasdr_stop_rx(g_dev);
		hydrasdr_close(g_dev);
	}
	if (g_out)
		fclose(g_out);

	printf("[INFO] Done.\n");
	return ret == HYDRASDR_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}
