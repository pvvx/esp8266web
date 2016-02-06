/******************************************************************************
 * FileName: rs485drv.c
 * RS-485 low level driver
 * Полудуплексный прием/передача по шине RS-485
 * Created on: 02/11/2015
 * Author: PV`
 ******************************************************************************/
#include "user_config.h"
#ifdef USE_RS485DRV
#include "user_interface.h"
#include "osapi.h"
#include "os_type.h"
#include "hw/esp8266.h"
#include "hw/eagle_soc.h"
#include "hw/uart_register.h"
#include "sdk/add_func.h"
#include "web_iohw.h"
#include "driver/rs485drv.h"
#include "modbusrtu.h"
#include "mdbrs485.h"
#include "mdbtab.h"

#define DEBUG_OUT(x) // UART1_FIFO = x

#define os_post ets_post
#define os_task ets_task

// PIN_TX == 1
#define MUX_TX1_UART0	GPIO1_MUX
#define VAL_MUX_TX1_UART0_OFF	VAL_MUX_GPIO1_IOPORT
#define VAL_MUX_TX1_UART0_ON	0
// PIN_RX == 3
#define MUX_RX3_UART0	GPIO3_MUX
#define VAL_MUX_RX3_UART0_OFF	VAL_MUX_GPIO3_IOPORT
#define VAL_MUX_RX3_UART0_ON	(1<<GPIO_MUX_PULLUP_BIT)
// PIN_TX == 15
#define MUX_TX15_UART0	GPIO15_MUX
#define VAL_MUX_TX15_UART0_OFF	((1<<GPIO_MUX_FUN_BIT0) | (1<<GPIO_MUX_FUN_BIT1) | (1<<GPIO_MUX_PULLUP_BIT))
#define VAL_MUX_TX15_UART0_ON	(1<<GPIO_MUX_FUN_BIT2)
// PIN_RX == 13
#define MUX_RX13_UART0	GPIO13_MUX
#define VAL_MUX_RX13_UART0_OFF	VAL_MUX_GPIO13_IOPORT
#define VAL_MUX_RX13_UART0_ON	((1<<GPIO_MUX_FUN_BIT2) | (1<<GPIO_MUX_PULLUP_BIT))


#define UART_FIFO_SIZE 128
#define UART0_CONF1_TOUT(a)	UART0_CONF1 = ((120 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) \
			| ((16 & UART_TXFIFO_EMPTY_THRHD) << UART_TXFIFO_EMPTY_THRHD_S) \
			| (((UART_FIFO_SIZE - 16) & UART_RX_FLOW_THRHD) << UART_RX_FLOW_THRHD_S) \
			| ((a /*& UART_RX_TOUT_THRHD*/) << UART_RX_TOUT_THRHD_S) |  UART_RX_TOUT_EN;

#define UART_RX_ERR_INTS (UART_BRK_DET_INT_ENA | UART_RXFIFO_OVF_INT_ENA | UART_FRM_ERR_INT_ENA | UART_PARITY_ERR_INT_ENA)

#define rs485_rx_enable()	GPIO_OUT_W1TC = rs485vars.pin_ena_mask; // 1<<PIN_TX_ENABLE;
#define rs485_tx_enable()	GPIO_OUT_W1TS = rs485vars.pin_ena_mask; // 1<<PIN_TX_ENABLE;

#define mMIN(a, b)  ((a<b)?a:b)
#define timer_arg(pt,arg) do { \
	uint32 * ptimer_arg = (uint32 *)&pt.timer_arg; \
	*ptimer_arg = arg; \
	} while(0)

srs485vars rs485vars; // рабочий блок переменных
srs485cfg rs485cfg; // конфигурация
uint8 bufi[MDB_SER_ADU_SIZE_MAX*2]; // минимум MDB_SER_ADU_SIZE_MAX*2

/* -------------------------------------------------------------------------
 * Удалить буфер приема, выбрать сообщение из очереди
 * ------------------------------------------------------------------------- */
#ifdef	RS485_DRV_USE_RX_LIST
static void free_rx_msg(struct srs485msg *msg)
{
	SLIST_REMOVE(&rs485vars.rxmsgs, msg, srs485msg, next);
	os_free(msg);
}
#endif
#ifdef MDB_RS485_MASTER
/* -------------------------------------------------------------------------
 * Сбросить флаги транзакторов
 * ------------------------------------------------------------------------- */
