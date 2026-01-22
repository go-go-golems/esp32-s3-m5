package mledproto

import "testing"

func TestHeaderRoundTrip(t *testing.T) {
	h := NewHeader(MsgPing)
	h.EpochID = 123
	h.MsgID = 456
	h.SenderID = 0
	h.SetTargetMode(TargetAll)
	h.ExecuteAtMS = 789
	h.PayloadLen = 0

	var buf [HeaderSize]byte
	if err := h.MarshalTo(buf[:]); err != nil {
		t.Fatalf("MarshalTo: %v", err)
	}
	got, err := UnmarshalHeader(buf[:])
	if err != nil {
		t.Fatalf("UnmarshalHeader: %v", err)
	}
	if got.EpochID != h.EpochID || got.MsgID != h.MsgID || got.ExecuteAtMS != h.ExecuteAtMS {
		t.Fatalf("roundtrip mismatch: got=%+v want=%+v", got, h)
	}
}
