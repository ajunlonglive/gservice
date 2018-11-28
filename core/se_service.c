#include "../gsoap/stdsoap2.h"
#include "se_errno.h"
#include "se_conf.h"
#include "se_utils.h"
#include "se_service.h"


struct SE_SHARED *global_shared = NULL;
static const char JWT_KEY[16] = { 0x6B, 0x6D, 0x63, 0x62, 0x30, 0x38, 0x37, 0x31, 0x36, 0x38, 0x32, 0x32, 0x37, 0x38, 0x33, 0x33 };

static void SE_pgcluste_free(struct SE_PGCLUSTER **ptr);
static void SE_shared_free(struct SE_SHARED **ptr);
static void SE_dbconfs_free(struct SE_DBCONF ***ptr, int32_t count);
static void SE_tsoap_free(int32_t pool, struct soap ***ptr);
static void SE_workthred_free(struct SE_SHARED *shared, int32_t pool, pthread_t ***ptr);
static void SE_soap_print_fault(struct soap *soap);
static int32_t enqueue(SOAP_SOCKET sock, struct SE_SHARED *shared);
static SOAP_SOCKET dequeue(struct SE_SHARED *shared, bool *isrun);
static void *process_queue(void *soap);
static bool SE_pgcluster_make(const char * const name, const struct SE_DBCONF *const*dbconfs, int32_t count, struct SE_PGCLUSTER **ptr);
static bool SE_dbconf_copy(const struct SE_CONF * const conf, bool iswriter, struct SE_DBCONF ***ptr, int32_t *arry_count);
static bool SE_connect_pgcluster(const struct SE_CONF * const conf, struct SE_SHARED *shared);
static bool SE_tsoap_make(const struct soap *master, void *process, int32_t pool, struct soap ***ptr);
static bool SE_workthred_make(struct SE_SHARED *shared, struct soap **slaves, int32_t pool, pthread_t ***ptr);
static void SE_soap_serve(struct soap *tsoap);
static void *SE_isten(void *process);

static void SE_pgcluste_free(struct SE_PGCLUSTER **ptr) {
	struct SE_PGCLUSTER *cluster = (*ptr);
	int32_t i = 0;
	if (!SE_PTR_ISNULL(cluster)) {
		if (!SE_PTR_ISNULL(cluster->items)) {
			for (i = 0; i < cluster->count; ++i) {
				if (!SE_PTR_ISNULL(cluster->items[i])) {
					if (!SE_PTR_ISNULL(cluster->items[i]->conn))
						SE_PQfinish(cluster->items[i]->conn);
					free(cluster->items[i]);
				}
			}
			free(cluster->items);
		}
		free(cluster);
	}
	(*ptr) = NULL;
}

static void SE_shared_free(struct SE_SHARED **ptr) {
	struct SE_SHARED *shared = (*ptr);
	if (!SE_PTR_ISNULL(shared)) {
		SE_pgcluste_free(&shared->rcluster);
		SE_pgcluste_free(&shared->wcluster);
		if (!SE_PTR_ISNULL(shared->locks)) {
			if (!SE_PTR_ISNULL(shared->locks->cond)) {
				if (SE_NUMBIT_ISONE(shared->lock_flag, 1))
					pthread_cond_destroy(shared->locks->cond);
				free(shared->locks->cond); shared->locks->cond = NULL;
			}
			if (!SE_PTR_ISNULL(shared->locks->mutex)) {
				if (SE_NUMBIT_ISONE(shared->lock_flag, 0))
					pthread_mutex_destroy(shared->locks->mutex);
				free(shared->locks->mutex); shared->locks->mutex = NULL;
			}
			free(shared->locks);
		}
		if (!SE_PTR_ISNULL(shared->queue)) {
			if (!SE_PTR_ISNULL(shared->queue->queues)) {
				free(shared->queue->queues); shared->queue->queues = NULL;
			}
			free(shared->queue);
		}
		free(shared);
	}
	(*ptr) = NULL;
}

static void SE_dbconfs_free(struct SE_DBCONF ***ptr, int32_t count) {
	struct SE_DBCONF **dbconfs = (*ptr);
	int32_t i = 0;
	if (!SE_PTR_ISNULL(dbconfs)) {
		for (i = 0; i < count; ++i) {
			if (!SE_PTR_ISNULL(dbconfs[i])) {
				SE_free(&dbconfs[i]->server);
				SE_free(&dbconfs[i]->dbname);
				SE_free(&dbconfs[i]->user);
				SE_free(&dbconfs[i]->password);
				SE_free(&dbconfs[i]->client_encoding);
				free(dbconfs[i]);
			}
		}
		free(dbconfs);
	}
	(*ptr) = NULL;
}

