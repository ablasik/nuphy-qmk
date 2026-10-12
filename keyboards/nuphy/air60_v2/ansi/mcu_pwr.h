/* 
Copyright 2023 NuPhy & jincao1
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

void enter_light_sleep(void);
void exit_light_sleep(bool stm32_init);
void enter_deep_sleep(void);
void exit_deep_sleep(void);

void pwr_led_off(void);
void pwr_led_on(void);

void led_pwr_sleep_handle(void);
void led_pwr_wake_handle(void);

void m_uart_gpio_set_low_speed(void);

void clear_report_buffer_and_queue(void);
void clear_report_buffer(void);

void idle_timer_init(void);
void idle_timer_start(void);
void idle_timer_stop(void);
void idle_enter_sleep(void);
