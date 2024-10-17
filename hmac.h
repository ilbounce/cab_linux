/*
 * hmac.h
 *
 *  Created on: May 23, 2023
 *      Author: ilbounce
 */

#ifndef HMAC_H_
#define HMAC_H_

#ifdef WIN32
#pragma once
#endif
/*
    hmac_sha256.h
    Originally written by https://github.com/h5p9sl
*/

#ifndef _HMAC_SHA256_H_
#define _HMAC_SHA256_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stddef.h>
#include "sha256.h"
#include "sha384.h"

    // Concatenate X & Y, return hash.
    static void* H(const void* x,
        const size_t xlen,
        const void* y,
        const size_t ylen,
        void* out,
        const size_t outlen);

    static void* H_384(const void* x,
        const size_t xlen,
        const void* y,
        const size_t ylen,
        void* out,
        const size_t outlen);

    // Wrapper for sha256
    static void* sha256(const void* data,
        const size_t datalen,
        void* out,
        const size_t outlen);

    static void* sha384(const void* data,
        const size_t datalen,
        void* out,
        const size_t outlen);

    size_t  // Returns the number of bytes written to `out`
        hmac_sha256(
            // [in]: The key and its length.
            //      Should be at least 32 bytes long for optimal security.
            const void* key,
            const size_t keylen,

            // [in]: The data to hash alongside the key.
            const void* data,
            const size_t datalen,

            // [out]: The output hash.
            //      Should be 32 bytes long. If it's less than 32 bytes,
            //      the resulting hash will be truncated to the specified length.
            void* out,
            const size_t outlen);
    size_t hmac_sha384(
        // [in]: The key and its length.
            //      Should be at least 32 bytes long for optimal security.
        const void* key,
        const size_t keylen,

        // [in]: The data to hash alongside the key.
        const void* data,
        const size_t datalen,

        // [out]: The output hash.
        //      Should be 32 bytes long. If it's less than 32 bytes,
        //      the resulting hash will be truncated to the specified length.
        void* out,
        const size_t outlen
    );

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _HMAC_SHA256_H_

#endif /* HMAC_H_ */
