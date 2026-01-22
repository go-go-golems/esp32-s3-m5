package mledtime

// Duration returns the signed (wrap-around-safe) duration from start to end,
// matching the behavior of mled_time_u32_duration in mledc/time.c.
func Duration(start, end uint32) int32 {
	diff := end - start
	if diff > 0x7FFFFFFF {
		return int32(int64(diff) - 0x100000000)
	}
	return int32(diff)
}

// Diff returns the signed (wrap-around-safe) difference a-b, matching
// mled_time_u32_diff in mledc/time.c.
func Diff(a, b uint32) int32 {
	diff := a - b
	if diff > 0x7FFFFFFF {
		return int32(int64(diff) - 0x100000000)
	}
	return int32(diff)
}

// IsDue returns true if executeAt is due at now (wrap-around-safe),
// matching mled_time_is_due in mledc/time.c.
func IsDue(now, executeAt uint32) bool {
	diff := now - executeAt
	return diff <= 0x7FFFFFFF
}
