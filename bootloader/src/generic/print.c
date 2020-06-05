#include "print.h"


static const char hex_table[16] = {'0', '1', '2', '3', '4', '5', '6', '7', 
								   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

// Static functions
uint8_t print_uint_to_buffer(char* buffer, uint32_t value);
uint32_t power(uint8_t base, uint8_t exp);

uint16_t print_to_buffer_va(char* buffer, const char* data, va_list obj) {

	// Make some pointer to the data
	char* dest_ptr		= buffer;
	const char* src_ptr = data;
	
	uint16_t size = 0;	
	
	while (*src_ptr) {
		if (*src_ptr == '%') {
			src_ptr++;
			
			// Retrive the optional formatiing number
			uint8_t opt_fmt = 0;
			uint8_t opt_fmt_size = 0;
			const char* opt_fmt_ptr = src_ptr;
			
			while ((*opt_fmt_ptr >= '0') && (*opt_fmt_ptr <= '9')) {
				opt_fmt_ptr++;
				opt_fmt_size++;
			}
			while (opt_fmt_size--) {
				opt_fmt += (*src_ptr++ - '0') * power(10, opt_fmt_size);
			}
			
			// Check the formatting character
			switch (*src_ptr) {
				// String formatting
				case 's' : {
					char* fmt_string_ptr = va_arg(obj, char*);
					// We have a pointer to the string argument
					// Copy the strign to the buffer
					if (opt_fmt) {
						// Print a number of chars
						for (uint8_t i = 0; i < opt_fmt; i++) {
							if (*fmt_string_ptr == '\0') {
								break;
							}
							*dest_ptr = *fmt_string_ptr;
							
							dest_ptr++;
							fmt_string_ptr++;
							size++;
						}
					}
					else {
						while (*fmt_string_ptr) {
							*dest_ptr = *fmt_string_ptr;
							
							dest_ptr++;
							fmt_string_ptr++;
							size++;
						}
					}
					break;
				}
				// Char formatting
				case 'c' : {
					char fmt_char = (char)va_arg(obj, int);
					*dest_ptr = fmt_char;
					dest_ptr++;
					size++;
					break;
				}
				// Unsigned integer formatting
				case 'u' : {
				}
				// Signed integer formatting
				case 'd' : {
					int32_t fmt_int = (int32_t)va_arg(obj, int);
					
					// Handle the minus sign
					if (fmt_int < 0) {
						fmt_int *= -1;
						*dest_ptr++ = '-';
						size++;
					}
					uint8_t fmt_int_size = print_uint_to_buffer(dest_ptr, 
                        (uint32_t)fmt_int);
					
					dest_ptr += fmt_int_size;
					size += fmt_int_size;
					break;
				}
				// Binary formatting
				case 'b' : {
					*dest_ptr++ = '0';
					*dest_ptr++ = 'b';
					size += 2;
					
					uint32_t fmt_bin = (uint32_t)va_arg(obj, int);
					for (uint8_t i = opt_fmt; i --> 0;) {
						// Check if the bit is set
						if (fmt_bin & (1 << i)) {
							*dest_ptr = '1';
						}
						else {
							*dest_ptr = '0';
						}
						dest_ptr++;
						size++;
					}
					break;
				}
				// Hexadecimal formating
				case 'h' : {
					*dest_ptr++ = '0';
					*dest_ptr++ = 'x';
					size += 2;
					
					uint32_t fmt_hex = (uint32_t)va_arg(obj, int);
					
					for (uint8_t i = 0; i < opt_fmt; i++) {
						uint8_t byte = (uint8_t)
                            (fmt_hex >> ((opt_fmt - 1 - i) * 8));
						
						uint8_t byte_upper = (byte >> 4);
						uint8_t byte_lower = byte & 0b1111;
						
						*dest_ptr++ = hex_table[byte_upper];
						*dest_ptr++ = hex_table[byte_lower];
						size += 2;
					}
					break;
				}
			}
			
			src_ptr++;
		} else {
			*dest_ptr = *src_ptr;
			dest_ptr++;
			src_ptr++;
			size++;
		}
	}
	return size;
}

uint8_t print_uint_to_buffer(char* buffer, uint32_t value) {
	uint8_t size = 0;
	char int_buffer[10];
	char* int_buffer_ptr = int_buffer;
	
	// This gets all characters except the last one
	while (value / 10) {
		*int_buffer_ptr = '0' + (value % 10);
		value /= 10;
		size++;
		int_buffer_ptr++;
	};
	
	// Get the last character
	*int_buffer_ptr = '0' + (value % 10);
	size++;
	
	for (uint8_t i = 0; i < size; i++) {
		*buffer++ = *int_buffer_ptr--;
	}
	
	return size;
}

uint32_t power(uint8_t base, uint8_t exp) {
	uint32_t result = 1;
	for (uint8_t i = 0; i < exp; i++) {
		result *= base;
	}
	return result;
}