static void reset_flg_trn(void)
{
	if(rs485vars.flg.b.int_msg != 0 && rs485vars.flg.b.trn_reset == 0 && rs485vars.flg.b.trn_num < MDB_TRN_MAX) {
		smdbtrn * trn = &mdb_buf.trn[rs485vars.flg.b.trn_num];
		if(trn->fifo_cnt != 0) trn->fifo_cnt--;
		if(trn->fifo_cnt == 0) trn->start_flg = 0;
		if(rs485vars.flg.b.wait) {
			trn->rx_err++;
			if(trn->rx_err == 0) trn->rx_err = -1;
		}
#if DEBUGSOO > 100
		os_printf("ctrn[%u],fifoc=%u, uflg=%08x\n", rs485vars.flg.b.trn_num, trn->fifo_cnt, rs485vars.flg.ui);
#endif
		rs485vars.flg.b.trn_reset = 1;
	}
}
#endif
/* -------------------------------------------------------------------------
 * Удалить буфер передачи, исключить сообщение из очереди
 * ------------------------------------------------------------------------- */
static void free_tx_msg(void)
{
	if(rs485vars.curtxmsg != NULL) {
		SLIST_REMOVE(&rs485vars.txmsgs, rs485vars.curtxmsg, srs485msg, next);
		os_free(rs485vars.curtxmsg);
		rs485vars.curtxmsg = NULL;
	}
}
/* -------------------------------------------------------------------------
 * выбрать новое сообщение из очереди
 * ------------------------------------------------------------------------- */
static struct srs485msg * get_nex_tx_msg(void)
{
	struct srs485msg *msg = rs485vars.curtxmsg; // сбрасывается в NULL в RS485_SIG_TX_OK
	if(msg == NULL) { // найти последний блок для передачи
		msg = SLIST_FIRST(&rs485vars.txmsgs);
		if(msg != NULL) { // есть блоки для передачи?
			while(SLIST_NEXT(msg, next) != NULL) msg = SLIST_NEXT(msg, next); // найти последний блок для передачи
		}
		rs485vars.curtxmsg = msg; // последний, не обработанный блок для передачи
		if(msg != NULL) { // новый пакет
			// запустить таймер предельного времени ожидания отправки пакета
//			ets_timer_disarm(&rs485vars.timerw); // остановить таймер предельного времени ожидания приема/передачи пакета // вызовет в ets_timer_arm_new()
			ets_timer_arm_new(&rs485vars.timerw, rs485cfg.timeout, 0, 1); // запустить таймер предельного времени ожидания приема/передачи пакета
		}
	}
	return msg;
}
/* -------------------------------------------------------------------------
 * Проверить наличие пакета в очереди и начать передачу, если возможно
 * ------------------------------------------------------------------------- */
