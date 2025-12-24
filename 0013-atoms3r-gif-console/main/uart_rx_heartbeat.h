#pragma once

// Non-invasive UART RX heartbeat (GPIO edge counter on the configured UART RX GPIO).
// Used to answer: "Is the device physically seeing activity on RX?"
void uart_rx_heartbeat_start(void);


