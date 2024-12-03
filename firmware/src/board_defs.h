/*
 * Bishi Controller Board Definitions
 * WHowe <github.com/whowechina>
 */

#if defined BOARD_BISHI_PICO

#define RGB_PIN_MAIN 27
#define RGB_PIN_CAB 28
#define RGB_ORDER GRB // or RGB

#define BUS_I2C i2c0
#define BUS_I2C_SDA 20
#define BUS_I2C_SCL 21
#define BUS_I2C_FREQ 400*1000

#define SPIN_VCC_DEF 19
 
 /* Spin, A, B, C */
#define BUTTON_DEF { 22, 13, 14, 15 }

#define UART_UP uart0
#define UART_UP_TX 0
#define UART_UP_RX 1

#define UART_DOWN uart1
#define UART_DOWN_TX 4
#define UART_DOWN_RX 5

#define UART_BAUDRATE 720000


#else

#endif
