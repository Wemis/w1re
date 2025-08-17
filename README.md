# w1re

#### Server Api

* register user
  ```json
  {
    "name": "",
    "user_id": "",
    "pubkey_sign": [],
    "pubkey_encr": [],
    "signature": []
  }
  ```
* send message
  ```json
  {
    "from": "",
    "to": "",
    "nonce": [],
    "pubkey": [],
    "message": {"content": "", "len": 0}
  }
  ```

#### Server response format

```json 
{
  "is_success": true,
  "err_message": ""
}
```
