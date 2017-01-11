#include "contiki-conf.h"
#include "cpu.h"
#include "gpio.h"
#include "nvic.h"
#include "ssi.h"
#include "reg.h"
#include "ioc.h"
#include "sys-ctrl.h"
#include "board.h"
#include "lib/ringbuf.h"

#if SLIP_ARCH_CONF_SPI == 1

#if !defined(SPI_INT_PORT) || !defined(SPI_INT_PIN)
#error Missing SPI configuration. Please check the hardware configuration in board.h.
#endif

#define SPI_CLK_PORT_BASE        GPIO_PORT_TO_BASE(SPI_CLK_PORT)
#define SPI_CLK_PIN_MASK         GPIO_PIN_MASK(SPI_CLK_PIN)
#define SPI_MOSI_PORT_BASE       GPIO_PORT_TO_BASE(SPI_MOSI_PORT)
#define SPI_MOSI_PIN_MASK        GPIO_PIN_MASK(SPI_MOSI_PIN)
#define SPI_MISO_PORT_BASE       GPIO_PORT_TO_BASE(SPI_MISO_PORT)
#define SPI_MISO_PIN_MASK        GPIO_PIN_MASK(SPI_MISO_PIN)
#define SPI_SEL_PORT_BASE        GPIO_PORT_TO_BASE(SPI_SEL_PORT)
#define SPI_SEL_PIN_MASK         GPIO_PIN_MASK(SPI_SEL_PIN)
#define SPI_CS_PORT_BASE         GPIO_PORT_TO_BASE(SPI_CS_PORT)
#define SPI_CS_PIN_MASK          GPIO_PIN_MASK(SPI_CS_PIN)
#define SPI_INT_PORT_BASE        GPIO_PORT_TO_BASE(SPI_INT_PORT)
#define SPI_INT_PIN_MASK         GPIO_PIN_MASK(SPI_INT_PIN)

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* Ring buffer size for the TX and RX SPI buffers */  
#define BUFSIZE                  1024
/*---------------------------------------------------------------------------*/
static struct ringbuf spi_rx_buf, spi_tx_buf;
static uint8_t rxbuf_data[BUFSIZE];
static uint8_t txbuf_data[BUFSIZE];
static int (* input_callback)(unsigned char c) = NULL;
static void ssi_reconfigure(int init_tx_buffer);

