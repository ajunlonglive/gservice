#include <jansson.h>
#include "se_errno.h"
#include "se_conf.h"
#if defined(_MSC_VER)
#if defined(_DEBUG )
#include <vld.h>
#endif
#endif

//配置文件名
#define CONFIG_FILE ("./conf.json")

//安全关闭文件句柄
#define SE_fclose(stream_)\
do {\
	if (!SE_PTR_ISNULL(stream_)) {\
		fclose(stream_); stream_ = NULL;\
	}\
} while (0);

//安全释放分配的内容
#define SE_json_free(ptr)\
do {\
	if (!SE_PTR_ISNULL(ptr)) {\
		json_decref(ptr); ptr = NULL;\
	}\
} while (0);

static void SE_databases_free(struct SE_DBCONFS **ptr) {
	struct SE_DBCONFS *dbs = *ptr;
	int32_t i = 0;
	if (!SE_PTR_ISNULL(dbs)) {
		if (!SE_PTR_ISNULL(dbs->item)) {
			for (i = 0; i < dbs->count; ++i) {
				if (!SE_PTR_ISNULL(dbs->item[i])) {
					free(dbs->item[i]->server);
					free(dbs->item[i]->dbname);
					free(dbs->item[i]->user);
					free(dbs->item[i]->password);
					free(dbs->item[i]->client_encoding);
					free(dbs->item[i]);
				}
			}
			free(dbs->item);
		}
		free(dbs);
	}
	(*ptr) = NULL;
}

/*释放创建的SE_CONF结构体*/
void SEAPI SE_conf_free(struct SE_CONF **ptr) {
	struct SE_CONF *conf = *ptr;
	if (!SE_PTR_ISNULL(conf)) {
		SE_databases_free(&conf->dbs);
		SE_free(&conf->name);
		SE_free(&conf->double_format);
		SE_free(&conf->redirect_url);
		free(conf);
	}
	(*ptr) = NULL;
}

