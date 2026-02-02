package nrf

import (
	"bufio"
	"context"
	"fmt"
	"strconv"
	"strings"

	"go.bug.st/serial"
)

type Session struct {
	PortName  string
	Baud      int
	Channel   int
	AutoSleep bool
	port      serial.Port
	reader    *bufio.Reader
	corrector *TimeCorrector
}

func Open(portName string, baud int, autoSleep bool) (*Session, error) {
	if baud == 0 {
		baud = 115200
	}

	port, err := serial.Open(portName, &serial.Mode{BaudRate: baud})
	if err != nil {
		return nil, err
	}

	return &Session{
		PortName:  portName,
		Baud:      baud,
		AutoSleep: autoSleep,
		port:      port,
		reader:    bufio.NewReader(port),
		corrector: &TimeCorrector{},
	}, nil
}

func (s *Session) Close() error {
	if s.port == nil {
		return nil
	}
	return s.port.Close()
}

func (s *Session) SendCommand(cmd string) error {
	if s.port == nil {
		return fmt.Errorf("serial port not open")
	}
	_, err := s.port.Write([]byte(cmd + "\r\n"))
	return err
}

func (s *Session) Start(channel int) error {
	s.Channel = channel
	if err := s.SendCommand("sleep"); err != nil {
		return err
	}
	if err := s.SendCommand("shell echo off"); err != nil {
		return err
	}
	if err := s.SendCommand(fmt.Sprintf("channel %d", channel)); err != nil {
		return err
	}
	if err := s.SendCommand("receive"); err != nil {
		return err
	}
	return nil
}

func (s *Session) Stop() error {
	if s.port == nil {
		return nil
	}
	return s.SendCommand("sleep")
}

func (s *Session) ReadPacket() (Packet, error) {
	if s.reader == nil {
		s.reader = bufio.NewReader(s.port)
	}
	if s.corrector == nil {
		s.corrector = &TimeCorrector{}
	}

	line, err := s.reader.ReadBytes('\n')
	if err != nil {
		return Packet{}, err
	}

	packet, err := ParseLine(line)
	if err != nil {
		return Packet{}, err
	}

	packet.Channel = s.Channel
	packet.CorrectedMicros = s.corrector.Correct(packet.TimestampMicros)
	return packet, nil
}

func (s *Session) ReadLine() (string, error) {
	if s.reader == nil {
		s.reader = bufio.NewReader(s.port)
	}
	line, err := s.reader.ReadString('\n')
	if err != nil {
		return "", err
	}
	return strings.TrimSpace(line), nil
}

func (s *Session) QueryChannel() (int, error) {
	if err := s.SendCommand("channel"); err != nil {
		return 0, err
	}

	for i := 0; i < 3; i++ {
		line, err := s.ReadLine()
		if err != nil {
			return 0, err
		}
		if strings.HasPrefix(line, "received:") || line == "" {
			continue
		}
		channel, err := strconv.Atoi(line)
		if err != nil {
			return 0, fmt.Errorf("parse channel response: %w", err)
		}
		return channel, nil
	}

	return 0, fmt.Errorf("no channel response received")
}

func (s *Session) Loop(ctx context.Context, handler func(Packet) error) error {
	if s.port == nil {
		return fmt.Errorf("serial port not open")
	}

	done := make(chan struct{})
	go func() {
		select {
		case <-ctx.Done():
			_ = s.Close()
		case <-done:
		}
	}()
	defer close(done)

	for {
		packet, err := s.ReadPacket()
		if err != nil {
			if ctx.Err() != nil {
				return ctx.Err()
			}
			return err
		}
		if err := handler(packet); err != nil {
			return err
		}
	}
}
