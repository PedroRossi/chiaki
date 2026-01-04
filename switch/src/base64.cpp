// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <base64.h>

#include <chiaki/base64.h>

std::string Base64Encode(const uint8_t *buf, size_t sz)
{
	size_t out_sz = (((4 * sz / 3) + 3) & ~3) + 1;
	char *out = new char[out_sz];
	if(!out)
		return std::string();
	ChiakiErrorCode err = chiaki_base64_encode(buf, sz, out, out_sz);
	if(err != CHIAKI_ERR_SUCCESS)
		return std::string();
	std::string r = out;
	delete[] out;
	return r;
}
