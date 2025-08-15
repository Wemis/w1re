# w1re

### Structures

```c
typedef struct{
  uint8_t name[64]; 
  uint8_t user_id[32]; 
  uint8_t pubkey_sign[32]; 
  uint8_t pubkey_encr[32]; 
  uint8_t signature[64];
} User;
```

```c
typedef struct{
  uint8_t from[32];
  uint8_t to[32];
  uint8_t nonce[crypto_box_NONCEBYTES];
  uint8_t pubkey[crypto_box_PUBLICKEYBYTES];
  Slice content;
} Message;
```

#### Server Api

* register user
  ```
  payload length | u64
  ```json
  {User}
  ```
* send message
  ```
  payload length | u64
  ```json
  {Message}
  ```

#### Server response format

```
payload length | u64
```json 
{
  "is_success":  "bool",
  "err_message": "const char* (May be NULL)"
}
```
