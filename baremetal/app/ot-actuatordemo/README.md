# cliapp-actuator-bm

A simple demo that demonstrates the control of several actuators (running cliapp-actuator-bm), by a central controller (also running cliapp-actuator-bm). This demo provides a CLI for interaction (serial-adapter). The controller can individually set and request two variables owned by each actuator, and those are `brightness` and `colour_mix`.

## Configuring a Chili as a controller or actuator

1- Connect the Chili to a Windows or Linux machine and run the file `serial-adapter.exe`, or `serial-adapter`. This will open up the CLI.

2- Type the command `actuatordemo actuator` or `actuatordemo controller` depending on which role you want the Chili to take.

## Connecting the controller to the actuators

Connect the Chili configured as a controller to a Windows or Linux machine and run the CLI.

Once the controller and actuators are powered, the controller will automatically try to connect to any actuators within its range (note: this will happen even if `serial-adapter` is not running). It does this by sending a discovery message every 30 seconds, which means that it can take up to 30 seconds for the controller to connect to an actuator. However, it is possible to prompt the controller to immediately send a discovery message by typing the command `actuatordemo discover`. If there are actuators nearby, it will immediately connect to them. When the controller successfully connects to an actuator, it internally assigns an ID number to that actuator, and the message  `Actuator ID <ID number> connected.` is displayed on the CLI.

To display a list of all the currently connected actuators, as well as their ID number and IPv6 address, enter the command `actuatordemo children`.

If a currently connected actuator is no longer able to communicate with the controller for more than 60 seconds (e.g. it is powered off, out of range, no longer working...), then the controller will consider the actuator to be disconnected. When that happens, the controller removes the actuator from its list of children, and the message  `Actuator ID <ID number> has disconnected` is displayed on the CLI.

## Sending data to the actuators

The actuators have a "brightness" and "colour\_mix" of 0 by default. When the controller is connected to one or more actuators, enter the command `actuatordemo <actuator ID> brightness <0-255> colour_mix <0-255>` to send a message that prompts the targeted actuator to set its "brightness" and "colour\_mix" variables to the values provided (other variants of this command are available for convenience, see the table at the end of this document). If a value outside the bounds of 0-255 is provided, the command will be ignored. 

When the actuator receives the message, it sets its internal "brightness" and "colour\_mix" variables to the values requested, and stores them until they are either changed or the actuator disconnects. The actuator also immediately sends the "brightness" and "colour\_mix" values as UART data.

## Getting information from the actuators

Because the actuators store the values of "brightness" and "colour\_mix", the controller can request those values at any time to display them on the CLI. To do that, type the command `actuatordemo <actuator ID> actuator_info`.

## Table of all possible commands

Enter the command `actuatordemo help` on the CLI to get a similar list of all possible commands.

<table border="1">
		<tr>
		<th align="center">Commands</th>
		<th align="center">Description</th>
		</tr>
		<tr>
		<td colspan=2 align="left"><b>General commands</b></td>
		</tr>
		<tr>
		<td align="left">actuatordemo</td>
		<td align="left">Shows the current configuration of the<br>device (actuator/controller/stopped).</td>
		</tr>
		<tr>
		<td align="left">actuatordemo help</td>
		<td align="left">Displays a list of all possible commands.</td>
		</tr>
		<tr>
		<td align="left">actuatordemo actuator</td>
		<td align="left">Configures the device as an actuator.</td>
		</tr>
		<tr>
		<td align="left">actuatordemo controller</td>
		<td align="left">Configures the device as a controller.</td>
		</tr>
		<tr>
		<td align="left">actuatordemo stop</td>
		<td align="left">Stops the device’s operation.</td>
		</tr>
		<tr>
		<td colspan=2 align="left"><b>Controller-only commands</b></td>
		</tr>
		<tr>
		<td align="left">actuatordemo discover</td>
		<td align="left">Immediately sends a discover for<br>actuators.</td>
		</tr>
		<tr>
		<td align="left">actuatordemo children</td>
		<td align="left">Displays a list of all connected<br>actuators, their ID and their IPv6<br>address.</td>
		</tr>
		<tr>
		<td align="left">actuatordemo &ltactuator ID&gt actuator_info<br>actuatordemo &ltactuator ID&gt i</td>
		<td align="left">Requests the "brightness" and "colour_mix"<br>values from the actuator with the given 		ID<br>and displays them.</td>
		</tr>
		<tr>
		<td align="left">actuatordemo &ltactuator ID&gt brightness &lt0-255&gt<br>actuatordemo &ltactuator ID&gt b &lt0-       		255&gt</td>
		<td align="left">Sends a message to set the "brightness" of<br>the actuator with the given ID to the<br>provided 		value.</td>
		</tr>
		<tr>
		<td align="left">actuatordemo &ltactuator ID&gt colour_mix &lt0-255&gt<br>actuatordemo &ltactuator ID&gt c &lt0-		255&gt</td>
		<td align="left">Sends a message to set the "colour_mix" of<br>the actuator with the given ID to the<br>provided 		value.</td>
		</tr>
		<tr>
		<td align="left">actuatordemo &ltactuator ID&gt brightness &lt0-255&gt colour_mix &lt0-255&gt<br>actuatordemo &ltactuator ID&gt b &lt0-255&gt c &lt0-255&gt</td>
		<td align="left">This is a combination of the two previous commands.</td>
		</tr>
</table>
