/* Teeworlds -> OUYA: bundled SHA-256.
 *
 * Teeworlds 0.7 has no bundled SHA-256 of its own -- upstream links it from
 * OpenSSL or LibTomCrypt. On the OUYA we link neither, so this file provides
 * the three functions base/hash_ctxt.h declares, against the LibTomCrypt-style
 * SHA256_CTX layout it defines: { uint64_t length(bits); uint32_t state[8];
 * uint32_t curlen(bytes buffered); unsigned char buf[64]; }.
 *
 * Public-domain SHA-256 (FIPS 180-4), standalone, no external deps.
 */

#include <base/hash_ctxt.h>   /* SHA256_CTX, SHA256_DIGEST, the 3 prototypes */
#include <string.h>

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define BSIG0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define BSIG1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SSIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SSIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static const uint32_t K[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void sha256_compress(SHA256_CTX *ctxt, const unsigned char *p)
{
	uint32_t w[64];
	uint32_t a, b, c, d, e, f, g, h, t1, t2;
	int i;

	for(i = 0; i < 16; i++)
		w[i] = ((uint32_t)p[i*4] << 24) | ((uint32_t)p[i*4+1] << 16)
			| ((uint32_t)p[i*4+2] << 8) | ((uint32_t)p[i*4+3]);
	for(i = 16; i < 64; i++)
		w[i] = SSIG1(w[i-2]) + w[i-7] + SSIG0(w[i-15]) + w[i-16];

	a = ctxt->state[0]; b = ctxt->state[1]; c = ctxt->state[2]; d = ctxt->state[3];
	e = ctxt->state[4]; f = ctxt->state[5]; g = ctxt->state[6]; h = ctxt->state[7];

	for(i = 0; i < 64; i++)
	{
		t1 = h + BSIG1(e) + CH(e, f, g) + K[i] + w[i];
		t2 = BSIG0(a) + MAJ(a, b, c);
		h = g; g = f; f = e; e = d + t1;
		d = c; c = b; b = a; a = t1 + t2;
	}

	ctxt->state[0] += a; ctxt->state[1] += b; ctxt->state[2] += c; ctxt->state[3] += d;
	ctxt->state[4] += e; ctxt->state[5] += f; ctxt->state[6] += g; ctxt->state[7] += h;
}

void sha256_init(SHA256_CTX *ctxt)
{
	ctxt->length = 0;
	ctxt->curlen = 0;
	ctxt->state[0] = 0x6a09e667; ctxt->state[1] = 0xbb67ae85;
	ctxt->state[2] = 0x3c6ef372; ctxt->state[3] = 0xa54ff53a;
	ctxt->state[4] = 0x510e527f; ctxt->state[5] = 0x9b05688c;
	ctxt->state[6] = 0x1f83d9ab; ctxt->state[7] = 0x5be0cd19;
}

void sha256_update(SHA256_CTX *ctxt, const void *data, size_t data_len)
{
	const unsigned char *p = (const unsigned char *)data;
	while(data_len--)
	{
		ctxt->buf[ctxt->curlen++] = *p++;
		if(ctxt->curlen == 64)
		{
			sha256_compress(ctxt, ctxt->buf);
			ctxt->length += 512;   /* length tracked in bits */
			ctxt->curlen = 0;
		}
	}
}

SHA256_DIGEST sha256_finish(SHA256_CTX *ctxt)
{
	SHA256_DIGEST digest;
	uint64_t total_bits;
	int i;

	total_bits = ctxt->length + (uint64_t)ctxt->curlen * 8;

	/* append 0x80 then pad with zeroes, leaving room for the 64-bit length */
	ctxt->buf[ctxt->curlen++] = 0x80;
	if(ctxt->curlen > 56)
	{
		while(ctxt->curlen < 64)
			ctxt->buf[ctxt->curlen++] = 0;
		sha256_compress(ctxt, ctxt->buf);
		ctxt->curlen = 0;
	}
	while(ctxt->curlen < 56)
		ctxt->buf[ctxt->curlen++] = 0;

	for(i = 7; i >= 0; i--)
		ctxt->buf[ctxt->curlen++] = (unsigned char)(total_bits >> (i*8));
	sha256_compress(ctxt, ctxt->buf);

	for(i = 0; i < 8; i++)
	{
		digest.data[i*4]   = (unsigned char)(ctxt->state[i] >> 24);
		digest.data[i*4+1] = (unsigned char)(ctxt->state[i] >> 16);
		digest.data[i*4+2] = (unsigned char)(ctxt->state[i] >> 8);
		digest.data[i*4+3] = (unsigned char)(ctxt->state[i]);
	}
	return digest;
}
