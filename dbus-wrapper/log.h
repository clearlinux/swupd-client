/*
 * Daemon for controlling Clear Linux Software Update Client
 *
 * Copyright (C) 2016 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * Contact: Dmitry Rozhkov <dmitry.rozhkov@intel.com>
 *
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define DEBUG(fmt, ...)	\
	printf(__FILE__ ":" STR(__LINE__) " " fmt "\n", ##__VA_ARGS__)

#define ERR(fmt, ...) \
	fprintf(stderr, "Error: " __FILE__ ":" STR(__LINE__) " " fmt "\n", ##__VA_ARGS__)

#endif
