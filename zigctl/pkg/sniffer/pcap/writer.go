package pcap

import (
	"bufio"
	"encoding/binary"
	"io"
	"time"
)

const (
	LinkTypeIEEE802154TAP   uint16 = 283
	LinkTypeIEEE802154NoFCS uint16 = 230
)

const (
	blockTypeSectionHeader   uint32 = 0x0A0D0D0A
	blockTypeInterfaceDesc   uint32 = 0x00000001
	blockTypeEnhancedPacket  uint32 = 0x00000006
	byteOrderMagic           uint32 = 0x1A2B3C4D
	pcapngMajor              uint16 = 1
	pcapngMinor              uint16 = 0
	sectionLengthUnspecified uint64 = 0xFFFFFFFFFFFFFFFF
)

type Writer struct {
	w *bufio.Writer
}

func NewWriter(out io.Writer, linkType uint16) (*Writer, error) {
	writer := &Writer{w: bufio.NewWriter(out)}
	if err := writer.writeSectionHeader(); err != nil {
		return nil, err
	}
	if err := writer.writeInterfaceDesc(linkType); err != nil {
		return nil, err
	}
	return writer, nil
}

func (w *Writer) writeSectionHeader() error {
	length := uint32(28)
	if err := binary.Write(w.w, binary.LittleEndian, blockTypeSectionHeader); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, length); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, byteOrderMagic); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, pcapngMajor); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, pcapngMinor); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, sectionLengthUnspecified); err != nil {
		return err
	}
	return binary.Write(w.w, binary.LittleEndian, length)
}

func (w *Writer) writeInterfaceDesc(linkType uint16) error {
	length := uint32(20)
	if err := binary.Write(w.w, binary.LittleEndian, blockTypeInterfaceDesc); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, length); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, linkType); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, uint16(0)); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, uint32(0)); err != nil {
		return err
	}
	return binary.Write(w.w, binary.LittleEndian, length)
}

func (w *Writer) WritePacket(ts time.Time, data []byte) error {
	captureLen := uint32(len(data))
	packetLen := captureLen
	paddedLen := captureLen
	if rem := captureLen % 4; rem != 0 {
		paddedLen += 4 - rem
	}

	blockLen := uint32(32) + paddedLen

	if err := binary.Write(w.w, binary.LittleEndian, blockTypeEnhancedPacket); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, blockLen); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, uint32(0)); err != nil {
		return err
	}

	tsMicros := ts.UnixNano() / int64(time.Microsecond)
	tsHigh := uint32(uint64(tsMicros) >> 32)
	tsLow := uint32(uint64(tsMicros) & 0xFFFFFFFF)
	if err := binary.Write(w.w, binary.LittleEndian, tsHigh); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, tsLow); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, captureLen); err != nil {
		return err
	}
	if err := binary.Write(w.w, binary.LittleEndian, packetLen); err != nil {
		return err
	}

	if _, err := w.w.Write(data); err != nil {
		return err
	}
	if paddedLen > captureLen {
		padding := make([]byte, paddedLen-captureLen)
		if _, err := w.w.Write(padding); err != nil {
			return err
		}
	}

	return binary.Write(w.w, binary.LittleEndian, blockLen)
}

func (w *Writer) Flush() error {
	return w.w.Flush()
}