/*解析配置文件database节点*/
static bool SE_parse_database(const json_t *database, struct SE_DBCONF *db) {
	json_t  *server, *port, *dbname, *user, *password, *client_encoding,
		*connect_timeout, *statement_timeout, *max_connection, *weight, *iswriter;
	const char *strserver, *strdbname, *struser, *strpassword, *strclient_encoding;


	SE_check_nullptr(database, "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, strerror(EINVAL));
	SE_check_nullptr(db, "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, strerror(EINVAL));

	SE_check_nullptr((server = json_object_get(database, "server")), "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, "config databases item ""server"" does not exist");
	SE_check_nullptr((port = json_object_get(database, "port")), "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, "config databases item ""port"" does not exist");
	SE_check_nullptr((dbname = json_object_get(database, "dbname")), "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, "config databases item ""dbname"" does not exist");
	SE_check_nullptr((user = json_object_get(database, "user")), "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, "config databases item ""user"" does not exist");
	SE_check_nullptr((password = json_object_get(database, "password")), "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, "config databases item ""password"" does not exist");
	SE_check_nullptr((client_encoding = json_object_get(database, "client_encoding")), "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, "config databases item ""client_encoding"" does not exist");
	SE_check_nullptr((connect_timeout = json_object_get(database, "connect_timeout")), "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, "config databases item ""connect_timeout"" does not exist");
	SE_check_nullptr((statement_timeout = json_object_get(database, "statement_timeout")), "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, "config databases item ""statement_timeout"" does not exist");
	SE_check_nullptr((max_connection = json_object_get(database, "max_connection")), "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, "config databases item ""max_connection"" does not exist");
	SE_check_nullptr((weight = json_object_get(database, "weight")), "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, "config databases item ""weight"" does not exist");
	SE_check_nullptr((iswriter = json_object_get(database, "iswriter")), "%s(%d),%s\n", SE_2STR(SE_parse_database), __LINE__, "config databases item ""iswriter"" does not exist");

	if (!json_is_string(server)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""server"" must is  string type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(port)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""port"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_string(dbname)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""dbname"" must is  string type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_string(user)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""user"" must is  string type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_string(password)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""password"" must is  string type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_string(client_encoding)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""client_encoding"" must is  string type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(connect_timeout)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""connect_timeout"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(statement_timeout)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""statement_timeout"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(max_connection)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""max_connection"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}

	if (!json_is_integer(weight)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""weight"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_boolean(iswriter)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""iswriter"" must is  boolean type");
		goto SE_ERROR_CLEAR;
	}

	strserver = json_string_value(server);
	strdbname = json_string_value(dbname);
	struser = json_string_value(user);
	strpassword = json_string_value(password);
	strclient_encoding = json_string_value(client_encoding);

	if (SE_STR_ISNULLOREMPTY(strserver) || '\0' == strserver[0]) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""server"" is  empty.");
		goto SE_ERROR_CLEAR;
	}
	if (SE_STR_ISNULLOREMPTY(strdbname) || '\0' == strdbname[0]) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""dbname"" is  empty.");
		goto SE_ERROR_CLEAR;
	}
	if (SE_STR_ISNULLOREMPTY(struser) || '\0' == struser[0]) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""user"" is  empty.");
		goto SE_ERROR_CLEAR;
	}
	if (SE_STR_ISNULLOREMPTY(strpassword) || '\0' == strpassword[0]) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""password"" is  empty.");
		goto SE_ERROR_CLEAR;
	}
	if (SE_STR_ISNULLOREMPTY(strclient_encoding) || '\0' == strclient_encoding[0]) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(parse_databases), __LINE__, "config databases item ""client_encoding"" is  empty.");
		goto SE_ERROR_CLEAR;
	}

	SE_string_copy(strserver, &db->server);
	db->port = (int32_t)json_integer_value(port);
	SE_string_copy(strdbname, &db->dbname);
	SE_string_copy(struser, &db->user);
	SE_throw(SE_aes_decrypt(NULL, NULL, strpassword, &db->password));
	SE_string_copy(strclient_encoding, &db->client_encoding);
	db->connect_timeout = (int32_t)json_integer_value(connect_timeout);
	db->statement_timeout = (int32_t)json_integer_value(statement_timeout);
	db->max_connection = (int32_t)json_integer_value(max_connection);
	db->weight = (int32_t)json_integer_value(weight);
	db->iswrite = json_boolean_value(iswriter);

	if (db->port < 1 || db->port > 65535) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "database ""port"" value range is 1-65535");
		goto SE_ERROR_CLEAR;
	}

	if (db->connect_timeout < 1 || db->connect_timeout > 180) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "database ""connect_timeout"" value range is 1-120");
		goto SE_ERROR_CLEAR;
	}
	if (db->statement_timeout < 1 || db->statement_timeout > 180) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "database ""statement_timeout"" value range is 1-120");
		goto SE_ERROR_CLEAR;
	}
	if (db->max_connection < 1 || db->max_connection > 1024) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "database ""max_connection"" value range is 1-100");
		goto SE_ERROR_CLEAR;
	}
	if (db->weight < 1 || db->weight > 100) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "database ""weight"" value range is 1-100");
		goto SE_ERROR_CLEAR;
	}
	return true;
SE_ERROR_CLEAR:
	SE_free(&db->server);
	SE_free(&db->dbname);
	SE_free(&db->user);
	SE_free(&db->password);
	SE_free(&db->client_encoding);
	return false;
}

