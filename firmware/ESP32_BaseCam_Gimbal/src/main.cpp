/* Modified BaseCam ESP32-S3 control firmware. Existing upstream notices are retained in the supplied source package. */
#include "app.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <string.h>
#include <stdlib.h>

GeneralSBGC_t SBGC32_Device;
static Control_t Control;
static ControlConfig_t ControlConfig;
static i16 servoOut[8] = {SERVO_OUT_DISABLED,SERVO_OUT_DISABLED,SERVO_OUT_DISABLED,SERVO_OUT_DISABLED,SERVO_OUT_DISABLED,SERVO_OUT_DISABLED,SERVO_OUT_DISABLED,SERVO_OUT_DISABLED};
static AverageValue_t JoystickAverage[3];
static ui32 currentTime, lastControlTime = 0;
static int joystickCenterX = 0, joystickCenterY = 0, joystickCenterZ = 0;
static float joystickDivisionFactor = JOYSTICK_SCALE_ANGLE;
static volatile int bleAxisX = 0, bleAxisY = 0, bleAxisZ = 0;
static volatile uint32_t bleLastPacketMs = 0;
static volatile bool bleClientConnected = false;

static bool axisInRange(int v){ return v >= BLE_AXIS_MIN && v <= BLE_AXIS_MAX; }
static int clampCommand(int v){ if(v>COMMAND_MAX)return COMMAND_MAX; if(v<COMMAND_MIN)return COMMAND_MIN; return v; }
static int scaleJoystick(int v){ return clampCommand((int)((float)v*joystickDivisionFactor)); }
static int applyDeadzone(int v){ return abs(v)<JOYSTICK_DEADZONE?0:v; }
static int readAdcAverage(int pin,int samples){ int32_t a=0; for(int i=0;i<samples;i++){a+=analogRead(pin);delay(2);} return (int)(a/samples); }
static int readDirectAxis(int pin,int center,AverageValue_t* filter){ int v=applyDeadzone(analogRead(pin)-center); #if JOYSTICK_FILTER_SHIFT > 0
AverageValue(filter,(i16)v); v=filter->avgRes; #endif
return scaleJoystick(v); }
static void applyControlValues(int pitchCmd,int yawCmd,int rollCmd){
#if OPERATING_MODE == OPERATING_MODE_SPEED
Control.AxisC[PITCH].speed=(i16)pitchCmd; Control.AxisC[YAW].speed=(i16)yawCmd; Control.AxisC[ROLL].speed=(i16)rollCmd;
#else
Control.AxisC[PITCH].angle=(i16)pitchCmd; Control.AxisC[YAW].angle=(i16)yawCmd; Control.AxisC[ROLL].angle=(i16)rollCmd;
#endif
}
static void zeroControl(){ applyControlValues(0,0,0); }

#if BLE_MODE_ENABLED
class ServerCallbacks: public BLEServerCallbacks { void onConnect(BLEServer*) override { bleClientConnected=true; } void onDisconnect(BLEServer* s) override { bleClientConnected=false; bleAxisX=bleAxisY=bleAxisZ=0; bleLastPacketMs=0; s->getAdvertising()->start(); } };
class CharacteristicCallbacks: public BLECharacteristicCallbacks { void onWrite(BLECharacteristic* c) override { std::string value=c->getValue(); if(value.empty()||value.length()>=48)return; int x=0,y=0,z=0; if(sscanf(value.c_str(),"%d,%d,%d",&x,&y,&z)!=3)return; if(!axisInRange(x)||!axisInRange(y)||!axisInRange(z))return; bleAxisX=x;bleAxisY=y;bleAxisZ=z;bleLastPacketMs=millis(); } };
#endif

