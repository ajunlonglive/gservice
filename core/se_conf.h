/*************************************************************************************
*	���ز����������ļ�������������ϢAPI
*	1.���ȼ��ڵ��Ƿ�ΪNULL
*	2.���ڵ�������Ƿ����Ԥ��
*	3.�ڵ�ֵ���
*			�ַ������ͼ���Ƿ�ΪNULL��""
*			�������ͼ���Ƿ񳬳���Χ
*************************************************************************************/
#ifndef SE_5F6CB59C_78DA_4774_A9DB_967232588026
#define SE_5F6CB59C_78DA_4774_A9DB_967232588026

#include "se_std.h"

/*���ݿ�������Ϣ*/
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

/*���ݿ⼯Ⱥ������Ϣ*/
struct SE_DBCONFS {
	//��������
	int32_t count;
	//������Ϣ����
	struct SE_DBCONF **item;
};

/*ϵͳ������Ϣ*/
struct SE_CONF {
	//Ӧ�ó�������
	char *name;
	//�����Ķ˿ں�
	int32_t port;
	//�������д�С
	int32_t pool;
	//socket lister backlog
	int32_t backlog;
	//socket������д�С
	int32_t queue_max;
	//�������ݳ�ʱʱ��
	int32_t send_timeout;
	//�������ݳ�ʱʱ��
	int32_t recv_timeout;
	//�����������ʱС��λ��
	char *double_format;
	//Token����ʱ��,��λΪ����
	int32_t token_keep;
	//���ݿ����ӵ�������ô���(100-10000)
	int32_t database_max_use_count;
	//�ض���ʱ��url
	char *redirect_url;
	//���ݿ�������Ϣ
	struct SE_DBCONFS *dbs;
};


#ifdef __cplusplus
extern "C" {
#endif	/*__cplusplus 1*/

	/*************************************************************************************
	*	���������ļ�����SE_CONF�ṹ��
	*	Parameter:
	*		[in,out] struct SE_CONF * * ptr - SE_CONF�ṹ��
	*	Returns:
	*	Remarks:,ʹ����ɺ����SE_conf_free�ͷ�
	*************************************************************************************/
	extern bool SEAPI SE_conf_create(struct SE_CONF **ptr);


	/*************************************************************************************
	*	�ͷ�SE_conf_create������SE_CONF�ṹ��
	*	Parameter:
	*		[out] struct SE_CONF * * ptr - SE_CONF�ṹ��
	*************************************************************************************/
	extern void SEAPI SE_conf_free(struct SE_CONF **ptr);

	/*************************************************************************************
	*	����PQconnectdbParams�Ĳ���,ʹ����ɺ���SE_dbconf_conn_free
	*	Parameter:
	*		[in] const char * const appname - Ӧ�ó�������
	*		[in] const SE_DBCONF * dbconf - ���ݿ�������Ϣ
	*		[out] char * * * keys - ���ݿ����ӹؼ���
	*		[out] char * * * vals - ���ݿ�����ֵ
	*	Remake:ʹ����ɺ����SE_dbconf_conn_free�ͷ���Դ
	*************************************************************************************/
	extern bool SEAPI SE_dbconf_conn(const char * const appname,
		const struct SE_DBCONF * const dbconf, char ***keys, char ***vals);

	/*************************************************************************************
	*	����PQpingParams�Ĳ���,ʹ����ɺ���SE_dbconf_conn_free
	*	Parameter:
	*		[in] const struct SE_DBCONF * const dbconf -
	*		[in] char * * * keys -
	*		[in] char * * * vals -
	*	Remarks:ʹ����ɺ����SE_dbconf_conn_free�ͷ���Դ
	*	���PQconnectdbParams,PQpingParamsֻ��Ҫ�������Ͷ˿�
	*************************************************************************************/
	extern bool SEAPI SE_dbconf_ping(const struct SE_DBCONF * const dbconf, char ***keys, char ***vals);

	/*************************************************************************************
	*	Remarks:�ͷ����ݿ�������Ϣ��ֵ������
	*************************************************************************************/
	extern void SEAPI SE_dbconf_conn_free(char ***keys, char ***vals);


#ifdef __cplusplus
};
#endif  /*__cplusplus 2*/

#endif /*SE_5F6CB59C_78DA_4774_A9DB_967232588026*/