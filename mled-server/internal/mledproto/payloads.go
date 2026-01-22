package mledproto

import (
	"encoding/binary"
	"errors"
)

const (
	PatternConfigSize = 20
	CuePrepareSize    = 28
	CueFireSize       = 4
	PongSize          = 43
	AckSize           = 8
	TimeRespSize      = 12
)

type PatternConfig struct {
	PatternType   PatternType
	BrightnessPct uint8
	Flags         uint8
	Reserved0     uint8
	Seed          uint32
	Data          [12]byte
}

func (p PatternConfig) MarshalTo(dst []byte) error {
	if len(dst) < PatternConfigSize {
		return errors.New("short dst")
	}
	dst[0] = uint8(p.PatternType)
	dst[1] = p.BrightnessPct
	dst[2] = p.Flags
	dst[3] = p.Reserved0
	binary.LittleEndian.PutUint32(dst[4:8], p.Seed)
	copy(dst[8:20], p.Data[:])
	return nil
}

func UnmarshalPatternConfig(b []byte) (PatternConfig, error) {
	var p PatternConfig
	if len(b) < PatternConfigSize {
		return p, errors.New("short pattern config")
	}
	p.PatternType = PatternType(b[0])
	p.BrightnessPct = b[1]
	p.Flags = b[2]
	p.Reserved0 = b[3]
	p.Seed = binary.LittleEndian.Uint32(b[4:8])
	copy(p.Data[:], b[8:20])
	return p, nil
}

type CuePrepare struct {
	CueID     uint32
	FadeInMS  uint16
	FadeOutMS uint16
	Pattern   PatternConfig
}

func (c CuePrepare) MarshalTo(dst []byte) error {
	if len(dst) < CuePrepareSize {
		return errors.New("short dst")
	}
	binary.LittleEndian.PutUint32(dst[0:4], c.CueID)
	binary.LittleEndian.PutUint16(dst[4:6], c.FadeInMS)
	binary.LittleEndian.PutUint16(dst[6:8], c.FadeOutMS)
	return c.Pattern.MarshalTo(dst[8:28])
}

func UnmarshalCuePrepare(b []byte) (CuePrepare, error) {
	var c CuePrepare
	if len(b) < CuePrepareSize {
		return c, errors.New("short cue prepare")
	}
	c.CueID = binary.LittleEndian.Uint32(b[0:4])
	c.FadeInMS = binary.LittleEndian.Uint16(b[4:6])
	c.FadeOutMS = binary.LittleEndian.Uint16(b[6:8])
	p, err := UnmarshalPatternConfig(b[8:28])
	if err != nil {
		return c, err
	}
	c.Pattern = p
	return c, nil
}

type CueFire struct {
	CueID uint32
}

func (c CueFire) MarshalTo(dst []byte) error {
	if len(dst) < CueFireSize {
		return errors.New("short dst")
	}
	binary.LittleEndian.PutUint32(dst[0:4], c.CueID)
	return nil
}

func UnmarshalCueFire(b []byte) (CueFire, error) {
	var c CueFire
	if len(b) < CueFireSize {
		return c, errors.New("short cue fire")
	}
	c.CueID = binary.LittleEndian.Uint32(b[0:4])
	return c, nil
}

type Pong struct {
	UptimeMS        uint32
	RSSIDbm         int8
	StateFlags      uint8
	BrightnessPct   uint8
	PatternType     PatternType
	FrameMS         uint16
	ActiveCueID     uint32
	ControllerEpoch uint32
	ShowMSNow       uint32
	Name            [16]byte
	Reserved        [5]byte
}

func UnmarshalPong(b []byte) (Pong, error) {
	var p Pong
	if len(b) < PongSize {
		return p, errors.New("short pong")
	}
	p.UptimeMS = binary.LittleEndian.Uint32(b[0:4])
	p.RSSIDbm = int8(b[4])
	p.StateFlags = b[5]
	p.BrightnessPct = b[6]
	p.PatternType = PatternType(b[7])
	p.FrameMS = binary.LittleEndian.Uint16(b[8:10])
	p.ActiveCueID = binary.LittleEndian.Uint32(b[10:14])
	p.ControllerEpoch = binary.LittleEndian.Uint32(b[14:18])
	p.ShowMSNow = binary.LittleEndian.Uint32(b[18:22])
	copy(p.Name[:], b[22:38])
	copy(p.Reserved[:], b[38:43])
	return p, nil
}

type Ack struct {
	AckForMsgID uint32
	Code        uint16
	Reserved    uint16
}

func UnmarshalAck(b []byte) (Ack, error) {
	var a Ack
	if len(b) < AckSize {
		return a, errors.New("short ack")
	}
	a.AckForMsgID = binary.LittleEndian.Uint32(b[0:4])
	a.Code = binary.LittleEndian.Uint16(b[4:6])
	a.Reserved = binary.LittleEndian.Uint16(b[6:8])
	return a, nil
}

type TimeResp struct {
	ReqMsgID       uint32
	MasterRxShowMS uint32
	MasterTxShowMS uint32
}

func (t TimeResp) MarshalTo(dst []byte) error {
	if len(dst) < TimeRespSize {
		return errors.New("short dst")
	}
	binary.LittleEndian.PutUint32(dst[0:4], t.ReqMsgID)
	binary.LittleEndian.PutUint32(dst[4:8], t.MasterRxShowMS)
	binary.LittleEndian.PutUint32(dst[8:12], t.MasterTxShowMS)
	return nil
}
