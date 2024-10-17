/*
 * mstime.h
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#ifndef MSTIME_H_
#define MSTIME_H_

#ifdef WIN32
#pragma once
#include "Windows.h"

#define WIN32_LEAN_AND_MEAN

#pragma comment (lib, "ws2_32.lib")
#elif __linux__
#include <sys/time.h>
#include <time.h>
#endif

unsigned long long GetTimeMs64();

#endif /* MSTIME_H_ */
