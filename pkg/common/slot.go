package common

const SLOT_NUM = 1024

//
// calculate payload hash slot
// @param payload random
// @return uint16  slot value (0~1023)
//
func StrToSlot(payload string) uint64 {
	return XXH3_64bits([]byte(payload)) % 1024
}
