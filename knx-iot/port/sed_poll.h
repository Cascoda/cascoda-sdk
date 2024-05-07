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
 * until it reaches max_delay. Useful polls (polls that result in a message being
 * received by the KNX-IoT Stack) reset the delay to the value of min_delay.
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
 * @param max_retries After max_retries unsuccessful polls in a row, the device stops polling
 * until reset using SED_PollNow or SED_PollSoon. If set to zero, the device polls forever.
 */
void SED_InitPolling(uint32_t min_delay, uint32_t max_delay, uint32_t max_retries);

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
 * @brief Poll immediately without resetting the delay
 * 
 * Use this function if you only want a single poll, with no retries if you do not receive
 * a message. This is useful if you are opportunistically polling without necessarily expecting
 * a message (e.g. the device has woken up to take a measurement, and it would save power to
 * receive other comms right now, as opposed to using the later scheduled poll)
 * 
 * If a message is received after the poll, more polls shall follow according to
 * the min_delay set during initialisation. If no message is received, the exponential backoffs
 * proceed as usual (e.g. polling after max_delay)
 * 
 */
void SED_PollNowNoReset();

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