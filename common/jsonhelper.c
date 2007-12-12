#include "simplejson.h"

/* $Id: jsonhelper.c,v 1.2 2007/05/09 20:55:00 derek Exp $ */

/** HELPER FUNCTION **/ 

json_value* json_create_null(apr_pool_t *mpool) {
	json_value *out = apr_palloc(mpool,sizeof(json_value));
	out->type = JSON_NULL;
	return out;
}
json_value* json_create_long(apr_pool_t *mpool,long number) {
	json_value *out = apr_palloc(mpool,sizeof(json_value));
	out->type = JSON_LONG;
	out->value.lnumber = number;
	return out;
}
json_value* json_create_double(apr_pool_t *mpool,double number){
	json_value *out = apr_palloc(mpool,sizeof(json_value));
	out->type = JSON_DOUBLE;
	out->value.dnumber = number;
	return out;
}
json_value* json_create_string(apr_pool_t *mpool,const char *string) {
	json_value *out = apr_palloc(mpool,sizeof(json_value));
	out->type = JSON_STRING;
	out->value.string = apr_pstrdup(mpool,string);
	return out;
}
json_value* json_create_boolean(apr_pool_t *mpool,char b){
	json_value *out = apr_palloc(mpool,sizeof(json_value));
	out->type = JSON_BOOLEAN;
	out->value.boolean = b;
	return out;
}
json_value* json_create_object(apr_pool_t *mpool){
	json_value *out = apr_palloc(mpool,sizeof(json_value));
	out->type = JSON_OBJECT;
	out->value.object = apr_hash_make(mpool);
	return out;
}
json_value* json_create_array(apr_pool_t *mpool,int asize) {
	json_value *out = apr_palloc(mpool,sizeof(json_value));
	out->type = JSON_ARRAY;
	out->value.array = apr_array_make(mpool,asize,sizeof(json_value *));
	return out;
}
void  json_add_object(json_value *jobject, const char *key, json_value *value) {
	apr_hash_set(jobject->value.object,key,APR_HASH_KEY_STRING,value);
}
void  json_append_array(json_value *jarray,  json_value *value) {
	*((json_value**)(apr_array_push(jarray->value.array))) = value;
}

apr_status_t json_get_string(json_value *injson, const char *key, json_value *value)
{
	if (injson->type == JSON_OBJECT && (value = (json_value*)apr_hash_get(
				injson->value.object, key, APR_HASH_KEY_STRING)) !=NULL
				&& value->type == JSON_STRING) {
		return APR_SUCCESS;
	} else {
		return APR_EINVAL;
	}

}

apr_status_t json_get_long(json_value *injson, const char *key, long *num) {
	json_value *value;
	if (injson->type == JSON_OBJECT && (value = (json_value*)apr_hash_get(
				injson->value.object, key, APR_HASH_KEY_STRING)) !=NULL
				&& value->type == JSON_LONG) {
		*num = value->value.lnumber;
		return APR_SUCCESS;
	}
	return APR_EINVAL;
}

apr_status_t json_get_sql(json_value *injson, json_value *sql) {
	return json_get_string(injson, JSON_KEY_SQL, sql);
}

apr_status_t json_get_cache_ttl(json_value *injson, long *cache_ttl ) {
	if ( json_get_long(injson, JSON_KEY_CACHE, cache_ttl) == APR_SUCCESS) {
		return APR_SUCCESS;
	}
	return APR_EINVAL;
}

int json_wants_caching(json_value *json) {
	long val=0; 
	if ( (json_get_cache_ttl(json, &val) == APR_SUCCESS) 
		&& ( val > 0 )) {
		return val;
	}
	return 0;
}

