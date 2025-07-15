/*
 * Copyright 2024-2025 Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 * This file is part of HydraSDR.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include <hydrasdr.h>
 
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <time.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif
#endif

#define HYDRASDR_FLASH_CALIB_OFFSET (0x20000) /* After 128KB (Reserved for Firmware + 64KB Spare) */
#define HYDRASDR_FLASH_CALIB_HEADER (0xCA1B0001)

typedef struct
{
	uint32_t header; /* Shall be equal to HYDRASDR_FLASH_CALIB_HEADER */
	uint32_t timestamp; /* Epoch Unix Time Stamp */
	int32_t correction_ppb;
} hydrasdr_calib_t;

static void usage()
{
	printf("Usage:\n");
	printf("\t-r: Read and display calibration data.\n");
	printf("\t-w <calibration in ppb>: Erase and Write calibration in ppb.\n");
}

int main(int argc, char **argv)
{
	int opt;
	struct hydrasdr_device* device = NULL;
	int result;
	bool read = false;
	bool write = false;
	int32_t calibration_ppb = 0;
	hydrasdr_calib_t calib;

	while ((opt = getopt(argc, argv, "rw:")) != EOF)
	{
		switch (opt) {
		case 'r':
			read = true;
			break;

		case 'w':
			write = true;
			calibration_ppb = atoi(optarg);
			break;

		default:
			fprintf(stderr, "opt error: %d\n", opt);
			usage();
			return EXIT_FAILURE;
		}
	}

	if (write == read) {
		if (write == true) {
			fprintf(stderr, "Read and write options are mutually exclusive.\n");
		} else {
			fprintf(stderr, "Specify either read or write option.\n");
		}
		usage();
		return EXIT_FAILURE;
	}

	result = hydrasdr_open(&device);
	if (result != HYDRASDR_SUCCESS) {
		fprintf(stderr, "Failed to open hydrasdr device.\n");
		return 1;
	}

	if (read) {
		printf("Reading %d bytes from 0x%06x.\n", (int)sizeof(calib), HYDRASDR_FLASH_CALIB_OFFSET);
		result = hydrasdr_spiflash_read(device, HYDRASDR_FLASH_CALIB_OFFSET, (int)sizeof(calib), (uint8_t *)&calib);
		if (result != HYDRASDR_SUCCESS) {
			fprintf(stderr, "hydrasdr_spiflash_read() failed: %s (%d)\n", hydrasdr_error_name(result), result);
			return EXIT_FAILURE;
		}
		time_t epoch_time = calib.timestamp;
		struct tm *local_time = localtime(&epoch_time);
		printf("Calibration timestamp: %04d/%02d/%02d %02d:%02d:%02d\nCalibration correction in ppb: %d\n", 
				local_time->tm_year + 1900,
				local_time->tm_mon + 1,
				local_time->tm_mday,
				local_time->tm_hour,
				local_time->tm_min,
				local_time->tm_sec,
				calib.correction_ppb);
	}
	if(write) {
		printf("Erasing sector 2 (calibration) in SPI flash.\n");
		result = hydrasdr_spiflash_erase_sector(device, 2);
		if (result != HYDRASDR_SUCCESS) {
			fprintf(stderr, "Failed to erase sector 2.\n");
			return 1;
		}

		calib.header = HYDRASDR_FLASH_CALIB_HEADER;
		calib.timestamp = (uint32_t)time(NULL);
		calib.correction_ppb = calibration_ppb;

		printf("Writing calibration %d bytes at 0x%06x.\n", (int)sizeof(calib), HYDRASDR_FLASH_CALIB_OFFSET);
		time_t epoch_time = calib.timestamp;
		struct tm *local_time = localtime(&epoch_time);
		printf("Calibration timestamp: %04d/%02d/%02d %02d:%02d:%02d\nCalibration correction in ppb: %d\n", 
				local_time->tm_year + 1900,
				local_time->tm_mon + 1,
				local_time->tm_mday,
				local_time->tm_hour,
				local_time->tm_min,
				local_time->tm_sec,
				calib.correction_ppb);
		result = hydrasdr_spiflash_write(device, HYDRASDR_FLASH_CALIB_OFFSET, sizeof(calib), (uint8_t *)&calib);
		if (result != HYDRASDR_SUCCESS) {
			fprintf(stderr, "Failed to write calibration data.\n");
			return 1;
		}
	}

	result = hydrasdr_close(device);
	if (result != HYDRASDR_SUCCESS) {
		fprintf(stderr, "hydrasdr_close() failed: %s (%d)\n", hydrasdr_error_name(result), result);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

