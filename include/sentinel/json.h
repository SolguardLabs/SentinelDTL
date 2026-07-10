#ifndef SENTINEL_JSON_H
#define SENTINEL_JSON_H

#include "sentinel/engine.h"

typedef struct sg_json {
    char *data;
    size_t len;
    size_t cap;
    uint8_t depth;
    bool needs_comma[SG_MAX_JSON_DEPTH];
} sg_json_t;

void sg_json_init(sg_json_t *json);
void sg_json_free(sg_json_t *json);
sg_status_t sg_json_begin_object(sg_json_t *json, sg_error_t *err);
sg_status_t sg_json_end_object(sg_json_t *json, sg_error_t *err);
sg_status_t sg_json_begin_array(sg_json_t *json, const char *key, sg_error_t *err);
sg_status_t sg_json_end_array(sg_json_t *json, sg_error_t *err);
sg_status_t sg_json_key_string(sg_json_t *json, const char *key, const char *value, sg_error_t *err);
sg_status_t sg_json_key_u64(sg_json_t *json, const char *key, uint64_t value, sg_error_t *err);
sg_status_t sg_json_key_bool(sg_json_t *json, const char *key, bool value, sg_error_t *err);
sg_status_t sg_json_array_string(sg_json_t *json, const char *value, sg_error_t *err);
sg_status_t sg_json_array_u64(sg_json_t *json, uint64_t value, sg_error_t *err);
sg_status_t sg_json_write_engine(
    sg_json_t *json,
    const sg_engine_t *engine,
    const char *scenario,
    const sg_error_t *last_error,
    sg_error_t *err);

#endif
