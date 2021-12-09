#pragma once

#include <stdint.h>
#include <stdbool.h>

// Shift register: tracks 64 most recently read bits.
typedef struct {
    uint32_t senior;
    uint32_t junior;
} ShiftRegister;

// Static elements of headers as shift register values.
extern const ShiftRegister block_header_template;
extern const ShiftRegister block_endmark_template;

void ShiftRegister_append(ShiftRegister *shift_register, int bit);
bool ShiftRegister_equal_with_senior_mask(const ShiftRegister *a, const ShiftRegister *b, uint32_t senior_mask);
int ShiftRegister_junior_byte(const ShiftRegister *shift_register);