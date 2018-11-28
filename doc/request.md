

# 1 为什么请求要用JSON格式
json相对字符串参数来说格式更规范.json标准规定的数据值类型共以下几种:<br />
- string
- number
- object
- array
- true
- false
- null

## 1.1 string
- ISO 8601格式的dateTime值<br />
- base64编码<br />
- 普通文本<br />

## 1.2 number
- int 对于json来说,不区分int16/int32/int64,全部为int64.<br />
- double 对于json来说,不区分float4/float8,全部为float8.<br />


数据规范后,可减少开发过程中失误,容易发现一下隐藏的bug,因此选择用json做为请求格式.

## 1.3 其它
- gosap服务需要占用一个端口,通常情况下不是80或443.但大多数网络环境只开放80或443;<br />
- 项目不仅需要获取数据,还需要访问其它资源,如html/css/javascript等,因此一般配合其它http服务反向代理使用(如http), 较长的url不利于其它http服务设置反向代理<br />
- 不管内网有多少种资源,对外提供服务的端口只有一个,80或443,然后通过url中的第二部分来确定访问那个资源
- 有利于http服务的负载均衡设置.可配置多台gsoap服务器,然后通过http服务的负载均衡来调度
- 证书.只需要在http服务上配置证书即可


例如:gosap服务为本机的7654端口,只要设置反向代理为即可.
```bash
/gsoap http://127.0.0.1:7654
```
客户端访问
```bash
http://www.xxx.xxx/gosap
#或
https://www.xxx.xxx/gosap
#/gosap确定访问的是http://127.0.0.1:7654这台内网主机
```


# 2 请求结构体
目前只支持POST一种HTTP方法,GET/DELETE/PUT等方法待以后支持.请求结构体最少情况只需要一个参数"method"
```json
{
  "method":"请求的方法名称",
  "token":"如果需要验证还可以加上token",
  请求的方法所需的其它参数
}
```