void setup(){
SBGC_SERIAL_PORT.begin(SBGC_UART_BAUD,SERIAL_8N1,RX_PIN,TX_PIN); DEBUG_SERIAL_PORT.begin(DEBUG_SERIAL_SPEED);
#if OPERATING_MODE == OPERATING_MODE_SPEED
joystickDivisionFactor=JOYSTICK_SCALE_SPEED;
#else
joystickDivisionFactor=JOYSTICK_SCALE_ANGLE;
#endif
#if !BLE_MODE_ENABLED
analogReadResolution(ADC_RESOLUTION_BITS); analogSetPinAttenuation(JOY_X_ANALOG_PIN,ADC_11db); analogSetPinAttenuation(JOY_Y_ANALOG_PIN,ADC_11db); analogSetPinAttenuation(JOY_Z_ANALOG_PIN,ADC_11db); joystickCenterX=readAdcAverage(JOY_X_ANALOG_PIN,JOYSTICK_CENTER_SAMPLES); joystickCenterY=readAdcAverage(JOY_Y_ANALOG_PIN,JOYSTICK_CENTER_SAMPLES); joystickCenterZ=readAdcAverage(JOY_Z_ANALOG_PIN,JOYSTICK_CENTER_SAMPLES);
#endif
SBGC32_Init(&SBGC32_Device);
#if BLE_MODE_ENABLED
BLEDevice::init(BLE_DEVICE_NAME); BLEServer* s=BLEDevice::createServer(); s->setCallbacks(new ServerCallbacks()); BLEService* svc=s->createService(SERVICE_UUID); BLECharacteristic* ch=svc->createCharacteristic(CHARACTERISTIC_UUID,BLECharacteristic::PROPERTY_READ|BLECharacteristic::PROPERTY_WRITE|BLECharacteristic::PROPERTY_WRITE_NR); ch->setCallbacks(new CharacteristicCallbacks()); svc->start(); BLEAdvertising* a=BLEDevice::getAdvertising(); a->addServiceUUID(SERVICE_UUID); a->setScanResponse(true); a->start();
#endif
#if OPERATING_MODE == OPERATING_MODE_SPEED
Control.controlMode[PITCH]=CtrlM_MODE_SPEED;Control.controlMode[YAW]=CtrlM_MODE_SPEED;Control.controlMode[ROLL]=CtrlM_MODE_SPEED;
#else
Control.controlMode[PITCH]=CtrlM_MODE_ANGLE;Control.controlMode[YAW]=CtrlM_MODE_ANGLE;Control.controlMode[ROLL]=CtrlM_MODE_ANGLE;
#endif
zeroControl(); ControlConfig.flags=RTCCF_CONTROL_CONFIG_FLAG_NO_CONFIRM;
#if JOYSTICK_FILTER_SHIFT > 0
AverageInit(&JoystickAverage[0],JOYSTICK_FILTER_SHIFT);AverageInit(&JoystickAverage[1],JOYSTICK_FILTER_SHIFT);AverageInit(&JoystickAverage[2],JOYSTICK_FILTER_SHIFT);
#endif
SBGC32_ControlConfig(&SBGC32_Device,&ControlConfig); SBGC32_SetServoOut(&SBGC32_Device,servoOut);
}

void loop(){ currentTime=SBGC32_Device.GetTimeFunc(SBGC32_Device.Drv); if((currentTime-lastControlTime)>CMD_CONTROL_DELAY){ lastControlTime=currentTime;
#if BLE_MODE_ENABLED
uint32_t now=millis(); bool have=bleLastPacketMs!=0; bool timeout=have&&((int32_t)(now-bleLastPacketMs)>(int32_t)BLE_FAILSAFE_TIMEOUT_MS); if(!bleClientConnected||!have||timeout)zeroControl(); else applyControlValues(scaleJoystick(bleAxisX),scaleJoystick(bleAxisY),scaleJoystick(bleAxisZ));
#else
applyControlValues(readDirectAxis(JOY_X_ANALOG_PIN,joystickCenterX,&JoystickAverage[0]),readDirectAxis(JOY_Y_ANALOG_PIN,joystickCenterY,&JoystickAverage[1]),readDirectAxis(JOY_Z_ANALOG_PIN,joystickCenterZ,&JoystickAverage[2]));
#endif
SBGC32_Control(&SBGC32_Device,&Control); } delay(1); }
