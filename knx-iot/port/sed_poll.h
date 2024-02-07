#ifndef SED_POLL_H
#define SED_POLL_H

#include <stdint.h>

/**
 * @brief Start polling for messages
 * 
 * After calling this function, the device will send 802.15.4 Data Requests
 * periodically, in order to receive frames cached by its parent. This function
 * should only be used by RxOffWhenIdle devices (sleepy transceiver).
 * 
 * The time between polls starts at min_delay, and then increases exponentially
 * until it reaches max_delay. Useful polls reset the delay to the value of min_delay.
 * 
 * If min_delay == max_delay, the device polls at a fixed frequency.
 * 
 * @param min_delay The minimum time between polls, in milliseconds. This value is
 * useful for controlling the throughput. Low values have increased throughput, but use
 * more power and may lead to unnecessary polls. Low values also lead to a higher number of
 * total polls until max_delay is reached.
 * @param max_delay The maximum time between polls, in milliseconds. This is useful for
 * controlling the maximum latency: the lower this is, the more responsive a device becomes at
 * the expense of power consumption.
 */
void SED_InitPolling(uint32_t min_delay, uint32_t max_delay);

/**
 * @brief Set the maximum delay between polls.
 * 
 * This function can be used after initialisation, to dynamically modify the polling rate
 * of the device.
 * 
 * @param max_delay The maximum time between polls, in milliseconds. This is useful for
 * controlling the latency: the lower this is, the more responsive a device becomes at
 * the expense of power consumption.
 */
void SED_SetMaxDelay(uint32_t max_delay);

/**
 * @brief Set the minimum delay between polls.
 * 
 * This function can be used after initialisation, to dynamically modify the polling rate
 * of the device.
 * 
 * @param min_delay The minimum time between polls, in milliseconds. This value is
 * useful for controlling the throughput. Low values have increased throughput, but use
 * more power and may lead to unnecessary polls. Low values also lead to a higher number of
 * total polls until max_delay is reached.
 */
void SED_SetMinDelay(uint32_t min_delay);

/**
 * @brief Poll immediately and reset the delay.
 * 
 * Use this function if you expect your parent to already have a packet destined for you.
 * Calling this function resets the time between polls to min_delay, after which the time
 * between polls increases exponentially.
 */
void SED_PollNow();

/**
 * @brief Schedule a poll to occur after the configured min_delay
 * 
 * Use this function if you expect your parent to have a packet for you 'soon.' For example,
 * this function is useful if you are sending a CoAP request to which you expect a response.
 * Calling this function resets the time between polls to min_delay, after which the time
 * between polls increases exponentially.
 */
void SED_PollSoon();

#endif