/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

struct location_info {
	char countryCode[10];
	double lat;
	double lng;
};

struct location_info* GetLocationData(void);
