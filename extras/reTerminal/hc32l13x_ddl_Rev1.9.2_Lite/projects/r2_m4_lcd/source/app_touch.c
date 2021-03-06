#include "device.h"
#include "goodix.h"


#define TP_I2C_PORT         M0P_I2C1
#define TP_I2C_ADDR         0x5d

static uint8_t tp_state = 0;

en_result_t goodix_i2c_read(u16_t reg, u8_t *buf, s32_t len)
{
    en_result_t ret = Error;
    uint8_t addr[2] = { 0 };

    addr[0] = reg >> 8;
    addr[1] = reg & 0xFF;
    ret = I2C_MasterWriteData(TP_I2C_PORT, TP_I2C_ADDR, addr, 2);
    if (ret != Ok) {
        return ret;
    }
    ret = I2C_MasterReadData(TP_I2C_PORT, TP_I2C_ADDR, buf, len);
    if (ret != Ok) {
        return ret;
    }

    return ret;
}

en_result_t goodix_i2c_write_u8(u16_t reg, u8_t value)
{
    uint8_t buf[3] = { 0 };

    buf[0] = reg >> 8;
    buf[1] = reg & 0xFF;
    buf[2] = value;

    return I2C_MasterWriteData(TP_I2C_PORT, TP_I2C_ADDR, buf, sizeof(buf));
}

uint8_t App_GetTpState(void)
{
    return tp_state;
}

static en_result_t App_I2cTPTest(void)
{
    en_result_t ret = Error;
    u8_t buf[6] = {0};

    DBG_FUNC();

    APP_Pdir(TP_INT, 1); // output
    APP_Pout(TP_INT, 0);
    APP_Pout(TP_RST, 0);
    delay1ms(100);
    APP_Pout(TP_RST, 1);
    delay1ms(1000);
    APP_Pdir(TP_INT, 0); // input

    ret = goodix_i2c_read(GOODIX_REG_ID, buf, sizeof(buf));
    DBG_FUNC("ret=%d,%s,0x%x,0x%x", ret, buf, buf[4], buf[5]);

#if 0
{
    u32_t addr = GOODIX_READ_COOR_ADDR;
    u8_t data[GOODIX_CONTACT_SIZE];

    for (;;) {
        addr = GOODIX_READ_COOR_ADDR;
        goodix_i2c_read(addr, data, 2);
/*      DBG_FUNC("0x%x,0x%x", data[0], data[1]);
        delay1ms(1000);*/
        addr += 2;
        if (data[0] & GOODIX_BUFFER_STATUS_READY) {
            int i = 0, n = 0;
            int x=0, y=0;
            n = data[0] & 0x0F;
            DBG_FUNC("MAX:%d", n);
            for (i=0; i<n; i++) {
                goodix_i2c_read(addr, data, GOODIX_CONTACT_SIZE);
                addr += GOODIX_CONTACT_SIZE;

                x = data[1];
                x <<= 8;
                x += data[0];
                y = data[3];
                y <<= 8;
                y += data[2];
                DBG_FUNC("[%d]x=%d,y=%d", i, x, y);
            }
            goodix_i2c_write_u8(GOODIX_READ_COOR_ADDR, 0);
        }
    }
}
#endif

    return ret;
}


///< IO端口配置
static void App_PortCfg(void)
{
    stc_gpio_cfg_t stcGpioCfg;
    
    DDL_ZERO_STRUCT(stcGpioCfg);
    
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);   //开启GPIO时钟门控 
    
    stcGpioCfg.enDir = GpioDirOut;                            ///< 端口方向配置->输出    
    stcGpioCfg.enOD = GpioOdEnable;                           ///< 开漏输出
    stcGpioCfg.enPu = GpioPuEnable;                           ///< 端口上拉配置->使能
    stcGpioCfg.enPd = GpioPdDisable;                          ///< 端口下拉配置->禁止
    
    Gpio_Init(GpioPortA,GpioPin11,&stcGpioCfg);               ///< 端口初始化
    Gpio_Init(GpioPortA,GpioPin12,&stcGpioCfg);
    
    Gpio_SetAfMode(GpioPortA,GpioPin11,GpioAf3);              ///< 配置PA11为SCL1
    Gpio_SetAfMode(GpioPortA,GpioPin12,GpioAf3);              ///< 配置PA12为SDA1
}

///< I2C 模块配置
void App_I2cTPCfg(void)
{
    stc_i2c_cfg_t stcI2cCfg;
    
    App_PortCfg();

    DDL_ZERO_STRUCT(stcI2cCfg);                            ///< 初始化结构体变量的值为0
    
    Sysctrl_SetPeripheralGate(SysctrlPeripheralI2c1,TRUE); ///< 开启I2C0时钟门控
    
    stcI2cCfg.u32Pclk = Sysctrl_GetPClkFreq();             ///< 获取PCLK时钟
    stcI2cCfg.u32Baud = 400000;                            ///< 1MHz
    stcI2cCfg.enMode = I2cMasterMode;                      ///< 主机模式
    stcI2cCfg.u8SlaveAddr = 0x66;                          ///< 从地址，主模式无效
    stcI2cCfg.bGc = FALSE;                                 ///< 广播地址应答使能关闭
    I2C_Init(TP_I2C_PORT,&stcI2cCfg);                      ///< 模块初始化

    /* test tp */
    if (Ok == App_I2cTPTest()) {
        tp_state = 1;
    }
    else {
        DBG_FUNC("TP was not detected!");
    }
}
