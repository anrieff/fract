/***************************************************************************
 *   Copyright (C) 2005 by Veselin Georgiev                                *
 *   vesko@ViruS                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *-------------------------------------------------------------------------*
 * Implementation of AES encryption/decryption, derived from the Paulo     *
 * Barreto's implementation. The source code is copied from                *
 * http://www.esat.kuleuven.ac.be/~rijmen/rijndael/rijndael-fst-3.0.zip    *
 ***************************************************************************/

/**
 * aes_encrypt and aes_decrypt - encode and decode multiple-of-16-byte sized
 * blocks of binary data. The number of bytes is given in `octets'. The 
 * input/output buffer is given in data. If the operation succeeds, functions
 * return true, otherwise false is returned.
*/
bool aes_encrypt(void *data, int octets);
bool aes_decrypt(void *data, int octets);