static void SE_tsoap_free(int32_t pool, struct soap ***ptr) {
	struct soap **slaves = NULL;
	struct SE_WORKINFO *obj;
	int32_t i = 0;

	slaves = (*ptr);
	if (!SE_PTR_ISNULL(slaves)) {
		for (i = 0; i < pool; ++i) {
			if (!SE_PTR_ISNULL(slaves[i])) {
				if (!SE_PTR_ISNULL(slaves[i]->user)) {
					obj = (struct SE_WORKINFO *)slaves[i]->user;
					freeStringBuilder(&obj->sys);
					//freeStringBuilder(&obj->err);
					freeStringBuilder(&obj->sql);
					freeStringBuilder(&obj->buf);
					free(obj);
				}
				SE_soap_free(slaves[i]);
			}
		}
		free(slaves);
	}
	(*ptr) = NULL;
}

static void SE_workthred_free(struct SE_SHARED *shared, int32_t pool, pthread_t ***ptr) {
	pthread_t **works;
	int32_t i = 0;

	works = *ptr;
	if (!SE_PTR_ISNULL(works)) {
		//发送信号,关闭工作线程
		pthread_mutex_lock(shared->locks->mutex);
		pthread_cond_broadcast(shared->locks->cond);
		pthread_mutex_unlock(shared->locks->mutex);
		//等待工作线程运行完成
		SE_usleep(SLEEP_MILLISEC(100));
		for (i = 0; i < pool; ++i) {
			if (!SE_PTR_ISNULL(works[i])) {
				pthread_join(*works[i], NULL);
				free(works[i]);
			}
		}
		free(works);
	}
	(*ptr) = NULL;
}

static void SE_soap_print_fault(struct soap *soap) {
	if (soap_check_state(soap)) {
		fprintf(stderr, "Error: soap struct state not initialized with soap_init\n");
	} else if (soap->error) {
		const char **c, *v = NULL, *s, *d;
		c = soap_faultcode(soap);
		if (!*c) {
			soap_set_fault(soap);
			c = soap_faultcode(soap);
		}
		if (soap->version == 2)
			v = soap_fault_subcode(soap);
		s = soap_fault_string(soap);
		d = soap_fault_detail(soap);
		fprintf(stderr, "%s%d fault %s [%s]\n\"%s\"\nDetail: %s\n", soap->version ? "SOAP 1." : "Error ", soap->version ? (int)soap->version : soap->error, *c, v ? v : "no subcode", s ? s : "[no reason]", d ? d : "[no detail]");
	}
}


/*
*	入队.如果队列已满的话需要等待片刻后再试
*/
static int32_t enqueue(SOAP_SOCKET sock, struct SE_SHARED *shared) {
	int32_t status = SOAP_OK;
	int32_t next;

	pthread_mutex_lock(shared->locks->mutex);
	next = shared->queue->tail + 1;
	if (next >= shared->conf->queue_max)
		next = 0;
	if (next == shared->queue->head) {
		status = SOAP_EOM;
	} else {
		shared->queue->queues[shared->queue->tail] = sock;
		shared->queue->tail = next;
		pthread_cond_signal(shared->locks->cond);
	}
	pthread_mutex_unlock(shared->locks->mutex);
	return status;
}

/*
*	出队,在出队中wait
*/
static SOAP_SOCKET dequeue(struct SE_SHARED *shared, bool *isrun) {
	SOAP_SOCKET sock = SOAP_INVALID_SOCKET;

	pthread_mutex_lock(shared->locks->mutex);
	//队首和队尾相同表示队列为空,等待任务.
	while (shared->queue->head == shared->queue->tail) {
		pthread_cond_wait(shared->locks->cond, shared->locks->mutex);
		if (!shared->isrun) {//服务已经停止			
			pthread_mutex_unlock(shared->locks->mutex);
			(*isrun) = false;
			return sock;
		}
	}
	/*	注意:
	*	array[++index]	++在前表示先index = index+1,然后再从数组index的位置获取值
	*	array[index++]	++在后表示先从数组index的位置获取值,然后再index = index+1
	*/
	sock = shared->queue->queues[shared->queue->head++];
	if (shared->queue->head >= shared->conf->queue_max)
		shared->queue->head = 0;
	pthread_mutex_unlock(shared->locks->mutex);
	(*isrun) = true;
	return sock;
}

