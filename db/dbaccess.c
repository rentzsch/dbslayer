#include "dbaccess.h"
#include <apr_strings.h>
/* $Id: dbaccess.c,v 1.10 2007/06/12 03:20:53 derek Exp $ */

db_handle_t * db_handle_reattach(db_handle_t *handle) { 
    int ct = handle->server_offset;
    for(ct = handle->server_offset ; ct < handle->server_count; ct++) { 
	if(handle->db) mysql_close(handle->db);
	handle->db = mysql_init(NULL);
	mysql_options(handle->db,MYSQL_READ_DEFAULT_FILE,handle->config);
	mysql_options(handle->db,MYSQL_READ_DEFAULT_GROUP,handle->server[ct]);
	if(mysql_real_connect(handle->db,NULL,handle->user,handle->pass,NULL,0,NULL,CLIENT_MULTI_STATEMENTS) != NULL){
	    handle->server_offset = ct;
	    return handle;
	}
    }
    for(ct = 0; ct < handle->server_offset; ct++) { 
	if(handle->db) mysql_close(handle->db);
	handle->db = mysql_init(NULL);
	mysql_options(handle->db,MYSQL_READ_DEFAULT_FILE,handle->config);
	mysql_options(handle->db,MYSQL_READ_DEFAULT_GROUP,handle->server[ct]);
	if(mysql_real_connect(handle->db,NULL,handle->user,handle->pass,NULL,0,NULL,CLIENT_MULTI_STATEMENTS) != NULL){
	    handle->server_offset = ct;
	    return handle;
	}
    }
    return NULL;
}

db_handle_t * db_handle_init(const char *_user, const char *_pass, const char *_server, const char *_config, void *userarg){
    db_handle_t *dbhandle= malloc(sizeof(db_handle_t));
    //should then be defined in the config if the user/pass is NULL
    dbhandle->user = _user ? strdup(_user) : NULL;
    dbhandle->pass = _pass ? strdup(_pass) : NULL;
    dbhandle->config = strdup(_config);
    dbhandle->db = NULL;

    //chop up the server string
    int i = 0;
    char *server = strdup(_server);
    char *e = server;
    dbhandle->server_count = 1;
    while((e = strchr(e,':'))!=NULL) {dbhandle->server_count++; e++;}
    char *b =  e = server;
    dbhandle->server = malloc(sizeof(char*) * dbhandle->server_count);
    while((e = strchr(e,':'))!=NULL)  { 
	*e = '\0';
	dbhandle->server[i++] = strdup(b);
	e++;
	b = e;
    }
    dbhandle->server[i++] = strdup(b);
    unsigned int seed = *((unsigned int*)userarg);
    free(server);
    dbhandle->server_offset = seed  % dbhandle->server_count;
    db_handle_reattach(dbhandle);
    return dbhandle;
}

void db_handle_destroy(db_handle_t *dbhandle) {
    mysql_close(dbhandle->db);
    mysql_thread_end();
    int i; 
    for(i = 0; i < dbhandle->server_count; i++) { 
	free(dbhandle->server[i]);	
    }
    free(dbhandle->server);
    free(dbhandle->config);
    free(dbhandle->user);
    free(dbhandle->pass);
    free(dbhandle);
}

