package mledtime

import "testing"

func TestDuration_NoWrap(t *testing.T) {
	if got := Duration(100, 150); got != 50 {
		t.Fatalf("Duration(100,150)=%d, want 50", got)
	}
}

func TestDuration_WrapNegative(t *testing.T) {
	// start near max, end near 0 => small positive duration.
	start := uint32(0xFFFFFFF0)
	end := uint32(0x00000010)
	if got := Duration(start, end); got != 0x20 {
		t.Fatalf("Duration(wrap)=%d, want 32", got)
	}
}

func TestDiff_NoWrap(t *testing.T) {
	if got := Diff(150, 100); got != 50 {
		t.Fatalf("Diff(150,100)=%d, want 50", got)
	}
}

func TestDiff_WrapNegative(t *testing.T) {
	// a small number minus a large number => negative small diff.
	a := uint32(0x00000010)
	b := uint32(0xFFFFFFF0)
	if got := Diff(a, b); got != 0x20 {
		t.Fatalf("Diff(wrap)=%d, want 32", got)
	}
}

func TestIsDue(t *testing.T) {
	if !IsDue(100, 100) {
		t.Fatalf("IsDue equal should be true")
	}
	if !IsDue(101, 100) {
		t.Fatalf("IsDue past should be true")
	}
	if IsDue(100, 101) {
		t.Fatalf("IsDue future should be false")
	}
}