/*
*	队列处理函数
*/
static void *process_queue(void *soap) {
	struct soap *tsoap = (struct soap*)soap;
	bool isrun = true;

	while (1) {
		tsoap->socket = dequeue(global_shared, &isrun);
		//服务是否停止
		if (!isrun)
			break;
		if (soap_valid_socket(tsoap->socket))
			SE_soap_serve(tsoap);
		SE_soap_clear(tsoap);
	}
	return NULL;
}

static bool SE_get_pg_timezone(PGconn *conn, int32_t *time_zone) {
	PGresult *result = NULL;
	char *sql = NULL;
	char *bin_result;
	int64_t tmp64_val;
	double val;

	SE_check_nullptr((sql = (char *)malloc(128)), "%s(%d),%s\n", SE_2STR(SE_get_pg_timezone), __LINE__, strerror(ENOMEM));
	snprintf(sql, 128, "select extract(timezone from now());");
	result = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, 1);
	if (PGRES_TUPLES_OK != PQresultStatus(result)) {
		fprintf(stderr, "get database time zone failed: %s\n", PQerrorMessage(conn));
		goto SE_ERROR_CLEAR;
	}
	bin_result = PQgetvalue(result, 0, 0);
	tmp64_val = (int64_t)pg_hton64(*((const uint64_t *)bin_result));
	val = *((double *)(&tmp64_val));
	*time_zone = (int32_t)val;
	SE_free(&sql);
	SE_PQclear(result);
	return true;
SE_ERROR_CLEAR:
	SE_free(&sql);
	SE_PQclear(result);
	return false;
}

static bool SE_pgcluster_make(const char * const name, const struct SE_DBCONF *const*dbconfs, int32_t count, struct SE_PGCLUSTER **ptr) {
	char **keys = NULL, **vals = NULL;
	struct SE_PGCLUSTER *cluster = NULL;
	PGresult *result = NULL;
	int32_t i = 0;
	char *sql = NULL;

	SE_validate_para_ptr(name, SE_2STR(pgcluster_make), __LINE__);
	SE_validate_para_ptr(dbconfs, SE_2STR(pgcluster_make), __LINE__);
	SE_validate_para_zero(count, SE_2STR(pgcluster_make), __LINE__);

	SE_check_nullptr((cluster = (struct SE_PGCLUSTER *)calloc(1, sizeof(struct SE_PGCLUSTER))), "%s(%d),%s\n", SE_2STR(pgcluster_make), __LINE__, strerror(ENOMEM));
	SE_check_nullptr((cluster->items = (struct SE_PGITEM **)calloc(count, sizeof(struct SE_PGITEM *))), "%s(%d),%s\n", SE_2STR(pgcluster_make), __LINE__, strerror(ENOMEM));
	SE_check_nullptr((sql = (char *)malloc(256)), "%s(%d),%s\n", SE_2STR(pgcluster_make), __LINE__, strerror(ENOMEM));
	sql[0] = '\0';
	cluster->count = count;
	for (i = 0; i < cluster->count; ++i) {
		SE_check_nullptr((cluster->items[i] = (struct SE_PGITEM *)calloc(1, sizeof(struct SE_PGITEM))), "%s(%d),%s\n", SE_2STR(pgcluster_make), __LINE__, strerror(ENOMEM));
		cluster->items[i]->weight = dbconfs[i]->weight;
		cluster->items[i]->current_weight = dbconfs[i]->weight;
		cluster->items[i]->id = i;
		cluster->items[i]->serverid = dbconfs[i]->serverid;
		SE_throw(SE_dbconf_conn(name, dbconfs[i], &keys, &vals));
		cluster->items[i]->use_count = 0;
		cluster->items[i]->isuse = false;
		/* 连接并检查 */
		cluster->items[i]->conn = PQconnectdbParams((const char *const*)keys, (const char *const*)vals, 1);
		if (CONNECTION_OK != PQstatus(cluster->items[i]->conn)) {
			fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(cluster->items[i]->conn));
			goto SE_ERROR_CLEAR;
		}
		/*设置sql运行超时时间*/
		snprintf(sql, 256, "set statement_timeout to %d;", dbconfs[i]->statement_timeout * 1000);
		result = PQexec(cluster->items[i]->conn, sql);
		if (PGRES_COMMAND_OK != PQresultStatus(result)) {
			fprintf(stderr, "set statement_timeout failed: %s\n", PQerrorMessage(cluster->items[i]->conn));
			goto SE_ERROR_CLEAR;
		}
		sql[0] = '\0';
		SE_PQclear(result);
		/*获取当前数据库时区*/
		SE_throw(SE_get_pg_timezone(cluster->items[i]->conn, &cluster->items[i]->time_zone));
		SE_dbconf_conn_free(&keys, &vals);
	}
	SE_free(&sql);
	SE_PQclear(result);
	SE_dbconf_conn_free(&keys, &vals);
	(*ptr) = cluster;
	return true;