char * field_to_string(MYSQL_FIELD field, apr_pool_t *mpool) {
    char *ret;
    switch(field.type) {
    case MYSQL_TYPE_TINY: 
	ret = "MYSQL_TYPE_TINY"; break;
    case MYSQL_TYPE_SHORT:
	ret = "MYSQL_TYPE_SHORT"; break;
    case MYSQL_TYPE_LONG: 
	ret = "MYSQL_TYPE_LONG"; break;
    case MYSQL_TYPE_INT24: 
	ret = "MYSQL_TYPE_INT24"; break;
    case MYSQL_TYPE_DECIMAL:
	ret = "MYSQL_TYPE_DECIMAL"; break;
    case MYSQL_TYPE_NEWDECIMAL:
	ret = "MYSQL_TYPE_NEWDECIMAL"; break;
    case MYSQL_TYPE_DOUBLE:
	ret = "MYSQL_TYPE_DOUBLE"; break;
    case MYSQL_TYPE_FLOAT:
	ret = "MYSQL_TYPE_FLOAT"; break;
    case MYSQL_TYPE_LONGLONG:
	ret = "MYSQL_TYPE_LONGLONG"; break;
    case MYSQL_TYPE_BIT:
	ret = "MYSQL_TYPE_BIT"; break;
    case MYSQL_TYPE_TIMESTAMP:
	ret = "MYSQL_TYPE_TIMESTAMP"; break;
    case MYSQL_TYPE_DATE:
	ret = "MYSQL_TYPE_DATE"; break;
    case MYSQL_TYPE_TIME:
	ret = "MYSQL_TYPE_TIME"; break;
    case MYSQL_TYPE_DATETIME:
	ret = "MYSQL_TYPE_DATETIME"; break;
    case MYSQL_TYPE_YEAR:
	ret = "MYSQL_TYPE_YEAR"; break;
    case MYSQL_TYPE_STRING:
	ret = "MYSQL_TYPE_STRING"; break;
    case MYSQL_TYPE_VAR_STRING:
	ret = "MYSQL_TYPE_VAR_STRING"; break;
    case MYSQL_TYPE_NEWDATE:
	ret = "MYSQL_TYPE_NEWDATE"; break;
    case MYSQL_TYPE_VARCHAR:
	ret = "MYSQL_TYPE_VARCHAR"; break;
    case MYSQL_TYPE_BLOB:
	//DON'T ASK - it is not 63 it is TEXT 
	//http://www.mysql.org/doc/refman/5.1/en/c-api-datatypes.html
	if (field.charsetnr == 63) {  
	    /**BINARY - NEEDS TO B64 **/
	    ret = "MYSQL_TYPE_TEXT";
	} else {
	    ret = "MYSQL_TYPE_BLOB"; 
	}
	break;
    case MYSQL_TYPE_TINY_BLOB:
	if (field.charsetnr == 63) {  
	    ret = "MYSQL_TYPE_TEXT"; 
	} else {
	    ret = "MYSQL_TYPE_TINY_BLOB"; 
	}
	break;
    case MYSQL_TYPE_MEDIUM_BLOB:
	if (field.charsetnr == 63) {  
	    ret = "MYSQL_TYPE_TEXT"; 
	} else {
	    ret = "MYSQL_TYPE_MEDIUM_BLOB";
	}
	break;
    case MYSQL_TYPE_LONG_BLOB:
	if (field.charsetnr == 63) {  
	    ret = "MYSQL_TYPE_TEXT"; 
	} else {
	    ret = "MYSQL_TYPE_LONG_BLOB"; 
	}
	break;
    case MYSQL_TYPE_SET:
	ret = "MYSQL_TYPE_SET"; break;
    case MYSQL_TYPE_ENUM:
	ret = "MYSQL_TYPE_ENUM"; break;
    case MYSQL_TYPE_GEOMETRY:
	ret = "MYSQL_TYPE_GEOMETRY"; break;
    case MYSQL_TYPE_NULL:
	ret = "MYSQL_TYPE_NULL"; break;
    default:
	ret = "MYSQL_TYPE_UNKNOWN"; break;
    }
    return apr_pstrdup(mpool, ret);
}

