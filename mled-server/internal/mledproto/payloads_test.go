package mledproto

import "testing"

func TestPayloadSizes(t *testing.T) {
	if PatternConfigSize != 20 {
		t.Fatalf("PatternConfigSize=%d, want 20", PatternConfigSize)
	}
	if CuePrepareSize != 28 {
		t.Fatalf("CuePrepareSize=%d, want 28", CuePrepareSize)
	}
	if CueFireSize != 4 {
		t.Fatalf("CueFireSize=%d, want 4", CueFireSize)
	}
	if PongSize != 43 {
		t.Fatalf("PongSize=%d, want 43", PongSize)
	}
	if AckSize != 8 {
		t.Fatalf("AckSize=%d, want 8", AckSize)
	}
	if TimeRespSize != 12 {
		t.Fatalf("TimeRespSize=%d, want 12", TimeRespSize)
	}
}

func TestCuePrepareRoundTrip(t *testing.T) {
	var pat PatternConfig
	pat.PatternType = PatternRainbow
	pat.BrightnessPct = 90
	pat.Seed = 0xAABBCCDD
	pat.Data[0] = 1
	pat.Data[1] = 2
	pat.Data[2] = 3

	cp := CuePrepare{
		CueID:     42,
		FadeInMS:  10,
		FadeOutMS: 20,
		Pattern:   pat,
	}
	buf := make([]byte, CuePrepareSize)
	if err := cp.MarshalTo(buf); err != nil {
		t.Fatalf("MarshalTo: %v", err)
	}
	got, err := UnmarshalCuePrepare(buf)
	if err != nil {
		t.Fatalf("UnmarshalCuePrepare: %v", err)
	}
	if got.CueID != cp.CueID || got.Pattern.PatternType != cp.Pattern.PatternType || got.Pattern.Seed != cp.Pattern.Seed {
		t.Fatalf("roundtrip mismatch: got=%+v want=%+v", got, cp)
	}
}
