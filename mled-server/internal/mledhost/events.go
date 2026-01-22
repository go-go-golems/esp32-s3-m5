package mledhost

type EventType string

const (
	EventNodeUpdate  EventType = "node.update"
	EventNodeOffline EventType = "node.offline"
	EventApplyAck    EventType = "apply.ack"
	EventError       EventType = "error"
)

type Event struct {
	Type    EventType
	Payload any
	Error   error
}
