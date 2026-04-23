1

2

3

4

5

6

7

8

9

10

11

12

13

14

15

16

17

18

19

20

21

22

23

24

25

26

27

28

29

30

31

32

33

34

35

36

37

38

39

40

41

42

43

44

45

46

47

48

49

50

51

52

53

54

55

56

57

58

59

60

61

62

63

64

65

66

/\*

\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*

\* Equipped with Atom-Lite/Matrix sample source code

\* 配套 Atom-Lite/Matrix 示例源代码

\* Visit for more information: https://docs.m5stack.com/en/atom/atomic\_qr

\* 获取更多资料请访问：https://docs.m5stack.com/zh\_CN/atom/atomic\_qr

\*

\* Product: ATOM QR-CODE UART control.

\* Date: 2021/8/30

\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*

Press the button to scan, and the scan result will be printed out through

Serial. More Info pls refer: \[QR module serial control command

list\](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/atombase/AtomicQR/ATOM\_QRCODE\_CMD\_EN.pdf)

\*/

#include <M5Atom.h>

uint8\_t wakeup\_cmd = 0x00;

uint8\_t start\_scan\_cmd\[\] = {0x04, 0xE4, 0x04, 0x00, 0xFF, 0x14};

uint8\_t stop\_scan\_cmd\[\] = {0x04, 0xE5, 0x04, 0x00, 0xFF, 0x13};

uint8\_t host\_mode\_cmd\[\] = {0x07, 0xC6, 0x04, 0x08, 0x00,

0x8A, 0x08, 0xFE, 0x95};

uint8\_t ack\_cmd\[\] = {0x04, 0xD0, 0x00, 0x00, 0xFF, 0x2C};

#define TRIG 23

void setup() {

M5.begin(true, false, true);

Serial2.begin(

9600, SERIAL\_8N1, 22,

19); // Set the baud rate of serial port 2 to 115200,8 data bits, no

// parity bits, and 1 stop bit, and set RX to 22 and TX to 19.

// 设置串口二的波特率为115200,8位数据位,没有校验位,1位停止位,并设置RX为22,TX为19

M5.dis.fillpix(0xfff000); // YELLOW 黄色

delay(1000);

Serial2.write(wakeup\_cmd);

delay(50);

Serial2.write(host\_mode\_cmd, sizeof(host\_mode\_cmd));

}

void loop() {

M5.update();

if (M5.Btn.isPressed()) {

M5.dis.fillpix(0x0000ff);

Serial2.write(wakeup\_cmd);

delay(50);

Serial2.write(start\_scan\_cmd, sizeof(start\_scan\_cmd));

}

if (Serial.available()) {

Serial2.write(wakeup\_cmd);

delay(50);

while (Serial.available()) {

Serial2.write(Serial.read());

}

}

if (Serial2.available()) {

int ch = Serial2.read();

Serial.write(ch);

}

}