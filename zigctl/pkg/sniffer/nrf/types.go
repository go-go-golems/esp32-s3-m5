package nrf

type Packet struct {
	Payload         []byte
	RSSI            int
	LQI             int
	TimestampMicros int64
	CorrectedMicros int64
	Channel         int
	RawLine         string
}

type DeviceInfo struct {
	PortName     string
	VID          string
	PID          string
	Manufacturer string
	Product      string
	SerialNumber string
}