json_value * col_to_json(MYSQL_FIELD field, char * val, apr_pool_t *mpool) {
    json_value * ret = NULL;
    if ( ! val ) {
	 return json_null_create(mpool);
    }
    switch(field.type) {
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
	    ret = json_long_create(mpool,atol(val));
	    break;
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DOUBLE:
    case MYSQL_TYPE_FLOAT:
    case MYSQL_TYPE_LONGLONG:
	ret = json_double_create(mpool,atof(val));
	break;
    case MYSQL_TYPE_BIT:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_TIME:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_YEAR:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_NEWDATE:
    case MYSQL_TYPE_VARCHAR:
	ret = json_string_create(mpool,val);
	break;
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
	//DON'T ASK - it is not 63 it is TEXT 
	//http://www.mysql.org/doc/refman/5.1/en/c-api-datatypes.html
	if (field.charsetnr == 63) {  
	    /**BINARY - NEEDS TO B64 **/
	    ret = json_null_create(mpool);
	} else {
	    ret = json_string_create(mpool,val); 
	}
	break;
    case MYSQL_TYPE_SET:
    case MYSQL_TYPE_ENUM:
    case MYSQL_TYPE_GEOMETRY:
    case MYSQL_TYPE_NULL:
	ret = json_null_create(mpool);
	break;
    }
    return ret;
}


json_value * result_to_json(MYSQL_RES * myresult,apr_pool_t *mpool) { 
    MYSQL_FIELD *fields;
    json_value *result = json_object_create(mpool);
    unsigned int num_fields;
    unsigned int i;
    num_fields = mysql_num_fields(myresult);
    fields = mysql_fetch_fields(myresult);
    json_value *header = json_array_create(mpool,num_fields);
    json_object_add(result,"HEADER",header);
    json_value *coltypes = json_array_create(mpool,num_fields);
    json_object_add(result,"TYPES",coltypes);
    for(i = 0; i < num_fields; i++) {
	json_array_append(header,json_string_create(mpool,fields[i].name));
	json_array_append(coltypes,
			  json_string_create(mpool, 
					     field_to_string(fields[i], mpool)));
	
	json_value *rows  = json_array_create(mpool,SLAYER_MAX_ROWS);
	json_object_add(result,"ROWS",rows);
	MYSQL_ROW myrow; 
	while( (myrow = mysql_fetch_row(myresult)) !=NULL) { 
	    json_value *orow = json_array_create(mpool,num_fields);
	    json_array_append(rows,orow);
	    //unsigned long *lengths = mysql_fetch_lengths(myresult);
	    for(i = 0; i < num_fields; i++) {
		json_array_append(orow,col_to_json(fields[i], myrow[i], mpool));
	    }
	}
    }
    return result;
}

json_value * dbschema(db_handle_t *dbhandle, apr_pool_t *mpool) {
    int num_tables = 0;
    char *tables[1024];
    MYSQL_RES *res = NULL;
    int num_fields = 0;
    MYSQL_FIELD *fields = NULL;
    json_value *schema = NULL;
    MYSQL_ROW row;
    int i=0;
    int n=0;
    int status = 0;
    schema = json_object_create(mpool);
    res = mysql_list_tables(dbhandle->db, NULL);
    while( (row = mysql_fetch_row(res)) ) {
	tables[num_tables] = apr_pstrdup(mpool, (char *)row[0]);
	num_tables++;
    }
    mysql_free_result(res);
    for (i=0; i < num_tables;  i++ ) {
	json_value *table = json_object_create(mpool);
	res = mysql_list_fields(dbhandle->db, tables[i], NULL);
	num_fields = mysql_num_fields(res);
	fields = mysql_fetch_fields(res);
	for (n=0; n < num_fields; n++) {
	    char * typename = field_to_string(fields[n], mpool);
	    json_object_add(table,apr_pstrdup(mpool,fields[n].name), json_string_create(mpool,typename));
	}
	mysql_free_result(res);
	json_object_add(schema, tables[i], table);
    }
    return schema;
}
  
