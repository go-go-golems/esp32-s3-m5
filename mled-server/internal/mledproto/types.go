package mledproto

const (
	Version    = 1
	HeaderSize = 32
)

var Magic = [4]byte{'M', 'L', 'E', 'D'}

type MessageType uint8

const (
	MsgBeacon     MessageType = 0x01
	MsgHello      MessageType = 0x02
	MsgTimeReq    MessageType = 0x03
	MsgTimeResp   MessageType = 0x04
	MsgCuePrepare MessageType = 0x10
	MsgCueFire    MessageType = 0x11
	MsgCueCancel  MessageType = 0x12
	MsgPing       MessageType = 0x20
	MsgPong       MessageType = 0x21
	MsgAck        MessageType = 0x22
)

type TargetMode uint8

const (
	TargetAll   TargetMode = 0
	TargetNode  TargetMode = 1
	TargetGroup TargetMode = 2
)

type PatternType uint8

const (
	PatternOff       PatternType = 0
	PatternRainbow   PatternType = 1
	PatternChase     PatternType = 2
	PatternBreathing PatternType = 3
	PatternSparkle   PatternType = 4
)

const FlagAckReq uint8 = 0x04