SE_ERROR_CLEAR:
	SE_free(&sql);
	SE_PQclear(result);
	SE_dbconf_conn_free(&keys, &vals);
	SE_pgcluste_free(&cluster);
	return false;
}

static bool SE_dbconf_copy(const struct SE_CONF * const conf, bool iswriter, struct SE_DBCONF ***ptr, int32_t *arry_count) {
	struct SE_DBCONF **dbconfs = NULL;
	const struct SE_DBCONFS *dbcluster;
	int32_t i = 0;
	int32_t index = 0, all_conn = 0;
	//iswriter在配置文件中的索引位置数组
	int32_t  *current_servers = NULL;
	//数组大小
	int32_t current_servers_count = 0, current_servers_index = 0;

	SE_validate_para_ptr(conf, SE_2STR(dbconf_copy), __LINE__);
	SE_validate_para_ptr(arry_count, SE_2STR(dbconf_copy), __LINE__);

	dbcluster = conf->dbs;
	//计算所有连接数
	for (i = 0; i < dbcluster->count; ++i) {
		if (iswriter == dbcluster->item[i]->iswrite) {
			all_conn += dbcluster->item[i]->max_connection;
			++current_servers_count;
		}
	}
	//获取当前服务器在配置文件中的索引
	SE_check_nullptr((current_servers = (int32_t *)calloc(current_servers_count, sizeof(int32_t))), "%s(%d),%s\n", SE_2STR(dbconf_copy), __LINE__, strerror(ENOMEM));
	index = 0;
	for (i = 0; i < dbcluster->count; ++i) {
		if (iswriter == dbcluster->item[i]->iswrite) {
			current_servers[index] = i;
			++index;
		}
	}
	index = 0;
	//为所有连接生成配置信息
	SE_check_nullptr((dbconfs = (struct SE_DBCONF **)calloc(all_conn, sizeof(struct SE_DBCONF *))), "%s(%d),%s\n", SE_2STR(dbconf_copy), __LINE__, strerror(ENOMEM));
	for (i = 0; i < all_conn; ++i) {
		current_servers_index = current_servers[(i % current_servers_count)];
		SE_check_nullptr((dbconfs[index] = (struct SE_DBCONF *)calloc(1, sizeof(struct SE_DBCONF))), "%s(%d),%s\n", SE_2STR(dbconf_copy), __LINE__, strerror(ENOMEM));
		SE_string_copy(dbcluster->item[current_servers_index]->server, &dbconfs[index]->server);
		dbconfs[index]->port = dbcluster->item[current_servers_index]->port;
		dbconfs[index]->serverid = current_servers_index;
		SE_string_copy(dbcluster->item[current_servers_index]->dbname, &dbconfs[index]->dbname);
		SE_string_copy(dbcluster->item[current_servers_index]->user, &dbconfs[index]->user);
		SE_string_copy(dbcluster->item[current_servers_index]->password, &dbconfs[index]->password);
		SE_string_copy(dbcluster->item[current_servers_index]->client_encoding, &dbconfs[index]->client_encoding);
		dbconfs[index]->connect_timeout = dbcluster->item[current_servers_index]->connect_timeout;
		dbconfs[index]->statement_timeout = dbcluster->item[current_servers_index]->statement_timeout;
		dbconfs[index]->max_connection = dbcluster->item[current_servers_index]->max_connection;
		dbconfs[index]->weight = dbcluster->item[current_servers_index]->weight;
		/*fprintf(stdout, "%s,%d,%d,%s,%s,%s,%s,%d,%d,%d,%d\n",
		dbconfs[index]->server->val, dbconfs[index]->port, dbconfs[index]->serverid, dbconfs[index]->dbname->val, dbconfs[index]->user->val, dbconfs[index]->password->val,
		dbconfs[index]->client_encoding->val, dbconfs[index]->connect_timeout, dbconfs[index]->statement_timeout, dbconfs[index]->max_connection, dbconfs[index]->weight);*/
		++index;
	}
	SE_free((char  **)&current_servers);
	(*ptr) = dbconfs;
	(*arry_count) = all_conn;
	return true;
SE_ERROR_CLEAR:
	SE_free((char  **)&current_servers);
	SE_dbconfs_free(&dbconfs, all_conn);
	return false;
}