static volatile uint8_t connected = 0;
static volatile uint8_t start_of_frame = 0; /* Start of Frame */
static volatile uint8_t ssi_reset_request = 0; /* SSI reset request */
/*---------------------------------------------------------------------------*/
void
spi_writeb(uint8_t b)
{
  if(connected) {
    /* put byte into ringbuffer, so the ISR can get them and send over SPI */
    ringbuf_put(&spi_tx_buf, b);
    /* Notify master that we have data to send */
    GPIO_SET_PIN(SPI_INT_PORT_BASE, SPI_INT_PIN_MASK);
    PRINTF("INFO: put %02x\r\n", b);
  }
}
/*---------------------------------------------------------------------------*/
void
spi_set_input(int (* input)(unsigned char c))
{
  input_callback = input;
}
/*---------------------------------------------------------------------------*/
void
ssi0_isr(void)
{
  uint8_t data, i, data_count = 0;

  PRINTF("INFO: ISR SR=%lx RIS=%lx MIS=%lx\r\n",
         REG(SSI0_BASE+SSI_SR), REG(SSI0_BASE+SSI_RIS), REG(SSI0_BASE+SSI_MIS));

  /* while there are data in the SPI FIFO (Receive FIFO Not Empty) */
  while(REG(SSI0_BASE + SSI_SR) & SSI_SR_RNE) {
    /* Crappy delay, because SSI can't handle all situations?
     * Without that delay, receiving long messages (8 bytes long) is impossible and we loose 8th byte.
     * In theory it shouldn't happened, because after CS goes HIGH we are checking if RX FIFO is not empty.
     * But it is always empty and we loose 8th byte. Handling shorter messages is not a problem.
     */
    for(i = 0; i < 12; i++) {
      asm("nop");
    }
    /* Get data from FIFO */
    data = REG(SSI0_BASE + SSI_DR);
    /* Only when we are connected (avoid noise on SPI) */
    if(connected) {
      if(start_of_frame == 1) {
        /* if transmission just started, look for initial byte */
        if((data & 0xf0) == 0x50) {
          /* get payload size from initial byte */
          data_count = (data & 0x0f);
          /* check if payload size is bigger than 7 bytes, if yes it means we encountered error */
          if(data_count > 7) {
            /* if we encountered error, request SSI reset */
            ssi_reset_request = 1;
            continue;
          }
          /* if initial byte has been found, mark that transmission is pending */
          start_of_frame = 0;
          /* clear SSI reset request (maybe it was set accidentally?) */
          ssi_reset_request = 0;
        } else {
          /* if initial byte not found, omit current byte (it should normally never happened) and request SSI reset */
          /* sometimes SSI hangs and receives bytes shifted for a few bits, so we need to reset SSI if received frame is wrong */
          PRINTF("WARN: omitting: %02x\r\n", data);
          /* if initial byte is other than 0x5x, it looks that something went wrong or master requests SSI reset */
          ssi_reset_request = 1;
          continue;
        }
      } else {
        /* if transmission is pending and if there is still data that we should collect */
        if(data_count > 0) {
          /* store data into ringbuffer */
          ringbuf_put(&spi_rx_buf, data);
          data_count--;
        }
      }
    }
  }
  /* Clear all interrupt flags */
  REG(SSI0_BASE + SSI_ICR) = SSI_ICR_RTIC | SSI_ICR_RORIC;
}
/*---------------------------------------------------------------------------*/
static void
cs_isr(uint8_t port, uint8_t pin)
{
  int d, i;

  /* check if ISR comes from CS pin */
  if((port != SPI_CS_PORT) && (pin != SPI_CS_PIN)) {
    return;
  }
  
  /* CS goes HIGH, End of Transmission */
  if(GPIO_READ_PIN(SPI_CS_PORT_BASE, SPI_CS_PIN_MASK)) {
    /* check if something left in RX FIFO after transaction, and put all remain data into RX ringbuffer */
    while(REG(SSI0_BASE + SSI_SR) & SSI_SR_RNE) {
      d = REG(SSI0_BASE + SSI_DR);
      ringbuf_put(&spi_rx_buf, d);
      PRINTF("ERR: Something left in FIFO!\r\n");
    }
    /* pass received  data to upper level driver via callback */
    d = ringbuf_get(&spi_rx_buf);
    while(d != -1) {
      if(input_callback) {
        input_callback((unsigned char)d);
      }
      d = ringbuf_get(&spi_rx_buf);
    }
    /* mark that there is no start of frame phase */
    /* TODO: is it necessary? */
    start_of_frame = 0;
    /* check if TX FIFO is not empty */
    if(!(REG(SSI0_BASE + SSI_SR) & SSI_SR_TNF)) {
      /* if TX FIFO is not empty, reset SSI to flush TX FIFO
         it is possible that previous transaction has been failed, so complete frame
         has not been transmitted. Eg. NBR has been turned off during transmission
      */
      PRINTF("ERR: TX FIFO not empty after transaction!\r\n");
      ssi_reset_request = 3;
    }
    if(ssi_reset_request) {
      /* if reset request is active, perform SSI reset */
      PRINTF("WARN: SSI reset request %u\r\n", ssi_reset_request);
      ssi_reconfigure(1);
      ssi_reset_request = 0;
    }
  } else {
    /* CS goes LOW, Start of Transmission */
    start_of_frame = 1;
    /* fill TX FIFO with data only if we were connected */
    if(connected) {
      /* get number of elements in ringbuffer */
      d = ringbuf_elements(&spi_tx_buf);
      /* send that number to master with characteristic upper nibble */
      d = 0x50 | (d > 7 ? 7 : d);
      REG(SSI0_BASE + SSI_DR) = d;
      for(i = 0; i < 7; i++) {
        if(!(REG(SSI0_BASE + SSI_SR) & SSI_SR_TNF)) {
          /* Error, we shouldn't overflow TX FIFO */
          PRINTF("ERR: TX FIFO overflow!\r\n");
          break;
        }
        d = ringbuf_get(&spi_tx_buf);
        if(d == -1) {
          REG(SSI0_BASE + SSI_DR) = 0xff;
        } else {
          REG(SSI0_BASE + SSI_DR) = d;
        }
      }
      /* If the CS interrupt was triggered due to slave requesting SPI transfer,
       * we clear the INT pin, as the transfer has now been completed.
       */
      if(ringbuf_elements(&spi_tx_buf) == 0) {
        GPIO_CLR_PIN(SPI_INT_PORT_BASE, SPI_INT_PIN_MASK);
      }
    } else {
      /* mark we are connected */
      connected = 1;
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
ssi_reconfigure(int init_tx_buffer)
{
  int i;

  /* Disable SSI0 interrupt in NVIC */
  NVIC_DisableIRQ(SSI0_IRQn);

  /* Reset SSI peripheral */
  REG(SYS_CTRL_SRSSI) = 1;
  for(i = 0; i < 24; i++) {
    asm("nop");
  }
  REG(SYS_CTRL_SRSSI) = 0;

  /* Enable the clock for the SSI peripheral */
  REG(SYS_CTRL_RCGCSSI) |= 1;

  /* Start by disabling the peripheral before configuring it */
  REG(SSI0_BASE + SSI_CR1) = 0;
  /* Set slave mode */
  REG(SSI0_BASE + SSI_CR1) = SSI_CR1_MS;// | SSI_CR1_SOD;
  /* Set the IO clock as the SSI clock */
  REG(SSI0_BASE + SSI_CC) = 1;
  /* Configure the clock */
  REG(SSI0_BASE + SSI_CPSR) = 2;//64;
  /* Configure the default SPI options.
   *   mode:  Motorola frame format
   *   clock: High when idle
   *   data:  Valid on rising edges of the clock
   *   bits:  8 byte data
   */
  REG(SSI0_BASE + SSI_CR0) = SSI_CR0_SPH | SSI_CR0_SPO | (0x07);

  if(init_tx_buffer) {
    /* initialize SPI TX FIFO (empty read answer) */
    REG(SSI0_BASE + SSI_DR) = 0x50;
    REG(SSI0_BASE + SSI_DR) = 0xff;
    REG(SSI0_BASE + SSI_DR) = 0xff;
    REG(SSI0_BASE + SSI_DR) = 0xff;
    REG(SSI0_BASE + SSI_DR) = 0xff;
    REG(SSI0_BASE + SSI_DR) = 0xff;
    REG(SSI0_BASE + SSI_DR) = 0xff;
    REG(SSI0_BASE + SSI_DR) = 0xff;
  }

  /* Unmask interrupts (RX FIFO half full or more, and RX timeout) */
  REG(SSI0_BASE + SSI_IM) = SSI_IM_RXIM | SSI_IM_RTIM;
  /* Enable the SSI */
  REG(SSI0_BASE + SSI_CR1) |= SSI_CR1_SSE;

  /* Enable SSI0 interrupt in NVIC */
  NVIC_EnableIRQ(SSI0_IRQn);
}
/*---------------------------------------------------------------------------*/
void
felicia_spi_init(void)
{
  /* Initialize ring buffers for RX and TX data */
  ringbuf_init(&spi_rx_buf, rxbuf_data, sizeof(rxbuf_data));
  ringbuf_init(&spi_tx_buf, txbuf_data, sizeof(txbuf_data));

  /* Configre SSI interface and init TX FIFO */
  ssi_reconfigure(1);

  /* Set the mux correctly to connect the SSI pins to the correct GPIO pins */
  /* set input pin with ioc */
  REG(IOC_CLK_SSIIN_SSI0) = ioc_input_sel(SPI_CLK_PORT, SPI_CLK_PIN);
  REG(IOC_SSIFSSIN_SSI0) = ioc_input_sel(SPI_SEL_PORT, SPI_SEL_PIN);
  REG(IOC_SSIRXD_SSI0) = ioc_input_sel(SPI_MOSI_PORT, SPI_MOSI_PIN);
  /* set output pin */
  ioc_set_sel(SPI_MISO_PORT, SPI_MISO_PIN, IOC_PXX_SEL_SSI0_TXD);

  /* Set pins as input and MISo as output */
  GPIO_SET_INPUT(SPI_CLK_PORT_BASE, SPI_CLK_PIN_MASK);
  GPIO_SET_INPUT(SPI_MOSI_PORT_BASE, SPI_MOSI_PIN_MASK);
  GPIO_SET_INPUT(SPI_SEL_PORT_BASE, SPI_SEL_PIN_MASK); /* it seems that setting SEL as input is not necessary */
  GPIO_SET_OUTPUT(SPI_MISO_PORT_BASE, SPI_MISO_PIN_MASK);
  /* Put all the SSI gpios into peripheral mode */
  GPIO_PERIPHERAL_CONTROL(SPI_CLK_PORT_BASE, SPI_CLK_PIN_MASK);
  GPIO_PERIPHERAL_CONTROL(SPI_MOSI_PORT_BASE, SPI_MOSI_PIN_MASK);
  GPIO_PERIPHERAL_CONTROL(SPI_MISO_PORT_BASE, SPI_MISO_PIN_MASK);
  GPIO_PERIPHERAL_CONTROL(SPI_SEL_PORT_BASE, SPI_SEL_PIN_MASK); /* it seems that setting SEL: as peripheral controlled is not necessary */
  /* Disable any pull ups or the like */
  ioc_set_over(SPI_CLK_PORT, SPI_CLK_PIN, IOC_OVERRIDE_DIS);
  ioc_set_over(SPI_MOSI_PORT, SPI_MOSI_PIN, IOC_OVERRIDE_DIS);
  ioc_set_over(SPI_MISO_PORT, SPI_MISO_PIN, IOC_OVERRIDE_DIS);
  ioc_set_over(SPI_SEL_PORT, SPI_SEL_PIN, IOC_OVERRIDE_PDE); /* it seems that configuring pull-ups/downs on SEL is not necessary */

  /* Configure output INT pin (from Felicia to Host */
  GPIO_SET_OUTPUT(SPI_INT_PORT_BASE, SPI_INT_PIN_MASK);
  GPIO_CLR_PIN(SPI_INT_PORT_BASE, SPI_INT_PIN_MASK);

  /* Configure CS pin and detection for both edges on that pin */
  GPIO_SOFTWARE_CONTROL(SPI_CS_PORT_BASE, SPI_CS_PIN_MASK);
  GPIO_SET_INPUT(SPI_CS_PORT_BASE, SPI_CS_PIN_MASK);
  GPIO_DETECT_EDGE(SPI_CS_PORT_BASE, SPI_CS_PIN_MASK);
  GPIO_TRIGGER_BOTH_EDGES(SPI_CS_PORT_BASE, SPI_CS_PIN_MASK);
  GPIO_ENABLE_INTERRUPT(SPI_CS_PORT_BASE, SPI_CS_PIN_MASK);
  ioc_set_over(SPI_CS_PORT, SPI_CS_PIN, IOC_OVERRIDE_PUE);
  /* Enable interrupt for CS pin */
  NVIC_EnableIRQ(GPIO_B_IRQn);
  gpio_register_callback(cs_isr, SPI_CS_PORT, SPI_CS_PIN);
}
/*---------------------------------------------------------------------------*/
#endif /* SLIP_ARCH_CONF_SPI == 1 */
