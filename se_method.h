/*************************************************************************************
*	WEBAPI 接口方法crc32代码
*	直接根据请求的方法字符串比较执行那个方法效率比较差
*	因此将请求的方法字符串转换为crc32后使用整数来确定执行的方法
*	WEBAPI 接口方法实际名称为去除SEM_并转换为小写
*	SEM_HELLO_WORLD客户端调用进为hello_world
*************************************************************************************/
#ifndef SE_BF460C55_F8B3_4DF3_8C20_36EB3DE4D22A
#define SE_BF460C55_F8B3_4DF3_8C20_36EB3DE4D22A

#ifndef  SE_COLLAPSED

/*测试方法*/
#define SEM_TEST_MSG										(301433704)
#define SEM_TEST_ERROR										(1443043962)
#define SEM_TEST_REDIRECT								(1450153341)
#define SEM_HELLO_WORLD								(4148080273)
#define SEM_TEST_SQLTYPE									(2911571229)
#define SEM_TEST_SQLARRAY								(2157530968)
#define SEM_TEST_INSERT									(1377147355)
#define SEM_TEST_ARRAY_INSERT						(3921805819)
#define SEM_TEST_JSON										(1853755274)
#define SEM_TEST_JSON_INSERT							(2094309505)
#define SEM_USER_LOGIN									(1221210184)
#define SEM_GRAB_PRIZES									(2858522760)
#endif /*代码折叠宏*/

#endif /*SE_BF460C55_F8B3_4DF3_8C20_36EB3DE4D22A*/