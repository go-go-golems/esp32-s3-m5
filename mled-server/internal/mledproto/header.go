package mledproto

import (
	"encoding/binary"
	"errors"
	"fmt"
)

type Header struct {
	Magic       [4]byte
	Version     uint8
	Type        MessageType
	Flags       uint8
	HdrLen      uint8
	EpochID     uint32
	MsgID       uint32
	SenderID    uint32
	Target      uint32
	ExecuteAtMS uint32
	PayloadLen  uint16
	Reserved    uint16
}

func NewHeader(t MessageType) Header {
	return Header{
		Magic:   Magic,
		Version: Version,
		Type:    t,
		HdrLen:  HeaderSize,
	}
}

func (h Header) TargetMode() TargetMode {
	return TargetMode(h.Flags & 0x03)
}

func (h *Header) SetTargetMode(mode TargetMode) {
	h.Flags = (h.Flags &^ 0x03) | (uint8(mode) & 0x03)
}

func (h Header) AckReq() bool {
	return (h.Flags & FlagAckReq) != 0
}

func (h *Header) SetAckReq(req bool) {
	if req {
		h.Flags |= FlagAckReq
	} else {
		h.Flags &^= FlagAckReq
	}
}

func (h Header) Validate() error {
	if h.Magic != Magic {
		return errors.New("bad magic")
	}
	if h.Version != Version {
		return fmt.Errorf("bad version: %d", h.Version)
	}
	if h.HdrLen != HeaderSize {
		return fmt.Errorf("bad hdr_len: %d", h.HdrLen)
	}
	return nil
}

func UnmarshalHeader(b []byte) (Header, error) {
	var h Header
	if len(b) < HeaderSize {
		return h, errors.New("short header")
	}
	copy(h.Magic[:], b[0:4])
	h.Version = b[4]
	h.Type = MessageType(b[5])
	h.Flags = b[6]
	h.HdrLen = b[7]
	h.EpochID = binary.LittleEndian.Uint32(b[8:12])
	h.MsgID = binary.LittleEndian.Uint32(b[12:16])
	h.SenderID = binary.LittleEndian.Uint32(b[16:20])
	h.Target = binary.LittleEndian.Uint32(b[20:24])
	h.ExecuteAtMS = binary.LittleEndian.Uint32(b[24:28])
	h.PayloadLen = binary.LittleEndian.Uint16(b[28:30])
	h.Reserved = binary.LittleEndian.Uint16(b[30:32])
	return h, h.Validate()
}

func (h Header) MarshalTo(dst []byte) error {
	if len(dst) < HeaderSize {
		return errors.New("short dst")
	}
	copy(dst[0:4], h.Magic[:])
	dst[4] = h.Version
	dst[5] = byte(h.Type)
	dst[6] = h.Flags
	dst[7] = h.HdrLen
	binary.LittleEndian.PutUint32(dst[8:12], h.EpochID)
	binary.LittleEndian.PutUint32(dst[12:16], h.MsgID)
	binary.LittleEndian.PutUint32(dst[16:20], h.SenderID)
	binary.LittleEndian.PutUint32(dst[20:24], h.Target)
	binary.LittleEndian.PutUint32(dst[24:28], h.ExecuteAtMS)
	binary.LittleEndian.PutUint16(dst[28:30], h.PayloadLen)
	binary.LittleEndian.PutUint16(dst[30:32], h.Reserved)
	return nil
}
