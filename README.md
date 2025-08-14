# w1re

### package format
#### Server Api
* register user
  * name: uint8_t[64]
  * user id: uint8_t[32]
  * public key sign: uint8_t[32]
  * public key encr: uint8_t[32]
  * signature: uint8_t[64]
* send message
  * from: uint8_t[32]
  * to: uint8_t[32]
  * nonce: uint8_t[crypto_box_NONCEBYTES]
