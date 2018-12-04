# 内存和异常
服务器内存和异常分为二部份,分界线为SE_start函数

## 调用SE_start之前
- 使用标准C的malloc,calloc,realloc,free对内存进行管理.分配资源的函数都有配对的释放资源的函数.<br />
- 在SE_start之前分配的内存,只要你正确使用,程序保证完全释放.<br />
- 如发生异常,异常信息将打印到stderr.你可以重定向stderr保存启动前的异常信息.<br />

## SE_start成功之后
- **使用gsoap自带的内存管理,如soap_malloc等,无需释放内存.**<br />
- **你在开发自己的程序时也应该使用gsoap自带的内存管理函数.**<br />
- **如果发生内存泄露,请检查你使用soap_malloc分配的内存使用时是否越界.如发生越界行为,关闭服务器时,整个soap分配的内存将无法释放,会出现值比较大的内存泄露.**<br />
- **如发生异常,异常信息将发送至请求的客户端.**

# gsoap内存管理
gsoap分配的内存在内部有一个链表维护,在调用soap_destroy时会释放所有手动分配的内存,因此你无需释放内存,只需要检查soap_malloc成功与否就可以了.
```c
/*************************************************************************************
* 内存分配函数
*/
//分配指定大小的内存
void * soap_malloc(struct soap *soap, size_t n)
//复制字符串
char * soap_strdup(struct soap *soap, const char *s)
//复制宽字节字符串
char * soap_wstrdup(struct soap *soap, const wchar_t *s)
//创建一个soap对象,并使用默认值初始化.仅C++,C不适用
T * soap_new_T(struct soap *soap)
//创建指定大小soap对象数组,并使用默认值初始化.n=-1时只创建一个soap对象,仅C++,C不适用
T * soap_new_T(struct soap *soap, int n)
//其它不常用就不写了
/*************************************************************************************/


/*************************************************************************************
* 释放资源函数
*/
//删除所有上下文管理的对象,实际上就是删除soap_malloc分配的内存资源.必须在soap_end之前调用,适用于C和C++
void soap_destroy(struct soap *soap)
//清理反序列化的数据和临时数据(不包含上下文管理的对象),适用于C和C++
void soap_end(struct soap *soap)
//删除临时数据但保持反序列化的数据不变,适用于C和C++,不常用
void soap_free_temp(struct soap *soap)
//提前释放你用内存分配函数(如soap_malloc)分配的内存,适用于C和C++.你不释放也没关系,调用soap_destroy时也会释放
void soap_dealloc(struct soap *soap, void *p)
//从gsoap上下文管理对象断开p对象,此时p对象必须由你手动调用free释放,适用于C和C++.
int soap_unlink(struct soap *soap, const void *p)
//完成上下文, 但不删除任何托管对象或数据
void soap_done(struct soap *soap)
//最后确定并释放上下文 (使用 soap _ new 或 soap _ copy 分配的上下文), 但不删除任何托管对象或数据。
void soap_free(struct soap *soap)
/*************************************************************************************/
```
释放内存正确的姿势
```c
//soap需要复用时的清理方法
#define SE_soap_clear(soap_) do {\
	if(!SE_PTR_ISNULL(soap_)){\
		soap_destroy(soap_);\
		soap_end(soap_);\
	}\
} while (0);
//完全释放soap
#define SE_soap_free(soap_)\
 do {\
	if(NULL !=soap_) { \
		soap_destroy(soap_);\
		soap_end(soap_);\
		soap_done(soap_);\
		soap_free(soap_);\
		soap_= NULL; \
	} \
} while (0)

struct soap *ctx = NULL;
char *ptr = NULL;

ctx = soap_new1(SOAP_C_UTFSTRING);
//提前释放ptr
ptr = soap_malloc(ctx, 100);
if(NULL != ptr){
  soap_dealloc(ctx, ptr);ptr = NULL;
}
//分离ptr
ptr = soap_malloc(ctx, 100);
if(NULL != ptr){
  soap_unlink(ctx, ptr);
  //此时需要手动释放ptr
  free(ptr);ptr=NULL;
}
//调用soap_destroy时释放
ptr = soap_malloc(ctx, 100);
//如果你的ctx需要复用,调用
SE_soap_clear(ctx)
//否则调用
SE_soap_free(ctx);
```