json_value * dbexecute(db_handle_t *dbhandle, json_value *injson, apr_pool_t *mpool) {
    json_value *out = json_object_create(mpool);
    json_value *sql = NULL;
    if(injson->type == JSON_OBJECT && (sql = (json_value*)apr_hash_get(injson->value.object,"SQL",APR_HASH_KEY_STRING)) !=NULL && sql->type == JSON_STRING) { 
	if(mysql_query(dbhandle->db,sql->value.string)) { 
	    /** NEED TO CHECK FOR BETTER ERROR TO MAKE SURE IT IS A CONNECTION ISSUE **/
	    if(db_handle_reattach(dbhandle) == NULL || mysql_query(dbhandle->db,sql->value.string)){
		json_object_add(out,"MYSQL_ERROR",json_string_create(mpool,mysql_error(dbhandle->db)));
		json_object_add(out,"MYSQL_ERRNO",json_long_create(mpool,mysql_errno(dbhandle->db)));
		json_object_add(out,"SERVER" , json_string_create(mpool,dbhandle->server[dbhandle->server_offset]));
		return out;
	    }
	}
	json_value *all_result = NULL;
	json_value *sql_result = NULL;
	int status = 0;
	do {
	    MYSQL_RES *myresult = mysql_store_result(dbhandle->db);
	    if(myresult == NULL  ) {  
		/** ERROR OCCURED ***/
		if(mysql_errno(dbhandle->db)) {
		    json_object_add(out,"SUCCESS",json_boolean_create(mpool,0));
		    json_object_add(out,"MYSQL_ERROR",json_string_create(mpool,mysql_error(dbhandle->db)));
		    json_object_add(out,"MYSQL_ERRNO",json_long_create(mpool,mysql_errno(dbhandle->db)));
		    json_object_add(out,"SERVER" , json_string_create(mpool,dbhandle->server[dbhandle->server_offset]));
		} else {
		    /** SUCCESS NO RESULT RETURNED ie UPDATE | DELETE | INSERT ***/
		    ;
		    if(sql_result && all_result ) { 
			sql_result = json_object_create(mpool);
			json_object_add(sql_result,"SUCCESS",json_boolean_create(mpool,1));
			json_array_append(all_result,sql_result);
		    } else if (sql_result && !all_result)  { 
			all_result = json_array_create(mpool,5);
			json_array_append(all_result,sql_result);
			sql_result = json_object_create(mpool);
			json_object_add(sql_result,"SUCCESS",json_boolean_create(mpool,1));
			json_array_append(all_result,sql_result);
			json_object_add(out,"RESULT",all_result);
		    } else { 
			sql_result = json_object_create(mpool);
			json_object_add(sql_result,"SUCCESS",json_boolean_create(mpool,1));
		    }
		}
	    } else { 
		if(sql_result && all_result ) { 
		    json_array_append(all_result,result_to_json(myresult,mpool));
		} else if (sql_result && !all_result)  { 
		    all_result = json_array_create(mpool,5);
		    json_array_append(all_result,sql_result);
		    json_array_append(all_result,result_to_json(myresult,mpool));
		    json_object_add(out,"RESULT",all_result);
		} else { 
		    sql_result = result_to_json(myresult,mpool);
		}
		mysql_free_result(myresult);
	    }
	    if ((status = mysql_next_result(dbhandle->db)) > 0) printf("Could not execute statement\n");
	} while(status ==0);

	if(sql_result && !all_result) { 
	    json_object_add(out,"RESULT",sql_result);
	}
	json_object_add(out,"SERVER" , json_string_create(mpool,dbhandle->server[dbhandle->server_offset]));
    } 
    if(injson->type == JSON_OBJECT && (sql = (json_value*)apr_hash_get(injson->value.object,"STAT",APR_HASH_KEY_STRING)) !=NULL && sql->type == JSON_BOOLEAN && sql->value.boolean) { 
	json_object_add(out,"STAT",json_string_create(mpool,mysql_stat(dbhandle->db)));
    }
    if(injson->type == JSON_OBJECT && (sql = (json_value*)apr_hash_get(injson->value.object,"CLIENT_INFO",APR_HASH_KEY_STRING)) !=NULL && sql->type == JSON_BOOLEAN && sql->value.boolean) { 
	json_object_add(out,"CLIENT_INFO",json_string_create(mpool,mysql_get_client_info()));
    }
    if(injson->type == JSON_OBJECT && (sql = (json_value*)apr_hash_get(injson->value.object,"HOST_INFO",APR_HASH_KEY_STRING)) !=NULL && sql->type == JSON_BOOLEAN && sql->value.boolean) { 
	json_object_add(out,"HOST_INFO",json_string_create(mpool,mysql_get_host_info(dbhandle->db)));
    }
    if(injson->type == JSON_OBJECT && (sql = (json_value*)apr_hash_get(injson->value.object,"SERVER_VERSION",APR_HASH_KEY_STRING)) !=NULL && sql->type == JSON_BOOLEAN && sql->value.boolean) { 
	json_object_add(out,"SERVER_VERSION",json_long_create(mpool,mysql_get_server_version(dbhandle->db)));
    }
    if(injson->type == JSON_OBJECT && (sql = (json_value*)apr_hash_get(injson->value.object,"CLIENT_VERSION",APR_HASH_KEY_STRING)) !=NULL && sql->type == JSON_BOOLEAN && sql->value.boolean) { 
	json_object_add(out,"CLIENT_VERSION",json_long_create(mpool,mysql_get_client_version()));
    }
    if(injson->type == JSON_OBJECT && (sql = (json_value*)apr_hash_get(injson->value.object,"SLAYER_DEBUG_RETURN_INPUT",APR_HASH_KEY_STRING)) !=NULL && sql->type == JSON_BOOLEAN && sql->value.boolean) { 
	json_object_add(out,"SLAYER_DEBUG_RETURN_INPUT",injson);
    }
    if(injson->type == JSON_OBJECT && (sql = (json_value*)apr_hash_get(injson->value.object,"SERVER_NAME",APR_HASH_KEY_STRING)) !=NULL && sql->type == JSON_BOOLEAN && sql->value.boolean) { 
	json_object_add(out,"SERVER_NAME",json_string_create(mpool,dbhandle->server[dbhandle->server_offset]));
    }
    if(injson->type == JSON_OBJECT && (sql = (json_value*)apr_hash_get(injson->value.object,"SLAYER_HELP",APR_HASH_KEY_STRING)) !=NULL && sql->type == JSON_BOOLEAN && sql->value.boolean) { 
	json_value *commands = json_object_create(mpool);
	json_object_add(out,"SLAYER_HELP",commands);
	json_object_add(commands,"SQL",json_string_create(mpool,"in(string)[sql statement to execute] : out()eturns results in RESULTS node"));
	json_object_add(commands,"STAT",json_string_create(mpool,"in(boolean) : out(string) result of mysql_stat()"));
	json_object_add(commands,"CLIENT_INFO",json_string_create(mpool,"in(boolean) : out(string) result of mysql_get_client_info()"));
	json_object_add(commands,"HOST_INFO",json_string_create(mpool,"in(boolean) : out(string) result of mysql_get_host_info()"));
	json_object_add(commands,"SERVER_VERSION",json_string_create(mpool,"in(boolean) : out(long) result of mysql_server_version()"));
	json_object_add(commands,"CLIENT_VERSION",json_string_create(mpool,"in(boolean) : out(long) result of mysql_client_version()"));
	json_object_add(commands,"SLAYER_HELP",json_string_create(mpool,"in(boolean) : true ? returns this result"));
	json_object_add(commands,"SLAYER_DEBUG_RETURN_INPUT",json_string_create(mpool,"in(boolean) : true ? returns this result"));
	json_object_add(commands,"SERVER_NAME",json_string_create(mpool,"in(boolean) : true ? returns stanza from the -s option that was used"));
    }
    if(out->type == JSON_OBJECT && apr_hash_count(out->value.object) == 0) { 
	json_object_add(out,"ERROR",json_string_create(mpool,"TRY {\"SLAYER_HELP\":true } "));
    }
    return out;
}
