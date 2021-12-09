#include "shift.h"

// Static elements of headers as shift register values.
const ShiftRegister block_header_template  = { 0x00003141UL, 0x59265359UL };
const ShiftRegister block_endmark_template = { 0x00001772UL, 0x45385090UL };

void ShiftRegister_append(ShiftRegister *shift_register, int bit) {
    shift_register->senior = (shift_register->senior << 1) | (shift_register->junior >> 31);
    shift_register->junior = (shift_register->junior << 1) | (bit & 1);  
}

bool ShiftRegister_equal_with_senior_mask(const ShiftRegister *a, const ShiftRegister *b, uint32_t senior_mask) {
    return (a->junior == b->junior) && ((a->senior & senior_mask) == (b->senior & senior_mask));
}

int ShiftRegister_junior_byte(const ShiftRegister *shift_register) {
    return shift_register->junior & 0x000000ff;
}