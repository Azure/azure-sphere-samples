/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "httpGet.h"

#include <curl/curl.h>
#include <curl/easy.h>
#include <stdlib.h>
#include <string.h>

// Curl stuff.
struct url_data {
	size_t size;
	char* data;
};

struct url_data data;

static size_t write_data(void* ptr, size_t size, size_t nmemb, struct url_data* data)
{
	size_t index = data->size;
	size_t n = (size * nmemb);
	char* tmp;

	data->size += (size * nmemb);

	tmp = realloc(data->data, data->size + 1); /* +1 for '\0' */

	if (tmp) {
		data->data = tmp;
	}
	else {
		if (data->data) {
			free(data->data);
		}
		return 0;
	}

	memcpy((data->data + index), ptr, n);
	data->data[data->size] = '\0';

	return size * nmemb;
}

char * getHttpData(const char *url)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);

	data.size = 0;
	data.data = malloc(1);

	CURLcode res = CURLE_OK;

	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	/* use a GET to fetch data */
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

	// based on the libcurl sample - https://curl.se/libcurl/c/https.html 
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

	/* Perform the request */
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (res == CURLE_OK)
	{
		// caller is responsible for freeing this.
		return data.data;
	}
	else
	{
		free(data.data);
	}

	return NULL;
}