static bool SE_connect_pgcluster(const struct SE_CONF * const conf, struct SE_SHARED *shared) {
	struct SE_PGCLUSTER *wcluster = NULL, *rcluster = NULL;
	struct SE_DBCONF **wdbconf = NULL, **rdbconf = NULL;
	int32_t wcount = 0, rcount = 0;

	SE_validate_para_ptr(conf, SE_2STR(connect_pgcluster), __LINE__);
	SE_validate_para_ptr(shared, SE_2STR(connect_pgcluster), __LINE__);

	SE_throw(SE_dbconf_copy(conf, true, &wdbconf, &wcount));
	SE_throw(SE_dbconf_copy(conf, false, &rdbconf, &rcount));

	//没有设置只写服务器,必须至少设置一台只写服务器
	if (wcount < 1) {
		fprintf(stderr, "%s(%d),%s\n", SE_2STR(SE_connect_pgcluster), __LINE__, "Contains at least one write-only database");
		goto SE_ERROR_CLEAR;
	}
	SE_throw(SE_pgcluster_make(conf->name, (const struct SE_DBCONF *const*)wdbconf, wcount, &wcluster));
	if (rcount > 0)
		SE_throw(SE_pgcluster_make(conf->name, (const struct SE_DBCONF *const*)rdbconf, rcount, &rcluster));


	SE_dbconfs_free(&wdbconf, wcount);
	SE_dbconfs_free(&rdbconf, rcount);
	shared->rcluster = rcluster;
	shared->wcluster = wcluster;
	return true;
SE_ERROR_CLEAR:
	SE_dbconfs_free(&wdbconf, wcount);
	SE_dbconfs_free(&rdbconf, rcount);
	SE_pgcluste_free(&rcluster);
	SE_pgcluste_free(&wcluster);
	return false;
}

static bool SE_tsoap_make(const struct soap *master, void *process, int32_t pool, struct soap ***ptr) {
	struct soap **slaves = NULL;
	struct SE_WORKINFO *obj;
	int32_t i = 0;

	SE_check_nullptr((slaves = (struct soap **)calloc(pool, sizeof(struct soap *))), "%s(%d),%s\n", SE_2STR(SE_tsoap_make), __LINE__, strerror(ENOMEM));
	for (i = 0; i < pool; ++i) {
		SE_check_nullptr((slaves[i] = soap_copy(master)), "%s(%d),%s\n", SE_2STR(SE_tsoap_make), __LINE__, strerror(ENOMEM));
		SE_check_nullptr((slaves[i]->user = calloc(1, sizeof(struct SE_WORKINFO))), "%s(%d),%s\n", SE_2STR(SE_tsoap_make), __LINE__, strerror(ENOMEM));
		obj = (struct SE_WORKINFO *)slaves[i]->user;
		obj->id = i;
		obj->process = process;
		obj->sys = makeStringBuilder(8192);
		obj->sql = makeStringBuilder(1024);
		//obj->err = makeStringBuilder(1024);
		obj->buf = makeStringBuilder(4096);
	}
	(*ptr) = slaves;
	return true;
SE_ERROR_CLEAR:
	SE_tsoap_free(pool, &slaves);
	(*ptr) = NULL;
	return false;
}

static bool SE_workthred_make(struct SE_SHARED *shared, struct soap **slaves, int32_t pool, pthread_t ***ptr) {
	pthread_t **works = NULL;
	int32_t i = 0, rc = 0;
	//初始化
	SE_check_nullptr((works = (pthread_t **)calloc(pool, sizeof(pthread_t *))), "%s(%d),%s", SE_2STR(SE_workthred_make), __LINE__, strerror(ENOMEM));
	for (i = 0; i < pool; ++i) {
		SE_check_nullptr((works[i] = (pthread_t *)calloc(1, sizeof(pthread_t))), "%s(%d),%s", SE_2STR(SE_workthred_make), __LINE__, strerror(ENOMEM));
		SE_check_rc((rc = pthread_create(works[i], NULL, (void*(*)(void*))process_queue, (void*)(slaves[i]))), "%s(%d),%s", SE_2STR(SE_workthred_make), __LINE__, "fail");
	}
	(*ptr) = works;
	return true;
SE_ERROR_CLEAR:
	SE_workthred_free(shared, pool, &works);
	return false;
}


