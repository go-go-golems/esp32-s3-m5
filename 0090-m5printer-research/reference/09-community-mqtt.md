- **[ddavitt](https://community.m5stack.com/user/ddavitt)**
	  
	Hi, I recently got the Atom Thermal printer and I’ve been trying to send an MQTT payload from NodeRED running on a Pi (just using inject node to test). I have it connecting but if I just send a string through as the payload nothing happens. If I send a string along with a print command it prints but it includes the command (see picture), any ideas how I format the payload so it can print a string?
	![0_1639178641784_IMG_0437.jpeg](https://community.m5stack.com/assets/uploads/files/1639178643350-img_0437.jpeg)
- **[schmid01](https://community.m5stack.com/user/schmid01)**
	  
	[@ddavitt](https://community.m5stack.com/uid/8117) Sounds like an interesting Project. Can you post the Code?
- **[ddavitt](https://community.m5stack.com/user/ddavitt)**
	  
	[@schmid01](https://community.m5stack.com/uid/11474) I hope it will become quite an interesting project once I get things working. In the long run I want to send order details from my Etsy store straight to the printer using Etsy's API. I've blacked out my printers MAC address from the screenshots for obvious reasons. I've tried adding it to msg.payload.command as well as I seem to remember that's how you send different variables in a payload but it didn't work. I'm a bit rusty with MQTT as its been a couple of years since I've used it. M5Stack list all the commands on the product page but they don't really explain how to issue them via MQTT and the bulk of the instructions are in Chinese. ![0_1639346051371_NodeRed.png](https://community.m5stack.com/assets/uploads/files/1639346056224-nodered-resized.png) ![0_1639346066649_Inject_node.png](https://community.m5stack.com/assets/uploads/files/1639346069988-inject_node-resized.png)
- **[m5stack](https://community.m5stack.com/user/m5stack)**
	  
	[@ddavitt](https://community.m5stack.com/uid/8117) that command you used is for the UART communication. for the time being. MQTT only support you printing some String. also we will update the firmware at soon at possible to let it support more functions.
- **[ddavitt](https://community.m5stack.com/user/ddavitt)**
	  
	[@m5stack](https://community.m5stack.com/uid/1) thanks for letting me know. However If I just send a string it doesn’t print, it only prints if I add one of the UART commands to the string, is that a bug that will be fixed with a firmware update?
- **[ddavitt](https://community.m5stack.com/user/ddavitt)**
	  
	[@m5stack](https://community.m5stack.com/uid/1) I’ve decided to give up on using MQTT and send commands direct to the printer from the atom but I’m struggling to figure out the right way to send commands via UART in UIFlow. Is it possible to get a simple “hello world” UART UIFlow example for the thermal printer?
- **[ajb2k3](https://community.m5stack.com/user/ajb2k3)**
	  
	This is what I wanted to try but I don't have a printer yet.