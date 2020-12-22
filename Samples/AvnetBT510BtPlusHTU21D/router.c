/* Copyright (c) qiio. All rights reserved.
Licensed under the MIT License. */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <applibs/log.h>
#include <tlsutils/deviceauth_curl.h>
#include <curl/curl.h>
#include "router.h"
#define QIIO_ROUTER_API_PATH "https://router.qiio.com:5001/config/api/v1.0"
struct MemoryStruct {
    char *memory;
    size_t size;
} __attribute__((packed));
static size_t curl_write_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = realloc(mem->memory, mem->size + realsize);
    if (ptr == NULL) {
        Log_Debug("not enough memory (realloc returned NULL)\n");
        return 0;
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, size * nmemb);
    mem->size += realsize;
    return nmemb * size;
}
static int curl_ops(char **data, const char *endpoint, const char *cert, char *post)
{
    int ret = -EINVAL;
    CURL *curl = NULL;
    CURLcode res;
    struct MemoryStruct chunk;
    struct curl_slist *hs = NULL;
    chunk.memory = malloc(1);
    chunk.size = 0;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, endpoint);
        curl_easy_setopt(curl, CURLOPT_CAINFO, cert);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        if (post) {
            hs = curl_slist_append(hs, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
        }
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            Log_Debug("%s(%d): curl_easy_perform() failed with %s\n", __func__, __LINE__,
                      curl_easy_strerror(res));
        } else {
            ret = 0;
            if (chunk.size) {
                *data = (char *)malloc(chunk.size + 1);
                memcpy(*data, chunk.memory, chunk.size);
                (*data)[chunk.size] = '\0';
            }
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    free(chunk.memory);
    return ret;
}
int curl_get(char **data, const char *endpoint, const char *cert)
{
    return curl_ops(data, endpoint, cert, NULL);
}
int curl_put(char **data, const char *endpoint, const char *cert, char *post)
{
    return curl_ops(data, endpoint, cert, post);
}
int router_get_cellinfo(char **data, const char *cert)
{
    const char *endpoint = QIIO_ROUTER_API_PATH "/modem/cellinfo";
    return curl_get(data, endpoint, cert);
}