static void SE_soap_serve(struct soap *tsoap) {
	struct value *request = NULL/*, *response = NULL*/;
	instr_time before, after;
	double elapsed_msec = 0;
	struct SE_WORKINFO *work_info;

	if (SE_PTR_ISNULL(tsoap) || SE_PTR_ISNULL(tsoap->user))
		return;
	work_info = (struct SE_WORKINFO *)tsoap->user;

	SE_send_malloc_fail((request = new_value(tsoap)), work_info, "soap_serve");
	//SE_send_malloc_fail((response = new_value(tsoap)), work_info, "soap_serve");
	//接收数据
	if (soap_begin_recv(tsoap) || json_recv(tsoap, request) || soap_end_recv(tsoap)) {
		SE_soap_print_fault(tsoap);
	} else {
		//开始计时	
		INSTR_TIME_SET_CURRENT(before);
		//输出设置_应答状态
		resetStringBuilder(work_info->sys);
		appendStringBuilder(work_info->sys, "{\"status\":200,");
		if (NULL != work_info->process) {
			work_info->tsoap = tsoap;
			work_info->request = request;
			//work_info->response = response;
			//处理请求	
			work_info->process(work_info);
			work_info->tsoap = NULL;
			work_info->request = NULL;
			//这里的pg已经和pg对象池分离了,所以操作不需要加锁
			if (!SE_PTR_ISNULL(work_info->pg)) {
				work_info->pg->isuse = false;
				work_info->pg = NULL;
			}
		}
		//停止计时
		INSTR_TIME_SET_CURRENT(after);
		INSTR_TIME_SUBTRACT(after, before);
		elapsed_msec += INSTR_TIME_GET_MILLISEC(after);
		//发送应答数据
		appendStringBuilder(work_info->sys, "%s\"elapsed\": %.3lf}",
			(',' == work_info->sys->data[work_info->sys->len - 1]) ? "" : ",",
			elapsed_msec);
		//#ifdef _DEBUG
		//		FILE *file = fopen("E:/aaa.json", PG_BINARY_W);
		//		if (!SE_PTR_ISNULL(file)) {
		//			fwrite(work_info->sys->data, work_info->sys->len, 1, file);
		//			fclose(file);
		//		}
		//#endif
		tsoap->http_content = "application/json; charset=utf-8";
		if (soap_response(tsoap, SOAP_FILE) ||
			soap_send_raw(tsoap, work_info->sys->data, work_info->sys->len) ||
			soap_end_send(tsoap))
			soap_send_fault(tsoap);
	}
	return;
SE_ERROR_CLEAR:
	return;
}

static void *SE_isten(void *process) {
	struct SE_SHARED *shared = global_shared;
	struct soap *master = NULL;
	struct soap **slaves = NULL;
	pthread_t **works = NULL;
	SOAP_SOCKET m, s;
	int32_t port = 0, pool = 0;
	bool isrun = true;

	SE_validate_para_ptr(process, SE_2STR(SE_isten), __LINE__);
	SE_validate_para_ptr(shared, SE_2STR(SE_isten), __LINE__);

	//创建侦听soap
	SE_check_nullptr((master = soap_new1(SOAP_C_UTFSTRING)), "%s(%d),%s", SE_2STR(soap_new1), __LINE__, "Failed to create gSOAP object");
	//获取配置信息
	pthread_mutex_lock(shared->locks->mutex);
	master->cors_origin = NULL; //不允许跨域访问
	master->cors_allow = NULL;
	master->double_format = shared->conf->double_format;
	master->float_format = shared->conf->double_format;
	master->long_double_format = shared->conf->double_format;
	master->send_timeout = shared->conf->send_timeout;
	master->recv_timeout = shared->conf->recv_timeout;
	port = shared->conf->port;
	pool = shared->conf->pool;
	isrun = shared->isrun;
	pthread_mutex_unlock(shared->locks->mutex);
	//创建工作soap/工作线程	
	SE_throw(SE_tsoap_make(master, process, pool, &slaves));
	SE_throw(SE_workthred_make(shared, slaves, pool, &works));
	//绑定侦听端口
	m = soap_bind(master, NULL, port, shared->conf->backlog);
	if (!soap_valid_socket(m)) {
		SE_soap_print_fault(master);
		shared->isrun = false;
		goto SE_ERROR_CLEAR;
	}
	//开始侦听
	while (isrun) {
		s = soap_accept(master);
		pthread_mutex_lock(shared->locks->mutex);
		isrun = shared->isrun;
		pthread_mutex_unlock(shared->locks->mutex);
		//服务停止
		if (!isrun) break;
		if (!soap_valid_socket(s)) {
			if (master->errnum) {
				SE_soap_print_fault(master);
				SE_soap_clear(master);
				continue; // retry
			} else {
				SE_soap_clear(master);
				fprintf(stderr, "Server timed out");
				break;
			}
		}
		//将新连接的socket添加到队列
		while (enqueue(s, shared) == SOAP_EOM)
			SE_usleep(SLEEP_MILLISEC(1));

		pthread_mutex_lock(shared->locks->mutex);
		isrun = shared->isrun;
		pthread_mutex_unlock(shared->locks->mutex);
	}
	//停止服务,等待工作线程完成
	SE_workthred_free(shared, pool, &works);
	SE_tsoap_free(pool, &slaves);
	SE_soap_free(master);
	pthread_exit(0);
	return NULL;
SE_ERROR_CLEAR:
	SE_workthred_free(shared, pool, &works);
	SE_tsoap_free(pool, &slaves);
	SE_soap_free(master);
	pthread_exit(0);
	return NULL;
}

