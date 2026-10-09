
//////////////////////////// H30数据格式定义 ///////////////////////////////
#pragma pack(1)
typedef struct{
    uint8_t dataId;
    uint8_t dataLen;
    int32_t ax;
    int32_t ay;
    int32_t az;
}AccelRawType_t;

typedef struct{
    uint8_t dataId;
    uint8_t dataLen;
    int32_t gx;
    int32_t gy;
    int32_t gz;
}GyroRawType_t;

typedef struct{
    uint8_t dataId;
    uint8_t dataLen;
    int32_t pitch;
    int32_t roll;
    int32_t yaw;
}AttitudeType_t;

typedef struct{
    uint8_t dataId;
    uint8_t dataLen;
    int32_t q0;
    int32_t q1;
    int32_t q2;
    int32_t q3;
}QuaternionType_t;

typedef struct{
    uint8_t head1;   //帧头
    uint8_t head2;
    uint16_t FrameNum; //帧号
    uint8_t packLen;   //数据总长

    AccelRawType_t accel; //数据区,新增数据时注意顺序  //14 bytes
    GyroRawType_t gyro;                           //14 bytes
    AttitudeType_t attitude;                      //14 bytes
    QuaternionType_t quaternion;                  //18 bytes

    uint8_t ck1; //数据区校验和
    uint8_t ck2;
}H30FrameType_t;
#pragma pack()
//////////////////////////// H30数据格式定义 END///////////////////////////////

//H30接收函数状态机
typedef enum {
    H30_WAITHEAD = 0,
    H30_STARTRECV,
    H30_CHECKSUM,
    H30_HANDLEDATA,
}H30Serial_Status;

//定义状态机用于处理接收函数内容
static H30Serial_Status H30_State = H30_WAITHEAD;

//H30校验和计算方式
uint16_t H30_CheckSum(uint8_t* data,uint16_t len)
{
    uint8_t ck1=0,ck2=0;
    for(uint16_t i=0;i<len;i++)
    {
        ck1 += data[i];
        ck2 += ck1;
    }
    uint16_t ck = (ck1<<8)|ck2;
    return ck;
}


H30FrameType_t H30Frame = { 0 };                     //1帧H30数据
const uint16_t H30FrameLen = sizeof(H30FrameType_t); //1帧H30数据长度
uint8_t H30FrameBuf[H30FrameLen] = { 0 };            //1帧H30数据缓存区域
uint16_t H30FrameRecvIndex = 0;                      //串口接收数据的索引值

//H30串口接收回调函数
uint8_t H30_Callback(uint8_t recv)
{
    static uint8_t lastrecv;
    uint8_t isReady = 0;
    uint16_t checksumVal = 0;
    
    switch (H30_State)
    {
      case H30_WAITHEAD:
          if( lastrecv == 0x59 && recv == 0x53 ) 
          {
              H30FrameBuf[0] = 0x59;//帧头
              H30FrameBuf[1] = 0x53;
              H30FrameRecvIndex = 2;
              H30_State = H30_STARTRECV;
          }
          break;
      case H30_STARTRECV:
          H30FrameBuf[H30FrameRecvIndex++] = recv;
          if( H30FrameRecvIndex == H30FrameLen ) 
          {
            H30_State = H30_CHECKSUM;
            }
          break;
  
      case H30_CHECKSUM:
          checksumVal = H30_CheckSum(&H30FrameBuf[2],H30FrameLen-4);//除去帧头和本身的校验位
          if( (checksumVal>>8&0xff) == H30FrameBuf[H30FrameLen-2] && (checksumVal&0xff) == H30FrameBuf[H30FrameLen-1] )
          {
              H30_State = H30_HANDLEDATA;
          }
          else 
          {
              H30_State = H30_WAITHEAD;
          }
          break;
      case H30_HANDLEDATA:
          memcpy(&H30Frame,H30FrameBuf,H30FrameLen);
          H30_State = H30_WAITHEAD;
          isReady = 1;
          break;
      default:
          break;
    }
    lastrecv = recv;
    
    return isReady;
}

void setup() {
  // put your setup code here, to run once:
 pinMode(LED_BUILTIN, OUTPUT);
 Serial.begin(115200); //usb调试口
}

bool LedStatus = false;
bool wait_for_ready = 0;

void loop() {

  //接收并解析数据
  if(Serial.available())
  {
    wait_for_ready = H30_Callback(Serial.read());
  }

  if( wait_for_ready ) //完成数据的解析接收
  {
    //LED提示
    LedStatus=!LedStatus;
    digitalWrite(LED_BUILTIN,LedStatus);

    //使用数据demo
    Serial.print("DataNum:");
    Serial.println(H30Frame.FrameNum); //数据帧号
    
    Serial.println( (float)H30Frame.gyro.gx*0.000001 ); //三轴角速度
    Serial.println( (float)H30Frame.gyro.gy*0.000001 );
    Serial.println( (float)H30Frame.gyro.gz*0.000001 );
    
    Serial.println( (float)H30Frame.accel.ax*0.000001 );//三轴加速度
    Serial.println( (float)H30Frame.accel.ay*0.000001 );
    Serial.println( (float)H30Frame.accel.az*0.000001 );
    
    Serial.println( (float)H30Frame.attitude.pitch*0.000001 );//欧拉角
    Serial.println( (float)H30Frame.attitude.roll*0.000001 );
    Serial.println( (float)H30Frame.attitude.yaw*0.000001 );
    

    Serial.println( (float)H30Frame.quaternion.q0*0.000001 );//四元数
    Serial.println( (float)H30Frame.quaternion.q1*0.000001 );
    Serial.println( (float)H30Frame.quaternion.q2*0.000001 );
    Serial.println( (float)H30Frame.quaternion.q3*0.000001 );

    Serial.println();
    Serial.println();
    Serial.println();
  }
    
}
