/*
 * Copyright (c) 2010-2025 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "crypt.h"
#include <cppcodec/base64_rfc4648.hpp>

#include "framework/core/graphicalapplication.h"
#include "framework/core/resourcemanager.h"
#include "framework/platform/platform.h"
#include "framework/stdext/math.h"

#ifndef USE_GMP
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>
#include <openssl/core_names.h>
#include <openssl/err.h>
#endif

constexpr std::size_t CHECKSUM_BYTES = sizeof(uint32_t);

Crypt g_crypt;

Crypt::Crypt()
{
#ifdef USE_GMP
    mpz_init(m_p);
    mpz_init(m_q);
    mpz_init(m_d);
    mpz_init(m_e);
    mpz_init(m_n);
#else
    m_pkey = nullptr;
    m_bn_n = nullptr;
    m_bn_e = nullptr;
    m_bn_d = nullptr;
    m_bn_p = nullptr;
    m_bn_q = nullptr;
    m_hasPrivateKey = false;
#endif
}

Crypt::~Crypt()
{
#ifdef USE_GMP
    mpz_clear(m_p);
    mpz_clear(m_q);
    mpz_clear(m_n);
    mpz_clear(m_d);
    mpz_clear(m_e);
#else
    EVP_PKEY_free(m_pkey);
    BN_free(m_bn_n);
    BN_free(m_bn_e);
    BN_free(m_bn_d);
    BN_free(m_bn_p);
    BN_free(m_bn_q);
#endif
}

std::string Crypt::base64Encode(const std::string& decoded_string) {
    return cppcodec::base64_rfc4648::encode(decoded_string);
}

std::string Crypt::base64Decode(const std::string_view& encoded_string) {
    try {
        return cppcodec::base64_rfc4648::decode<std::string>(encoded_string);
    } catch (const std::invalid_argument&) {
        return {};
    }
}

void Crypt::xorCrypt(std::string& buffer, const std::string& key)
{
    if (key.empty())
        return;

    const size_t keySize = key.size();
    for (size_t i = 0; i < buffer.size(); ++i)
        buffer[i] ^= key[i % keySize];
}

std::string Crypt::genUUID() {
    return uuids::to_string(uuids::uuid_random_generator{ stdext::random_gen() }());
}

bool Crypt::setMachineUUID(std::string uuidstr)
{
    if (uuidstr.empty())
        return false;

    uuidstr = _decrypt(uuidstr, false);

    if (uuidstr.length() != 36)
        return false;

    m_machineUUID = uuids::uuid::from_string(uuidstr).value();

    return true;
}

std::string Crypt::getMachineUUID()
{
    if (m_machineUUID.is_nil()) {
        m_machineUUID = uuids::uuid_random_generator{ stdext::random_gen() }();
    }
    return _encrypt(uuids::to_string(m_machineUUID), false);
}

std::string Crypt::getCryptKey(const bool useMachineUUID) const
{
    static const uuids::uuid default_uuid{};
    const uuids::uuid& uuid = useMachineUUID ? m_machineUUID : default_uuid;

    const uuids::uuid u = uuids::uuid_name_generator(uuid)(
        g_app.getCompactName() + g_platform.getCPUName() + g_platform.getOSName() + g_resources.getUserDir()
    );

    const std::size_t hash = std::hash<uuids::uuid>{}(u);

    return std::string(reinterpret_cast<const char*>(&hash), sizeof(hash));
}

std::string Crypt::_encrypt(const std::string& decrypted_string, const bool useMachineUUID)
{
    const uint32_t sum = stdext::computeChecksum(
        { reinterpret_cast<const uint8_t*>(decrypted_string.data()),
          decrypted_string.size() });

    std::string tmp(CHECKSUM_BYTES, '\0');
    tmp.append(decrypted_string);

    stdext::writeULE32(reinterpret_cast<uint8_t*>(tmp.data()), sum);

    const auto key = getCryptKey(useMachineUUID);
    xorCrypt(tmp, key);
    return base64Encode(tmp);
}

std::string Crypt::_decrypt(const std::string& encrypted_string, const bool useMachineUUID)
{
    std::string decoded = base64Decode(encrypted_string);
    if (decoded.size() < CHECKSUM_BYTES)
        return {};

    const auto key = getCryptKey(useMachineUUID);
    xorCrypt(decoded, key);

    const uint32_t readsum =
        stdext::readULE32(reinterpret_cast<const uint8_t*>(decoded.data()));
    decoded.erase(0, CHECKSUM_BYTES);

    const uint32_t sum = stdext::computeChecksum(
        { reinterpret_cast<const uint8_t*>(decoded.data()), decoded.size() });

    return (readsum == sum) ? std::move(decoded) : std::string();
}

void Crypt::rsaSetPublicKey(const std::string& n, const std::string& e)
{
#ifdef USE_GMP
    mpz_set_str(m_n, n.c_str(), 10);
    mpz_set_str(m_e, e.c_str(), 10);
#else
    // Free existing BIGNUMs
    BN_free(m_bn_n);
    BN_free(m_bn_e);
    m_bn_n = nullptr;
    m_bn_e = nullptr;

    // Parse new values
    BN_dec2bn(&m_bn_n, n.c_str());
    BN_dec2bn(&m_bn_e, e.c_str());

    // Rebuild EVP_PKEY
    rebuildKey();
#endif
}

void Crypt::rsaSetPrivateKey(const std::string& p, const std::string& q, const std::string& d)
{
#ifdef USE_GMP
    mpz_set_str(m_p, p.c_str(), 10);
    mpz_set_str(m_q, q.c_str(), 10);
    mpz_set_str(m_d, d.c_str(), 10);

    // n = p * q
    mpz_mul(m_n, m_p, m_q);
#else
    // Free existing BIGNUMs
    BN_free(m_bn_p);
    BN_free(m_bn_q);
    BN_free(m_bn_d);
    m_bn_p = nullptr;
    m_bn_q = nullptr;
    m_bn_d = nullptr;

    // Parse new values
    BN_dec2bn(&m_bn_p, p.c_str());
    BN_dec2bn(&m_bn_q, q.c_str());
    BN_dec2bn(&m_bn_d, d.c_str());

    // Calculate n = p * q if not already set
    if (!m_bn_n) {
        m_bn_n = BN_new();
        BN_CTX* ctx = BN_CTX_new();
        BN_mul(m_bn_n, m_bn_p, m_bn_q, ctx);
        BN_CTX_free(ctx);
    }

    m_hasPrivateKey = true;

    // Rebuild EVP_PKEY
    rebuildKey();
#endif
}

#ifndef USE_GMP
void Crypt::rebuildKey()
{
    // Free existing key
    EVP_PKEY_free(m_pkey);
    m_pkey = nullptr;

    if (!m_bn_n || !m_bn_e)
        return;

    OSSL_PARAM_BLD* bld = OSSL_PARAM_BLD_new();
    if (!bld)
        return;

    // Add public key components
    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_N, m_bn_n);
    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_E, m_bn_e);

    // Add private key components if available
    if (m_hasPrivateKey && m_bn_d) {
        OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_D, m_bn_d);

        if (m_bn_p && m_bn_q) {
            OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_FACTOR1, m_bn_p);
            OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_FACTOR2, m_bn_q);
        }
    }

    OSSL_PARAM* params = OSSL_PARAM_BLD_to_param(bld);
    OSSL_PARAM_BLD_free(bld);

    if (!params)
        return;

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr);
    if (!ctx) {
        OSSL_PARAM_free(params);
        return;
    }

    int selection = m_hasPrivateKey ? EVP_PKEY_KEYPAIR : EVP_PKEY_PUBLIC_KEY;

    if (EVP_PKEY_fromdata_init(ctx) <= 0 ||
        EVP_PKEY_fromdata(ctx, &m_pkey, selection, params) <= 0) {
        // Failed to create key
        m_pkey = nullptr;
    }

    EVP_PKEY_CTX_free(ctx);
    OSSL_PARAM_free(params);
}
#endif

bool Crypt::rsaEncrypt(uint8_t* msg, int size)
{
    if (size != rsaGetSize())
        return false;

#ifdef USE_GMP
    mpz_t c, m;
    mpz_init(c);
    mpz_init(m);
    mpz_import(m, size, 1, 1, 0, 0, msg);

    // c = m^e mod n
    mpz_powm(c, m, m_e, m_n);

    size_t count = (mpz_sizeinbase(m, 2) + 7) / 8;
    memset((char*)msg, 0, size - count);
    mpz_export((char*)msg + (size - count), nullptr, 1, 1, 0, 0, c);

    mpz_clear(c);
    mpz_clear(m);

    return true;
#else
    if (!m_pkey)
        return false;

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(m_pkey, nullptr);
    if (!ctx)
        return false;

    bool success = false;

    if (EVP_PKEY_encrypt_init(ctx) > 0) {
        // Set no padding (raw RSA)
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_NO_PADDING);

        size_t outlen = size;
        std::vector<uint8_t> out(size);

        if (EVP_PKEY_encrypt(ctx, out.data(), &outlen, msg, size) > 0) {
            memcpy(msg, out.data(), size);
            success = true;
        }
    }

    EVP_PKEY_CTX_free(ctx);
    return success;
#endif
}

bool Crypt::rsaDecrypt(uint8_t* msg, int size)
{
    if (size != rsaGetSize())
        return false;

#ifdef USE_GMP
    mpz_t c, m;
    mpz_init(c);
    mpz_init(m);
    mpz_import(c, size, 1, 1, 0, 0, msg);

    // m = c^d mod n
    mpz_powm(m, c, m_d, m_n);

    size_t count = (mpz_sizeinbase(m, 2) + 7) / 8;
    memset((char*)msg, 0, size - count);
    mpz_export((char*)msg + (size - count), nullptr, 1, 1, 0, 0, m);

    mpz_clear(c);
    mpz_clear(m);

    return true;
#else
    if (!m_pkey || !m_hasPrivateKey)
        return false;

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(m_pkey, nullptr);
    if (!ctx)
        return false;

    bool success = false;

    if (EVP_PKEY_decrypt_init(ctx) > 0) {
        // Set no padding (raw RSA)
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_NO_PADDING);

        size_t outlen = size;
        std::vector<uint8_t> out(size);

        if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, msg, size) > 0) {
            memcpy(msg, out.data(), size);
            success = true;
        }
    }

    EVP_PKEY_CTX_free(ctx);
    return success;
#endif
}

int Crypt::rsaGetSize()
{
#ifdef USE_GMP
    size_t count = (mpz_sizeinbase(m_n, 2) + 7) / 8;
    return ((int)count / 128) * 128;
#else
    if (!m_pkey)
        return 0;
    return EVP_PKEY_get_size(m_pkey);
#endif
}

std::string Crypt::crc32(const std::string& decoded_string, const bool upperCase)
{
    uint32_t crc = ::crc32(0, nullptr, 0);
    crc = ::crc32(crc, (const Bytef*)decoded_string.c_str(), decoded_string.size());
    std::string result = stdext::dec_to_hex(crc);
    if (upperCase)
        std::ranges::transform(result, result.begin(), toupper);
    else
        std::ranges::transform(result, result.begin(), tolower);
    return result;
}

// NOSONAR - Intentional use of SHA-1 as there is no security impact in this context
std::string Crypt::sha1Encrypt(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);

    std::ostringstream oss;
    for (unsigned char byte : hash)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);

    return oss.str();
}