bool rs485_next_txmsg(void)
{
	bool ret = false;
	if(rs485vars.status == RS485_RX_ENA && rs485vars.flg.b.tx_ready != 0) { // включен прием (можно начать передачу)?
		struct srs485msg *msg = rs485vars.curtxmsg; // сбрасывается в NULL в RS485_SIG_TX_OK
		if(msg == NULL) { // найти последний блок для передачи
			msg = SLIST_FIRST(&rs485vars.txmsgs);
			if(msg != NULL) { // есть блоки для передачи?
				while(SLIST_NEXT(msg, next) != NULL) msg = SLIST_NEXT(msg, next); // найти последний блок для передачи
			}
			if(msg != NULL) { // есть ещё сообщение, тогда перейти к передаче
				ets_intr_lock();
				if(rs485vars.flg.b.tx_ready != 0  // возможен старт следующей передачи?
				&& rs485vars.flg.b.rx_chars == 0  // не супели принять символы после TOUT?
				&& ((UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT) == 0) { // в fifo нет символов ?
					// старт транзакции нового msg
					// перекинуть из msg все флаги в локальные рабочие пременные
					rs485vars.curtxmsg = msg; // последний, не обработанный блок для передачи
					rs485vars.flg.ui = msg->flg.ui; // + сброс флагов
					rs485vars.pbufo = msg->buf;
					rs485vars.cntro = msg->len;
			    	UART0_INT_ENA = UART_TXFIFO_EMPTY_INT_ST; // старт передачи
					ets_intr_unlock();
					rs485vars.out[0] = msg->buf[0];
					rs485vars.out[1] = msg->buf[1]&0x7F;
		    		ets_timer_disarm(&rs485vars.timerw); // остановить таймер предельного времени ожидания передачи пакета
			    	ret = true;
				} // если не удалось, то ждем таймаута или следующей паузы приема
				else ets_intr_unlock();
			}
		}
	}
	return ret;
}
/* =========================================================================
 * rs485_task
 * ------------------------------------------------------------------------- */
void rs485_task(os_event_t *e){
	if(rs485vars.status != RS485_TX_RX_OFF) {
		switch(e->sig) {
		case RS485_SIG_RX_OK:	//	Принято ADU (в буфере bufo)
		{
			uint32 len = e->par >> 16; // получить длину
			if(len >= MDB_SER_ADU_SIZE_MIN) { // есть msg? это не шум?
				DEBUG_OUT('r');
				// ADU принято, переместить в буфер приема если валидно
				ets_timer_disarm(&rs485vars.timerp); // остановить таймер окончания паузы
//				timer_arg(rs485vars.timerp,1); // запретить выбрать следующее txmsg
				if (system_get_free_heap_size() > MDB_TCP_RS485_MIN_HEAP) {
					uint8 * pbufi = &bufi[e->par & 0xFFFF]; // получить укзатель на начало блока
#ifdef	RS485_DRV_USE_RX_LIST
					len = rs485_test_msg(pbufi, len);
					if(len) { // msg валидно, копируем в новый буфер
						ets_timer_disarm(&rs485vars.timerw); // остановить таймер предельного времени ожидания приема пакета
						struct srs485msg *msg = os_malloc(len + SIZE_RS485MSG_HEAD);
						if(msg != NULL) {
							os_memcpy(msg->buf, pbufi, len);
							msg->len = len;
							msg->flg.ui = rs485vars.flg.ui;
							SLIST_INSERT_HEAD(&rs485vars.rxmsgs, msg, next);
							if(rs485_rx_msg(msg)) free_rx_msg(msg); // обработать сообщение
						}
#if DEBUGSOO > 1
						else os_printf("rs485rx - out of mem!\n");
#endif
					};
#else
					if(rs485_test_msg(pbufi, len)) {
						ets_timer_disarm(&rs485vars.timerw); // остановить таймер предельного времени ожидания приема пакета
///						timer_arg(rs485vars.timerp, 0); // разрешить выбрать следующее txmsg
						// Зарядить таймер на 12 тиков или на 1750 us - 16*1000000/baud при baud > 19200
						ets_timer_arm_new(&rs485vars.timerp, rs485vars.waitust, 0, 0);
					}
#endif
				};
			}
			else {
				DEBUG_OUT('e');
				// ADU менее MDB_SER_ADU_SIZE_MIN -> шум на линии
				// В режиме rs485vars.flg.b.tx_only ожидать паузу и переключиться на следующую передачу
				// иначе ожидание по таймауту (но если нет очереди...)
			};
			// Зарядить таймер на 12 тиков или на 1750 us - 16*1000000/baud при baud > 19200
//			ets_timer_arm_new(&rs485vars.timerp, rs485vars.waitust, 0, 0);
		}
			break;
		case RS485_SIG_TX_OK:	//	ADU передано
			DEBUG_OUT('t');
			// ADU передано, удалить буфер передачи, определить что дальше - прием или опять передача
			ets_timer_disarm(&rs485vars.timerp); // остановить таймер окончания паузы
			ets_timer_disarm(&rs485vars.timerw); // остановить таймер предельного времени ожидания передачи пакета
			free_tx_msg(); // удалить буфер передачи
			if(rs485vars.flg.b.tx_only) { // после передачи не идет прием -> ждать паузу и обработать новое txmsg
///				timer_arg(rs485vars.timerp, 0); // разрешить выбрать следующее txmsg
				// Зарядить таймер на 12 тиков или на 1750 us - 16*1000000/baud при baud > 19200
				ets_timer_arm_new(&rs485vars.timerp, rs485vars.waitust, 0, 0);
			}
			else {
//				rs485vars.timerw.timer_arg = 1; // не выбрать следующее txmsg
				// запустить таймер предельного времени ожидания приема пакета
				ets_timer_arm_new(&rs485vars.timerw, rs485cfg.timeout, 0, 1); // запустить таймер предельного времени ожидания приема пакета
			}
			break;
#ifdef RS485_TIMEOUT_ALARM_ENA
		case RS485_SIG_TIMEOUT:	// Таймаут приема
			DEBUG_OUT('w');
			// вызов процедуры уведомления о таймауте
			// e->par = rs485vars.flg.tid пропущенного сообщения
			// ....
			//
			break;
#endif
		};
	};
}
/* =========================================================================
 * Таймер обработки приема и передачи символов по rs485
 * срабатывает после паузы rs485vars.waitust (3.5 char)
 * Если flg_no_next_msg = 1 то, стартовать новую передачу не будет
 * ------------------------------------------------------------------------- */
static void rs485_timerp_isr(void) //uint32 flg_no_next_msg)
{
	struct srs485msg *msg;
	DEBUG_OUT('x');
	if(rs485vars.status == RS485_RX_ENA) { // включен прием (можно начать передачу)?
#ifdef MDB_RS485_MASTER
		reset_flg_trn(); // Сбросить флаги транзакторов
#endif
		// если очередь не пуста, проверить состояние на возможность передачи и стартануть передачу
///		if(flg_no_next_msg == 0 // стартовать новую передачу?
///		&&(msg = get_nex_tx_msg()) != NULL) { // есть новое сообщение и разрешена обработка следующего msg, тогда перейти к передаче
		if((msg = get_nex_tx_msg()) != NULL) { // есть новое сообщение и разрешена обработка следующего msg, тогда перейти к передаче
			ets_intr_lock();
			if(rs485vars.status == RS485_RX_ENA // включен прием (можно начать передачу)?
			&& ((UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT) == 0 // в fifo нет символов ?
			&& rs485vars.flg.b.rx_chars == 0) { // ещё не приняты символы?
				// старт транзакции нового msg
				// перекинуть из нового msg все флаги в локальные рабочие пременные
				rs485vars.flg.ui = msg->flg.ui; // + сброс флагов
				rs485vars.pbufo = msg->buf;
				rs485vars.cntro = msg->len;
    	    	UART0_INT_ENA = UART_TXFIFO_EMPTY_INT_ST; // hard старт передачи
    	    	ets_intr_unlock();
				rs485vars.out[0] = msg->buf[0];
				rs485vars.out[1] = msg->buf[1]&0x7F;
    	    	ets_timer_disarm(&rs485vars.timerw); // остановить таймер предельного времени ожидания передачи пакета
			} // если не удалось, то ждем таймаута или следующей паузы приема
			else ets_intr_unlock();
		}
		else { // очередь пуста или заперт старта новой передачи
			// если очередь пуста, определить и отметить возможность передачи
			ets_intr_lock();
			if(rs485vars.status == RS485_RX_ENA // включен прием (можно начать передачу)?
			&& ((UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT) == 0 // в fifo нет символов ?
			&& rs485vars.flg.b.rx_chars == 0) { // ещё не приняты символы?
				 rs485vars.flg.b.tx_ready = 1; // возможен старт следующей передачи (требует проверки UART_RXFIFO_CNT == 0 && rs485vars.flg.b.rx_chars == 0)
				 // конец транзкции старого msg
				 DEBUG_OUT('+');
			}
			ets_intr_unlock();
		}
	}
}
/* -------------------------------------------------------------------------
 * Зарядить таймер определения паузы на 3.5 символа
 * или на 1750 us при baud > 19200
 * ------------------------------------------------------------------------- */
static void start_full_timerp(void)
{
	uint32 waitus = 1750;
	if(!(rs485cfg.flg.b.spdtw == 0 && rs485cfg.baud > 19203))
		waitus = 35*11*100000/rs485cfg.baud; // пауза в us на 3.5 символа
//	ets_timer_disarm(&rs485vars.timerp); // вызовет в ets_timer_arm_new()
///	timer_arg(rs485vars.timerp, 0); // разрешить выбрать следующее txmsg
	ets_timer_arm_new(&rs485vars.timerp, waitus, 0, 0);
}
/* =========================================================================
 * Таймер предельного времени ожидания приема/передачи пакета
 * срабатывает после паузы rs485cfg.timeout (таймаут приема/передачи)
 * ------------------------------------------------------------------------- */
static void rs485_timerw_isr(void)
{
	uint32 status = rs485vars.status;
	if(status == RS485_RX_ENA) {
		DEBUG_OUT('z');
		ets_intr_lock();
		rs485vars.flg.b.wait = 1; // отметка таймаута
		ets_intr_unlock();
#ifdef RS485_TIMEOUT_ALARM_ENA
		uint32 tid = rs485vars.flg.tid;
#endif
		if(rs485vars.flg.b.tx_end == 0) { // был конец передачи + пауза 24..32 бита?
			// передача не удалась (сплошной шум на линии?)
//#ifdef MDB_RS485_MASTER
			if(rs485vars.curtxmsg != NULL) {
				// скопировать флаги из необработанного msg
				ets_intr_lock();
				rs485vars.flg.ui = rs485vars.curtxmsg->flg.ui | (rs485vars.flg.ui & 0x00ff0000);
				ets_intr_unlock();
				free_tx_msg(); // удалить старый буфер передачи
			}
//#endif
		}
		if(((UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT) == 0) { // в fifo нет символов ?
			if(rs485vars.flg.b.tx_ready != 0 // возможен старт следующей передачи?
			|| (rs485vars.flg.b.rx_end == 0 && rs485vars.flg.b.rx_chars == 0)) { // вообще не было движения на RX?
				rs485_timerp_isr(); // Проверить наличие пакета в очереди и начать передачу, если возможно
			}
			else {
				if(rs485vars.timerp.timer_next == (ETSTimer *)0xffffffff) { // уже запущен таймер паузы на 3.5 символа?
					start_full_timerp(); // Зарядить таймер определения паузы на 3.5 символа
				}
///				else timer_arg(rs485vars.timerp, 0); // разрешить выбрать следующее txmsg
			}
		}
#ifdef RS485_TIMEOUT_ALARM_ENA
		os_post(RS485_TASK_PRIO + SDK_TASK_PRIO, RS485_SIG_TIMEOUT, tid); // Таймаут приема/передачи
#endif
	}
	else if(status == RS485_TX_ENA) { // не верно задан тайм-аут
		DEBUG_OUT('m');
		// дождаться rs485vars.status == RS485_RX_ENA
		ets_timer_arm_new(&rs485vars.timerw, 2, 0, 1); // запустить таймер предельного времени ожидания приема пакета
	}
}
/* -------------------------------------------------------------------------
 *  отключить loopback и сбросить rx fifo, ошибки приема и буфер
 * ------------------------------------------------------------------------- */
static void rs485_rx_clr_buf(void)
{
	// отключим loopback и сбросим rx fifo
	uint32 conf0 = UART0_CONF0 & (~ (UART_RXFIFO_RST | UART_LOOPBACK));
	UART0_CONF0 = conf0 | UART_RXFIFO_RST;
	UART0_CONF0 = conf0;
	// обнулим приемный буфер
	rs485vars.cntri = 0;
	// сбросить ошибки в UART0_INT_RAW
	UART0_INT_CLR = UART_RX_ERR_INTS;
}
/* =========================================================================
 * обработка прерываний UART для приема и передачи символов по rs485
 * ------------------------------------------------------------------------- */
static void rs485_uart_isr(void *para)
{
	MEMW(); // синхронизация и ожидание отработки fifo-write на шинах CPU,  на всякий случай
    uint32 ints = UART0_INT_ST;
    if(ints) {
    	DEBUG_OUT('I');
    	if(ints & UART_TXFIFO_EMPTY_INT_ST) { // fifo tx пусто?
    		DEBUG_OUT('O');
    		if(rs485vars.status != RS485_TX_ENA) {
        		// переключение шины/драйвера на режим tx
        		rs485_tx_enable();
    			// tx как вывод UART tx
        		if(rs485cfg.flg.b.swap)	MUX_TX15_UART0 = VAL_MUX_TX15_UART0_ON;
        		else MUX_TX1_UART0 = VAL_MUX_TX1_UART0_ON;
//        		UART0_CONF1_TOUT(2); // Установить TOUT 8*2 такта для переключения направления по окончанию вывода
        		rs485vars.status = RS485_TX_ENA;
    			// запретим все прерывания, кроме пустого tx fifo
    			UART0_INT_ENA = UART_TXFIFO_EMPTY_INT_ENA;
    		}
    		// включим loopback и сбросим rx fifo
			uint32 conf0 = (UART0_CONF0 & (~ (UART_RXFIFO_RST))) | UART_LOOPBACK;
			UART0_CONF0 = conf0 | UART_RXFIFO_RST;
			UART0_CONF0 = conf0;
    		if(rs485vars.cntro) { // есть ещё байты для передачи? да.
    			// узнаем размер свободного tx fifo с учетом сколько ещё передавать
    			uint32 len = mMIN(UART_FIFO_SIZE - ((UART0_STATUS >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT), rs485vars.cntro);
    			// заполним tx fifo
    			while(len--) {
    				UART0_FIFO = *rs485vars.pbufo++;
    				rs485vars.cntro--;
    			}
    			if(rs485vars.cntro == 0) { // всё вывели ? да.
    				// запретим все прерывания, кроме rx tout
        			UART0_INT_ENA = UART_RXFIFO_TOUT_INT_ENA;
    			}
    		}
	    	rs485vars.flg.b.tx_ready = 0; // брос разрешения старта передачи
    	}
    	else if(rs485vars.status == RS485_RX_ENA) { // прием ?
			if(UART0_INT_RAW & UART_RX_ERR_INTS) { // ошибки при приеме?
				DEBUG_OUT('E');
				rs485_rx_clr_buf(); // отключим loopback и сбросим rx fifo, ошибки приема и буфер
				rs485vars.flg.b.rx_chars = 1; // отметим прием символов после TOUT
			}
			else {
				DEBUG_OUT('R');
				// дополнить буфер приема символами из rx fifo, если есть
				while((UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT) {
					rs485vars.flg.b.rx_chars = 1; // отметим прием символов
					if(rs485vars.cntri >= MDB_SER_ADU_SIZE_MAX) { // больше возможного?
						rs485_rx_clr_buf(); // отключим loopback и сбросим rx fifo, ошибки приема и буфер
						// rs485vars.flg.b.rx_buf_ovr = 1; // переполнение буфера приемка
						break;
					}
	    			// скопируем символ в буфер
					rs485vars.pbufi[rs485vars.cntri++] = UART0_FIFO;
				}
			}
			if(ints & UART_RXFIFO_TOUT_INT_ST) { // Rx time-out event ?
				DEBUG_OUT('S');
				uint32 x = rs485vars.pbufi - bufi; // вычислить индекс в буфере на начало msg
				if(x + rs485vars.cntri <= MDB_SER_ADU_SIZE_MAX) { // в первой части буфера?
						rs485vars.pbufi = &bufi[x + rs485vars.cntri]; // переместить указатель за msg
				}
				else rs485vars.pbufi = bufi; // переместить указатель на начало буфера
				os_post(RS485_TASK_PRIO + SDK_TASK_PRIO, RS485_SIG_RX_OK, x | (rs485vars.cntri << 16)); // есть msg? или шум?
				rs485vars.cntri = 0; // обнулить кол-во символов в буфере приема
				rs485vars.flg.b.rx_chars = 0; // пока нет принятых символов (для процедуры ожидания паузы)
		    	rs485vars.flg.b.tx_ready = 0; // сброс разрешения старта передачи
		    	rs485vars.flg.b.rx_end = 1; // был конец приема + пауза 24..32 бита
			}
    	}
    	else if(rs485vars.status == RS485_TX_ENA && (ints & UART_RXFIFO_TOUT_INT_ST)) { // конец передачи -> Rx time-out event?
    		DEBUG_OUT('T');
			// tx как i/o порт ввода
    		if(rs485cfg.flg.b.swap != 0)	MUX_TX15_UART0 = VAL_MUX_TX15_UART0_OFF;
    		else MUX_TX1_UART0 = VAL_MUX_TX1_UART0_OFF;
    		// переключение шины/драйвера на режим rx
			rs485_rx_enable();
    		// Установить TOUT 8*x тактов для определения паузы конца сообщения
    		// UART0_CONF1_TOUT(rs485vars.tout_thrhd); // 3.5*11 = 28 тиков, 28/8 = 3.5 символа, TOUT=3 -> 16..24 тика
    		// включение приема
    		rs485_rx_clr_buf(); // отключим loopback и сбросим rx fifo, ошибки приема и буфер
    		rs485vars.flg.b.rx_chars = 0; // пока нет принятых символов
			UART0_INT_ENA = UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA; // | UART_RX_ERR_INTS;
			rs485vars.status = RS485_RX_ENA;
			os_post(RS485_TASK_PRIO + SDK_TASK_PRIO, RS485_SIG_TX_OK, 0);
	    	rs485vars.flg.b.tx_ready = 0; // брос разрешения старта передачи
	    	rs485vars.flg.b.tx_end = 1; // был конец передачи + пауза 24..32 бита
    	}
   		else if((UART0_STATUS >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT) {
   				DEBUG_OUT('?');
        		// сбросим rx fifo
    			uint32 conf0 = (UART0_CONF0 & (~ (UART_RXFIFO_RST)));
    			UART0_CONF0 = conf0 | UART_RXFIFO_RST;
    			UART0_CONF0 = conf0;
    			// отметим прием символов
    			rs485vars.flg.b.rx_chars = 1;
//    	    	rs485vars.flg.b.tx_ready = 0; // брос разрешения старта передачи
        };
    	UART0_INT_CLR = ints;
    }
    else {
    	UART1_INT_ENA = 0;
    	UART1_INT_CLR = UART1_INT_ST;
    }
}
/* ********************************
 * Запустить драйвер RS485 (прием)
 * Ожидает паузу в 3.5 символа перед первой передачей
 * Запускает таймаут ожидания
 ******************************** */
void ICACHE_FLASH_ATTR rs485_drv_start(void)
{
//	ets_intr_lock();
	if(rs485vars.status == RS485_TX_RX_OFF) {
		UART0_INT_ENA = 0;
		// включение приема
		// tx как i/o порт ввода
		if(rs485cfg.flg.b.swap != 0)	MUX_TX15_UART0 = VAL_MUX_TX15_UART0_OFF;
		else MUX_TX1_UART0 = VAL_MUX_TX1_UART0_OFF;
		// переключение шины/драйвера на режим rx
		rs485_rx_enable();

		rs485_rx_clr_buf(); // отключим loopback и сбросим rx fifo, ошибки приема и буфер
		// enable rx time-out function
		// Установить TOUT 8*x тактов для определения паузы конца сообщения
		//UART0_CONF1_TOUT(rs485vars.tout_thrhd); // 3.5*11 = 28 тиков, 28/8 = 3.5 символа, TOUT=3 -> 16..24 тика
		rs485vars.flg.ui = 0;
		UART0_INT_CLR = ~0;
		UART0_INT_ENA = UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA; // | UART_RX_ERR_INTS;
		rs485vars.status = RS485_RX_ENA;
		// запустить таймер предельного времени ожидания приема/отправки пакета
//		ets_timer_disarm(&rs485vars.timerw); // остановить таймер предельного времени ожидания приема пакета // вызовет в ets_timer_arm_new()
		ets_timer_arm_new(&rs485vars.timerw, rs485cfg.timeout, 0, 1); // запустить таймер предельного времени ожидания приема пакета
		start_full_timerp(); // Зарядить таймер определения паузы на 3.5 символа или на 1750 us при baud > 19200
	}
//	ets_intr_unlock();
}
/* *************************************
 * Очистить все очереди сообщений RS485
 * Запуск при остановленном драйвере!
 ************************************* */
void ICACHE_FLASH_ATTR rs485_drv_clr_bufs(void)
{
	struct srs485msg *msg;
#ifdef	RS485_DRV_USE_RX_LIST
	SLIST_FOREACH(msg, &rs485vars.rxmsgs, next) os_free(msg);
	SLIST_INIT(&rs485vars.rxmsgs);
#endif
	rs485vars.curtxmsg = NULL;
	SLIST_FOREACH(msg, &rs485vars.txmsgs, next) os_free(msg);
	SLIST_INIT(&rs485vars.txmsgs);
}
/* *******************************
 * Остановить драйвер RS485
 ******************************** */
void ICACHE_FLASH_ATTR rs485_drv_stop(void)
{
	UART0_INT_ENA = 0;

	uint32 conf0 = UART0_CONF0 & (~ (UART_RXFIFO_RST | UART_TXFIFO_RST | UART_LOOPBACK));
	UART0_CONF0 = conf0 | UART_RXFIFO_RST | UART_TXFIFO_RST;
	UART0_CONF0 = conf0;

	ets_timer_disarm(&rs485vars.timerp);
	ets_timer_disarm(&rs485vars.timerw);
	// tx как i/o порт ввода
	if(rs485cfg.flg.b.swap != 0)	MUX_TX15_UART0 = VAL_MUX_TX15_UART0_OFF;
	else MUX_TX1_UART0 = VAL_MUX_TX1_UART0_OFF;
	// переключение шины/драйвера на режим rx
	rs485_rx_enable();
	UART0_INT_CLR = ~0;
	rs485vars.status = RS485_TX_RX_OFF;
	rs485_drv_clr_bufs();
}
/* **********************************************
 * (Ре)Инициализация драйвера RS-485 по скорости
 ********************************************** */
void ICACHE_FLASH_ATTR rs485_drv_set_baud(void)
{
	if(rs485cfg.baud > RS485_MAX_BAUD) rs485cfg.baud = RS485_MAX_BAUD;
	else if(rs485cfg.baud < RS485_MIN_BAUD) rs485cfg.baud = RS485_MIN_BAUD;
	ets_intr_lock();
	uint32 x = (ets_get_cpu_frequency()  * 1000000) >> (CLK_PRE_PORT & 1);
	UART0_CLKDIV = x / rs485cfg.baud;
	rs485cfg.baud = x / UART0_CLKDIV;
	uint32 tout_thrhd;
	if((rs485cfg.flg.b.spdtw == 0) && rs485cfg.baud > 19203) {
		tout_thrhd = (750 * rs485cfg.baud + 4000000)/8000000;
		if(tout_thrhd > 125) tout_thrhd = 125;
		rs485vars.waitust = 1750 - (tout_thrhd*8000000)/rs485cfg.baud;
		tout_thrhd += 2;
	}
	else {
		tout_thrhd = 4; // 24..32 bits
		rs485vars.waitust = ((39-((4-1)*8))*1000000)/rs485cfg.baud; // in us // (Pause(bits) - (tout-1)*8) * 1000000(us in sec)/baud
	}
	rs485vars.waitust += rs485cfg.pause; // добавка времени в us между сообщениями, 1…65535 мкс, 0 - не используется
	UART0_CONF0 = 0x1C | (rs485cfg.flg.b.parity & 3) | ((rs485cfg.flg.b.parity & UART_PARITY_EN)? 0 : (2-(rs485cfg.flg.b.parity&1)) << UART_STOP_BIT_NUM_S);
    UART0_CONF1_TOUT(tout_thrhd);
	ets_intr_unlock();
}
/* ********************************************
 * (Ре)Инициализация драйвера RS-485 по пинам
 ******************************************** */
void ICACHE_FLASH_ATTR rs485_drv_set_pins(void)
{
	uint32 pin_ena = rs485cfg.flg.b.pin_ena;
	uint32 x = (rs485cfg.flg.b.swap)? 0x503F : 0xF035; // биты пинов
	if(pin_ena < 16 && ((x & (1 << pin_ena)) == 0)) pin_ena = -1;
	uint32 pin_tx = 1;
	PERI_IO_SWAP &= ~(PERI_IO_UART_PORT_SWAP | PERI_IO_UART0_PIN_SWAP);
	if(rs485cfg.flg.b.swap != 0) {
		pin_tx = 15;
		PERI_IO_SWAP |= PERI_IO_UART0_PIN_SWAP; // swap uart0 pins (u0rxd <-> u0cts), (u0txd <-> u0rts)
		MUX_RX13_UART0 = VAL_MUX_RX13_UART0_ON; // настроить пин RX
	}
	else {
		MUX_RX3_UART0 = VAL_MUX_RX3_UART0_ON; // настроить пин RX
	}
	// pin TX при приеме
	GPIO_PIN(pin_tx) = GPIO_PIN_DRIVER;
	GPIO_OUT_W1TS = 1 << pin_tx;
	GPIO_ENABLE_W1TC = 1 << pin_tx;
	// pin OE_TX in I/O
	if(pin_ena < 16) {
		rs485vars.pin_ena_mask = 1 << pin_ena;
		rs485_rx_enable();
		GPIO_ENABLE_W1TS = rs485vars.pin_ena_mask;
		set_gpiox_mux_func_ioport(pin_ena); //	SET_PIN_FUNC_IOPORT(pin_ena);
	}
	else rs485vars.pin_ena_mask = 0;
}
/* *****************************
 * Инициализация драйвера rs485
 ******************************** */
void ICACHE_FLASH_ATTR rs485_drv_init(void)
{
	UART0_INT_ENA = 0;

	rs485_drv_set_pins();
    rs485_drv_set_baud();

    rs485_rx_clr_buf();
	UART0_INT_CLR = ~0;

	system_os_task(rs485_task, RS485_TASK_PRIO, rs485vars.taskQueue, RS485_TASK_QUEUE_LEN);

	ets_isr_attach(ETS_UART_INUM, rs485_uart_isr, NULL);
	ets_isr_unmask(1 << ETS_UART_INUM);

	rs485vars.status = RS485_TX_RX_OFF;
	rs485vars.pbufi = bufi;

#ifdef	RS485_DRV_USE_RX_LIST
	SLIST_INIT(&rs485vars.rxmsgs);
#endif
	SLIST_INIT(&rs485vars.txmsgs);

	ets_timer_disarm(&rs485vars.timerp);
	ets_timer_setfn(&rs485vars.timerp, (os_timer_func_t *)rs485_timerp_isr, NULL);
	ets_timer_disarm(&rs485vars.timerw);
	ets_timer_setfn(&rs485vars.timerw, (os_timer_func_t *)rs485_timerw_isr, NULL);
}

#endif // USE_RS485DRV
