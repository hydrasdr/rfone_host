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
#include <string.h>
#include <getopt.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif
#endif

#define RF_PORT_INVALID 255
#define RF_PORT_NUM_MIN (0)
#define RF_PORT_NUM_MAX (2)

bool serial_number = false;
uint64_t serial_number_val;
uint8_t rf_port = RF_PORT_INVALID;

char version[255 + 1];
hydrasdr_read_partid_serialno_t read_partid_serialno;
struct hydrasdr_device* device = NULL;

int parse_u8(char* const s, uint8_t* const value) {
	char* s_end = s;
	const long int long_value = strtol(s, &s_end, 10);
	if( (s != s_end) && (*s_end == 0) ) {
		if((long_value >=0 ) && (long_value < 256)) {
			*value = (uint8_t)long_value;
			return HYDRASDR_SUCCESS;
		} else {
			return HYDRASDR_ERROR_INVALID_PARAM;
		}
	} else {
		return HYDRASDR_ERROR_INVALID_PARAM;
	}
}

int parse_u64(char* s, uint64_t* const value) {
	uint_fast8_t base = 10;
	char* s_end;
	uint64_t u64_value;

	if( strlen(s) > 2 ) {
		if( s[0] == '0' ) {
			if( (s[1] == 'x') || (s[1] == 'X') ) {
				base = 16;
				s += 2;
			} else if( (s[1] == 'b') || (s[1] == 'B') ) {
				base = 2;
				s += 2;
			}
		}
	}

	s_end = s;
	u64_value = strtoull(s, &s_end, base);
	if( (s != s_end) && (*s_end == 0) ) {
		*value = u64_value;
		return HYDRASDR_SUCCESS;
	} else {
		return HYDRASDR_ERROR_INVALID_PARAM;
	}
}

static void usage(void)
{
	printf("Usage:\n");
	printf("\t-p: set rf port (valid values from %d to %d)\n", RF_PORT_NUM_MIN, RF_PORT_NUM_MAX);
	printf("\tIf no serial_number_64bits are used open and set first board found.\n");
	printf("\t[-s serial_number_64bits]: Open board with specified 64bits serial number.\n");
}

int main(int argc, char** argv)
{
	int result;
	int opt;
	uint32_t serial_number_msb_val;
	uint32_t serial_number_lsb_val;
	hydrasdr_lib_version_t lib_version;

	result = HYDRASDR_ERROR_OTHER;
	while( (opt = getopt(argc, argv, "s:p:")) != EOF )
	{
		switch( opt ) 
		{
		case 's':
			serial_number = true;
			result = parse_u64(optarg, &serial_number_val);
			serial_number_msb_val = (uint32_t)(serial_number_val >> 32);
			serial_number_lsb_val = (uint32_t)(serial_number_val & 0xFFFFFFFF);
			printf("Board serial number to open: 0x%08X%08X\n", serial_number_msb_val, serial_number_lsb_val);
			break;

		case 'p':
			result = parse_u8(optarg, &rf_port);
			if((result != HYDRASDR_SUCCESS) || (rf_port > RF_PORT_NUM_MAX))
			{
				rf_port = RF_PORT_INVALID;
				printf("Error parameter -p shall be between %d and %d\n", RF_PORT_NUM_MIN, RF_PORT_NUM_MAX);
				result = HYDRASDR_ERROR_OTHER;
			}
			break;
			
		default:
			printf("unknown argument '-%c %s'\n", opt, optarg);
			usage();
			return EXIT_FAILURE;
		}
		
		if( result != HYDRASDR_SUCCESS ) {
			printf("argument error: '-%c %s' %s (%d)\n", opt, optarg, hydrasdr_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}		
	}

	if (result != HYDRASDR_SUCCESS) {
		usage();
		return EXIT_FAILURE;
	}

	hydrasdr_lib_version(&lib_version);
	printf("hydrasdr_lib_version: %d.%d.%d\n", 
					lib_version.major_version, lib_version.minor_version, lib_version.revision); 

	if(serial_number == true)
	{
		result = hydrasdr_open_sn(&device, serial_number_val);
	}else
	{
		result = hydrasdr_open(&device);
	}
	if (result != HYDRASDR_SUCCESS)
	{
		fprintf(stderr, "hydrasdr_open() board failed: %s (%d)\n",
				hydrasdr_error_name(result), result);
		return EXIT_FAILURE;
	}

	if(device != NULL)
	{
		printf("\nFound HydraSDR board\n");
		printf("Execute set rf port on port %d\n", rf_port);
		fflush(stdout);
		result = hydrasdr_set_rf_port(device, (hydrasdr_rf_port_t)rf_port);
		if (result != HYDRASDR_SUCCESS) {
			fprintf(stderr, "hydrasdr_set_rf_port() failed: %s (%d)\n",
				hydrasdr_error_name(result), result);
		}
		printf("Close board\n");
		result = hydrasdr_close(device);
		if (result != HYDRASDR_SUCCESS) {
			fprintf(stderr, "hydrasdr_close() board failed: %s (%d)\n",
					hydrasdr_error_name(result), result);
		}
	}

	return EXIT_SUCCESS;
}
