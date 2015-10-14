Modbus TCP

TCP Port 502

Таблица переменных Modbus:

Адрес 	Тип 	Имя 			Описание
0..99	u16		0xAA55			Резерв
100		16bit	GPIO_IN			Значения со входов: бит 0 = GPIO0, …, бит 15 = GPIO15
101		16bit	GPIO_OUT		Выходные значения:  бит 0 = GPIO0, …, бит 15 = GPIO15
102		16bit	GPIO_OUT_SET	Установка выхода: бит равный 1 выставлет  бит в GPIO_OUT
103		16bit	GPIO_OUT_CLEAR	Сброс выхода: бит равный 1 сбрасывает бит в GPIO_OUT
104		16bit	GPIO_ENABLE		Бит равный 1 переключает вывод в режим выхода: бит 0 = GPIO0, …, бит 15 = GPIO15
105		u16		GPIO_NUM		Номер пина  для последующих функций (от 0 до 15 включительно)
106		u16		GPIO_FUNC		Номер функции для вывода (0..4). Используется номер пина из GPIO_NUM. 
107		u16		GPIO_PULLUP		Встроенная подтяжка к +VDD (= 0 - откл, =1 - вкл). Используется номер пина из GPIO_NUM. 
108		u16		GPIO_OD			"Открытый коллектор" (= 0 - откл, =1 - вкл) Используется номер пина из GPIO_NUM. 
109		u16		GPIO_MUX		Регистр конфигурации вывода. Используется номер пина из GPIO_NUM. 
110		u16		ADC				Значение с ADC
111		u16		VDD				Значение VDD c ADC
112		u16		arg_ UserFunc	Аргумент к UserFunc
113		u16		UserFunc		Номер UserFunc

Для доступа из WEB к полям Modbus назначены такие переменные:
~mdbwuNNN~ - чтение word unsigned, NNN - номер ячейки
~mdbwsNNN~ - чтение word signed, NNN - номер ячейки
~mdbduNNN~ - чтение dword unsigned, NNN - номер ячейки
~mdbdsNNN~ - чтение dword signed, NNN - номер ячейки
mdbwNNN=XXX - запись XXX word значения (dec или hex) в ячейку c номером NNN 
mdbdNNN=XXX - запись XXX dword значения (dec или hex) в ячейку c номером NNN