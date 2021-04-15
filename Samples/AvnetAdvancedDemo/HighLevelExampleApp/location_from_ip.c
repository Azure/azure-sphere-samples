/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "location_from_ip.h"
#include "parson.h"
#include "httpGet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <applibs/log.h>


struct location_info locationInfo;

static const char* geoIfyURL = "https://get.geojs.io/v1/ip/geo.json";

struct location_info* GetLocationData(void)
{
	memset(&locationInfo, 0x00, sizeof(locationInfo));

	char* data = getHttpData(geoIfyURL);
	if (data != NULL)
	{
		JSON_Value* rootProperties = NULL;
		rootProperties = json_parse_string(data);
		JSON_Object* rootObject = json_value_get_object(rootProperties);

		const char* countryCode = json_object_get_string(rootObject, "country_code");
		const char* latitude = json_object_get_string(rootObject, "latitude");
		const char* longitude = json_object_get_string(rootObject, "longitude");

		double lat = atof(latitude);
		double lng = atof(longitude);
		Log_Debug("Country Code %s\n", countryCode);
		Log_Debug("Lat %f\n", lat);
		Log_Debug("Lng %f\n", lng);

		snprintf(locationInfo.countryCode, 10, countryCode);
		locationInfo.lat = lat;
		locationInfo.lng = lng;

		// need to free data since this was allocated in the getHttpData function.
		free(data);
		json_value_free(rootProperties);
		return &locationInfo;

	}
	return NULL;
}
