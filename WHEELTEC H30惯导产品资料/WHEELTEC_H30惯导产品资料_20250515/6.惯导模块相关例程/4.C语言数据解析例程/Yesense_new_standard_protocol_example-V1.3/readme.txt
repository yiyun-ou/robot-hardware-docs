1、使用__USE_STATIC_METHOD宏定义则解析已知报文

2、使用__USE_COM_PORT宏定义则是在windows实时解析串口报文
实时解析报文时，在InitComPort中sprintf(com, "%s%s", "\\\\.\\", "COM25")根据实际串口修改