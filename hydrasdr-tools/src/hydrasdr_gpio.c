/*
 * Copyright 2013-2025 Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 * This file is part of HydraSDR (based on HackRF project).
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

#define PORT_NUM_INVALID (255)
#define PIN_NUM_INVALID  (255)

#define PORT_NUM_MIN (0)
#define PORT_NUM_MAX (7)

#define PIN_NUM_MIN (0)
#define PIN_NUM_MAX (31)

static void usage() {
	printf("Usage:\n");
	printf("\t-p, --port_no <p>: set port number<p>[0,7] for subsequent read/write operations\n");
	printf("\t-n, --pin_no <n>: set pin number<n>[0,31] for subsequent read/write operations\n");
	printf("\t-r, --read: read port number/pin number value and direction specified by last -n argument, or all port/pin\n");
	printf("\t-w, --write <v>: write value specified by last -n argument with value<v>[0,1]\n");
	printf("\t[-s serial_number_64bits]: Open board with specified 64bits serial number.\n");
	printf("\nExamples:\n");
	printf("\t<command> -p 0 -n 12 -r # reads from port 0 pin number 12\n");
	printf("\t<command> -r          # reads all pins on all ports\n");
	printf("\t<command> -p 0 -n 10 -w 1 # writes port 0 pin number 10 with 1 decimal\n");
	printf("\nHardware Info HydraSDR:\n");
	printf("LED1(out): -p 0 -n 12 (0=OFF, 1=ON)\n");
	printf("Enable R828D(out): -p 1 -n 7 (0=OFF, 1=ON)\n");
	printf("Enable BiasT(out): -p 1 -n 13 (0=OFF, 1=ON)\n");
}

static struct option long_options[] = {
	{ "port_no", required_argument, 0, 'p' },
	{ "pin_no", required_argument, 0, 'n' },
	{ "write", required_argument, 0, 'w' },
	{ "read", no_argument, 0, 'r' },
	{ 0, 0, 0, 0 },
};

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

int dump_port_pin(struct hydrasdr_device* device,
									hydrasdr_gpio_port_t port_number,
									hydrasdr_gpio_pin_t pin_number)
{
	uint8_t value;
	int result = hydrasdr_gpio_read(device, port_number, pin_number, &value);

	if( result == HYDRASDR_SUCCESS ) {
		printf("gpio[%1d][%2d] -> 0x%02X", port_number, pin_number, value);

		result = hydrasdr_gpiodir_read(device, port_number, pin_number, &value);
		if( result == HYDRASDR_SUCCESS ) {
			if(value == 1)
				printf(" out(1)\n");
			else
				printf(" in(0)\n");
		} else {
			printf("hydrasdr_gpiodir_read() failed: %s (%d)\n", hydrasdr_error_name(result), result);
		}
	} else {
		printf("hydrasdr_gpio_read() failed: %s (%d)\n", hydrasdr_error_name(result), result);
	}

	return result;
}

int dump_port(struct hydrasdr_device* device, hydrasdr_gpio_port_t port_number)
{
	hydrasdr_gpio_pin_t pin_number;
	int result = HYDRASDR_SUCCESS;

	for(pin_number = GPIO_PIN0; pin_number < (GPIO_PIN31+1); pin_number++)
	{
		result = dump_port_pin(device, port_number, pin_number);
	}
	return result;
}

int dump_ports(struct hydrasdr_device* device)
{
	uint8_t port_number;
	int result = HYDRASDR_SUCCESS;

	for(port_number = GPIO_PORT0; port_number < (GPIO_PORT7+1); port_number++)
	{
		result = dump_port(device, port_number);
		if( result != HYDRASDR_SUCCESS ) {
			break;
		}
	}
	return result;
}

int write_port_pin(struct hydrasdr_device* device,
										hydrasdr_gpio_port_t port_number,
										hydrasdr_gpio_pin_t pin_number,
										uint8_t value)
{
	int result;
	result = hydrasdr_gpio_write(device, port_number, pin_number, value);

	if( result == HYDRASDR_SUCCESS ) {
		printf("0x%02X -> gpio[%1d][%2d]\n", value, port_number, pin_number);
	} else {
		printf("hydrasdr_gpio_write() failed: %s (%d)\n", hydrasdr_error_name(result), result);
	}
	return result;
}

bool serial_number = false;
uint64_t serial_number_val;

int main(int argc, char** argv) {
	int opt;
	uint8_t port_number = PORT_NUM_INVALID;
	uint8_t pin_number = PIN_NUM_INVALID;
	uint8_t value;
	struct hydrasdr_device* device = NULL;
	int option_index;
	uint32_t serial_number_msb_val;
	uint32_t serial_number_lsb_val;
	int result;

	option_index = 0;
	while( (opt = getopt_long(argc, argv,  "p:n:rw:s:", long_options, &option_index)) != EOF )
	{
		switch( opt ) {

		case 's':
			serial_number = true;
			result = parse_u64(optarg, &serial_number_val);
			serial_number_msb_val = (uint32_t)(serial_number_val >> 32);
			serial_number_lsb_val = (uint32_t)(serial_number_val & 0xFFFFFFFF);
			printf("Board serial number to open: 0x%08X%08X\n", serial_number_msb_val, serial_number_lsb_val);
			break;
		}
	}

	if(serial_number == true)
	{
		result = hydrasdr_open_sn(&device, serial_number_val);
		if( result != HYDRASDR_SUCCESS ) {
			printf("hydrasdr_open_sn() failed: %s (%d)\n", hydrasdr_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}
	}else
	{
		result = hydrasdr_open(&device);
		if( result != HYDRASDR_SUCCESS ) {
			printf("hydrasdr_open() failed: %s (%d)\n", hydrasdr_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}
	}

	result = HYDRASDR_ERROR_OTHER;
	option_index = 0;
	optind = 0;
	while( (opt = getopt_long(argc, argv, "p:n:rw:", long_options, &option_index)) != EOF )
	{
		switch( opt ){
		case 'p':
			result = parse_u8(optarg, &port_number);
			if((result != HYDRASDR_SUCCESS) || (port_number > PORT_NUM_MAX))
			{
				printf("Error parameter -p shall be between %d and %d\n", PORT_NUM_MIN, PORT_NUM_MAX);
				result = HYDRASDR_ERROR_OTHER;
			}
			break;

		case 'n':
			result = parse_u8(optarg, &pin_number);
			if((result != HYDRASDR_SUCCESS) || (pin_number > PIN_NUM_MAX))
			{
				printf("Error parameter -n shall be between %d and %d\n", PIN_NUM_MIN, PIN_NUM_MAX);
				result = HYDRASDR_ERROR_OTHER;
			}
			break;

		case 'r':
			if( port_number == PORT_NUM_INVALID )
			{
				result = dump_ports(device);
			} else
			{
				if( pin_number == PORT_NUM_INVALID )
				{
					result = dump_port(device, port_number);
				} else
				{
					result = dump_port_pin(device, port_number, pin_number);
				}
			}
			if( result != HYDRASDR_SUCCESS )
				printf("argument error: %s (%d)\n", hydrasdr_error_name(result), result);
			break;

		case 'w':
			result = parse_u8(optarg, &value);
			if( result == HYDRASDR_SUCCESS ) {
				result = write_port_pin(device, port_number, pin_number, value);
				if( result != HYDRASDR_SUCCESS )
					printf("argument error: %s (%d)\n", hydrasdr_error_name(result), result);
			}
			break;
		}

		if( result != HYDRASDR_SUCCESS )
		{
			break;
		}
	}

	if( result != HYDRASDR_SUCCESS )
	{
		usage();
	}

	result = hydrasdr_close(device);
	if( result ) {
		printf("hydrasdr_close() failed: %s (%d)\n", hydrasdr_error_name(result), result);
		return EXIT_FAILURE;
	}

	return 0;
}

