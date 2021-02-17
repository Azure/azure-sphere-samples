/* Copyright (c) qiio. All rights reserved.
Licensed under the MIT License. */
#ifndef ROUTER_H
#define ROUTER_H
int curl_get(char **data, const char *endpoint, const char *cert);
int curl_put(char **data, const char *endpoint, const char *cert, char *post);
int router_get_cellinfo(char **data, const char *cert);
#endif /* ROUTER_H */