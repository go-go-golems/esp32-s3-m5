package pcap

import (
	"io"
	"time"

	"github.com/google/gopacket"
	"github.com/google/gopacket/layers"
	"github.com/google/gopacket/pcapgo"
)

const (
	LinkTypeIEEE802154TAP   layers.LinkType = 283
	LinkTypeIEEE802154NoFCS layers.LinkType = 230
)

type Writer struct {
	ng *pcapgo.NgWriter
}

func NewWriter(out io.Writer, linkType layers.LinkType) (*Writer, error) {
	ng, err := pcapgo.NewNgWriter(out, linkType)
	if err != nil {
		return nil, err
	}
	return &Writer{ng: ng}, nil
}

func (w *Writer) WritePacket(ts time.Time, data []byte) error {
	ci := gopacket.CaptureInfo{
		Timestamp:     ts,
		CaptureLength: len(data),
		Length:        len(data),
	}
	return w.ng.WritePacket(ci, data)
}

func (w *Writer) Flush() error {
	return w.ng.Flush()
}