bool SEAPI SE_start(const struct SE_CONF * const conf, SE_process process) {
	struct SE_SHARED *shared = NULL;
	int32_t rc = 0;

	SE_validate_para_ptr(conf, SE_2STR(SE_start), __LINE__);
	SE_validate_para_ptr(process, SE_2STR(SE_start), __LINE__);
	//初始化
	SE_check_nullptr((shared = (struct SE_SHARED *)calloc(1, sizeof(struct SE_SHARED))), "%s(%d),%s\n", SE_2STR(SE_start), __LINE__, strerror(ENOMEM));
	SE_check_nullptr((shared->queue = (struct SE_QUEUE *)calloc(1, sizeof(struct SE_QUEUE))), "%s(%d),%s\n", SE_2STR(SE_start), __LINE__, strerror(ENOMEM));
	SE_check_nullptr((shared->queue->queues = (SOAP_SOCKET *)calloc(conf->queue_max, sizeof(SOAP_SOCKET))), "%s(%d),%s\n", SE_2STR(SE_start), __LINE__, strerror(ENOMEM));
	SE_check_nullptr((shared->locks = (struct SE_SHARED_LOCK *)calloc(1, sizeof(struct SE_SHARED_LOCK))), "%s(%d),%s\n", SE_2STR(SE_start), __LINE__, strerror(ENOMEM));
	//创建互斥对象
	SE_check_nullptr((shared->locks->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t))), "%s(%d),%s\n", SE_2STR(SE_start), __LINE__, strerror(ENOMEM));
	SE_check_rc((rc = pthread_mutex_init(shared->locks->mutex, NULL)), "%s(%d),%s\n", SE_2STR(pthread_mutex_init), __LINE__, "fail");
	shared->lock_flag += SE_THREAD_MUTEX;
	//创建条件变量
	SE_check_nullptr((shared->locks->cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t))), "%s(%d),%s\n", SE_2STR(SE_start), __LINE__, strerror(ENOMEM));
	SE_check_rc((rc = pthread_cond_init(shared->locks->cond, NULL)), "%s(%d),%s\n", SE_2STR(pthread_cond_init), __LINE__, "fail");
	shared->lock_flag += SE_THREAD_COND;
	//创建数据库连接对象
	SE_throw(SE_connect_pgcluster(conf, shared));
	//设置相关的内容
	shared->isrun = true;
	shared->conf = conf;
	//启动线程
	SE_check_rc((rc = pthread_create(&shared->tctrl, NULL, (void*(*)(void*))SE_isten, process)), "%s(%d),%s\n", SE_2STR(pthread_create), __LINE__, "fail");
	//设置全局变量和输出参数值
	global_shared = shared;
	return true;
SE_ERROR_CLEAR:
	SE_shared_free(&shared);
	global_shared = NULL;
	return false;
}

void SEAPI SE_stop() {
	struct SE_SHARED *shared = global_shared;
	struct soap *ctx = NULL;
	struct value *request, *response;
	static char url[32];
	int32_t count = 0;

	if (SE_PTR_ISNULL(shared) || SE_PTR_ISNULL(shared->locks) ||
		SE_PTR_ISNULL(shared->locks->mutex) || !SE_NUMBIT_ISONE(shared->lock_flag, 0) ||
		SE_PTR_ISNULL(shared->locks->cond) || !SE_NUMBIT_ISONE(shared->lock_flag, 1)) {
		SE_shared_free(&global_shared);
		return;
	}
	//设置状态为停止
	pthread_mutex_lock(shared->locks->mutex);
	shared->isrun = false;
	pthread_mutex_unlock(shared->locks->mutex);
	//关闭侦听线程
	do {
		++count;
		ctx = soap_new1(SOAP_C_UTFSTRING/*| SOAP_XML_INDENT*/);
		if (NULL != ctx) {
			ctx->connect_timeout = -100 * 1000; //收发超时为100毫秒
			ctx->send_timeout = -100 * 1000; //收发超时为100毫秒
			ctx->recv_timeout = -100 * 1000;
			request = new_value(ctx);
			response = new_value(ctx);
			snprintf(url, 32, "http://127.0.0.1:%d", shared->conf->port);
			json_call(ctx, url, request, response);
			SE_soap_free(ctx);
			break;
		}
	} while (count < 3);
	if (3 == count) {				//多次尝试内存溢出
		SE_shared_free(&global_shared);
		return;
	}
	//发送信号,关闭所有工作线程
	pthread_mutex_lock(shared->locks->mutex);
	pthread_cond_broadcast(shared->locks->cond);
	pthread_mutex_unlock(shared->locks->mutex);
	//使工作线程有机会执行
	SE_usleep(SLEEP_MILLISEC(100));
	//等待控制线程结束
	pthread_join(shared->tctrl, NULL);
	//释放共享对象
	SE_shared_free(&global_shared);
}

