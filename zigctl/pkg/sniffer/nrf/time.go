package nrf

import "time"

type TimeCorrector struct {
	initialized        bool
	firstLocalMicros   int64
	firstSnifferMicros int64
}

func (c *TimeCorrector) Correct(snifferMicros int64) int64 {
	if !c.initialized {
		c.firstLocalMicros = time.Now().UnixNano() / int64(time.Microsecond)
		c.firstSnifferMicros = snifferMicros
		c.initialized = true
		return c.firstLocalMicros
	}

	return c.firstLocalMicros - c.firstSnifferMicros + snifferMicros
}
