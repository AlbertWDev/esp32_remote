

#define APPLY_HEADERS(req) \
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
