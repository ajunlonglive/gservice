/*************************************************************************************
*	加载并解析配置文件并创建配置信息API
*	1.首先检查节点是否为NULL
*	2.检查节点的类型是否符合预期
*	3.节点值检查
*			字符串类型检查是否为NULL或""
*			数字类型检查是否超出范围
*************************************************************************************/
#ifndef SE_5F6CB59C_78DA_4774_A9DB_967232588026
#define SE_5F6CB59C_78DA_4774_A9DB_967232588026

#include "se_std.h"

/*数据库配置信息*/
struct SE_DBCONF {
	char *server;
	int32_t port;
	int32_t serverid;
	char *dbname;
	char *user;
	char *password;
	char *client_encoding;
	int32_t connect_timeout;
	int32_t statement_timeout;
	int32_t max_connection;
	int32_t weight;	
	bool iswrite;
};

/*数据库集群配置信息*/
struct SE_DBCONFS {
	//连接数量
	int32_t count;
	//连接信息数组
	struct SE_DBCONF **item;
};

/*系统配置信息*/
struct SE_CONF {
	//应用程序名称
	char *name;
	//侦听的端口号
	int32_t port;
	//工作队列大小
	int32_t pool;
	//socket lister backlog
	int32_t backlog;
	//socket请求队列大小
	int32_t queue_max;
	//接收数据超时时间
	int32_t send_timeout;
	//发送数据超时时间
	int32_t recv_timeout;
	//输出浮点数据时小数位数
	char *double_format;
	//Token保持时间,单位为分钟
	int32_t token_keep;
	//数据库连接的最大重用次数(100-10000)
	int32_t database_max_use_count;
	//重定向时的url
	char *redirect_url;
	//数据库配置信息
	struct SE_DBCONFS *dbs;
};


#ifdef __cplusplus
extern "C" {
#endif	/*__cplusplus 1*/

	/*************************************************************************************
	*	根据配置文件创建SE_CONF结构体
	*	Parameter:
	*		[in,out] struct SE_CONF * * ptr - SE_CONF结构体
	*	Returns:
	*	Remarks:,使用完成后调用SE_conf_free释放
	*************************************************************************************/
	extern bool SEAPI SE_conf_create(struct SE_CONF **ptr);


	/*************************************************************************************
	*	释放SE_conf_create创建的SE_CONF结构体
	*	Parameter:
	*		[out] struct SE_CONF * * ptr - SE_CONF结构体
	*************************************************************************************/
	extern void SEAPI SE_conf_free(struct SE_CONF **ptr);

	/*************************************************************************************
	*	生成PQconnectdbParams的参数,使用完成后用SE_dbconf_conn_free
	*	Parameter:
	*		[in] const char * const appname - 应用程序名称
	*		[in] const SE_DBCONF * dbconf - 数据库链接信息
	*		[out] char * * * keys - 数据库链接关键字
	*		[out] char * * * vals - 数据库链接值
	*	Remake:使用完成后调用SE_dbconf_conn_free释放资源
	*************************************************************************************/
	extern bool SEAPI SE_dbconf_conn(const char * const appname,
		const struct SE_DBCONF * const dbconf, char ***keys, char ***vals);

	/*************************************************************************************
	*	生成PQpingParams的参数,使用完成后用SE_dbconf_conn_free
	*	Parameter:
	*		[in] const struct SE_DBCONF * const dbconf -
	*		[in] char * * * keys -
	*		[in] char * * * vals -
	*	Remarks:使用完成后调用SE_dbconf_conn_free释放资源
	*	相较PQconnectdbParams,PQpingParams只需要主机名和端口
	*************************************************************************************/
	extern bool SEAPI SE_dbconf_ping(const struct SE_DBCONF * const dbconf, char ***keys, char ***vals);

	/*************************************************************************************
	*	Remarks:释放数据库连接信息键值对数组
	*************************************************************************************/
	extern void SEAPI SE_dbconf_conn_free(char ***keys, char ***vals);


#ifdef __cplusplus
};
#endif  /*__cplusplus 2*/

#endif /*SE_5F6CB59C_78DA_4774_A9DB_967232588026*/