struct SE_SHARED_LOCK * SEAPI SE_shared_lock() {
	return (SE_PTR_ISNULL(global_shared) || SE_PTR_ISNULL(global_shared->locks) ? NULL : global_shared->locks);
}


static struct SE_PGITEM * get_next_dbserver_index(struct SE_SHARED *shared, struct SE_WORKINFO *arg, bool iswrite, int32_t *current_index) {
	struct SE_PGCLUSTER *pgcluster;
	int32_t  index = 0, total = 0;
	int32_t i = 0;
	//只写服务器必须设置,只读取服务器可以不设置
	pgcluster = (iswrite ? shared->wcluster : shared->rcluster);
	pgcluster = (SE_PTR_ISNULL(pgcluster) ? shared->wcluster : pgcluster);
	for (i = 0; i < pgcluster->count; ++i) {
		pgcluster->items[i]->current_weight += pgcluster->items[i]->weight;
		total += pgcluster->items[i]->weight;
		if (-1 == index || pgcluster->items[index]->current_weight < pgcluster->items[i]->current_weight)
			index = i;
	}
	pgcluster->items[index]->current_weight -= total;
	*current_index = index;
	return pgcluster->items[index];
}

struct SE_PGITEM * SEAPI get_dbserver(struct SE_WORKINFO *arg, bool iswrite) {
	struct SE_PGITEM *pg;
	int32_t current_index;
	int32_t database_max_use_count;

	if (SE_PTR_ISNULL(global_shared) ||
		SE_PTR_ISNULL(global_shared->locks->mutex) || !SE_NUMBIT_ISONE(global_shared->lock_flag, 0) ||
		SE_PTR_ISNULL(global_shared->wcluster) || SE_PTR_ISNULL(arg))
		return NULL;

	pthread_mutex_lock(global_shared->locks->mutex);
	do {
		pg = get_next_dbserver_index(global_shared, arg, iswrite, &current_index);
		//检查连接是否在使用中...
		if (pg->isuse) {
			/*
			*	这里不能用pthread_cond_timedwait
			*	可能会受到新连接时发送的信号影响
			*	因此只能用sleep
			*/
			/*struct timespec to;
			to.tv_sec = time(NULL) + TIMEOUT;
			to.tv_nsec = 0;
			pthread_cond_timedwait(&c, &m, &to);*/
			SE_usleep(SLEEP_MILLISEC(2));
		} else {
			//标记为使用中...
			pg->isuse = true;
			break;
		}
	} while (1);
	database_max_use_count = global_shared->conf->database_max_use_count;
	pthread_mutex_unlock(global_shared->locks->mutex);
	/*
	*	从现在开始pg对象在共享区中已为使用中...
	*	其它线程不能获取这个对象
	*	因此以后操作这个pg对象无需加锁
	*/
	++pg->use_count;
	/*
	*	连接使用次数达到配置的值时,重新连接数据库
	*	有些插件pmalloc分配的内存如没手动释放,只能等到连接关闭时释放
	*	为防止质量不高的插件,因此定期关闭数据后重新连接
	*/
	if (database_max_use_count > 0 && pg->use_count > database_max_use_count) {
		PQreset(pg->conn);
		pg->use_count = 0;
	}
	arg->pg = pg;
	return arg->pg;
}

const struct SE_CONF * SEAPI get_conf() {
	if (SE_PTR_ISNULL(global_shared) || SE_PTR_ISNULL(global_shared->conf))
		return NULL;
	else
		return global_shared->conf;
}

const char * SEAPI get_jwt_key() {
	return JWT_KEY;
}