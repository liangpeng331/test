#ifndef G711_CODEC_H
#define G711_CODEC_H

#include <stdint.h>
#include <stddef.h>

// Placeholder: Assume PCM 16-bit input/output for these functions
// G.711 compresses 16-bit linear PCM to 8-bit G.711 (A-law or u-law)

/**
 * @brief Encodes a buffer of 16-bit linear PCM samples to G.711 A-law.
 *
 * @param pcm_in Pointer to the input PCM data buffer.
 * @param g711_out Pointer to the output G.711 A-law data buffer.
 * @param pcm_sample_count Number of PCM samples in the input buffer.
 * @return Number of G.711 bytes produced (should be pcm_sample_count).
 */
size_t g711_encode_alaw(const int16_t *pcm_in, uint8_t *g711_out, size_t pcm_sample_count);

/**
 * @brief Decodes a buffer of G.711 A-law samples to 16-bit linear PCM.
 *
 * @param g711_in Pointer to the input G.711 A-law data buffer.
 * @param pcm_out Pointer to the output PCM data buffer.
 * @param g711_byte_count Number of G.711 bytes in the input buffer.
 * @return Number of PCM samples produced (should be g711_byte_count).
 */
size_t g711_decode_alaw(const uint8_t *g711_in, int16_t *pcm_out, size_t g711_byte_count);

/**
 * @brief Encodes a buffer of 16-bit linear PCM samples to G.711 u-law.
 *
 * @param pcm_in Pointer to the input PCM data buffer.
 * @param g711_out Pointer to the output G.711 u-law data buffer.
 * @param pcm_sample_count Number of PCM samples in the input buffer.
 * @return Number of G.711 bytes produced (should be pcm_sample_count).
 */
size_t g711_encode_ulaw(const int16_t *pcm_in, uint8_t *g711_out, size_t pcm_sample_count);

/**
 * @brief Decodes a buffer of G.711 u-law samples to 16-bit linear PCM.
 *
 * @param g711_in Pointer to the input G.711 u-law data buffer.
 * @param pcm_out Pointer to the output PCM data buffer.
 * @param g711_byte_count Number of G.711 bytes in the input buffer.
 * @return Number of PCM samples produced (should be g711_byte_count).
 */
size_t g711_decode_ulaw(const uint8_t *g711_in, int16_t *pcm_out, size_t g711_byte_count);

#endif // G711_CODEC_H