/*解析配置文件databases节点*/
static bool SE_parse_databases(const json_t *databases, struct SE_DBCONFS **ptr) {
	struct SE_DBCONFS *dbs = NULL;
	json_t *item;
	int32_t i = 0, size = 0;

	SE_check_nullptr(databases, "%s(%d),%s\n", SE_2STR(SE_parse_databases), __LINE__, strerror(EINVAL));

	if (!json_is_array(databases)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_databases), __LINE__, "config ""databases"" must is array");
		goto SE_ERROR_CLEAR;
	}

	size = (int32_t)json_array_size(databases);
	if (size < 1) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_databases), __LINE__, "config ""databases"" number of items less than 1");
		goto SE_ERROR_CLEAR;
	}
	SE_check_nullptr((dbs = (struct SE_DBCONFS *)calloc(size, sizeof(struct SE_DBCONFS))), "%s(%d),%s\n", SE_2STR(calloc), __LINE__, strerror(ENOMEM));
	dbs->count = size;
	SE_check_nullptr((dbs->item = (struct SE_DBCONF **)calloc(size, sizeof(struct SE_DBCONF *))), "%s(%d),%s\n", SE_2STR(calloc), __LINE__, strerror(ENOMEM));
	for (i = 0; i < size; ++i) {
		SE_check_nullptr((dbs->item[i] = (struct SE_DBCONF *)calloc(1, sizeof(struct SE_DBCONF))), "%s(%d),%s\n", SE_2STR(calloc), __LINE__, strerror(ENOMEM));
		SE_check_nullptr((item = json_array_get(databases, i)), "%s(%d),%s\n", SE_2STR(SE_parse_databases), __LINE__, "config databases item does not exist");
		SE_throw(SE_parse_database(item, dbs->item[i]));
	}
	(*ptr) = dbs;
	return true;
SE_ERROR_CLEAR:
	SE_databases_free(&dbs);
	return false;
}

