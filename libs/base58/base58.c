#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static const char* BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

char* base58encode(const uint8_t* input, size_t len) {
    size_t size = len * 138 / 100 + 1;

    uint8_t* buffer = calloc(size, 1);
    if (!buffer) return NULL;

    size_t i, j, zero_count = 0;
    while (zero_count < len && input[zero_count] == 0) {
        zero_count++;
    }

    //size_t length = 0;
    for (i = zero_count; i < len; i++) {
        int carry = input[i];
        for (j = size; j-- > 0;) {
            carry += 256 * buffer[j];
            buffer[j] = carry % 58;
            carry /= 58;
        }
    }

    i = 0;
    while (i < size && buffer[i] == 0) {
        i++;
    }

    size_t result_len = zero_count + (size - i);
    char* result = malloc(result_len + 1);
    if (!result) {
        free(buffer);
        return NULL;
    }

    memset(result, '1', zero_count);

    j = zero_count;
    while (i < size) {
        result[j++] = BASE58_ALPHABET[buffer[i++]];
    }
    result[result_len] = '\0';

    free(buffer);
    return result;
}

int base58_char_to_value(char c) {
    const char* ptr = strchr(BASE58_ALPHABET, c);
    if (ptr) return ptr - BASE58_ALPHABET;
    return -1;  // invalid character
}

int base58decode(const char* input, uint8_t output[10]) {
    const int input_len = 14;
    const int output_len = 10;

    memset(output, 0, output_len);

    for (int i = 0; i < input_len; ++i) {
        int val = base58_char_to_value(input[i]);
        if (val < 0) {
            return 1; // invalid base58 character
        }

        int carry = val;
        for (int j = output_len - 1; j >= 0; --j) {
            int temp = output[j] * 58 + carry;
            output[j] = temp & 0xFF;
            carry = temp >> 8;
        }

        if (carry != 0) {
            return 1; // overflow
        }
    }

    return 0; // success
}