/*解析配置文件根节点*/
static bool SE_parse_root(json_t *root, struct SE_CONF *conf) {
	json_t *name, *port, *token_keep, *pool, *queue_max, *backlog, *send_timeout, *recv_timeout, *double_format, *database_max_use_count, *databases, *redirect_url;
	const char *str_name, *str_double_format, *str_redirect_url;

	SE_check_nullptr(root, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, strerror(EINVAL));
	SE_check_nullptr(conf, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, strerror(EINVAL));

	SE_check_nullptr((name = json_object_get(root, "name")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""name"" does not exist");
	SE_check_nullptr((port = json_object_get(root, "port")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""port"" does not exist");
	SE_check_nullptr((token_keep = json_object_get(root, "token_keep")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""token_keep"" does not exist");
	SE_check_nullptr((pool = json_object_get(root, "pool")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""pool"" does not exist");
	SE_check_nullptr((queue_max = json_object_get(root, "queue_max")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""queue_max"" does not exist");
	SE_check_nullptr((backlog = json_object_get(root, "backlog")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""backlog"" does not exist");
	SE_check_nullptr((send_timeout = json_object_get(root, "send_timeout")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""send_timeout"" does not exist");
	SE_check_nullptr((recv_timeout = json_object_get(root, "recv_timeout")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""recv_timeout"" does not exist");
	SE_check_nullptr((double_format = json_object_get(root, "double_format")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""double_format"" does not exist");
	SE_check_nullptr((database_max_use_count = json_object_get(root, "database_max_use_count")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""database_max_use_count"" does not exist");
	SE_check_nullptr((redirect_url = json_object_get(root, "redirect_url")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""redirect_url"" does not exist");
	SE_check_nullptr((databases = json_object_get(root, "databases")), "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""databases"" does not exist");
	if (!json_is_string(name)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""name"" must is  string type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(port)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""port"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(token_keep)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""token_keep"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(pool)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""pool"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(queue_max)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""queue_max"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(backlog)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""backlog"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(send_timeout)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""send_timeout"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(recv_timeout)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""recv_timeout"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_string(redirect_url)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""redirect_url"" must is  string type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_string(double_format)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""double_format"" must is  string type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_integer(database_max_use_count)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""database_max_use_count"" must is  integer type");
		goto SE_ERROR_CLEAR;
	}
	if (!json_is_array(databases)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""databases"" must is array type");
		goto SE_ERROR_CLEAR;
	}

	str_name = json_string_value(name);
	str_double_format = json_string_value(double_format);
	str_redirect_url = json_string_value(redirect_url);
	if (SE_STR_ISNULLOREMPTY(str_name)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""name"" is  empty.");
		goto SE_ERROR_CLEAR;
	}
	if (SE_STR_ISNULLOREMPTY(str_double_format)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""double_format"" is  empty.");
		goto SE_ERROR_CLEAR;
	}
	if (SE_STR_ISNULLOREMPTY(str_redirect_url)) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""redirect_url"" is  empty.");
		goto SE_ERROR_CLEAR;
	}

	SE_string_copy(str_name, &conf->name);
	conf->port = (int32_t)json_integer_value(port);
	conf->token_keep = (int32_t)json_integer_value(token_keep);
	conf->pool = (int32_t)json_integer_value(pool);
	conf->queue_max = (int32_t)json_integer_value(queue_max);
	conf->backlog = (int32_t)json_integer_value(backlog);
	conf->send_timeout = (int32_t)json_integer_value(send_timeout);
	conf->recv_timeout = (int32_t)json_integer_value(recv_timeout);
	SE_string_copy(str_double_format, &conf->double_format);
	SE_string_copy(str_redirect_url, &conf->redirect_url);
	conf->database_max_use_count = (int32_t)json_integer_value(database_max_use_count);


	if (conf->port < 1 || conf->port > 65535) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""port"" value range is 1-65535");
		goto SE_ERROR_CLEAR;
	}

	if (conf->token_keep < 20 || conf->token_keep > 120) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""token_keep"" value range is 20-120");
		goto SE_ERROR_CLEAR;
	}

	if (conf->pool < 1 || conf->pool > 512) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""pool"" value range is 1-512");
		goto SE_ERROR_CLEAR;
	}

	if (conf->queue_max < 1024 || conf->queue_max > 8192) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""queue_max"" value range is 1024-8192");
		goto SE_ERROR_CLEAR;
	}

	if (conf->backlog < 64 || conf->backlog > 1024) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""backlog"" value range is 64-1024");
		goto SE_ERROR_CLEAR;
	}

	if (conf->send_timeout < 1 || conf->send_timeout > 120) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""send_timeout"" value range is 1-120");
		goto SE_ERROR_CLEAR;
	}
	if (conf->recv_timeout < 1 || conf->recv_timeout > 120) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""recv_timeout"" value range is 1-120");
		goto SE_ERROR_CLEAR;
	}
	if (conf->database_max_use_count < 0 || conf->database_max_use_count > 32768) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_parse_root), __LINE__, "config item ""database_max_use_count"" value range is 0-32768");
		goto SE_ERROR_CLEAR;
	}
	SE_throw(SE_parse_databases(databases, &conf->dbs));
	return true;
SE_ERROR_CLEAR:
	return false;
}

/*解析配置文件*/
static bool SE_parse_conf(struct SE_CONF *conf) {
	json_t *object = NULL;
	json_error_t error;

	SE_check_nullptr(conf, "%s(%d),%s\n", SE_2STR(SE_parse_conf), __LINE__, strerror(EINVAL));
	//不允许有重复的key
	object = json_load_file(CONFIG_FILE, JSON_REJECT_DUPLICATES, &error);
	if (SE_PTR_ISNULL(object)) {
		fprintf(stderr, "%s(%d:%d)\n", error.text, error.line, error.column);
		return false;
	}
	SE_throw(SE_parse_root(object, conf));
	SE_json_free(object);
	return true;
SE_ERROR_CLEAR:
	SE_json_free(object);
	return false;
}
//
///*读取配置文件*/
//static bool SE_read_conf(char **ptr, int32_t *mem_len) {
//	FILE *stream = NULL;
//	char *data = NULL, *tmp_txt;
//	int64_t  length;
//	int32_t rc = 0;
//
//	//SE_check_nullptr(ptr, "%s(%d),%s\n", SE_2STR(SE_read_conf), __LINE__, strerror(EINVAL));
//	SE_check_nullptr(mem_len, "%s(%d),%s\n", SE_2STR(SE_read_conf), __LINE__, strerror(EINVAL));
//
//	if (0 != se_access(CONFIG_FILE, 0)) {
//		fprintf(stderr, "%s(%d),%s ""%s""\n", SE_2STR(SE_read_conf), __LINE__, strerror(errno), CONFIG_FILE);
//		goto SE_ERROR_CLEAR;
//	}
//	(*mem_len) = 0;
//	SE_check_nullptr((stream = fopen(CONFIG_FILE, PG_BINARY_R)), "%s(%d),%s\n", SE_2STR(fopen), __LINE__, strerror(errno));
//	//计算文件大小
//	SE_check_rc((rc = fseek(stream, 0L, SEEK_END)), "%s(%d),%s\n", SE_2STR(fseek), __LINE__, strerror(rc));
//	length = ftell(stream);
//	SE_check_rc((rc = fseek(stream, 0L, SEEK_SET)), "%s(%d),%s\n", SE_2STR(fseek), __LINE__, strerror(rc));
//	if (length < 1 || length > 2147483647)
//		SE_gotoerr("%s(%d),%s\n", SE_2STR(SE_read_conf), __LINE__, strerror(EFBIG));
//	//分配内存
//	SE_check_nullptr((data = (char *)malloc((int32_t)length + 1)), "%s(%d),%s\n", SE_2STR(malloc), __LINE__, strerror(ENOMEM));
//	tmp_txt = data;
//	//注意当fread读取数据时，缓冲区指针也会移动(也就是fread的第一个参数)
//	//在本例中为tmp_txt,所以要设置临时缓冲区tmp_txt
//	while ((rc = fread(tmp_txt, sizeof(char), (size_t)(length > SE_BUFSZIE ? SE_BUFSZIE : length), stream)) > 0) {
//		(*mem_len) += rc;
//		if ((rc = ferror(stream)))
//			SE_gotoerr("%s\n", strerror(rc));
//		if (feof(stream)) {
//			break;
//		}
//	}
//	SE_fclose(stream);
//	data[(*mem_len)] = '\0';
//	(*ptr) = data;
//	return true;
//SE_ERROR_CLEAR:
//	(*mem_len) = 0;
//	SE_free(&data);
//	SE_fclose(stream);
//	return false;
//}


bool SEAPI SE_conf_create(struct SE_CONF **ptr) {
	struct SE_CONF *conf = NULL;
	SE_check_nullptr((conf = (struct SE_CONF *)calloc(1, sizeof(struct SE_CONF))), "%s(%d),%s\n", SE_2STR(SE_conf_create), __LINE__, strerror(ENOMEM));
	SE_throw(SE_parse_conf(conf));
	(*ptr) = conf;
	return true;
SE_ERROR_CLEAR:
	SE_conf_free(&conf);
	*ptr = NULL;
	return false;
}


/*	PQpingParams连接参数数量
*	最后一个元素为NULL,表示数组结束
*	所以数组头实际大小要+1
*/
#define SE_LIBPQPING_COUNT (2)
/*	PQconnectdbParams连接参数数量
*	最后一个元素为NULL,表示数组结束
*	所以数组头实际大小要+1
*/
#define SE_LIBPQPARAM_COUNT (8)
/*多字节单个字符最大内存大小,如果关键字和值仅由英文和数字组成时为1,否则修改为4*/
#define SE_UNI_SINGLE_CHAR_SIZE (1)
/*连接参数字符串最大字符数*/
#define SE_LIBPQ_CONN_MAXCOUNT (16)
/*	PQconnectdbParams连接参数key字符串的最大内存大小*/
#define SE_LIBPQKEY_MAX_MEMLEN (SE_LIBPQ_CONN_MAXCOUNT * SE_UNI_SINGLE_CHAR_SIZE + sizeof(char))
/*	PQconnectdbParams连接参数values字符串的最大内存大小*/
#define SE_LIBPQVALUE_MAX_MEMLEN (SE_LIBPQ_CONN_MAXCOUNT * SE_UNI_SINGLE_CHAR_SIZE+ sizeof(char))



/*将数据库连接信息转换键值对数组,必须使用SE_dbconf_conn_free释放ptr*/
bool SEAPI SE_dbconf_conn(const char * const appname,
	const struct SE_DBCONF * const dbconf, char ***ptr_keys, char ***ptr_vals) {
	static char szBuffer[64];
	char **keys = NULL, **vals = NULL, *tmpKeys, *tmpVals;
	size_t arrHeadSize = 0;
	int32_t i = 0;

	SE_validate_para_ptr(appname, SE_2STR(SE_dbconf_conn), __LINE__);
	SE_validate_para_ptr(dbconf, SE_2STR(SE_dbconf_conn), __LINE__);

	/*数组实际具有9个元素,其中8个为有效信息,最后一个元素为NULL表示数组结束(libpq格式) */
	arrHeadSize = (sizeof(char*) * SE_LIBPQPARAM_COUNT) + sizeof(char*);
	/*一次性为数组分配存储空间*/
	SE_check_nullptr((keys = (char **)malloc(arrHeadSize + (SE_LIBPQPARAM_COUNT * SE_LIBPQKEY_MAX_MEMLEN))),
		"%s(%d),%s\n", SE_2STR(malloc), __LINE__, strerror(ENOMEM));
	SE_check_nullptr((vals = (char **)malloc(arrHeadSize + (SE_LIBPQPARAM_COUNT * SE_LIBPQVALUE_MAX_MEMLEN))),
		"%s(%d),%s\n", SE_2STR(malloc), __LINE__, strerror(ENOMEM));
	/*跳过数组头*/
	tmpKeys = (char *)keys + arrHeadSize;
	tmpVals = (char *)vals + arrHeadSize;
	/*设置数组各项目指针*/
	for (i = 0; i < SE_LIBPQPARAM_COUNT; ++i) {
		keys[i] = tmpKeys;
		tmpKeys += SE_LIBPQKEY_MAX_MEMLEN;

		vals[i] = tmpVals;
		tmpVals += SE_LIBPQVALUE_MAX_MEMLEN;
	}
	//最后一个元素为NULL, 表示数组结束
	keys[SE_LIBPQPARAM_COUNT] = NULL;
	vals[SE_LIBPQPARAM_COUNT] = NULL;
	/*设置连接参数名,最多SE_LIBPQ_CONN_MAXCOUNT个字符*/
	SE_strcpy(keys[0], SE_LIBPQKEY_MAX_MEMLEN, "host");							//主机ip,必须使用ip地址,如要用名称,修改为host
	SE_strcpy(keys[1], SE_LIBPQKEY_MAX_MEMLEN, "port");							//端口
	SE_strcpy(keys[2], SE_LIBPQKEY_MAX_MEMLEN, "dbname");						//数据库名
	SE_strcpy(keys[3], SE_LIBPQKEY_MAX_MEMLEN, "user");							//用户名
	SE_strcpy(keys[4], SE_LIBPQKEY_MAX_MEMLEN, "password");					//密码
	SE_strcpy(keys[5], SE_LIBPQKEY_MAX_MEMLEN, "connect_timeout");			//连接超时,单位为毫秒
	SE_strcpy(keys[6], SE_LIBPQKEY_MAX_MEMLEN, "client_encoding");			//客户端字符串编码
	SE_strcpy(keys[7], SE_LIBPQKEY_MAX_MEMLEN, "application_name");		//应用程序名称,用于区分链接来自那里
	//设置连接参数值,最多SE_LIBPQ_CONN_MAXCOUNT个字符,注意数组下标必须和keys对应
	SE_strcpy(vals[0], SE_LIBPQVALUE_MAX_MEMLEN, dbconf->server);			//使用hostaddr时必须是一个ip地址.使用host时为主机名,host较hostaddr将发生一次主机名查找
	snprintf(szBuffer, 64, "%d", dbconf->port);
	SE_strcpy(vals[1], SE_LIBPQVALUE_MAX_MEMLEN, szBuffer);
	SE_strcpy(vals[2], SE_LIBPQVALUE_MAX_MEMLEN, dbconf->dbname);
	SE_strcpy(vals[3], SE_LIBPQVALUE_MAX_MEMLEN, dbconf->user);
	SE_strcpy(vals[4], SE_LIBPQVALUE_MAX_MEMLEN, dbconf->password);
	snprintf(szBuffer, 64, "%d", dbconf->connect_timeout);
	SE_strcpy(vals[5], SE_LIBPQVALUE_MAX_MEMLEN, szBuffer);
	SE_strcpy(vals[6], SE_LIBPQVALUE_MAX_MEMLEN, dbconf->client_encoding);
	SE_strcpy(vals[7], SE_LIBPQVALUE_MAX_MEMLEN, appname);
	(*ptr_keys) = keys;
	(*ptr_vals) = vals;
	return true;
SE_ERROR_CLEAR:
	SE_dbconf_conn_free(&keys, &vals);
	return false;
}

bool SEAPI SE_dbconf_ping(const struct SE_DBCONF * const dbconf, char ***ptr_keys, char ***ptr_vals) {
	static char szBuffer[64];
	char **keys = NULL, **vals = NULL, *tmpKeys, *tmpVals;
	size_t arrHeadSize = 0;
	int32_t i = 0;

	SE_validate_para_ptr(dbconf, SE_2STR(SE_dbconf_ping), __LINE__);

	/*数组实际具有9个元素,其中8个为有效信息,最后一个元素为NULL表示数组结束(libpq格式) */
	arrHeadSize = (sizeof(char*) * SE_LIBPQPING_COUNT) + sizeof(char*);
	/*一次性为数组分配存储空间*/
	SE_check_nullptr((keys = (char **)malloc(arrHeadSize + (SE_LIBPQPING_COUNT * SE_LIBPQKEY_MAX_MEMLEN))),
		"%s(%d),%s\n", SE_2STR(malloc), __LINE__, strerror(ENOMEM));
	SE_check_nullptr((vals = (char **)malloc(arrHeadSize + (SE_LIBPQPING_COUNT * SE_LIBPQVALUE_MAX_MEMLEN))),
		"%s(%d),%s\n", SE_2STR(malloc), __LINE__, strerror(ENOMEM));
	/*跳过数组头*/
	tmpKeys = (char *)keys + arrHeadSize;
	tmpVals = (char *)vals + arrHeadSize;
	/*设置数组各项目指针*/
	for (i = 0; i < SE_LIBPQPING_COUNT; ++i) {
		keys[i] = tmpKeys;
		tmpKeys += SE_LIBPQKEY_MAX_MEMLEN;

		vals[i] = tmpVals;
		tmpVals += SE_LIBPQVALUE_MAX_MEMLEN;
	}
	//最后一个元素为NULL, 表示数组结束
	keys[SE_LIBPQPING_COUNT] = NULL;
	vals[SE_LIBPQPING_COUNT] = NULL;
	/*设置连接参数名,最多SE_LIBPQ_CONN_MAXCOUNT个字符*/
	SE_strcpy(keys[0], SE_LIBPQKEY_MAX_MEMLEN, "host");							//主机ip,必须使用ip地址,如要用名称,修改为host
	SE_strcpy(keys[1], SE_LIBPQKEY_MAX_MEMLEN, "port");							//端口

	SE_strcpy(vals[0], SE_LIBPQVALUE_MAX_MEMLEN, dbconf->server);			//使用hostaddr时必须是一个ip地址.使用host时为主机名,host较hostaddr将发生一次主机名查找
	snprintf(szBuffer, 64, "%d", dbconf->port);
	SE_strcpy(vals[1], SE_LIBPQVALUE_MAX_MEMLEN, szBuffer);
	(*ptr_keys) = keys;
	(*ptr_vals) = vals;
	return true;
SE_ERROR_CLEAR:
	SE_dbconf_conn_free(&keys, &vals);
	return false;
}

/*释放数据库连接信息键值对数组*/
void SEAPI SE_dbconf_conn_free(char ***keys, char ***vals) {
	char **ptr_keys = (*keys);
	char **ptr_vals = (*vals);
	if (!SE_PTR_ISNULL(ptr_keys))
		free(ptr_keys);
	if (!SE_PTR_ISNULL(ptr_vals))
		free(ptr_vals);
	(*keys) = NULL;
	(*vals) = NULL;
}
